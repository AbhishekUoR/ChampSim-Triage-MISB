#!/bin/bash
if [ $# -lt 1 ] 
then
    echo "Usage : ./compute_assoc_4core_irregular.sh <dut>"
    exit
fi

dut=$1
echo $dut
dir=$(dirname "$0")

for i in `seq 1 30`;
do
    dut_file="$dut/mix""$i.txt"
    echo -n "mix$i "
    perl ${dir}/assoc.pl $dut_file
done
