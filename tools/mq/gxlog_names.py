"""Names the host stack offsets in MQ_GXLOG lines (the runtime's debug log of who sends 3D commands).

    python3 gxlog_names.py <libcfie_game.so> <log.txt> [count]

Each 'host' offset becomes the recompiled function it falls in (fn_<module>_<guest address>), so a line reads as the
guest call chain, innermost first."""
import bisect, subprocess, sys

lib, log = sys.argv[1], sys.argv[2]
count = int(sys.argv[3]) if len(sys.argv) > 3 else 20
syms = []
for line in subprocess.run(["nm", "--defined-only", lib], capture_output=True, text=True).stdout.splitlines():
    parts = line.split()
    if len(parts) == 3 and parts[1] in "tT":
        syms.append((int(parts[0], 16), parts[2]))
syms.sort()
addrs = [a for a, _ in syms]


def name(off):
    i = bisect.bisect_right(addrs, off) - 1
    return syms[i][1].replace("fn_", "") if i >= 0 else hex(off)


shown = 0
for line in open(log, errors="replace"):
    if not line.startswith("gxlog:") or " host" not in line:
        continue
    head, rest = line.split(" host", 1)
    offs, tail = rest.rsplit(":", 1)
    chain = []
    for o in offs.split():
        n = name(int(o, 16))
        if not chain or chain[-1] != n:
            chain.append(n)
    print(tail.strip(), "|", " <- ".join(chain))
    shown += 1
    if shown >= count:
        break
