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

for i in `seq 1 50`;
do
    dut_file="$dut/mix""$i.txt"
    echo -n "mix$i "
    perl ${dir}/assoc.pl $dut_file
done
