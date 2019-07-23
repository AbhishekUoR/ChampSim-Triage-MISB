TRACE_DIR=/scratch/cluster/akanksha/CRCRealTraces
binary=${1}
n_warm=${2}
n_sim=${3}
num=${4}
outputdir=${5}
option=${6}

trace1=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $1}'`
trace2=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $2}'`
trace3=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $3}'`
trace4=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $4}'`
trace5=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $5}'`
trace6=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $6}'`
trace7=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $7}'`
trace8=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $8}'`
trace9=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $9}'`
trace10=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $10}'`
trace11=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $11}'`
trace12=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $12}'`
trace13=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $13}'`
trace14=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $14}'`
trace15=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $15}'`
trace16=`sed -n ''$num'p' /scratch/cluster/haowu/isb-meta/ChampSim_DPC3/sim_list/16core_workloads.txt | awk '{print $16}'`

(/scratch/cluster/haowu/isb-meta/ChampSim_DPC3/bin/${binary} -warmup_instructions ${n_warm}000000 -simulation_instructions ${n_sim}000000 ${option} -hide_heartbeat -traces ${TRACE_DIR}/${trace1}.trace.gz ${TRACE_DIR}/${trace2}.trace.gz ${TRACE_DIR}/${trace3}.trace.gz ${TRACE_DIR}/${trace4}.trace.gz ${TRACE_DIR}/${trace5}.trace.gz ${TRACE_DIR}/${trace6}.trace.gz ${TRACE_DIR}/${trace7}.trace.gz ${TRACE_DIR}/${trace8}.trace.gz ${TRACE_DIR}/${trace9}.trace.gz ${TRACE_DIR}/${trace10}.trace.gz ${TRACE_DIR}/${trace11}.trace.gz ${TRACE_DIR}/${trace12}.trace.gz ${TRACE_DIR}/${trace13}.trace.gz ${TRACE_DIR}/${trace14}.trace.gz ${TRACE_DIR}/${trace15}.trace.gz ${TRACE_DIR}/${trace16}.trace.gz )&> ${outputdir}/mix${num}.txt
