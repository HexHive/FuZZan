#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "shrink.h"

static main_t orig_main;
uintptr_t oldstackptr;

int new_main(int argc, char **argv, char **envp)
{
    struct mapinfo maps[256];
    size_t num_maps;
    num_maps = get_proc_maps(maps, 256);
    dump_maps(maps, num_maps);

    unmap_old_stack(oldstackptr);
    create_new_tls();
    setup_debug_sighandlers();

    return orig_main(argc, argv, envp);
}

int __libc_start_main(main_t main, int argc, char **ubp_av, void (*init)(void),
        void (*fini)(void), void (*rtld_fini)(void), void *stack_end)
{
    libc_start_main_t orig_libc_start_main;
    uintptr_t new_stack_ptr, new_stack_end, new_argv;

    orig_libc_start_main =
        (libc_start_main_t)dlsym(RTLD_NEXT, "__libc_start_main");

    if (whitelisted_program(ubp_av[0])) {
        orig_libc_start_main(main, argc, ubp_av, init, fini, rtld_fini,
                stack_end);
        exit(EXIT_FAILURE);
    }

    create_new_stack(ubp_av, stack_end,
        &new_stack_ptr, &new_stack_end, &new_argv);

    /* We will wrap main to unmap the (currently active) old stack and move the
     * TLS. */
    orig_main = main;

    /* Record a (random) pointer into the old stack so we can find the old stack
     * mapping later on for unmapping it. */
    asm volatile (
        "mov %%rsp, %0\n\t"
        : "=r"(oldstackptr));

    debug_print("Calling into orig program now\n");

    {
        /* Switch stack and call original __libc_start_main, with args (as taken
           from start.S):
            main:       %rdi
            argc:       %rsi
            argv:       %rdx
            init:       %rcx
            fini:       %r8
            rtld_fini:  %r9
            stack_end:  stack
        */
        register void *r8 asm("r8") = fini;
        register void *r9 asm("r9") = rtld_fini;
        asm volatile (
            "xor   %%rbp, %%rbp \n\t"
            "movq  %%rax, %%rsp \n\t"
            "call *%%rbx        \n\t"
            :
            : "a"(new_stack_ptr), "b"(orig_libc_start_main),
              "D"(new_main), "S"(argc), "d"(new_argv), "c"(init), "r"(r8),
              "r"(r9));
    }

    exit(EXIT_FAILURE); /* This is never reached. */
}

void __shrinkaddrspace_preinit(int argc, char **argv, char **envp)
{
    (void)argc, (void)envp;

    if (whitelisted_program(argv[0]))
        return;

    fill_high_holes();

}
__attribute__((section(".preinit_array")))
    typeof(__shrinkaddrspace_preinit) *__shrinkaddrspace_preinit_array
        = __shrinkaddrspace_preinit;
