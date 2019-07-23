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
for i in `seq 1 50`;
do
    baseline_file="$baseline/mix""$i.txt"
    dut_file="$dut/mix""$i.txt"
    
    #echo "$baseline_file $dut_file"

    trace0=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $1}'`
    trace1=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $2}'`
    trace2=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $3}'`
    trace3=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $4}'`
    trace4=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $5}'`
    trace5=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $6}'`
    trace6=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $7}'`
    trace7=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $8}'`
    trace8=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $9}'`
    trace9=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $10}'`
    trace10=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $11}'`
    trace11=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $12}'`
    trace12=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $13}'`
    trace13=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $14}'`
    trace14=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $15}'`
    trace15=`sed -n ''$i'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_irregular.txt | awk '{print $16}'`

    sc_file0="$sc_baseline/$trace0"".stats"
    sc_file1="$sc_baseline/$trace1"".stats"
    sc_file2="$sc_baseline/$trace2"".stats"
    sc_file3="$sc_baseline/$trace3"".stats"
    sc_file4="$sc_baseline/$trace4"".stats"
    sc_file5="$sc_baseline/$trace5"".stats"
    sc_file6="$sc_baseline/$trace6"".stats"
    sc_file7="$sc_baseline/$trace7"".stats"
    sc_file8="$sc_baseline/$trace8"".stats"
    sc_file9="$sc_baseline/$trace9"".stats"
    sc_file10="$sc_baseline/$trace10"".stats"
    sc_file11="$sc_baseline/$trace11"".stats"
    sc_file12="$sc_baseline/$trace12"".stats"
    sc_file13="$sc_baseline/$trace13"".stats"
    sc_file14="$sc_baseline/$trace14"".stats"
    sc_file15="$sc_baseline/$trace15"".stats"

    core0_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 0`
    core1_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 1`
    core2_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 2`
    core3_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 3`
    core4_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 4`
    core5_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 5`
    core6_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 6`
    core7_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 7`
    core8_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 8`
    core9_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 9`
    core10_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 10`
    core11_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 11`
    core12_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 12`
    core13_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 13`
    core14_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 14`
    core15_dut_ipc=`perl ${dir}/get_percore_ipc.pl $dut_file 15`

    core0_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 0`
    core1_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 1`
    core2_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 2`
    core3_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 3`
    core4_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 4`
    core5_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 5`
    core6_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 6`
    core7_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 7`
    core8_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 8`
    core9_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 9`
    core10_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 10`
    core11_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 11`
    core12_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 12`
    core13_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 13`
    core14_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 14`
    core15_baseline_ipc=`perl ${dir}/get_percore_ipc.pl $baseline_file 15`

    core0_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file0`
    core1_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file1`
    core2_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file2`
    core3_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file3`
    core4_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file4`
    core5_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file5`
    core6_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file6`
    core7_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file7`
    core8_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file8`
    core9_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file9`
    core10_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file10`
    core11_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file11`
    core12_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file12`
    core13_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file13`
    core14_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file14`
    core15_sc_ipc=`perl ${dir}/get_ipc.pl $sc_file15`

    weighted_dut=`echo "($core0_dut_ipc/$core0_sc_ipc) + ($core1_dut_ipc/$core1_sc_ipc) + ($core2_dut_ipc/$core2_sc_ipc) + ($core3_dut_ipc/$core3_sc_ipc) + ($core4_dut_ipc/$core4_sc_ipc) + ($core5_dut_ipc/$core5_sc_ipc) + ($core6_dut_ipc/$core6_sc_ipc) + ($core7_dut_ipc/$core7_sc_ipc) + ($core8_dut_ipc/$core8_sc_ipc) + ($core9_dut_ipc/$core9_sc_ipc) + ($core10_dut_ipc/$core10_sc_ipc) + ($core11_dut_ipc/$core11_sc_ipc) + ($core12_dut_ipc/$core12_sc_ipc) + ($core13_dut_ipc/$core13_sc_ipc) + ($core14_dut_ipc/$core14_sc_ipc) + ($core15_dut_ipc/$core15_sc_ipc)" | bc -l`
    weighted_baseline=`echo "($core0_baseline_ipc/$core0_sc_ipc) + ($core1_baseline_ipc/$core1_sc_ipc) + ($core2_baseline_ipc/$core2_sc_ipc) + ($core3_baseline_ipc/$core3_sc_ipc) + ($core4_baseline_ipc/$core4_sc_ipc) + ($core5_baseline_ipc/$core5_sc_ipc) + ($core6_baseline_ipc/$core6_sc_ipc) + ($core7_baseline_ipc/$core7_sc_ipc) + ($core8_baseline_ipc/$core8_sc_ipc) + ($core9_baseline_ipc/$core9_sc_ipc) + ($core10_baseline_ipc/$core10_sc_ipc) + ($core11_baseline_ipc/$core11_sc_ipc) + ($core12_baseline_ipc/$core12_sc_ipc) + ($core13_baseline_ipc/$core13_sc_ipc) + ($core14_baseline_ipc/$core14_sc_ipc) + ($core15_baseline_ipc/$core15_sc_ipc)" | bc -l`

    weighted_speedup=`echo "100*(($weighted_dut/$weighted_baseline)-1)" | bc -l`
    #echo "$i, $weighted_dut, $weighted_baseline, $weighted_speedup"
    echo "$i, $weighted_speedup"
    average=`perl ${dir}/geomean.pl $weighted_speedup $average $count`
done

echo "Average: $average"
