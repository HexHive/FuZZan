#!/usr/bin/env python2

import os
import sys
import shlex
import shutil
import subprocess
import re

from elftools.elf.elffile import ELFFile
from elftools.elf.segments import InterpSegment


def ex(cmd, env_override=None):
    """Execute a given command (string), returning stdout if succesfull and
    raising an exception otherwise."""

    env = os.environ.copy()
    if env_override:
        for k, v in env_override.items():
            env[k] = v

    p = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, env=env)
    p.wait()
    if p.returncode:
        print "Error while executing command '%s': %d" % (cmd, p.returncode)
        print "stdout: '%s'" % p.stdout.read()
        print "stderr: '%s'" % p.stderr.read()
        raise Exception("Error while executing command")
    return p.stdout.read()


def get_overlap(mapping, other_mappings):
    """Returns the *last* area out of `other_mappings` that overlaps with
    `mapping`, or None. Assumes `other_mappings` is ordered."""

    start, size = mapping
    end = start + size

    for m in other_mappings[::-1]:
        m_start, m_size = m
        m_end = m_start + m_size
        if (start >= m_start and start < m_end) or \
           (end > m_start and end <= m_end) or \
           (start <= m_start and end >= m_end):
            return m

    return None


def get_binary_info(prog):
    """Look for the loader requested by the program, and record existing
    mappings required by program itself."""

    interp = None
    binary_mappings = []

    with open(prog, 'rb') as f:
        e = ELFFile(f)
        for seg in e.iter_segments():
            if isinstance(seg, InterpSegment):
                interp = seg.get_interp_name()
            if seg['p_type'] == 'PT_LOAD':
                binary_mappings.append((seg['p_vaddr'], seg['p_memsz']))
    if interp is None:
        raise Exception("Could not find interp in binary")

    return interp, binary_mappings


def get_library_deps(library, library_path):
    """Look for all dependency libraries for a given ELF library/binary, and
    return a list of full paths. Uses the ldd command to find this information
    at load-time, so may not be complete."""

    # TODO: do we have to do this recursively for all deps?

    deps = []

    ldd = ex("ldd \"%s\"" % library, {"LD_LIBRARY_PATH": library_path})
    for l in ldd.split("\n"):
        m = re.search(r".*.so.* => (/.*\.so[^ ]*)", l)
        if not m:
            continue
        deps.append(m.group(1))
    return deps


def prelink_libs(libs, outdir, existing_mappings, baseaddr=0xf0ffffff):
    """For every library we calculate its size and alignment, find a space in
    our new compact addr space and create a copy of the library that is
    prelinked to the addr. Start mapping these from the *end* of the addr space
    down, but leaving a bit of space at the top for stuff like the stack."""

    for lib in libs:
        newlib = os.path.join(outdir, os.path.basename(lib))
        shutil.copy(lib, newlib)

        reallib = os.path.realpath(lib)
        for debuglib in (reallib + ".debug", "/usr/lib/debug" + reallib):
            if os.path.exists(debuglib):
                newdebuglib = newlib + ".debug"
                shutil.copy(debuglib, newdebuglib)
                ex("objcopy --remove-section=.gnu_debuglink \"%s\"" % newlib)
                ex("objcopy --add-gnu-debuglink=\"%s\" \"%s\"" % (newdebuglib, newlib))
                break

        with open(newlib, 'rb') as f:
            # Determine the alignment and size required for all LOAD segments
            # combined
            align, size = 0, 0
            e = ELFFile(f)
            for seg in e.iter_segments():
                if seg['p_type'] != 'PT_LOAD':
                    continue
                if seg['p_align'] > align:
                    align = seg['p_align']
                size = seg['p_vaddr'] + seg['p_memsz']

            baseaddr -= size

            if baseaddr < 0:
                print >>sys.stderr, '[ShrinkAddrSpace] Error: not enough space to prelink libraries'
                sys.exit(1)

            # Search for a slot that is not overlapping with anything else and
            # aligned properly
            found = False
            while not found:
                if baseaddr % align:
                    baseaddr -= baseaddr % align
                overlap = get_overlap((baseaddr, size), existing_mappings)
                if overlap:
                    baseaddr = overlap[0] - size
                else:
                    found = True

            print "Found %08x - %08x for %s" % (baseaddr, baseaddr + size, lib)
            ex("prelink -r 0x%x \"%s\"" % (baseaddr, newlib))


def baseaddr_from_bits(bits):
    assert bits > 8
    reserve_for_stack = (1 << (bits - 4)) - (1 << (bits - 8))
    return (1 << bits) - 1 - reserve_for_stack


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Shrink address space of "
            "given binary by prelinking all dependency libraries.")
    parser.add_argument("binary", help="The ELF binary")
    parser.add_argument("--in-place", help="Modify binary in-place",
            action="store_true", default=False)
    parser.add_argument("--set-rpath",
            help="Set RPATH of (new) binary and preload lib to out-dir",
            action="store_true", default=False)
    parser.add_argument("--preload-lib", default="libshrink-preload.so",
            help="Library used via LD_PRELOAD that moves stack and mmap_base")
    parser.add_argument("--out-dir", default="",
            help="Output directory for prelinked libs")
    parser.add_argument("--library-path", default="",
            help="LD_LIBRARY_PATH to export during library scan of binary")
    parser.add_argument("--static-lib",
            help="Compile for static linking (exclude libpreload)",
            action="store_true", default=False)
    parser.add_argument("--addrspace-bits",
            help="number of bits in the address space (for base address)",
            type=int, default=32)
    parser.add_argument("--min-shadow-mode",
            help="min-shadow-mode (1G, 4G, 8G, 16G)", type=int, default=1)

    args = parser.parse_args()

    outdir = args.out_dir
    if not args.out_dir:
        outdir = os.path.abspath("prelink-%s" % args.binary.replace("/", "_"))

    print "[ShrinkAddrSpace] for %s, using output dir %s, linked %s" % \
            (args.binary, outdir, "statically" if args.static_lib else
            "dynamically")
    if os.path.isdir(outdir):
        shutil.rmtree(outdir)
    os.mkdir(outdir)

    # Get loader and existing mappings for binary
    interp, binary_mappings = get_binary_info(args.binary)

    # Determine all dependency libraries
    libs = set()
    libs.add(interp)
    libs.update(get_library_deps(args.binary, args.library_path))
    if not args.static_lib:
        libs.update(get_library_deps(args.preload_lib, args.library_path))
        libs.add(args.preload_lib)

    # The magic, construct new addr space by prelinking all dependency libs
    baseaddr = baseaddr_from_bits(args.addrspace_bits)
    if args.min_shadow_mode == 1:
        baseaddr = 0x00010FFFFFFF
    elif args.min_shadow_mode == 4:
        baseaddr = 0x0001C9FFFFFF
    elif args.min_shadow_mode == 8:
        baseaddr = 0x0002C9FFFFFF
    elif args.min_shadow_mode == 16:
        baseaddr = 0x0004C9FFFFFF
    prelink_libs(libs, outdir, binary_mappings, baseaddr)

    # Update the loader to use our prelinked version
    if args.in_place:
        newprog = args.binary
    else:
        newprog = os.path.join(outdir, os.path.basename(args.binary))
        shutil.copy(args.binary, newprog)
    newinterp = os.path.realpath(os.path.join(outdir, os.path.basename(interp)))
    ex("patchelf --set-interpreter \"%s\" \"%s\"" % (newinterp, newprog))

    # By setting the rpath, we can avoid having to specify LD_LIBRARY_PATH
    if args.set_rpath:
        absoutdir = os.path.realpath(outdir)
        ex("patchelf --set-rpath \"%s\" \"%s\"" % (absoutdir, newprog))
        if not args.static_lib:
            newpreload = os.path.join(outdir, os.path.basename(args.preload_lib))
            ex("patchelf --set-rpath \"%s\" \"%s\"" % (absoutdir, newpreload))

if __name__ == '__main__':
    main()
