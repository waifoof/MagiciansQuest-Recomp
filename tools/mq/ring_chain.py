"""ring_chain.py <functions.txt> <log> : rebuild call chains from gxring lines (entry ring, newest first)."""
import bisect, sys
starts = sorted(int(l.split()[0], 16) for l in open(sys.argv[1]) if l[:1] in "0123456789abcdef" and len(l.split()) >= 2)
def containing(a):
    i = bisect.bisect_right(starts, a & ~1) - 1
    return starts[i] if i >= 0 else None
seen = {}
for line in open(sys.argv[2], errors="replace"):
    if not line.startswith("gxring"): continue
    head, ring = line.split(":", 1)
    ents = [tuple(int(x, 16) for x in e.split("/")) for e in ring.split()]
    chain = []
    addr, lr = ents[0]
    chain.append(f"{addr:08x}")
    idx = 0
    for _ in range(20):
        f = containing(lr - 1 if lr & 1 else lr - 4)
        if f is None: break
        j = next((k for k in range(idx + 1, len(ents)) if ents[k][0] & ~1 == f), None)
        chain.append(f"{f:08x}@{lr:08x}")
        if j is None: break
        idx, lr = j, ents[j][1]
    key = " <- ".join(chain)
    w = head.split("stack")[0]
    print(key, "  |", head[:60])
