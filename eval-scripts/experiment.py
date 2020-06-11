#!/usr/bin/python3

from stats import Stat, Stats
import os, re, subprocess, glob

class Experiment:
    def __init__(self, exp_obj):
        self.name = exp_obj['name']
        self.binary = exp_obj['binary']
        if('base' in exp_obj and exp_obj['base']):
            self.base = True
        else:
            self.base = False

    """
    def create_execute_script(self, benchmark):
        execute_script_path = "{0}/{1}.sh".format(self.expr_dir, benchmark)
        with open(execute_script_path, 'w') as output_file:
            command = "{0} -warmup_instructions {1} -simulation_instructions {2} -hide_heartbeat -traces {3}/{4}.trace.gz >& {5}/{4}.stats".format(
                self.binary_path, self.warmup_insns, self.simulation_insns,
                self.trace_dir, benchmark, self.expr_dir)
            print("#!/lusr/bin/bash", file=output_file)
            print(command, file=output_file)
        os.chmod(execute_script_path, 0o744)
        return execute_script_path
    """
    def form_trace_txt(self, multicore, benchmark_list):
        if multicore:
            self.trace_list = []
            with open(benchmark_list) as bm_file:
                mix_count = 1
                for line in bm_file:
                    mix_name = "MIX{0}".format(mix_count)
                    benchmarks = line.split()
                    trace_name = " ".join("{0}/{1}.trace.gz".format(
                        self.trace_dir, benchmark) for benchmark in benchmarks)
                    self.trace_list.append((mix_name, trace_name))
                    mix_count += 1
        else:
            self.trace_list = [(benchmark, "{0}/{1}.trace.gz".format(self.trace_dir, benchmark)) for benchmark in benchmark_list]

    def create_condor_script(self, trace):
        if not os.path.exists(self.expr_dir + "/scripts"):
            os.mkdir(self.expr_dir + "/scripts")
        condor_script_path = "{0}/scripts/{1}.condor".format(self.expr_dir,
                trace[0])
        command = "{0} -warmup_instructions {1} -simulation_instructions {2} {6} -hide_heartbeat -traces {3} >& {5}/{4}.stats".format(
            self.binary_path, self.warmup_insns, self.simulation_insns,
            trace[1], trace[0], self.expr_dir, "-low_bandwidth" if self.low_bandwidth else "")
        subprocess.call(['condor_shell',
#            '--silent',
            '--log',
            '--condor_dir={0}'.format(self.expr_dir),
            '--condor_suffix={0}'.format(trace[0]),
            '--output_dir={0}/scripts'.format(self.expr_dir),
            '--simulate',
            '--script_name={0}'.format(trace[0]),
            '--cmdline={0}'.format(command)
            ])
        assert os.path.exists(condor_script_path)
        #with open(condor_script_path, 'w') as output_file:
        return condor_script_path


    def launch(self, desc, output_dir):
        print("Launching to ", output_dir, "for expr", self.name)
        self.warmup_insns = desc.warmup_insns
        self.simulation_insns = desc.simulation_insns
        self.trace_dir = desc.trace_dir
        self.binary_path = desc.champsim_dir + "/bin/" + self.binary
        self.low_bandwidth = desc.low_bandwidth
        self.expr_dir = output_dir + "/" + self.name
        if not os.path.exists(self.expr_dir):
            os.mkdir(self.expr_dir)
        self.form_trace_txt(desc.multicore, desc.benchmark_list)
        for trace in self.trace_list:
            # Create Run Script
            #exec_path = self.create_execute_script(benchmark)
            # Create Condor Script
            condor_script_path = self.create_condor_script(trace)
            # Launch Condor Script
            subprocess.run(['/lusr/opt/condor/bin/condor_submit',
                condor_script_path])

    def evaluate(self, desc):
        if desc.eval_all:
            stats_dir = desc.stats_dir + "/" + self.name
            self.stats = Stats(desc, stats_dir)
            self.stats.parse(desc.eval_all)
        else:
            print("Non Eval All Mode Not Implemented Yet!")

