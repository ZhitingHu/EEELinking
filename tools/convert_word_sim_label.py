#!/usr/bin/python
import os, sys, random

if len(sys.argv) < 4:
    print ''
    print 'Usage: python %s <entity-filename> <wordsim-filename> <output-filename> ' % sys.argv[0]
    print ''
    sys.exit(1)

entity_filename = sys.argv[1]
wordsim_filename = sys.argv[2]
output_filename = 'dump.dump'
label_filename = 'wordsim_2be_labeled.txt'
wordsim_headline = 1
# Read entity file to get map from entity to id
entities_dict = {}
with open(entity_filename, 'r') as fin:
    entity_idx = 0
    for line in fin.readlines():
        entities_dict[line.decode('UTF-8').lower().strip()] = entity_idx
        word = line.decode('UTF-8').lower().strip()
        entity_idx = entity_idx + 1
print len(entities_dict)
# adds redirect file
redirect_filename = '/home/yuntiand/zhiting/EL/EEEL/data/redirects.txt'
with open(redirect_filename, 'r') as fredirect:
    line_idx = 0
    for line in fredirect.readlines():
        line_idx = line_idx + 1
        line_conv = line.decode('UTF-8').strip().lower()
        words = line_conv.split('\t')
        if not len(words) == 2:
            print str(line_idx), 'Line has not exactly 2 words'
            continue
        if not '_'.join(words[1].split(' ')) in entities_dict:
            print '_'.join(words[1].split(' ')).encode('UTF-8'), 'Not in entity dictionay'
        else:
            word = '_'.join(words[0].split(' '))
            if not word in entities_dict:
                entities_dict['_'.join(words[0].split(' '))] = entities_dict['_'.join(words[1].split(' '))]
            else:
                print word.encode('UTF-8'), 'DUPLICATE'
not_in_dict = set([])
with open(wordsim_filename, 'r') as fsim:
    with open(output_filename, 'w') as fout:
        sim_idx = 0
        not_in = 0
        for line in fsim.readlines():
            line = line.lower()
            sim_idx = sim_idx + 1
            if sim_idx > wordsim_headline:
                words = line.split()
                assert len(words) == 3
                if (words[0] in entities_dict) and (words[1] in entities_dict):
                    line_out = str(entities_dict[words[0]]) + '\t' + str(entities_dict[words[1]]) + '\t' + words[2] + '\n'
                else:
                    print (words[0] + ' or ' + words[1] + ' not found in entity file')
                    if not words[0] in entities_dict:
                        not_in_dict.add(words[0])
                    else:
                        not_in_dict.add(words[1])
                    not_in = not_in + 1
                    line_out = '\n'
                #fout.write(line_out)
with open(label_filename, 'w') as flabel:
    label_list = []
    for item in not_in_dict:
        label_list.append(item)
    label_list_sort = sorted(label_list)
    for item in label_list_sort:
        flabel.write((item+'\n').encode('UTF-8'))
print (str(not_in) + ' word pairs not found in entity file')
