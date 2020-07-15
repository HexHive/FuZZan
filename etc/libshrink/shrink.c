#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <asm/prctl.h>

/* Replace call system mmap instead of metalloc mmap to avoid metapagetable
 * entries */
#include "linux_syscall_support.h"

#include "shrink.h"

/* Not defined in glibc */
int arch_prctl(int code, unsigned long *addr);

#ifndef REDUCED_ADDRSPACE_SIZE
# ifndef ADDRSPACE_BITS
#  define ADDRSPACE_BITS 32
# endif
# define REDUCED_ADDRSPACE_SIZE (1ULL << ADDRSPACE_BITS)
#endif
#define KERN_ADDRSPACE (1ULL << 47)

/*
 * Often we are put in the environment too early, and thus we're preloaded for
 * wrapper scripts and such. These programs (e.g., dash) are not prelinked and
 * thus cannot run with this library. As such, we whitelist certain programs.
 * Anything in [/usr]/bin cannot be prelinked (or shouldn't be).
 */
int whitelisted_program(char *program)
{
    if (strstr(program, "specinvoke"))
        return 1;
    if (strstr(program, "strace"))
        return 1;
    if (!strncmp(program, "/bin/", 5))
        return 1;
    if (!strncmp(program, "/usr/", 5))
        return 1;

    return 0;
}

/*
 * This functions sets up a new stack using the information contained in the old
 * one.
 */
static void setup_new_stack(char **ubp_av, void *stack_end, uintptr_t
        new_stack_top, uintptr_t *new_stack_ptr, uintptr_t *new_stack_end,
        uintptr_t *new_ubp)
{
    /*
     * ------------------------------
     *  [ 0 ]  <-- top of the stack
     *  [ envp strings ]
     *  [ argv strings ] <-- ubp_av
     *  [ 0 ]
     *  [ auxv ]
     *  [ 0 ]
     *  [ envp ]
     *  [ 0 ]
     *  [ argv ]
     *  [ argc ] <-- stack pointer
     * -----------------------------
     *
     * First we will try to locate the top of the stack (starting from ubp_ab),
     * then we fix the argv/envp pointers, and finally copy over all data on the
     * stack to the new mapping.
     */

    char **stack_top_tmp = ubp_av;
    char *last_string = 0;

    /* argv */
    while (*stack_top_tmp != 0) {
        if (last_string < *stack_top_tmp) {
            last_string = *stack_top_tmp;
        }
        stack_top_tmp++;
    }
    stack_top_tmp++;

    /* env */
    environ = stack_top_tmp;
    while (*stack_top_tmp != 0) {
        if (last_string < *stack_top_tmp) {
            last_string = *stack_top_tmp;
        }
        stack_top_tmp++;
    }

    /* Go to end of last string.*/
    while (*last_string++ != 0);
    /* There seems to be another string on the stack */
    while (*last_string++ != 0);

    /* The stack should end here... */
    uintptr_t top_of_stack = (uintptr_t)last_string;
    top_of_stack = (top_of_stack + 0x3) & ~(0x3);

    /* Check for top stack marker */
    if (*(uintptr_t *)top_of_stack != 0) {
        fprintf(stderr, "ERROR: couldn't locate top of the stack\n");
        exit(1);
    }

    /* Distance between the old and new stack, to modify any ptr to the stack
     * easily with a simply subtraction. */
    uintptr_t stack_offset = top_of_stack + sizeof(uintptr_t) - new_stack_top;

    /* Fix the pointers to the new argv and env values */
    char **tmp = ubp_av;
    while (*tmp != 0) /* argv */
        *tmp++ -= stack_offset;
    tmp++;
    while (*tmp != 0) /* envp */
        *tmp++ -= stack_offset;

    /* Copy over all stack contents */
    uintptr_t from = (uintptr_t)ubp_av - sizeof(uintptr_t); /* argc */
    size_t stack_size = top_of_stack - from;
    uintptr_t *newstack =
        (void *)((uintptr_t)new_stack_top - sizeof(uintptr_t) - stack_size);
    memcpy((void *)newstack, (void *)from, stack_size);

    /* Set up the arguments for __libc_start_main call. */
    /* Garbage for alignment */
    newstack--;
    /* 7th param (stack_env), as the rest is passed via regs. */
    *newstack-- = (uintptr_t)stack_end - stack_offset;

    environ = (void *)(((uintptr_t)environ) - stack_offset);
    *new_stack_end = (uintptr_t)stack_end - stack_offset;
    *new_stack_ptr = (uintptr_t)newstack;
    *new_ubp = (uintptr_t)ubp_av - stack_offset;
}

/*
 * Create a new stack and move all state over to from the original one.
 */
void create_new_stack(char **ubp_av, void *stack_end,
        uintptr_t *new_stack_ptr, uintptr_t *new_stack_end, uintptr_t *new_ubp)
{
    struct rlimit stacklimit;
    void *new_stack;
    uintptr_t new_stack_top;
    if (getrlimit(RLIMIT_STACK, &stacklimit) < 0) {
        perror("create_new_stack");
        fprintf(stderr, "ERROR: Could not get stack limit\n");
        exit(1);
    }
    if (stacklimit.rlim_cur == RLIM_INFINITY)
        stacklimit.rlim_cur = DEFAULT_STACK_SIZE;
    debug_print("rlimit stack: %lx\n", stacklimit.rlim_cur);

    int min_mode_on = atoi(getenv("MINMODE_ON"));

    if (min_mode_on == 1) {
      new_stack = mmap(0x0000C0010000, stacklimit.rlim_cur, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_STACK |
                       MAP_GROWSDOWN, 0, 0);
    } else if(min_mode_on == 4) {
      new_stack = mmap(0x000180010000, stacklimit.rlim_cur, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_STACK |
                       MAP_GROWSDOWN, 0, 0);
    } else if(min_mode_on == 8) {
      new_stack = mmap(0x000280010000, stacklimit.rlim_cur, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_STACK |
                       MAP_GROWSDOWN, 0, 0);
    } else if(min_mode_on == 16) {
      new_stack = mmap(0x000480010000, stacklimit.rlim_cur, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_STACK |
                       MAP_GROWSDOWN, 0, 0);
    }

    if (new_stack == MAP_FAILED) {
        perror("create_new_stack");
        fprintf(stderr, "ERROR: Could not mmap required %zx byte for new stack\n",
                stacklimit.rlim_cur);
        exit(1);
    }
    new_stack_top = (uintptr_t)new_stack + stacklimit.rlim_cur;

    setup_new_stack(ubp_av, stack_end, new_stack_top, new_stack_ptr,
            new_stack_end, new_ubp);
}



#define LOC_START 1
#define LOC_END 2
#define LOC_REM 3
int get_proc_maps(struct mapinfo *maps, int maps_size)
{
    char buf[4096];
    int fd;
    ssize_t bytes;
    int i;
    int num_maps = 0;
    int loc = LOC_START;
    char addrbuf[32];
    int addrbuf_pos = 0;

    fd = open("/proc/self/maps", O_RDONLY);
    if (fd == -1) {
        perror("get_proc_maps");
        fprintf(stderr, "Could not open /proc/self/maps!\n");
        abort();
    }

    /* The easiest solution would be to use strtol directly (and use its endptr)
     * twice, followed by a strchr for the \n. However, we are dealing with
     * chunks complicating that scheme a bit (as an addr might be split over
     * chunks). */
    while ((bytes = read(fd, buf, sizeof(buf))) > 0) {
        for (i = 0; i < bytes; i++) {
            if (loc == LOC_START || loc == LOC_END) {
                if (buf[i] == '-') {
                    if (num_maps == maps_size) {
                        fprintf(stderr, "Error: more than %d maps\n",
                                maps_size);
                        return num_maps;
                    }
                    addrbuf[addrbuf_pos] = '\0';
                    maps[num_maps].start = strtoll(addrbuf, NULL, 16);
                    addrbuf_pos = 0;
                    loc = LOC_END;
                } else if (buf[i] == ' ') {
                    addrbuf[addrbuf_pos] = '\0';
                    maps[num_maps].end = strtoll(addrbuf, NULL, 16);
                    addrbuf_pos = 0;
                    loc = LOC_REM;
                    num_maps++;
                } else
                    addrbuf[addrbuf_pos++] = buf[i];
            } else if (loc == LOC_REM) {
                if (buf[i] == '\n')
                    loc = LOC_START;
            }
        }
    }
    close(fd);
    return num_maps;
}
#undef LOC_START
#undef LOC_END
#undef LOC_REM

void dump_maps(struct mapinfo *maps, int num_maps)
{
    int i;
    debug_print("-MAPPINGS- (%d total)\n", num_maps);
    for (i = 0; i < num_maps; i++)
        debug_print(" %lx - %lx\n", maps[i].start, maps[i].end);
}


/*
 * Walks through the address space mappings and fills any hole above our reduced
 * address space with an empty mmap.
 * DO NOT CALL GLIBC FUNCTIONS THAT MAY ALLOC BEFORE/DURING THIS. For example,
 * printf can alloc/dealloc memory.
 */
void fill_high_holes(void)
{
    struct mapinfo maps[256];
    size_t num_maps;
    size_t i;
    uintptr_t prev_end = REDUCED_ADDRSPACE_SIZE;
    num_maps = get_proc_maps(maps, 256);
    for (i = 0; i < num_maps; i++)
        if (maps[i].start > REDUCED_ADDRSPACE_SIZE &&
            maps[i].start < KERN_ADDRSPACE) {
            size_t sz = maps[i].start - prev_end;
            if (prev_end && sz) {
                void *tmp;
                /*
                debug_print("Found hole at %lx-%lx of size %zx\n", prev_end,
                        procmap[i].begin, sz);
                */
                tmp = sys_mmap((void*)prev_end, sz, PROT_NONE,
                        MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
                assert(tmp == (void*)prev_end);

            }
            prev_end = maps[i].end;
        }
}

void sig_handler(int sig, siginfo_t *si, void *ptr)
{
    fprintf(stderr, " *** Signal %d (%s)\n", sig, strsignal(sig));
    struct mapinfo maps[256];
    size_t num_maps;
    num_maps = get_proc_maps(maps, 256);
    dump_maps(maps, num_maps);

    ucontext_t *uc = (ucontext_t *)ptr;
    fprintf(stderr, "Faulting addr: %p\n", si->si_addr);
    fprintf(stderr, " RIP: %llx\n", uc->uc_mcontext.gregs[REG_RIP]);
    fprintf(stderr, " rax: %016llx     rbx: %016llx\n"
                    " rcx: %016llx     rdx: %016llx\n"
                    " rsi: %016llx     rdi: %016llx\n"
                    " rsp: %016llx     rbp: %016llx\n"
                    "  r8: %016llx      r9: %016llx\n"
                    " r10: %016llx     r11: %016llx\n"
                    " r12: %016llx     r13: %016llx\n"
                    " r14: %016llx     r15: %016llx\n",
            uc->uc_mcontext.gregs[REG_RAX], uc->uc_mcontext.gregs[REG_RBX],
            uc->uc_mcontext.gregs[REG_RCX], uc->uc_mcontext.gregs[REG_RDX],
            uc->uc_mcontext.gregs[REG_RSI], uc->uc_mcontext.gregs[REG_RDI],
            uc->uc_mcontext.gregs[REG_RSP], uc->uc_mcontext.gregs[REG_RBP],
            uc->uc_mcontext.gregs[REG_R8],  uc->uc_mcontext.gregs[REG_R9],
            uc->uc_mcontext.gregs[REG_R10], uc->uc_mcontext.gregs[REG_R11],
            uc->uc_mcontext.gregs[REG_R12], uc->uc_mcontext.gregs[REG_R13],
            uc->uc_mcontext.gregs[REG_R14], uc->uc_mcontext.gregs[REG_R15]);

    exit(sig);
}


void unmap_old_stack(uintptr_t oldstackptr)
{
    struct mapinfo maps[256];
    size_t num_maps;
    size_t i;
    num_maps = get_proc_maps(maps, 256);
    for (i = 0; i < num_maps; i++)
        if (maps[i].start <= oldstackptr && oldstackptr < maps[i].end)
        {
            debug_print("Found old stack mapping: %016lx-%016lx\n",
                    maps[i].start, maps[i].end);
            mprotect((void*)maps[i].start, maps[i].end - maps[i].start,
                    PROT_NONE);
        }
}

/*
 * Move thread-local storage (TLS) of glibc.
 * XXX: Not completely functional at the moment as there are a lot of pointers
 * stored internally to glibc pointing to the old TLS still. We can hunt these
 * pointers down in the TLS mapping itself, in the temporary ld heap (.bss of
 * ld.so) but there seem to be even more, and doing this only partially will
 * cause inconsistencies.
 */
void create_new_tls(void)
{
    struct mapinfo maps[256];
    int num_maps;
    struct mapinfo *tls_map = NULL;
    void *newtlsmap;
    uintptr_t tlsptr, newtlsptr, mapoffset;
    size_t sz;
    int i;
    uintptr_t *t;

    arch_prctl(ARCH_GET_FS, &tlsptr);
    //debug_print("TLS ptr: %p\n", tlsptr);

    num_maps = get_proc_maps(maps, 256);
    for (i = 0; i < num_maps; i++)
        if (maps[i].start <= tlsptr && maps[i].end >= tlsptr) {
            assert(tls_map == NULL);
            tls_map = &maps[i];
        }

    sz = tls_map->end - tls_map->start;

    newtlsmap = mmap((void*)0x000100000000ULL, sz, PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (newtlsmap == MAP_FAILED) {
        perror("create_new_tls");
        fprintf(stderr, "ERROR: Could not mmap required %zx byte for "
                "new TLS", sz);
        exit(1);
    }

    mapoffset = tls_map->start - (uintptr_t)newtlsmap;
    memcpy(newtlsmap, (void*)tls_map->start, sz);
    newtlsptr = (uintptr_t)newtlsmap + (tlsptr - tls_map->start);

    /* XXX: replace with below ideally, but we have to find all pointers then */

    /* struct tcbhead_t->tcb */
    assert(((unsigned long*)newtlsptr)[0] == tlsptr);
    ((unsigned long *)newtlsptr)[0] = (uintptr_t)newtlsptr;
    /* struct tcbhead_t->self */
    assert(((unsigned long*)newtlsptr)[2] == tlsptr);
    ((unsigned long *)newtlsptr)[2] = (uintptr_t)newtlsptr;

#if 0
    for (t = (uintptr_t*)newtlsmap;
            t < (uintptr_t*)((char*)newtlsmap + sz);
            t++) {
        if (tls_map->start <= *t && *t < tls_map->end) {
            *t -= mapoffset;

            uintptr_t *ot = (uintptr_t*)((char*)t + mapoffset);
            *ot -= mapoffset;
        }

    }
#endif
    arch_prctl(ARCH_SET_FS, (void*)newtlsptr);
    /* Below would be ideal, but internally glibc has ptrs to this
     * making, for example, printf segfault... */
    //mprotect((void*)tls_map->start, sz, PROT_NONE);
}

void setup_debug_sighandlers(void)
{
    /* Catch SEGV for debugging... */
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = sig_handler;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
}

