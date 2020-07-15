//===-- asan_rtl.cc -------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
// Main file of the ASan run-time library.
//===----------------------------------------------------------------------===//

#include "asan_activation.h"
#include "asan_allocator.h"
#include "asan_interceptors.h"
#include "asan_interface_internal.h"
#include "asan_internal.h"
#include "asan_mapping.h"
#include "asan_poisoning.h"
#include "asan_report.h"
#include "asan_stack.h"
#include "asan_stats.h"
#include "asan_suppressions.h"
#include "asan_thread.h"
#include "asan_rbtree.h"
#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_flags.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_symbolizer.h"
#include "lsan/lsan_common.h"
#include "ubsan/ubsan_init.h"
#include "ubsan/ubsan_platform.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <execinfo.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef ENABLESAMPLEING
#include<stdlib.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#endif

uptr __asan_shadow_memory_dynamic_address = 0;  // Global interface symbol.
int __asan_option_detect_stack_use_after_return;  // Global interface symbol.
uptr *__asan_test_only_reported_buggy_pointer;  // Used only for testing asan.

#ifdef ENABLEMINSHADOW
__attribute__((visibility("default"))) char *MinShadowTable = NULL;
__attribute__((visibility("default"))) char *MinShadowTableInit = NULL;
__attribute__((visibility("default"))) uptr UMinShadowTable;
__attribute__((visibility("default"))) uptr UMinShadowTableEnd;
#endif

namespace __asan {

#if defined(ENABLESAMPLEING)
uint64_t RangeCheck = 0, SingleCheck = 0;
uint64_t rbtreeInsert = 0, rbtreeDelete = 0, rbtreeLookup = 0;
uint64_t rbtreeStackInsert = 0, rbtreeStackDelete = 0;
uint64_t rbtreeGlobalInsert = 0, rbtreeGlobalDelete = 0;
uint64_t rbtreeHeapInsert = 0, rbtreeHeapDelete = 0;
#endif

#ifdef ENABLERBTREE
class rbtree_control {
 public:
  rbtree_control() {
    __asan::checkStart = true;
  }

  ~rbtree_control() {
    __asan::checkStart = false;
  }
};

static rbtree_control RBtreeControl;
#endif

#ifdef ENABLESAMPLEING
class samplingInit {
 public:
  samplingInit() {
  }

  ~samplingInit() {
    key_t key = ftok(getenv("SHM_STR"), atoi(getenv("SHM_INT")));

    int shmid = shmget(key, SAMPLESIZE, 0666|IPC_CREAT);
    uint64_t *datas = (uint64_t*) shmat(shmid, (void*) 0, 0);
    ++datas[0];
#ifdef Dynamic_metadata_MODE1
    datas[1] += (RangeCheck + SingleCheck + rbtreeInsert + rbtreeDelete);
#endif
    shmdt(datas);
  }
};

static samplingInit SamplingInit;
#endif

#ifdef ENABLEPERRBTREE
__attribute__((visibility("default"))) rbtree_type **TopRBTree;
#endif

#ifdef ENABLERBTREE
rbtree ASanTree = NULL;
bool checkStart = false;
#endif

uptr AsanMappingProfile[kAsanMappingProfileSize];

static void AsanDie() {
  static atomic_uint32_t num_calls;
  if (atomic_fetch_add(&num_calls, 1, memory_order_relaxed) != 0) {
    // Don't die twice - run a busy loop.
    while (1) { }
  }
  if (common_flags()->print_module_map >= 1) PrintModuleMap();
  if (flags()->sleep_before_dying) {
    Report("Sleeping for %d second(s)\n", flags()->sleep_before_dying);
    SleepForSeconds(flags()->sleep_before_dying);
  }
  if (flags()->unmap_shadow_on_exit) {
    if (kMidMemBeg) {
      UnmapOrDie((void*)kLowShadowBeg, kMidMemBeg - kLowShadowBeg);
      UnmapOrDie((void*)kMidMemEnd, kHighShadowEnd - kMidMemEnd);
    } else {
      if (kHighShadowEnd)
        UnmapOrDie((void*)kLowShadowBeg, kHighShadowEnd - kLowShadowBeg);
    }
  }
}

static void AsanCheckFailed(const char *file, int line, const char *cond,
                            u64 v1, u64 v2) {
  Report("AddressSanitizer CHECK failed: %s:%d \"%s\" (0x%zx, 0x%zx)\n", file,
         line, cond, (uptr)v1, (uptr)v2);

  // Print a stack trace the first time we come here. Otherwise, we probably
  // failed a CHECK during symbolization.
  static atomic_uint32_t num_calls;
  if (atomic_fetch_add(&num_calls, 1, memory_order_relaxed) == 0) {
    PRINT_CURRENT_STACK_CHECK();
  }

  Die();
}

// -------------------------- Globals --------------------- {{{1
int asan_inited;
bool asan_init_is_running;

#if !ASAN_FIXED_MAPPING
uptr kHighMemEnd, kMidMemBeg, kMidMemEnd;
#endif

// -------------------------- Misc ---------------- {{{1
void ShowStatsAndAbort() {
  __asan_print_accumulated_stats();
  Die();
}

// --------------- LowLevelAllocateCallbac ---------- {{{1
static void OnLowLevelAllocate(uptr ptr, uptr size) {
  PoisonShadow(ptr, size, kAsanInternalHeapMagic);
}

// -------------------------- Run-time entry ------------------- {{{1
// exported functions
#define ASAN_REPORT_ERROR(type, is_write, size)                     \
extern "C" NOINLINE INTERFACE_ATTRIBUTE                             \
void __asan_report_ ## type ## size(uptr addr) {                    \
  GET_CALLER_PC_BP_SP;                                              \
  ReportGenericError(pc, bp, sp, addr, is_write, size, 0, true);    \
}                                                                   \
extern "C" NOINLINE INTERFACE_ATTRIBUTE                             \
void __asan_report_exp_ ## type ## size(uptr addr, u32 exp) {       \
  GET_CALLER_PC_BP_SP;                                              \
  ReportGenericError(pc, bp, sp, addr, is_write, size, exp, true);  \
}                                                                   \
extern "C" NOINLINE INTERFACE_ATTRIBUTE                             \
void __asan_report_ ## type ## size ## _noabort(uptr addr) {        \
  GET_CALLER_PC_BP_SP;                                              \
  ReportGenericError(pc, bp, sp, addr, is_write, size, 0, false);   \
}                                                                   \

ASAN_REPORT_ERROR(load, false, 1)
ASAN_REPORT_ERROR(load, false, 2)
ASAN_REPORT_ERROR(load, false, 4)
ASAN_REPORT_ERROR(load, false, 8)
ASAN_REPORT_ERROR(load, false, 16)
ASAN_REPORT_ERROR(store, true, 1)
ASAN_REPORT_ERROR(store, true, 2)
ASAN_REPORT_ERROR(store, true, 4)
ASAN_REPORT_ERROR(store, true, 8)
ASAN_REPORT_ERROR(store, true, 16)

#define ASAN_REPORT_ERROR_N(type, is_write)                                 \
extern "C" NOINLINE INTERFACE_ATTRIBUTE                                     \
void __asan_report_ ## type ## _n(uptr addr, uptr size) {                   \
  GET_CALLER_PC_BP_SP;                                                      \
  ReportGenericError(pc, bp, sp, addr, is_write, size, 0, true);            \
}                                                                           \
extern "C" NOINLINE INTERFACE_ATTRIBUTE                                     \
void __asan_report_exp_ ## type ## _n(uptr addr, uptr size, u32 exp) {      \
  GET_CALLER_PC_BP_SP;                                                      \
  ReportGenericError(pc, bp, sp, addr, is_write, size, exp, true);          \
}                                                                           \
extern "C" NOINLINE INTERFACE_ATTRIBUTE                                     \
void __asan_report_ ## type ## _n_noabort(uptr addr, uptr size) {           \
  GET_CALLER_PC_BP_SP;                                                      \
  ReportGenericError(pc, bp, sp, addr, is_write, size, 0, false);           \
}                                                                           \

ASAN_REPORT_ERROR_N(load, false)
ASAN_REPORT_ERROR_N(store, true)

#define ASAN_MEMORY_ACCESS_CALLBACK_BODY(type, is_write, size, exp_arg, fatal) \
    if (SANITIZER_MYRIAD2 && !AddrIsInMem(addr) && !AddrIsInShadow(addr))      \
      return;                                                                  \
    uptr sp = MEM_TO_SHADOW(addr);                                             \
    uptr s = size <= SHADOW_GRANULARITY ? *reinterpret_cast<u8 *>(sp)          \
                                        : *reinterpret_cast<u16 *>(sp);        \
    if (UNLIKELY(s)) {                                                         \
      if (UNLIKELY(size >= SHADOW_GRANULARITY ||                               \
                   ((s8)((addr & (SHADOW_GRANULARITY - 1)) + size - 1)) >=     \
                       (s8)s)) {                                               \
        if (__asan_test_only_reported_buggy_pointer) {                         \
          *__asan_test_only_reported_buggy_pointer = addr;                     \
        } else {                                                               \
          GET_CALLER_PC_BP_SP;                                                 \
          ReportGenericError(pc, bp, sp, addr, is_write, size, exp_arg,        \
                              fatal);                                          \
        }                                                                      \
      }                                                                        \
    }

#define ASAN_MEMORY_ACCESS_CALLBACK(type, is_write, size)                      \
  extern "C" NOINLINE INTERFACE_ATTRIBUTE                                      \
  void __asan_##type##size(uptr addr) {                                        \
    ASAN_MEMORY_ACCESS_CALLBACK_BODY(type, is_write, size, 0, true)            \
  }                                                                            \
  extern "C" NOINLINE INTERFACE_ATTRIBUTE                                      \
  void __asan_exp_##type##size(uptr addr, u32 exp) {                           \
    ASAN_MEMORY_ACCESS_CALLBACK_BODY(type, is_write, size, exp, true)          \
  }                                                                            \
  extern "C" NOINLINE INTERFACE_ATTRIBUTE                                      \
  void __asan_##type##size ## _noabort(uptr addr) {                            \
    ASAN_MEMORY_ACCESS_CALLBACK_BODY(type, is_write, size, 0, false)           \
  }                                                                            \

ASAN_MEMORY_ACCESS_CALLBACK(load, false, 1)
ASAN_MEMORY_ACCESS_CALLBACK(load, false, 2)
ASAN_MEMORY_ACCESS_CALLBACK(load, false, 4)
ASAN_MEMORY_ACCESS_CALLBACK(load, false, 8)
ASAN_MEMORY_ACCESS_CALLBACK(load, false, 16)
ASAN_MEMORY_ACCESS_CALLBACK(store, true, 1)
ASAN_MEMORY_ACCESS_CALLBACK(store, true, 2)
ASAN_MEMORY_ACCESS_CALLBACK(store, true, 4)
ASAN_MEMORY_ACCESS_CALLBACK(store, true, 8)
ASAN_MEMORY_ACCESS_CALLBACK(store, true, 16)

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_loadN(uptr addr, uptr size) {
  if (__asan_region_is_poisoned(addr, size)) {
    GET_CALLER_PC_BP_SP;
    ReportGenericError(pc, bp, sp, addr, false, size, 0, true);
  }
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_exp_loadN(uptr addr, uptr size, u32 exp) {
  if (__asan_region_is_poisoned(addr, size)) {
    GET_CALLER_PC_BP_SP;
    ReportGenericError(pc, bp, sp, addr, false, size, exp, true);
  }
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_loadN_noabort(uptr addr, uptr size) {
  if (__asan_region_is_poisoned(addr, size)) {
    GET_CALLER_PC_BP_SP;
    ReportGenericError(pc, bp, sp, addr, false, size, 0, false);
  }
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_storeN(uptr addr, uptr size) {
  if (__asan_region_is_poisoned(addr, size)) {
    GET_CALLER_PC_BP_SP;
    ReportGenericError(pc, bp, sp, addr, true, size, 0, true);
  }
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_exp_storeN(uptr addr, uptr size, u32 exp) {
  if (__asan_region_is_poisoned(addr, size)) {
    GET_CALLER_PC_BP_SP;
    ReportGenericError(pc, bp, sp, addr, true, size, exp, true);
  }
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_storeN_noabort(uptr addr, uptr size) {
  if (__asan_region_is_poisoned(addr, size)) {
    GET_CALLER_PC_BP_SP;
    ReportGenericError(pc, bp, sp, addr, true, size, 0, false);
  }
}

#if defined(ENABLEHEXASAN)
inline int get_min(int a, int b) {
  if (a < b) return a;
  else
    return b;
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_stack_hexasan_insert(uptr addr, uptr size) {
#ifdef ENABLERBTREE
  atreekey input;
  input.start = reinterpret_cast<void *>(addr);
  input.end = reinterpret_cast<void *>(addr + size - 1);
  hexasan_insert(input);
#endif
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_stack_hexasan_delete_rbtree_add(uptr addr, uptr size) {
#ifdef ENABLERBTREE
  atreekey input;
  input.start = reinterpret_cast<void *>(addr);
  input.end = reinterpret_cast<void *>(addr + size - 1);
  hexasan_delete(input);
#endif
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_stack_hexasan_delete(uptr addr, uptr size) {
#ifdef ENABLERBTREE
  atreekey input;
  input.start = reinterpret_cast<void *>(addr);
  input.end = reinterpret_cast<void *>(addr + size - 1);
  hexasan_delete(input);
#endif
}

#ifdef ENABLERBTREE
void verify_access(void *addr, int TypeSize) {
  uptr targetAddr = (uptr)addr + TypeSize - 1;

  node n = nullptr;
  if (ASanTree) {
    node nTemp = ASanTree->root;
    while (nTemp != NULL) {
      if ((uptr)nTemp->key.end < targetAddr)
        nTemp = nTemp->right;
      else if ((uptr)nTemp->key.start > targetAddr)
        nTemp = nTemp->left;
      else if (((uptr)nTemp->key.start <= targetAddr) &&
               targetAddr <= (uptr)nTemp->key.end) {
        n = nTemp;
        break;
      } else
        break;
    }
  }
  if (n == nullptr) {
    return;
  }

#ifdef PRINT_CHECK_RESULT
    Printf("\t Single Memory safety violation %p\n", addr);
#endif
#ifdef TERMINATE_PROGRAM
    Die();
#endif
    return;
}
#else
void verify_access(void *addr, int MapIndex) {
}
#endif

inline void *calc_address(void *add, int changeVal) {
  uptr p = reinterpret_cast<uptr>(add);
  uptr realAddressP = p + changeVal;
  return reinterpret_cast<void *>(realAddressP);
}

#define CHECK_SMALL_REGION(p, size, isWrite)                  \
  do {                                                        \
    uptr __p = reinterpret_cast<uptr>(p);                     \
    uptr __size = size;                                       \
    if (UNLIKELY(__asan::AddressIsPoisoned(__p) ||            \
                 __asan::AddressIsPoisoned(__p + __size - 1))) {       \
      GET_CURRENT_PC_BP_SP;                                   \
      uptr __bad = __asan_region_is_poisoned(__p, __size);    \
      __asan_report_error(pc, bp, sp, __bad, isWrite, __size, 0);\
    }                                                         \
  } while (false)


#ifdef ENABLERBTREE
#ifndef INLINE_OPT
extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_safety_check(void *addr, int typeSize) {
#ifdef DISABLE_META
  return;
#endif
  if ((addr == nullptr) || !checkStart) return;

  verify_access(addr, typeSize);
}
#endif
#endif

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_safety_hit(void *addr) {
}

#ifdef INLINE_OPT
extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __asan_safety_check(void *addr, int MapIndex) {
#ifdef DISABLE_META
  return;
#endif
  if (MapIndex < 0 || MapIndex > MAXCACHESIZE) {
    return;
  }

  verify_access(addr, MapIndex);
}
#endif
#endif

#if defined(ENABLESAMPLEING)
extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __sample_safety_check() {
#if (defined(Dynamic_metadata_MODE1))
  SingleCheck++;
#endif
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __sample_stack_insert() {
  rbtreeStackInsert++;
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __sample_stack_delete() {
  rbtreeStackDelete++;
}

#endif

// Force the linker to keep the symbols for various ASan interface functions.
// We want to keep those in the executable in order to let the instrumented
// dynamic libraries access the symbol even if it is not used by the executable
// itself. This should help if the build system is removing dead code at link
// time.
static NOINLINE void force_interface_symbols() {
  volatile int fake_condition = 0;  // prevent dead condition elimination.
  // __asan_report_* functions are noreturn, so we need a switch to prevent
  // the compiler from removing any of them.
  // clang-format off
  switch (fake_condition) {
    case 1: __asan_report_load1(0); break;
    case 2: __asan_report_load2(0); break;
    case 3: __asan_report_load4(0); break;
    case 4: __asan_report_load8(0); break;
    case 5: __asan_report_load16(0); break;
    case 6: __asan_report_load_n(0, 0); break;
    case 7: __asan_report_store1(0); break;
    case 8: __asan_report_store2(0); break;
    case 9: __asan_report_store4(0); break;
    case 10: __asan_report_store8(0); break;
    case 11: __asan_report_store16(0); break;
    case 12: __asan_report_store_n(0, 0); break;
    case 13: __asan_report_exp_load1(0, 0); break;
    case 14: __asan_report_exp_load2(0, 0); break;
    case 15: __asan_report_exp_load4(0, 0); break;
    case 16: __asan_report_exp_load8(0, 0); break;
    case 17: __asan_report_exp_load16(0, 0); break;
    case 18: __asan_report_exp_load_n(0, 0, 0); break;
    case 19: __asan_report_exp_store1(0, 0); break;
    case 20: __asan_report_exp_store2(0, 0); break;
    case 21: __asan_report_exp_store4(0, 0); break;
    case 22: __asan_report_exp_store8(0, 0); break;
    case 23: __asan_report_exp_store16(0, 0); break;
    case 24: __asan_report_exp_store_n(0, 0, 0); break;
    case 25: __asan_register_globals(nullptr, 0); break;
    case 26: __asan_unregister_globals(nullptr, 0); break;
    case 27: __asan_set_death_callback(nullptr); break;
    case 28: __asan_set_error_report_callback(nullptr); break;
    case 29: __asan_handle_no_return(); break;
    case 30: __asan_address_is_poisoned(nullptr); break;
    case 31: __asan_poison_memory_region(nullptr, 0); break;
    case 32: __asan_unpoison_memory_region(nullptr, 0); break;
    case 34: __asan_before_dynamic_init(nullptr); break;
    case 35: __asan_after_dynamic_init(); break;
    case 36: __asan_poison_stack_memory(0, 0); break;
    case 37: __asan_unpoison_stack_memory(0, 0); break;
    case 38: __asan_region_is_poisoned(0, 0); break;
    case 39: __asan_describe_address(0); break;
    case 40: __asan_set_shadow_00(0, 0); break;
    case 41: __asan_set_shadow_f1(0, 0); break;
    case 42: __asan_set_shadow_f2(0, 0); break;
    case 43: __asan_set_shadow_f3(0, 0); break;
    case 44: __asan_set_shadow_f5(0, 0); break;
    case 45: __asan_set_shadow_f8(0, 0); break;
  }
  // clang-format on
}

#ifdef ENABLEHEXASAN
static void FuZZan_init() {

// RBTREE mode init
#ifdef ENABLERBTREE
  ASanTree = rbtree_create();
#endif

// min shadow memory init
#ifdef ENABLEMINSHADOW
  #if defined(MIN_1G)
  const uptr TOTALSIZE_INIT = 0x2000000;
  const uptr MINSHADOWSTART_INIT = 0x10010E000000ULL;
  const uptr MINSHADOWSTART = 0x000110000000ULL;
  const uptr MINSIZE = 0x22000000;
  const uptr TOTALSIZE = 0x26400000;
#elif defined(MIN_4G)
  const uptr TOTALSIZE_INIT = 0x4000000;
  const uptr MINSHADOWSTART_INIT = 0x1001C6000000ULL;
  const uptr MINSHADOWSTART = 0x0001CA000000ULL;
  const uptr MINSIZE = 0x39400000;
  const uptr TOTALSIZE = 0x40680000;
#elif defined(MIN_8G)
  const uptr TOTALSIZE_INIT = 0x10000000;
  const uptr MINSHADOWSTART_INIT = 0x1002BA000000ULL;
  const uptr MINSHADOWSTART = 0x0002CA000000ULL;
  const uptr MINSIZE = 0x59400000;
  const uptr TOTALSIZE = 0x64680000;
#elif defined(MIN_16G)
  const uptr TOTALSIZE_INIT = 0x20000000;
  const uptr MINSHADOWSTART_INIT = 0x1004AA000000ULL;
  const uptr MINSHADOWSTART = 0x0004CA000000ULL;
  const uptr MINSIZE = 0x99400000;
  const uptr TOTALSIZE = 0xAC680000;
#endif
  MinShadowTable = (char *) mmap((void*) MINSHADOWSTART, TOTALSIZE, PROT_READ |
                                 PROT_WRITE, MAP_PRIVATE |
                                 MAP_ANONYMOUS, -1, 0);

  MinShadowTableInit = (char *) mmap((void*) MINSHADOWSTART_INIT,
                                     TOTALSIZE_INIT, PROT_READ |
                                     PROT_WRITE, MAP_PRIVATE |
                                     MAP_ANONYMOUS, -1, 0);

  __asan_shadow_memory_dynamic_address = reinterpret_cast<uptr>(MinShadowTable);
  UMinShadowTable = __asan_shadow_memory_dynamic_address;
  UMinShadowTableEnd = UMinShadowTable + MINSIZE - 1;

  uptr gap_start = MEM_TO_SHADOW(UMinShadowTable);
  uptr gap_end = MEM_TO_SHADOW(UMinShadowTableEnd);

  ProtectGap(gap_start, (gap_end - gap_start));
#endif
}
#endif

static void asan_atexit() {
  Printf("AddressSanitizer exit stats:\n");
  __asan_print_accumulated_stats();
  // Print AsanMappingProfile.
  for (uptr i = 0; i < kAsanMappingProfileSize; i++) {
    if (AsanMappingProfile[i] == 0) continue;
    Printf("asan_mapping.h:%zd -- %zd\n", i, AsanMappingProfile[i]);
  }
}

static void InitializeHighMemEnd() {
#if !SANITIZER_MYRIAD2
#if !ASAN_FIXED_MAPPING
  kHighMemEnd = GetMaxUserVirtualAddress();
  // Increase kHighMemEnd to make sure it's properly
  // aligned together with kHighMemBeg:
  kHighMemEnd |= SHADOW_GRANULARITY * GetMmapGranularity() - 1;
#endif  // !ASAN_FIXED_MAPPING
#ifndef ENABLEHEXASAN
  CHECK_EQ((kHighMemBeg % GetMmapGranularity()), 0);
#endif
#endif  // !SANITIZER_MYRIAD2
}

void PrintAddressSpaceLayout() {
  if (kHighMemBeg) {
    Printf("|| `[%p, %p]` || HighMem    ||\n",
           (void*)kHighMemBeg, (void*)kHighMemEnd);
    Printf("|| `[%p, %p]` || HighShadow ||\n",
           (void*)kHighShadowBeg, (void*)kHighShadowEnd);
  }
  if (kMidMemBeg) {
    Printf("|| `[%p, %p]` || ShadowGap3 ||\n",
           (void*)kShadowGap3Beg, (void*)kShadowGap3End);
    Printf("|| `[%p, %p]` || MidMem     ||\n",
           (void*)kMidMemBeg, (void*)kMidMemEnd);
    Printf("|| `[%p, %p]` || ShadowGap2 ||\n",
           (void*)kShadowGap2Beg, (void*)kShadowGap2End);
    Printf("|| `[%p, %p]` || MidShadow  ||\n",
           (void*)kMidShadowBeg, (void*)kMidShadowEnd);
  }
  Printf("|| `[%p, %p]` || ShadowGap  ||\n",
         (void*)kShadowGapBeg, (void*)kShadowGapEnd);
  if (kLowShadowBeg) {
    Printf("|| `[%p, %p]` || LowShadow  ||\n",
           (void*)kLowShadowBeg, (void*)kLowShadowEnd);
    Printf("|| `[%p, %p]` || LowMem     ||\n",
           (void*)kLowMemBeg, (void*)kLowMemEnd);
  }
  Printf("MemToShadow(shadow): %p %p",
         (void*)MEM_TO_SHADOW(kLowShadowBeg),
         (void*)MEM_TO_SHADOW(kLowShadowEnd));
  if (kHighMemBeg) {
    Printf(" %p %p",
           (void*)MEM_TO_SHADOW(kHighShadowBeg),
           (void*)MEM_TO_SHADOW(kHighShadowEnd));
  }
  if (kMidMemBeg) {
    Printf(" %p %p",
           (void*)MEM_TO_SHADOW(kMidShadowBeg),
           (void*)MEM_TO_SHADOW(kMidShadowEnd));
  }
  Printf("\n");
  Printf("redzone=%zu\n", (uptr)flags()->redzone);
  Printf("max_redzone=%zu\n", (uptr)flags()->max_redzone);
  Printf("quarantine_size_mb=%zuM\n", (uptr)flags()->quarantine_size_mb);
  Printf("thread_local_quarantine_size_kb=%zuK\n",
         (uptr)flags()->thread_local_quarantine_size_kb);
  Printf("malloc_context_size=%zu\n",
         (uptr)common_flags()->malloc_context_size);

  Printf("SHADOW_SCALE: %d\n", (int)SHADOW_SCALE);
  Printf("SHADOW_GRANULARITY: %d\n", (int)SHADOW_GRANULARITY);
  Printf("SHADOW_OFFSET: 0x%zx\n", (uptr)SHADOW_OFFSET);
  CHECK(SHADOW_SCALE >= 3 && SHADOW_SCALE <= 7);
  if (kMidMemBeg)
    CHECK(kMidShadowBeg > kLowShadowEnd &&
          kMidMemBeg > kMidShadowEnd &&
          kHighShadowBeg > kMidMemEnd);
}

static void AsanInitInternal() {
  if (LIKELY(asan_inited)) return;
  SanitizerToolName = "AddressSanitizer";
  CHECK(!asan_init_is_running && "ASan init calls itself!");
  asan_init_is_running = true;

  CacheBinaryName();
  CheckASLR();

  // Initialize flags. This must be done early, because most of the
  // initialization steps look at flags().
  InitializeFlags();

  AsanCheckIncompatibleRT();
  AsanCheckDynamicRTPrereqs();
  AvoidCVE_2016_2143();

  SetCanPoisonMemory(flags()->poison_heap);
  SetMallocContextSize(common_flags()->malloc_context_size);

  InitializePlatformExceptionHandlers();

  InitializeHighMemEnd();

  // Make sure we are not statically linked.
  AsanDoesNotSupportStaticLinkage();

  // Install tool-specific callbacks in sanitizer_common.
  AddDieCallback(AsanDie);
  SetCheckFailedCallback(AsanCheckFailed);
  SetPrintfAndReportCallback(AppendToErrorMessageBuffer);

  __sanitizer_set_report_path(common_flags()->log_path);

  __asan_option_detect_stack_use_after_return =
      flags()->detect_stack_use_after_return;

  // Re-exec ourselves if we need to set additional env or command line args.
  MaybeReexec();

  // Setup internal allocator callback.
  SetLowLevelAllocateMinAlignment(SHADOW_GRANULARITY);
  SetLowLevelAllocateCallback(OnLowLevelAllocate);

  InitializeAsanInterceptors();

  // Enable system log ("adb logcat") on Android.
  // Doing this before interceptors are initialized crashes in:
  // AsanInitInternal -> android_log_write -> __interceptor_strcmp
  AndroidLogInit();

  ReplaceSystemMalloc();

  DisableCoreDumperIfNecessary();

#ifdef ENABLEHEXASAN
  FuZZan_init();
#else
  InitializeShadowMemory();
#endif
  AsanTSDInit(PlatformTSDDtor);
  InstallDeadlySignalHandlers(AsanOnDeadlySignal);

  AllocatorOptions allocator_options;
  allocator_options.SetFrom(flags(), common_flags());
  InitializeAllocator(allocator_options);

  MaybeStartBackgroudThread();
  SetSoftRssLimitExceededCallback(AsanSoftRssLimitExceededCallback);

  // On Linux AsanThread::ThreadStart() calls malloc() that's why asan_inited
  // should be set to 1 prior to initializing the threads.
  asan_inited = 1;
  asan_init_is_running = false;

  if (flags()->atexit)
    Atexit(asan_atexit);

  InitializeCoverage(common_flags()->coverage, common_flags()->coverage_dir);

  // Now that ASan runtime is (mostly) initialized, deactivate it if
  // necessary, so that it can be re-activated when requested.
  if (flags()->start_deactivated)
    AsanDeactivate();

  // interceptors
  InitTlsSize();

  // Create main thread.
  AsanThread *main_thread = CreateMainThread();
  CHECK_EQ(0, main_thread->tid());
  force_interface_symbols();  // no-op.
  SanitizerInitializeUnwinder();

  if (CAN_SANITIZE_LEAKS) {
    __lsan::InitCommonLsan();
    if (common_flags()->detect_leaks && common_flags()->leak_check_at_exit) {
      if (flags()->halt_on_error)
        Atexit(__lsan::DoLeakCheck);
      else
        Atexit(__lsan::DoRecoverableLeakCheckVoid);
    }
  }

#if CAN_SANITIZE_UB
  __ubsan::InitAsPlugin();
#endif

  InitializeSuppressions();

  if (CAN_SANITIZE_LEAKS) {
    // LateInitialize() calls dlsym, which can allocate an error string buffer
    // in the TLS.  Let's ignore the allocation to avoid reporting a leak.
    __lsan::ScopedInterceptorDisabler disabler;
    Symbolizer::LateInitialize();
  } else {
    Symbolizer::LateInitialize();
  }

  VReport(1, "AddressSanitizer Init done\n");

  if (flags()->sleep_after_init) {
    Report("Sleeping for %d second(s)\n", flags()->sleep_after_init);
    SleepForSeconds(flags()->sleep_after_init);
  }
}

// Initialize as requested from some part of ASan runtime library (interceptors,
// allocator, etc).
void AsanInitFromRtl() {
  AsanInitInternal();
}

#if ASAN_DYNAMIC
// Initialize runtime in case it's LD_PRELOAD-ed into unsanitized executable
// (and thus normal initializers from .preinit_array or modules haven't run).

class AsanInitializer {
public:  // NOLINT
  AsanInitializer() {
    AsanInitFromRtl();
  }
};

static AsanInitializer asan_initializer;
#endif  // ASAN_DYNAMIC

} // namespace __asan

// ---------------------- Interface ---------------- {{{1
using namespace __asan;  // NOLINT

void NOINLINE __asan_handle_no_return() {
  if (asan_init_is_running)
    return;

  int local_stack;
  AsanThread *curr_thread = GetCurrentThread();
  uptr PageSize = GetPageSizeCached();
  uptr top, bottom;
  if (curr_thread) {
    top = curr_thread->stack_top();
    bottom = ((uptr)&local_stack - PageSize) & ~(PageSize - 1);
  } else if (SANITIZER_RTEMS) {
    // Give up On RTEMS.
    return;
  } else {
    CHECK(!SANITIZER_FUCHSIA);
    // If we haven't seen this thread, try asking the OS for stack bounds.
    uptr tls_addr, tls_size, stack_size;
    GetThreadStackAndTls(/*main=*/false, &bottom, &stack_size, &tls_addr,
                         &tls_size);
    top = bottom + stack_size;
  }
  static const uptr kMaxExpectedCleanupSize = 64 << 20;  // 64M
  if (top - bottom > kMaxExpectedCleanupSize) {
    static bool reported_warning = false;
    if (reported_warning)
      return;
    reported_warning = true;

    //TODO(Jeon): enable later
    return;
    Report("WARNING: ASan is ignoring requested __asan_handle_no_return: "
           "stack top: %p; bottom %p; size: %p (%zd)\n"
           "False positive error reports may follow\n"
           "For details see "
           "https://github.com/google/sanitizers/issues/189\n",
           top, bottom, top - bottom, top - bottom);
    return;
  }
  PoisonShadow(bottom, top - bottom, 0);
  if (curr_thread && curr_thread->has_fake_stack())
    curr_thread->fake_stack()->HandleNoReturn();
}

void NOINLINE __asan_set_death_callback(void (*callback)(void)) {
  SetUserDieCallback(callback);
}

// Initialize as requested from instrumented application code.
// We use this call as a trigger to wake up ASan from deactivated state.
void __asan_init() {
  AsanActivate();
  AsanInitInternal();
}

void __asan_version_mismatch_check() {
  // Do nothing.
}

#if defined(ENABLESAMPLEING)
using namespace __asan;

void sample_range_check() {
#if (defined(Dynamic_metadata_MODE1))
  RangeCheck++;
#endif
}
#endif
