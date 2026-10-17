#!/bin/bash
# Seeds in three passes: trace the strong seeds' bodies, keep weak seeds outside them, then drop weak seeds that a
# body runs into (mid-function).
T=/home/claude/mqtools; C=$(mktemp -d)
R=/home/claude/mqrecomp/build/linux-release/recompiler/ntr-recomp
cp /home/claude/mqrecomp/game/config/ynne/hooks.txt /home/claude/mqrecomp/game/config/ynne/regions.txt $C/
python3 -I $T/find_functions.py /home/claude/rom/mods $C/functions.txt $T/names.txt $T/runtime_seeds.txt --strong
$R --rom /home/claude/rom/mq.nds --config $C --out - --cover $C/cover1.txt --allow-unknown-rom > /dev/null
python3 -I $T/find_functions.py /home/claude/rom/mods $C/functions.txt $T/names.txt $T/runtime_seeds.txt $C/cover1.txt
$R --rom /home/claude/rom/mq.nds --config $C --out - --cover $C/cover2.txt --allow-unknown-rom > /dev/null
cat $C/cover1.txt > $C/cover.txt; grep "^falls" $C/cover2.txt >> $C/cover.txt
python3 -I $T/find_functions.py /home/claude/rom/mods $T/functions.txt $T/names.txt $T/runtime_seeds.txt $C/cover.txt
cp $T/functions.txt /home/claude/mqrecomp/game/config/ynne/functions.txt
rm -rf $C
