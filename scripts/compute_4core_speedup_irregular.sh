#!/bin/bash
if [ $# -lt 3 ] 
then
    echo "Usage : ./compute_4core_opt_miss_reduction.sh <baseline> <dut> <sc_baseline>"
    exit
fi

baseline=$1
echo $baseline
dut=$2
echo $dut
sc_baseline=$3
echo $sc_baseline

average=1.0
count=`ls -lh $baseline/*.txt | wc -l`
echo $count

dir=$(dirname "$0")
for i in `seq 1 30`;
do
    baseline_file="$baseline/mix""$i.txt"
    dut_file="$dut/mix""$i.txt"
    
    #echo "$baseline_file $dut_file"

    trace0=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/4core_irregular.txt | awk '{print $1}'`
    trace1=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/4core_irregular.txt | awk '{print $2}'`
    trace2=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/4core_irregular.txt | awk '{print $3}'`
    trace3=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/4core_irregular.txt | awk '{print $4}'`

    sc_file0="$sc_baseline/$trace0"".stats"
    sc_file1="$sc_baseline/$trace1"".stats"
    sc_file2="$sc_baseline/$trace2"".stats"
    sc_file3="$sc_baseline/$trace3"".stats"

#    echo "$sc_file0 $sc_file1 $sc_file2 $sc_file3"

    core0_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 0`
    core1_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 1`
    core2_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 2`
    core3_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 3`

    core0_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 0`
    core1_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 1`
    core2_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 2`
    core3_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 3`

    core0_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file0`
    core1_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file1`
    core2_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file2`
    core3_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file3`

    #echo "$core0_sc_ipc $core1_sc_ipc $core2_sc_ipc $core3_sc_ipc"

   # echo "$core0_baseline_ipc $core0_sc_ipc" 
    weighted_dut=`echo "($core0_dut_ipc/$core0_sc_ipc) + ($core1_dut_ipc/$core1_sc_ipc) + ($core2_dut_ipc/$core2_sc_ipc) + ($core3_dut_ipc/$core3_sc_ipc)" | bc -l`
    weighted_baseline=`echo "($core0_baseline_ipc/$core0_sc_ipc) + ($core1_baseline_ipc/$core1_sc_ipc) + ($core2_baseline_ipc/$core2_sc_ipc) + ($core3_baseline_ipc/$core3_sc_ipc)" | bc -l`

    weighted_speedup=`echo "100*(($weighted_dut/$weighted_baseline)-1)" | bc -l`
    #echo "$i, $weighted_dut, $weighted_baseline, $weighted_speedup"
    echo "$i, $weighted_speedup"
    average=`perl ${dir}/geomean.pl $weighted_speedup $average $count`
done

echo "Average: $average"
