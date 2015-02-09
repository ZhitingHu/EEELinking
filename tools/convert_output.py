#!/usr/bin/python
import os, sys, random

if len(sys.argv) <> 5:
    print ''
    print 'Usage: python %s <wordsim-filename> <converted-filename> <result-filename> <output-filename>' % sys.argv[0]
    print ''
    sys.exit(1)

wordsim_filename = sys.argv[1]
converted_filename = sys.argv[2]
result_filename = sys.argv[3]
output_filename = sys.argv[4]

wordsim_headlines = 1

with open(wordsim_filename, 'r') as fsim:
    wordsim_lines = fsim.readlines()
with open(converted_filename, 'r') as fconv:
    converted_lines = fconv.readlines()
with open(result_filename, 'r') as fres:
    result_lines = fres.readlines()

line_dict = []
# Format: (score_gnd, score_pred, word1, word2, cnt, weight)
r_idx = 0
for l_idx in range(0, len(wordsim_lines)-wordsim_headlines):
    if len(converted_lines[l_idx].split()) > 0:
        words = wordsim_lines[l_idx+1].split()
        result_words = result_lines[r_idx].split()
        r_idx = r_idx + 1
        line_dict.append((result_words[0], 1.0/(float(result_words[1])+1e-5), words[0], words[1], result_words[2], result_words[3]))

line_dict_sorted = sorted(line_dict, key=lambda line:float(line[0]))
with open(output_filename, 'w') as fout:
    for line_tu in line_dict_sorted:
        line = str(line_tu[0])+'\t'+str(line_tu[1])+'\t'+line_tu[2]+'\t'+line_tu[3]+'\t'+line_tu[4]+'\t'+line_tu[5] + '\n'
        fout.write(line)
