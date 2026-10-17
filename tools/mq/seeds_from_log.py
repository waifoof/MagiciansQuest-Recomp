"""Turn the runtime's interpreter entries into extra seeds: <module> <addr>.

An entry reached by `call X` (an indirect call) is a function. An entry reached by `jump A -> X` is usually a return
to a caller whose frame did not match (a bad seed split it), so it is only taken when X is not a return address.
Ambiguous overlay addresses are written for every overlay holding X."""
import sys, pathlib, re, struct
d = pathlib.Path(sys.argv[1]); out = pathlib.Path(sys.argv[2])
mods = []
for line in (d / "modules.txt").read_text().split("\n"):
    if line.strip():
        n, b, s = line.split(); mods.append((n, int(b, 16), int(s, 16)))
data = {n: (d / f"{n}.bin").read_bytes() for n, b, s in mods}
base = {n: b for n, b, s in mods}

def after_call(n, v):
    a = (v & ~1) - base[n]; buf = data[n]
    if a < 4: return False
    if v & 1:
        hi, lo = struct.unpack_from("<HH", buf, a - 4)
        return ((hi & 0xF800) == 0xF000 and (lo & 0xE800) == 0xE800) or (lo & 0xFF87) == 0x4780
    w = struct.unpack_from("<I", buf, a - 4)[0]
    return (w & 0x0F000000) == 0x0B000000 or (w & 0xFE000000) == 0xFA000000 or (w & 0x0FFFFFF0) == 0x012FFF30

have = set(x for x in out.read_text().split("\n") if x.strip()) if out.exists() else set()
for log in sys.argv[3:]:
    lines = pathlib.Path(log).read_text().split("\n")
    for i, line in enumerate(lines):
        m = re.search(r"first entry at ([0-9a-f]{8})", line)
        if not m: continue
        a = m.group(1); v = int(a, 16)
        prev = lines[i - 1] if i else ""
        via_jump = (prev.startswith("jump ") and prev.endswith(a)) or line.endswith("by jump")
        cands = [n for n, b, s in mods if b <= (v & ~1) < b + s]
        if len(cands) > 1: cands = [n for n in cands if not n.startswith("ov")] or cands
        if len(cands) > 1:
            # Several overlays share the address: keep those where it starts a function (push with lr).
            def starts(n):
                o = (v & ~1) - base[n]; buf = data[n]
                if v & 1: return (struct.unpack_from("<H", buf, o)[0] & 0xFF00) == 0xB500
                return (struct.unpack_from("<I", buf, o)[0] & 0xFFFF4000) == 0xE92D4000
            cands = [n for n in cands if starts(n)] or cands
        for n in cands:
            if via_jump and after_call(n, v): continue
            have.add(f"{n} {a}")
out.write_text("\n".join(sorted(have)) + "\n")
print(len(have), "runtime seeds")
