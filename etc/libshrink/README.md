# ShrinkAddrSpace

Reduces the address space of a program to be run to a certain amount of bits
(e.g., 32-bits). It does this with several steps:

 * Prelink all libraries needed by the program to low addresses, and set the
   rpath so these libraries are used. This include the loader (ld-linux.so).
 * Preload a library that:
   * Moves the stack to the upper part of the new valid address space
   * Map all holes outside of the new reduces address space.
   * Move the TLS of glibc

By default the addr space is reduced to 32-bit, but this can be changed by
modifying both `REDUCED_ADDRSPACE_SIZE` in `libpreload.so` and `baseaddr` in
`prelink_binary.py`. `REDUCED_ADDRSPACE_SIZE` can also be configured by setting
the number of address space bits, e.g.: `make ADDRSPACE_BITS=32`, and
`prelink_binary` accepts an `--addrspace-bits` argument to compute the base
address dynamically.

## Usage

A built binary should be run through `prelink_binary.py` (with the appropriate
args), and should be run using `LD_PRELOAD=/path/to/prelinked/libpreload.so`.
The `wrap.sh` script does all these steps, e.g.:

    ./wrap.sh cat /proc/self/maps

For SPEC the separate steps can also be included in the build system:
https://www.spec.org/cpu2006/docs/monitors.html#build_pre_and_post_bench.


## Implementation details

### Prelinking
To ensure all dynamic libraries are mapped below 32-bit by the loader, we
prelink them at a fixed offset. The script `prelink_binary.py` determines all
dependencies of a binary (using `ldd`), constructs a valid mapping that doesn't
overlap with the new stack, old binary or possible brk locations.

The loader (`ld-linux.so`) is also prelinked, and the binary is updated with
both the new interp and an rpath to use all these new libraries.

### Moving the stack
The stack is mapped in the new address space, and its contents are constructed
based on the old stack. This is done in an override of `__libc_start_main`,
which then updated `rbp` and `rsp` and calls the original `__libc_start_main`.
The main function is also overridden, so the old stack can later be completely
unmapped (to be sure no references exist anymore).

Using `prctl` calls we could inform the kernel about the new location of the
stack (and data on it). However, this requires additional priviliges and does
not seem to matter as far as we could tell from our experiments. This method is
thus disabled (but still present) in the code.

### Moving TLS
Glibc already allocates some small area of memory where its TLS is stored, among
other things. In the overridden main we also move this to the 32-bit address
space. On some machines we encountered issues where glibc would internally
already have pointers to this area, and unmapping it caused functions like
printf to segfault. This is most likely due to the procmap scanning that happens
in `__libc_start_main`, which causes glibc to touch/allocate some internal
state.
