"""randwalk.py <seed> <frames> <shot prefix> > script: continue the saved game, then wander (D-pad holds, A, B, touches)."""
import random, sys
seed, frames, prefix = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
r = random.Random(seed)
L = ["1900 touch 128,150 8", "2210 touch 128,55 8"]
for f in range(2400, 4600, 30): L.append(f"{f} press A 4")
f = 4600
while f < frames:
    k = r.random()
    if k < 0.6:
        d = r.choice(["Left", "Right", "Up", "Down"]); n = r.randint(60, 400)
        L.append(f"{f} press {d} {n}"); f += n + 10
    elif k < 0.85:
        L.append(f"{f} press A 4"); f += 30
    elif k < 0.92:
        L.append(f"{f} press B 4"); f += 30
    else:
        L.append(f"{f} touch {r.randint(20,236)},{r.randint(20,172)} 6"); f += 40
for s in range(5000, frames, 1000): L.append(f"{s} shot {prefix}{s:06d}.png")
L.sort(key=lambda l: int(l.split()[0]))
print("\n".join(L))
