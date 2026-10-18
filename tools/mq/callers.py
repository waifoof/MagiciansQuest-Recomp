"""callers.py <mods dir> <hex target> [modules...]: find BL/BLX calls (ARM and Thumb) to a target address."""
import struct, sys, pathlib
d = pathlib.Path(sys.argv[1]); target = int(sys.argv[2], 16) & ~1
want = set(sys.argv[3:])
for line in (d / "modules.txt").read_text().split("\n"):
    if not line.strip(): continue
    nm, b, s = line.split(); b = int(b, 16)
    if want and nm not in want: continue
    data = (d / f"{nm}.bin").read_bytes()
    # Thumb BL/BLX pairs at every halfword
    for o in range(0, len(data) - 3, 2):
        h1, h2 = struct.unpack_from("<HH", data, o)
        if (h1 & 0xF800) == 0xF000 and (h2 & 0xE800) == 0xE800:
            off = ((h1 & 0x7FF) << 12) | ((h2 & 0x7FF) << 1)
            if off & 0x400000: off -= 0x800000
            t = b + o + 4 + off
            if (h2 & 0x1000) == 0: t &= ~3  # BLX: to ARM
            if t == target: print(f"{nm} {b+o:08x} thumb {'bl' if h2 & 0x1000 else 'blx'}")
    for o in range(0, len(data) - 3, 4):
        w = struct.unpack_from("<I", data, o)[0]
        if (w & 0x0F000000) == 0x0B000000 or (w & 0xFE000000) == 0xFA000000:
            off = (w & 0xFFFFFF) << 2
            if off & 0x2000000: off -= 0x4000000
            t = b + o + 8 + off
            if (w & 0xFE000000) == 0xFA000000: t += ((w >> 24) & 1) * 2
            if t == target: print(f"{nm} {b+o:08x} arm")
    # code pointers
    for o in range(0, len(data) - 3, 4):
        w = struct.unpack_from("<I", data, o)[0]
        if (w & ~1) == target: print(f"{nm} {b+o:08x} pointer {w:08x}")
