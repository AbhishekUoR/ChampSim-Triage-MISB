#!/bin/bash
if [ $# -lt 1 ] 
then
    echo "Usage : ./compute_speedup.sh <dut>"
    exit
fi

dut=$1
echo $dut

speedup_average=1.0
count=`ls -lh /scratch/cluster/akanksha/CloudSuiteTraces/*core0.trace.gz | wc -l`
echo $count

dir=$(dirname "$0")

for f in /scratch/cluster/akanksha/CloudSuiteTraces/*core0.trace.gz
do
    benchmark=$(basename "$f")
    benchmark="${benchmark%.*}"
    benchmark="${benchmark%.*}"
    benchmark="${benchmark%_*}"
    dut_file="$dut/$benchmark"".txt"
    echo -n "$benchmark "
    perl ${dir}/assoc.pl $dut_file
done
