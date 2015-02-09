EEEL
====

1. Get wordsim scores: `sh script/sim.sh`

Input: Defined in `script/sim.sh`

Output: `output.txt`

2. Get the score

`matlab -nodisplay -nodesktop -r "run score.m, quit()"`

3. Convert output to human readable file: `python tools/convert_output.py data/wordsim/combined.tab data/wordsim/combined.tab.conv output.txt sort.txt`

Input: output.txt

Output: sort.txt
