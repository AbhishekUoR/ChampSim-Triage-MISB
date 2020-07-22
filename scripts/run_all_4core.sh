#!/bin/bash
echo $@
./run_all_4core_irregular.sh $@
./run_all_4core_mix.sh $@
./run_cloudsuite_mc.sh $@
