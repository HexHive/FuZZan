#!/usr/bin/env python2

import struct
import sys

from elftools.elf.elffile import ELFFile

def elf_get_preinit(elffile):
    preinit_offset = None
    preinit_funcs = tuple()
    for sec in elffile.iter_sections():
        if sec.name == '.preinit_array':
            sz = sec['sh_size']
            preinit_offset = sec['sh_offset']
            ptrsize = struct.calcsize("P")
            num_funcs = sz / ptrsize
            elffile.stream.seek(preinit_offset)
            buf = elffile.stream.read(sz)
            preinit_funcs = struct.unpack_from("%dP" % num_funcs, buf)
    return preinit_offset, preinit_funcs


def elf_get_symbol(elffile, symname):
    for sec in elffile.iter_sections():
        if sec.name == '.symtab':
            for symbol in sec.iter_symbols():
                if symbol.name == symname:
                    return symbol['st_value']
    return None


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Move a certain preinit "
            "function to the front of the preinit array in an ELF binary.")
    parser.add_argument("--preinit-name", help="Name of the preinit function")
    parser.add_argument("binary", help="The ELF binary")
    args = parser.parse_args()

    print "[fix_preinit] Binary %s, preinit func %s" % \
            (args.binary, args.preinit_name)

    with open(args.binary, 'rb+') as f:
        elffile = ELFFile(f)

        func_loc = elf_get_symbol(elffile, args.preinit_name)
        if func_loc is None:
            print "Could not find function %s in binary %s" % \
                    (args.preinit_name, args.binary)
            sys.exit(1)

        preinit_loc, preinit_funcs = elf_get_preinit(elffile)
        if preinit_loc is None:
            print "Could not find preinit-array location in binary %s" % \
                    args.binary
            sys.exit(1)

        print "Original preinit-array:", map(hex, preinit_funcs)

        if func_loc not in preinit_funcs:
            print "Could not find func %s(%x) in preinit-array in binary %s" % \
                    (args.preinit_name, func_loc, args.binary)
            sys.exit(1)

        preinit_funcs = sorted(preinit_funcs,
                key=lambda addr: 0 if addr == func_loc else 1)
        print "New preinit-array:", map(hex, preinit_funcs)

        f.seek(preinit_loc)
        f.write(struct.pack("%dP" % len(preinit_funcs), *preinit_funcs))


if __name__ == '__main__':
    main()
