#!/usr/bin/env python3
"""Check the civ cap compiled into a CvGameCoreDLL.dll.

  check_civ_cap.py DLL [EXPECTED]

CvGlobals::getMaxCivPlayers() compiles to "mov eax, N; ret" (b8 NN NN NN NN c3). This finds it through the DLL's
export table, prints N and, with EXPECTED, exits with an error when they differ.
"""
import struct
import sys

EXPORT = b'?getMaxCivPlayers@CvGlobals@@QBEHXZ'


def read_cap(path):
    data = open(path, 'rb').read()
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    if data[pe:pe + 4] != b'PE\0\0':
        sys.exit('%s: not a PE file' % path)
    num_sections = struct.unpack_from('<H', data, pe + 6)[0]
    opt_size = struct.unpack_from('<H', data, pe + 20)[0]
    opt = pe + 24
    export_rva = struct.unpack_from('<I', data, opt + 96)[0]  # PE32: data directory 0 = exports
    sections = [struct.unpack_from('<8sIIII', data, opt + opt_size + 40 * i) for i in range(num_sections)]

    def offset(rva):
        for _name, vsize, vaddr, rawsize, rawptr in sections:
            if vaddr <= rva < vaddr + max(vsize, rawsize):
                return rva - vaddr + rawptr
        sys.exit('%s: RVA %#x is in no section' % (path, rva))

    exports = offset(export_rva)
    _nfuncs, nnames, funcs, names, ordinals = struct.unpack_from('<IIIII', data, exports + 20)
    for i in range(nnames):
        name_at = offset(struct.unpack_from('<I', data, offset(names) + 4 * i)[0])
        if data[name_at:data.index(b'\0', name_at)] == EXPORT:
            ordinal = struct.unpack_from('<H', data, offset(ordinals) + 2 * i)[0]
            code_at = offset(struct.unpack_from('<I', data, offset(funcs) + 4 * ordinal)[0])
            code = data[code_at:code_at + 6]
            if code[0] != 0xB8 or code[5] != 0xC3:
                sys.exit('%s: unexpected code for getMaxCivPlayers: %s' % (path, code.hex()))
            return struct.unpack_from('<I', code, 1)[0]
    sys.exit('%s: getMaxCivPlayers is not exported' % path)


cap = read_cap(sys.argv[1])
print('%s: civ cap %d' % (sys.argv[1], cap))
if len(sys.argv) > 2 and cap != int(sys.argv[2]):
    sys.exit('expected civ cap %s' % sys.argv[2])
