#!/bin/bash
# One round: regenerate seeds, rebuild, boot for N frames with an input script, harvest interpreter entries.
set -u
N=${1:-900}; SCRIPT=${2:-/home/claude/run/none.txt}; TAG=${3:-r}
T=/home/claude/mqtools; R=/home/claude/run
$T/two_pass.sh

cmake --build --preset linux-release --project-dir /home/claude/mqrecomp 2>/dev/null >/dev/null || (cd /home/claude/mqrecomp && cmake --build --preset linux-release -j$(nproc) 2>&1 | grep -E "error|FAILED" | head)
cd /home/claude/mqrecomp/build/linux-release/game
SDL_AUDIO_DRIVER=dummy timeout 1200 xvfb-run -a ./ffcc /home/claude/rom/mq.nds --renderer software --unpaced --no-settings --cpu-speed 1 \
  --save-file $R/test.sav --fps-probe --input-script $SCRIPT --exit-after-frames $N > $R/$TAG.log 2>&1
echo "exit=$?"
python3 -I $T/seeds_from_log.py /home/claude/rom/mods $T/runtime_seeds.txt $R/$TAG.log
grep -c "first entry" $R/$TAG.log
grep -E "^fatal|^thread [0-9]+, cpsr" $R/$TAG.log | head -3
grep "new 3D" $R/$TAG.log | tail -2
