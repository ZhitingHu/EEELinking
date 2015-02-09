#!/usr/bin/env bash

# Entity linking parameters
el_can_filename="/home/zhitingh/ml_proj/entity_linking/data/CSAW_new/CSAW_final_candidates_30.dat"
el_gt_filename="/home/zhitingh/ml_proj/entity_linking/data/CSAW_new/CSAW_final_gt.dat"
el_out_filename="el_out.dat"
max_iter=100
# Figure out the paths.
script_path=`readlink -f $0`
script_dir=`dirname $script_path`
app_dir=`dirname $script_dir`
progname=entity_linking.bin
prog_path=${app_dir}/build/tools/${progname}

## Data
#dataset_name=tech
dataset_name=whole
#dataset_path="${app_dir}/data/${dataset_name}"
dataset_path="${app_dir}/../EEEL/data/${dataset_name}"

resume_iter=352000

## Parameters
# embedding
dim_embedding=100;
distance_metric_mode="DIAG";
num_neg_sample=5

output_dir="/home/zhitingh/ml_proj/EEEL_dim100_whole_min_ca/output/eeel_whole_D100_MDIAG_lr10_N5_B500-whole-min-ca-336000"
log_dir=${output_dir}/entitylinking_logs
mkdir -p ${log_dir}

# Run
echo Running entity linking

GLOG_logtostderr=0 \
GLOG_stderrthreshold=0 \
GLOG_log_dir=$log_dir \
GLOG_v=-1 \
GLOG_minloglevel=0 \
GLOG_vmodule="" \
    $prog_path \
    --dim_embedding $dim_embedding \
    --distance_metric_mode $distance_metric_mode \
    --resume_path $output_dir \
    --resume_iter $resume_iter \
    --dataset_path $dataset_path \
    --output_file_prefix $output_dir \
    --num_neg_sample $num_neg_sample \
    --el_can_filename $el_can_filename \
    --el_gt_filename $el_gt_filename \
    --el_out_filename $el_out_filename \
    --max_iter $max_iter
