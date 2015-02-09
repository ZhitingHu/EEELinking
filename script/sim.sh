#!/usr/bin/env bash

# Figure out the paths.
script_path=`readlink -f $0`
script_dir=`dirname $script_path`
app_dir=`dirname $script_dir`
progname=sim.bin
prog_path=${app_dir}/build/tools/${progname}

## Data
#dataset_name=tech
dataset_name=whole
#dataset_path="${app_dir}/data/${dataset_name}"
dataset_path="${app_dir}/../EEEL/data/${dataset_name}"

resume_iter=352000

wordsim_filename="${app_dir}/../data/wordsim/combined.tab.conv"
## Parameters
# embedding
dim_embedding=100;
distance_metric_mode="DIAG";

# solver parameters (for specifying output_dir)
learning_rate=10
num_neg_sample=5
batch_size=500

# Output
output_dir=${app_dir}/output
output_dir="${output_dir}/eeel_${dataset_name}_D${dim_embedding}_M${distance_metric_mode}"
output_dir="${output_dir}_lr${learning_rate}_N${num_neg_sample}_B${batch_size}-whole-min-ca"
log_dir=${output_dir}/analysis_logs
mkdir -p ${log_dir}

# Run
echo Running sim

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
    --wordsim_filename $wordsim_filename
