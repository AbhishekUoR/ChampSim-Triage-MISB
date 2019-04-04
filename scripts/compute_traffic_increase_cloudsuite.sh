#!/bin/bash
if [ $# -lt 2 ] 
then
    echo "Usage : ./run_all_sims.sh <baseline> <dut>"
    exit
fi

baseline=$1
echo $baseline
dut=$2
echo $dut

traffic_increase_average=0
count=`ls -lh /scratch/cluster/akanksha/CloudSuiteTraces/*core0.trace.gz | wc -l`
echo $count

dir=$(dirname "$0")

for f in /scratch/cluster/akanksha/CloudSuiteTraces/*core0.trace.gz
do
    benchmark=$(basename "$f")
    benchmark="${benchmark%.*}"
    benchmark="${benchmark%.*}"
    benchmark="${benchmark%_*}"

    baseline_file="$baseline/$benchmark"".txt"
    dut_file="$dut/$benchmark"".txt"

#    traffic_increase=`perl stms_traffic.pl $baseline_file $dut_file`
   traffic_increase=`perl traffic.pl $baseline_file $dut_file`
    echo "$traffic_increase"
    traffic_increase_average=`perl arithmean.pl $traffic_increase $traffic_increase_average $count`
done

echo "Average: $traffic_increase_average"
