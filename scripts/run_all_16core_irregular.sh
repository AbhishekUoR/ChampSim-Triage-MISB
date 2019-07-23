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

for i in `seq 1 30`;
do
#    echo $f $benchmark 
    num=$i

#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC_AMPM/$output/"
#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC_BO/$output/"
#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC_KPC/$output/"
#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC_LLC/$output/"
#      output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC/$output/"
#      output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/NP/$output/"
#      output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/NP_4.5/$output/"
#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/BO/$output/"
#      output_dir="/scratch/cluster/haowu/isb-meta/tisb-output-4core-regular/$output/"
#      output_dir="/scratch/cluster/haowu/isb-meta/tisb-output-4core-irregular/$output/"
      output_dir="/scratch/cluster/haowu/isb-meta/triage-output-16core-irregular/$output/"
    if [ ! -e "$output_dir" ] ; then
        mkdir $output_dir
        mkdir "$output_dir/scripts"
    fi
    condor_dir="$output_dir"
    script_name="$num"

    #cd $output_dir
    command="/scratch/cluster/haowu/isb-meta/ChampSim_DPC3/scripts/run_16core_irregular.sh $binary 30 30 $num $output_dir"
    #command="/u/akanksha/MyChampSim/ChampSim/scripts/run_4core.sh $binary 200 1000 $num $output_dir -ped_coefficient\ 10"
#    echo $command

    condor_shell --silent --log --condor_dir="$condor_dir" --condor_suffix="$num" --output_dir="$output_dir/scripts" --simulate --script_name="$script_name" --cmdline="$command"

        #Submit the condor file
     /lusr/opt/condor/bin/condor_submit $output_dir/scripts/$script_name.condor

done

