#!/usr/bin/env python3
"""Show or change the mod folder a Civ4 BtS save asks for.

  save_mod_name.py SAVE                      print the mod path stored in SAVE (e.g. "Mods\\K-Mod\\")
  save_mod_name.py SAVE NEWMOD OUT           write a copy of SAVE to OUT that asks for Mods\\NEWMOD\\

A save starts with an int32 format version and the mod path as an int32 length plus that many characters; the
game data follows. Loading a save restarts the game in the mod it names, so after renaming a mod folder its old
saves need this to load in the new folder. OUT must not exist; SAVE is never changed.
"""
import os
import struct
import sys


def read_mod(data):
    version, length = struct.unpack_from('<iI', data, 0)
    if not 0 <= length <= 260:
        sys.exit('not a Civ4 save (mod path length %d)' % length)
    return version, data[8:8 + length].decode('latin-1')


data = open(sys.argv[1], 'rb').read()
version, mod = read_mod(data)
if len(sys.argv) == 2:
    print('%s: format %d, mod path %r' % (sys.argv[1], version, mod))
    sys.exit(0)
if len(sys.argv) != 4:
    sys.exit(__doc__)
new = ('Mods\\%s\\' % sys.argv[2]).encode('latin-1') if sys.argv[2] else b''
if os.path.exists(sys.argv[3]):
    sys.exit('%s exists already' % sys.argv[3])
with open(sys.argv[3], 'xb') as out:
    out.write(struct.pack('<iI', version, len(new)) + new + data[8 + len(mod.encode('latin-1')):])
print('%s: mod path %r -> %r' % (sys.argv[3], mod, new.decode('latin-1')))
