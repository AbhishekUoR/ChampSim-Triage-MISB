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

   # echo "$core0_baseline_ipc $core0_sc_ipc" 
    traffic_increase=`perl stms_traffic.pl $baseline_file $dut_file`
#    traffic_increase=`perl traffic.pl $baseline_file $dut_file`
    echo "$traffic_increase"
done


