"""Seed list for ntr-recomp's functions.txt, found by scanning each module for call instructions and code pointers.

Facts only: start address, size 0 (unknown; the recompiler traces bodies), a/t, and a name when it is known.
    find_functions.py <mods dir> <out functions.txt> [names.txt]
"""
import struct, sys, pathlib, collections

d = pathlib.Path(sys.argv[1])
mods = []  # (name, base, data)
for line in (d / "modules.txt").read_text().split("\n"):
    if line.strip():
        n, b, s = line.split(); mods.append((n, int(b, 16), (d / f"{n}.bin").read_bytes()))
by_name = {m[0]: m for m in mods}
names = {}
if len(sys.argv) > 3:
    for line in pathlib.Path(sys.argv[3]).read_text().split("\n"):
        p = line.split("#")[0].split()
        if len(p) >= 3: names[(p[0], int(p[1], 16))] = p[2]

def is_overlay(n): return n.startswith("ov")

def owner(from_mod, addr):
    """Module a static call from `from_mod` to `addr` lands in (same rule as the recompiler)."""
    n, b, data = by_name[from_mod]
    if is_overlay(n) and b <= addr < b + len(data): return n
    for m in mods:
        if not is_overlay(m[0]) and m[1] <= addr < m[1] + len(m[2]): return m[0]
    return None

def rd32(m, a):
    n, b, data = by_name[m]
    o = a - b
    return struct.unpack_from("<I", data, o)[0] if 0 <= o <= len(data) - 4 else None

def rd16(m, a):
    n, b, data = by_name[m]
    o = a - b
    return struct.unpack_from("<H", data, o)[0] if 0 <= o <= len(data) - 2 else None

def arm_plausible(m, a):
    w = rd32(m, a)
    if w is None or w == 0 or (w >> 28) == 0xF and (w >> 25) & 7 != 5: return False
    return (w >> 28) != 0xF or (w >> 25) & 7 == 5

def arm_prologue(m, a):
    w = rd32(m, a)
    return w is not None and ((w & 0xFFFF4000) == 0xE92D4000 or (w & 0xFFFFF000) == 0xE24DD000 or w == 0xE52DE004)

def thumb_prologue(m, a):
    h = rd16(m, a)
    return h is not None and ((h & 0xFF00) == 0xB500 or (h & 0xFF80) == 0xB080)

def thumb_after_end(m, a):
    p = rd16(m, a - 2)
    if p is None: return True
    return (p == 0x4770 or (p & 0xFF00) == 0xBD00 or (p & 0xF800) == 0xE000 or p == 0x0000 or p == 0x46C0
            or (p & 0xFF87) == 0x4700 or rd32(m, a - 4) is not None and (rd32(m, a - 4) >> 24) in (0x02, 0x01, 0x03, 0x00))

def arm_after_end(m, a):
    p = rd32(m, a - 4)
    if p is None: return True
    return ((p & 0x0FFFFFF0) == 0x012FFF10 and (p >> 28) == 0xE) or (p & 0xFFFF8000) == 0xE8BD8000 or \
        (p & 0xFF000000) == 0xEA000000 or (p >> 24) in (0x00, 0x01, 0x02, 0x03) or p == 0xE49DF004

def thumb_start(m, a): return thumb_prologue(m, a) or thumb_after_end(m, a)
def arm_start(m, a): return arm_prologue(m, a) or arm_after_end(m, a)

calls = collections.Counter()  # (module, addr, thumb) -> callers
weak = set()  # candidates that only follow the end of something (no prologue): kept unless inside a traced body
for n, b, data in mods:
    # ARM: BL (cond AL) and BLX imm, word-aligned.
    for i in range(0, len(data) - 3, 4):
        w = struct.unpack_from("<I", data, i)[0]
        a = b + i
        if (w & 0xFF000000) == 0xEB000000 or (w & 0xFE000000) == 0xFA000000:
            off = (w & 0xFFFFFF); off = off - (1 << 24) if off & 0x800000 else off
            t = a + 8 + off * 4
            thumb = (w >> 28) == 0xF
            if thumb: t += ((w >> 24) & 1) * 2
            tm = owner(n, t)
            if not tm: continue
            if thumb and (rd16(tm, t) is None or not thumb_start(tm, t)): continue
            if not thumb and (not arm_plausible(tm, t) or not arm_start(tm, t)): continue
            if not (thumb_prologue(tm, t) if thumb else arm_prologue(tm, t)): weak.add((tm, t, thumb))
            calls[(tm, t, thumb)] += 1
    # Thumb: BL / BLX pairs, halfword-aligned.
    for i in range(0, len(data) - 3, 2):
        hi, lo = struct.unpack_from("<HH", data, i)
        if (hi & 0xF800) != 0xF000 or (lo & 0xE800) != 0xE800: continue
        off = ((hi & 0x7FF) << 12) | ((lo & 0x7FF) << 1)
        if off & 0x400000: off -= 1 << 23
        a = b + i
        t = a + 4 + off
        thumb = (lo & 0xF800) == 0xF800
        if not thumb: t &= ~3
        tm = owner(n, t)
        if not tm: continue
        if thumb and not thumb_start(tm, t): continue
        if not thumb and (not arm_plausible(tm, t) or not arm_start(tm, t)): continue
        if not (thumb_prologue(tm, t) if thumb else arm_prologue(tm, t)): weak.add((tm, t, thumb))
        calls[(tm, t, thumb)] += 1
for (m, t, th), c in list(calls.items()):
    if th == "maybe":
        del calls[(m, t, th)]
        if c >= 2: calls[(m, t, True)] += c

# Code pointers in data: a word naming an address that starts with a prologue (bit 0 = Thumb).
ptrs = 0
for n, b, data in mods:
    for i in range(0, len(data) - 3, 4):
        v = struct.unpack_from("<I", data, i)[0]
        if not (0x01FF8000 <= v < 0x02400000): continue
        tm = owner(n, v & ~1)
        if not tm:
            # A pointer into overlay space from elsewhere (main's tables of scene functions): a function in every
            # overlay there that starts with a prologue.
            for on, ob, od in mods:
                if on.startswith("ov") and ob <= (v & ~1) < ob + len(od):
                    if v & 1 and thumb_start(on, v & ~1):
                        if not thumb_prologue(on, v & ~1): weak.add((on, v & ~1, True))
                        calls[(on, v & ~1, True)] += 1; ptrs += 1
                    elif not v & 3 and arm_prologue(on, v): calls[(on, v, False)] += 1; ptrs += 1
            continue
        if v & 1:
            if thumb_start(tm, v & ~1):
                if not thumb_prologue(tm, v & ~1): weak.add((tm, v & ~1, True))
                calls[(tm, v & ~1, True)] += 1; ptrs += 1
        elif v & 3 == 0 and arm_plausible(tm, v) and arm_start(tm, v):
            if not arm_prologue(tm, v): weak.add((tm, v, False))
            calls[(tm, v, False)] += 1; ptrs += 1

strong_forced = set((m, a) for (m, a) in names)
extra = pathlib.Path(sys.argv[4]) if len(sys.argv) > 4 else None
if extra and extra.exists():
    for line in extra.read_text().split("\n"):
        p_ = line.split("#")[0].split()
        if len(p_) >= 2:
            a_ = int(p_[1], 16); calls[(p_[0], a_ & ~1, bool(a_ & 1))] += 100; strong_forced.add((p_[0], a_ & ~1))

# Entry points: the program entry and each overlay's static initialisers.
seeds = set((m, a, th) for (m, a, th) in calls)
seeds.add(("main", 0x02000800, False))
for (m, a), nm in names.items(): seeds.add((m, a, False)); calls[(m, a, False)] += 1000

# Drop Thumb/ARM conflicts at one address: keep the mode with more callers.
best = {}
for (m, a, th) in seeds:
    k = (m, a)
    if k not in best or calls[(m, a, th)] > calls[(m, a, best[k])]: best[k] = th
covered = collections.defaultdict(set)
fallen = set()  # entries some body runs into: a weak seed there splits a function
cover = pathlib.Path(sys.argv[5]) if len(sys.argv) > 5 and sys.argv[5] != "--strong" else None
strong_only = "--strong" in sys.argv
if cover:
    for line in cover.read_text().split("\n"):
        p_ = line.split()
        if p_[:1] == ["falls"]:
            fallen.add((p_[1], int(p_[2], 16))); continue
        if len(p_) > 3:
            entry = int(p_[1], 16)
            covered[p_[0]].update(int(x, 16) for x in p_[3:] if int(x, 16) != entry)
sized = {}
sz = pathlib.Path(__file__).with_name("sizes.txt")
if sz.exists():
    for line in sz.read_text().split("\n"):
        p_ = line.split("#")[0].split()
        if len(p_) >= 4: sized[(p_[0], int(p_[1], 16))] = (int(p_[2], 16), p_[3] == "t")
for (m, a), (n, th) in sized.items(): best[(m, a)] = th
dropped = 0
by_mod = collections.defaultdict(list)
for (m, a), th in best.items():
    is_weak = (m, a, th) in weak and (m, a) not in strong_forced and calls[(m, a, th)] < 100
    if is_weak and strong_only: continue
    if is_weak and (a in covered[m] or (m, a) in fallen): dropped += 1; continue
    if any(mm == m and sa < a < sa + n for (mm, sa), (n, _) in sized.items()): dropped += 1; continue
    by_mod[m].append((a, th))
print(f"dropped {dropped} weak seeds inside traced bodies")
with open(sys.argv[2], "w") as f:
    f.write("# Function seeds for Magician's Quest: Mysterious Times (YNNE).\n")
    f.write("# Facts only: start address, size (0: traced by the recompiler), a = ARM / t = Thumb, optional name.\n")
    f.write("# Generated by tools/find_functions.py from call instructions and code pointers; names added by hand.\n")
    for m, *_ in mods:
        if m not in by_mod: continue
        f.write(f"\n[{m}]\n")
        for a, th in sorted(by_mod[m]):
            size = sized.get((m, a), (0, th))[0]
            f.write(f"{a:08x} {size:x} {'t' if th else 'a'}" + (f" {names[(m, a)]}" if (m, a) in names else "") + "\n")
print(f"{sum(len(v) for v in by_mod.values())} seeds in {len(by_mod)} modules ({ptrs} from code pointers)")
