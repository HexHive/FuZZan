#ifndef ASAN_RBTREE_H
#define ASAN_RBTREE_H

#include "sanitizer_common/sanitizer_stacktrace.h"
#include "sanitizer_common/asan_options.h"

#include <inttypes.h>

using namespace __sanitizer;

#ifdef ENABLERBTREE
#ifdef ENABLECACHE
typedef struct CacheEntry_t {
  void* address;
  void* end;
  bool availability;
  int version;
} CacheEntry;
#ifdef USEMMAPCACHE
__attribute__((visibility("default"))) extern CacheEntry *HashMapCache;
#else
__attribute__((visibility("default"))) extern CacheEntry
HashMapCache[MAXCACHESIZE];
#endif
#endif

namespace __asan {
#if defined(PRINT_STATICS)
  extern uint64_t InitPid;
  extern uint64_t totalExec;
  extern uint64_t totalAccess, safeAccess, badAccess;
  extern uint64_t rbtreeInsert, rbtreeDelete, rbtreeLookup;
  extern uint64_t rbtreeStackInsert, rbtreeStackDelete;
  extern uint64_t rbtreeGlobalInsert, rbtreeGlobalDelete;
  extern uint64_t rbtreeHeapInsert, rbtreeHeapDelete;
  extern uint64_t Inline, Callfun, InlineError;
  extern uint64_t CacheHit, CacheMiss, CacheInit, EntryUse,
         OverAccess, TotalCheck;
  extern uint64_t RangeCheck, SingleCheck;
#endif

  typedef struct QuarantineStruct_t {
    void* address;
    int size;
  } QuarantineStruct;

  typedef struct {
    int front;
    int rear;
    int count;
    QuarantineStruct ele[QUARANTINEQSIZE];
  } CirQueue;

  enum rbtree_node_color { RED, BLACK };

  typedef struct atreekey_t {
    void* start;
    void* end;
    void* realAddr;
#ifdef ENABLECACHE
    int saveEntryCnt;
    int cacheEntry[MAXSAVEENTRY];
#endif
  } atreekey;

  typedef struct rbtree_node_t {
    atreekey key;
    struct rbtree_node_t* left;
    struct rbtree_node_t* right;
    struct rbtree_node_t* parent;
    enum rbtree_node_color color;
  } *rbtree_node;

  typedef struct rbtree_t {
    rbtree_node root;
#ifdef ENABLECACHE
    long long versionNum;
#endif
  } *rbtree, rbtree_type;

  typedef rbtree_node node;
  extern rbtree ASanTree;
#ifdef ENABLETHREETREE
  extern rbtree ASanTree_stack;
  extern rbtree ASanTree_heap;
  extern rbtree ASanTree_global;
#endif
  extern bool checkStart;
  extern int removedTarget[MAXCACHESIZE];
  extern int removedTargetCnd;

  inline uint32_t getHash(uptr a) {
    return (((a >> 32) ^ (a)) & 0xffff);
  }

#ifdef ENABLETHREETREE
  rbtree getTargetRBTree(void *ptr);
#endif

  extern "C" NOINLINE INTERFACE_ATTRIBUTE rbtree rbtree_create();
  extern "C" NOINLINE INTERFACE_ATTRIBUTE void hexasan_insert(atreekey key);
  extern "C" NOINLINE INTERFACE_ATTRIBUTE int hexasan_delete(atreekey key);
#ifdef ENABLEPERRBTREE
  __attribute__((visibility("default"))) extern rbtree_type **TopRBTree;
#endif

} // namespace __asan
#endif
#endif
