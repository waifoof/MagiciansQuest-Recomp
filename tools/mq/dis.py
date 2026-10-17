"""dis.py <mods dir> <hex addr> [count] [a|t]: disassemble a module range with llvm-mc (addresses printed)."""
import struct, sys, pathlib, subprocess
d = pathlib.Path(sys.argv[1]); addr = int(sys.argv[2], 16)
n = int(sys.argv[3]) if len(sys.argv) > 3 else 32
thumb = len(sys.argv) > 4 and sys.argv[4] == "t"
mod = sys.argv[5] if len(sys.argv) > 5 else None
for line in (d / "modules.txt").read_text().split("\n"):
    if not line.strip(): continue
    nm, b, s = line.split(); b = int(b, 16); s = int(s, 16)
    if (mod is None or nm == mod) and b <= addr < b + s:
        data = (d / f"{nm}.bin").read_bytes(); break
else:
    sys.exit("address not in any module")
step = 2 if thumb else 4
off = addr - b
for i in range(n):
    o = off + i * step
    raw = data[o:o + step]
    if len(raw) < step: break
    # Thumb BL is two halfwords: feed four bytes so llvm-mc decodes the pair.
    feed = data[o:o + 4] if thumb else raw
    hexs = " ".join(f"0x{x:02x}" for x in feed)
    triple = "thumbv5te" if thumb else "armv5te"
    r = subprocess.run(["llvm-mc", f"--triple={triple}", "--disassemble"], input=hexs, capture_output=True, text=True)
    txt = [l.strip() for l in r.stdout.splitlines() if l.strip() and not l.strip().startswith(".text")]
    word = struct.unpack_from("<I" if not thumb else "<H", raw)[0]
    print(f"{b+o:08x}: {word:0{step*2}x}  {txt[0] if txt else '???'}")
