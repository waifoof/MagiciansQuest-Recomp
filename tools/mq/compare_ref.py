"""Compare the recompiled game's screenshots with the reference emulator's (tools/mqref) for the same input script.

Run both on one script (the frontend with `--renderer software --layout stacked --no-fast-loading`, so that loads
take as long as on the DS; shots named <prefix>NNNNNN.png; mqref
with `N shot` lines, which writes refNNNNN.ppm), then:

    python3 compare_ref.py <prefix> <mqref out dir> [first last step]

For each frame both have, prints how many pixels differ by more than a little (the frontend's picture is scaled down
to 256x384 first, so edges differ by a few thousand pixels even when the pictures are the same). A run that keeps in
step with the reference stays low; a large number marks where the two diverge (timing, or a bug to look at)."""
import sys, os
from PIL import Image, ImageChops

prefix, ref = sys.argv[1], sys.argv[2]
first, last, step = (int(x) for x in sys.argv[3:6]) if len(sys.argv) >= 6 else (0, 10**7, 1000)
for f in range(first, last + 1, step):
    a_path, b_path = f"{prefix}{f:06d}.png", os.path.join(ref, f"ref{f:05d}.ppm")
    if not (os.path.exists(a_path) and os.path.exists(b_path)):
        continue
    a = Image.open(a_path).convert("RGB").resize((256, 384), Image.NEAREST)
    b = Image.open(b_path).convert("RGB")
    hist = ImageChops.difference(a, b).convert("L").histogram()
    print(f, sum(hist[16:]))
