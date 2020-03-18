#!/bin/bash
if [ $# -lt 1 ] 
then
    echo "Usage : ./run_all_sims.sh <binary> <output>"
    exit
fi

binary=$1
echo $binary
output=$2
echo $output

for i in `seq 1 5`;
do
#    echo $f $benchmark 
    num=$i

    output_dir="/scratch/cluster/haowu/isb-meta/triage-output-2core-selected/$output/"
    if [ ! -e "$output_dir" ] ; then
        mkdir $output_dir
        mkdir "$output_dir/scripts"
    fi
    condor_dir="$output_dir"
    script_name="$num"

    #cd $output_dir
    command="/scratch/cluster/haowu/isb-meta/ChampSim_DPC3/scripts/run_2core_selected.sh $binary 30 30 $num $output_dir"
#    echo $command

    condor_shell --silent --log --condor_dir="$condor_dir" --condor_suffix="$num" --output_dir="$output_dir/scripts" --simulate --script_name="$script_name" --cmdline="$command"

        #Submit the condor file
     /lusr/opt/condor/bin/condor_submit $output_dir/scripts/$script_name.condor

done

