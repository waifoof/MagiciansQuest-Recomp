"""Locate NitroSDK routines the runtime replaces, by their instruction patterns. Prints addresses only."""
import struct, sys, pathlib
d = pathlib.Path(sys.argv[1])
mods = {}
for line in (d / "modules.txt").read_text().split("\n"):
    if line.strip():
        n, b, s = line.split(); mods[n] = (int(b, 16), (d / f"{n}.bin").read_bytes())

def words(name):
    base, data = mods[name]
    return base, struct.unpack_from(f"<{len(data)//4}I", data)

def find_seq(name, seq, window=24):
    """Addresses where seq[0] occurs and every later word of seq follows in order within `window` words."""
    base, w = words(name); out = []
    for i, x in enumerate(w):
        if x != seq[0]: continue
        j = i; ok = True
        for s in seq[1:]:
            try: j = w.index(s, j + 1, min(len(w), i + window))
            except ValueError: ok = False; break
        if ok: out.append(base + i * 4)
    return out

def func_start(name, addr, limit=64):
    """Walk back to the nearest push-with-lr (stmdb sp!,{..lr}) or a previous return."""
    base, w = words(name); i = (addr - base) // 4
    for k in range(i, max(0, i - limit), -1):
        x = w[k]
        if (x & 0xFFFF4000) == 0xE92D4000: return base + k * 4
        if k < i and (x in (0xE12FFF1E,) or (x & 0xFFFF8000) == 0xE8BD8000): return base + (k + 1) * 4
    return None

MRS_R2 = 0xE10F2000   # mrs r2, cpsr
MOV_D3 = 0xE3A000D3   # mov r0, #0xd3
STM_ALL = 0xE8817FFF  # stmia r1, {r0-r14}
LDM_ALL = 0xE8917FFF  # ldmia r1, {r0-r14}^ variants
for m in mods:
    for a in find_seq(m, [MRS_R2, MOV_D3, STM_ALL]):
        print(m, "OS_SaveContext body", hex(a), "start", hex(func_start(m, a) or 0))
    base, w = words(m)
    for i, x in enumerate(w):
        if (x & 0xFFFFFFF0) == 0xE8D17FFF or x == 0xE8D17FFF or (x & 0x0FFF7FFF) == 0x08D17FFF:
            print(m, "ldmia r1,{r0-r14}^ (OS_LoadContext?)", hex(base + i * 4), "start", hex(func_start(m, base + i * 4) or 0))
        # mov r12,#0x04000000 ; add r12,r12,#0x210  (OS_IrqHandler: ldr from IE)
        if x == 0xE3A0C301 and i + 1 < len(w) and w[i + 1] == 0xE28CCE21:
            print(m, "IE access (irq handler?)", hex(base + i * 4), "start", hex(func_start(m, base + i * 4) or 0))
