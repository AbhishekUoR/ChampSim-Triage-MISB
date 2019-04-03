TRACE_DIR=/scratch/cluster/akanksha/CloudSuiteTraces/
binary=${1}
n_warm=${2}
n_sim=${3}
trace=${4}
outputdir=${5}
option=${6}

echo "(/u/akanksha/MyChampSim/ChampSim/bin/${binary} -warmup_instructions ${n_warm}000000 -simulation_instructions ${n_sim}000000 -cloudsuite -${option} -traces ${TRACE_DIR}/${trace}_core0.trace.gz ${TRACE_DIR}/${trace}_core1.trace.gz ${TRACE_DIR}/${trace}_core2.trace.gz ${TRACE_DIR}/${trace}_core3.trace.gz) &> ${outputdir}/${trace}.txt"
(/u/akanksha/MyChampSim/ChampSim/bin/${binary} -warmup_instructions ${n_warm}000000 -simulation_instructions ${n_sim}000000 -cloudsuite -hide_heartbeat  -${option} -traces ${TRACE_DIR}/${trace}_core0.trace.gz ${TRACE_DIR}/${trace}_core1.trace.gz ${TRACE_DIR}/${trace}_core2.trace.gz ${TRACE_DIR}/${trace}_core3.trace.gz) &> ${outputdir}/${trace}.txt

