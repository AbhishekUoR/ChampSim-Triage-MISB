#!/usr/bin/python3

import os, re, sys

class Stat:
    def __init__(self, desc, path):
        self.desc = desc
        self.path = path
        self.error = False
        self.stat = {'error': False}
        self.weight = 0.0

    # Major Function to Parse Champsim Results
    def parse(self):
        error = False
        stat = {}
        # TODO: Implement Multi Core. Current Version doesn't work with
        # multi-core
        with open(self.path) as stat_file:
            for line in stat_file:
                res = re.match('ACCURACY: (\d+(\.\d+)?)', line)
                if res != None:
                    stat['accuracy'] = float(res.group(1))

                res = re.match( '(\w+) (\w+)\s+ACCESS:\s+(\d+)\s+HIT:\s+(\d+)\s+MISS:\s+(\d+)', line)
                if res != None:
                    stat[res.group(1)+'_'+res.group(2)+'_access'] = int(res.group(3))
                    stat[res.group(1)+'_'+res.group(2)+'_hit'] = int(res.group(4))
                    stat[res.group(1)+'_'+res.group(2)+'_miss'] = int(res.group(5))

                res = re.match('(\w+) PREFETCH\s+REQUESTED:\s+(\d+)\s+ISSUED:\s+(\d+)\s+USEFUL:\s+(\d+)\s+USELESS:\s+(\d+)', line)
                if res != None:
                    stat[res.group(1)+'_prefetch_requested'] = int(res.group(2))
                    stat[res.group(1)+'_prefetch_issued'] = int(res.group(3))
                    stat[res.group(1)+'_prefetch_useful'] = int(res.group(4))
                    stat[res.group(1)+'_prefetch_useless'] = int(res.group(5))

                res = re.match('CPU (\d+) cumulative IPC:\s+(\d+(\.\d+)?)\s+instructions:\s+(\d+)\s+cycles:\s+(\d+)', line)
                if res != None:
                    stat['cpu_'+res.group(1)+'_ipc'] = float(res.group(2))
                    stat['cpu_'+res.group(1)+'_insns'] = int(res.group(4))
                    stat['cpu_'+res.group(1)+'_cycle'] = int(res.group(5))

                res = re.match('Average latency: (\d+(\.\d+)?)', line)
                if res != None:
                    stat['dram_latency'] = float(res.group(1))

                res = re.match('trigger_count=(\d+)', line)
                if res != None:
                    stat['triage_trigger_count'] = int(res.group(1))

                res = re.match('predict_count=(\d+)', line)
                if res != None:
                    stat['triage_predict_count'] = int(res.group(1))

                res = re.match('SAMPLEOPTGEN 1 accesses: (\d+), hits: (\d+), traffic: (\d+), hit rate: (\d+(\.\d+))', line)
                if res != None:
                    stat['sample_optgen_hit_rate'] = float(res.group(4))

                res = re.match('total_assoc=(\d+)', line)
                if res != None:
                    stat['triage_total_assoc'] = int(res.group(1))

                res = re.match('ISB_nb_(\w+)=(\d+)', line)
                if res != None:
                    stat['isb_'+res.group(1)] = int(res.group(2))

                res = re.search('Average Degree: (\d+(\.\d+)?)', line)
                if res != None:
                    stat['average_degree'] = float(res.group(1))

                res = re.match('OPTgen hits: (\d+)', line)
                if res != None:
                    stat['optgen_hits'] = int(res.group(1))

                res = re.match('OPTgen accesses: (\d+)', line)
                if res != None:
                    stat['optgen_accesses'] = int(res.group(1))

                res = re.match('Traffic: (\d+)', line)
                if res != None:
                    stat['optgen_traffic'] = int(res.group(1))

                res = re.match('RAH Core 0 Config (\d+) (\w+): (\d+)', line)
                if res != None:
                    stat['rah_config_{0}_{1}'.format(res.group(1), res.group(2))] = int(res.group(3))

        # TODO: Implement Multi Core. Current Version doesn't work with
        # multi-core
        # Now we just give no value to crashed executions
        try:
            stat['ipc'] = stat['cpu_0_ipc']
        except:
            print("ipc does not exist", self.path)
            stat['llc_mpki'] = 0.0
            stat['ipc'] = 0.0
            error = True

        if "rah_config_0_Accesses" not in stat:
            for config in range(6):
                for n in ["Accesses", "Hits", "Traffic"]:
                    stat['rah_config_{0}_{1}'.format(config, n)] = 0


        try:
            stat['l1i_apki'] = float(stat['L1I_TOTAL_access'])*1000/float(stat['cpu_0_insns'])
            stat['l1i_mpki'] = float(stat['L1I_TOTAL_miss'])*1000/float(stat['cpu_0_insns'])
            stat['l1d_apki'] = float(stat['L1D_TOTAL_access'])*1000/float(stat['cpu_0_insns'])
            stat['l1d_mpki'] = float(stat['L1D_TOTAL_miss'])*1000/float(stat['cpu_0_insns'])
            stat['l2c_apki'] = float(stat['L2C_TOTAL_access'])*1000/float(stat['cpu_0_insns'])
            stat['l2c_mpki'] = float(stat['L2C_TOTAL_miss'])*1000/float(stat['cpu_0_insns'])
            stat['llc_apki'] = float(stat['LLC_TOTAL_access'])*1000/float(stat['cpu_0_insns'])
            stat['llc_mpki'] = float(stat['LLC_TOTAL_miss'])*1000/float(stat['cpu_0_insns'])
            stat['llc_load_hit_rate'] = float(stat['LLC_LOAD_hit'])/float(stat['LLC_LOAD_access'])
            stat['llc_demand_mpki'] = float(stat['LLC_LOAD_miss']+stat['LLC_RFO_miss']+stat['LLC_WRITEBACK_miss'])*1000/float(stat['cpu_0_insns'])
        except:
            None

        try:
            stat['traffic_load'] = float(stat['LLC_LOAD_miss'])*1000/float(stat['cpu_0_insns'])
            stat['traffic_rfo'] = float(stat['LLC_RFO_miss'])*1000/float(stat['cpu_0_insns'])
            stat['traffic_write'] = float(stat['LLC_WRITEBACK_miss'])*1000/float(stat['cpu_0_insns'])
            stat['traffic_prefetch'] = float(stat['LLC_PREFETCH_miss'])*1000/float(stat['cpu_0_insns'])
        except:
            stat['traffic_load'] = 0.0
            stat['traffic_rfo'] = 0.0
            stat['traffic_write'] = 0.0
            stat['traffic_prefetch'] = 0.0

        try:
            stat['traffic_metadata'] = float(stat['LLC_METADATA_miss'])*1000/float(stat['cpu_0_insns'])
        except:
            stat['traffic_metadata'] = 0.0

            
        stat['traffic'] = stat['traffic_load'] + stat['traffic_rfo'] + stat['traffic_write'] + stat['traffic_prefetch'] + stat['traffic_metadata']

        stat['accuracy'] = 0.0
        stat['coverage'] = 0.0
#            stat['timeliness'] = 0.0
        try:
            stat['accuracy'] = float(stat['L2C_prefetch_useful'])/(float(stat['L2C_prefetch_useful'])
                    + float(stat['L2C_prefetch_useless']))
            stat['coverage'] = float(stat['L2C_prefetch_useful'])/float(stat['L2C_prefetch_useful']+stat['L2C_LOAD_miss']+stat['L2C_RFO_miss'])
#            stat['timeliness'] = float(stat['LLC_prefetch_useful'])/float(stat['LLC_prefetch_late']+stat['LLC_prefetch_useful'])
        except:
            None

        stat['isb_ps_hit_rate'] = 0.0
        stat['isb_sp_hit_rate'] = 0.0
        try:
            stat['isb_ps_hit_rate'] = float(stat['isb_on_chip_ps_hits']) / float(stat['isb_on_chip_ps_accesses'])
            stat['isb_sp_hit_rate'] = float(stat['isb_on_chip_sp_hits']) / float(stat['isb_on_chip_sp_accesses'])
        except:
            None

        stat['optgen_hit_rate'] = 0.0
        try:
            stat['optgen_hit_rate'] = float(stat['optgen_hits'])/float(stat['optgen_accesses'])
        except:
            None

        stat['triage_predict_rate'] = 0.0
        try:
            stat['triage_predict_rate'] = float(stat['triage_predict_count']) / float(stat['triage_trigger_count'])
        except:
            None

        if 'sample_optgen_hit_rate' not in stat:
            stat['sample_optgen_hit_rate'] = 0.0

        stat['triage_average_assoc'] = 0.0
        try:
            stat['triage_average_assoc'] = float(stat['triage_total_assoc']) / float(stat['triage_trigger_count'])
        except:
            None

        self.stat = stat
        self.error = error

        return error

    def add_stat(self, stat):
        self.weight += stat.weight
        if stat.error:
            self.error = True
        for item in stat.stat:
            if item not in self.stat:
                self.stat[item] = 0.0
            self.stat[item] += stat.stat[item] * stat.weight

    def divide_weight(self):
        for item in self.stat:
            self.stat[item] = self.stat[item]/self.weight

class Stats:
    def __init__(self, desc, stats_dir):
        self.desc = desc
        self.stats_dir = stats_dir
        self.acc_benchmark_list = []

    def parse_weight(self, weight_filename):
        weights = {}
        for benchmark in self.desc.benchmark_list:
            weights[benchmark] = 1.0
        with open(weight_filename) as weight_file:
            for line in weight_file:
                res = re.match('(\w+) (\d+(\.\d+)?)', line)
                if res != None:
                    weights[res.group(1)] = float(res.group(2))
        return weights

    def parse(self, eval_all):
        benchmark_list = self.desc.benchmark_list
        self.stat_list = {}
        weights = self.parse_weight(self.desc.weights_file)
        for benchmark in benchmark_list:
#            filepath = self.stats_dir + '/' + benchmark + ".txt"
            filepath = self.stats_dir + '/' + benchmark + ".stats"
            # XXX: a hacking way to determine whether a file is a stats file
            if os.path.isfile(filepath) and filepath.endswith('.stats'):
                self.stat_list[benchmark] = Stat(self.desc, filepath)
                error = self.stat_list[benchmark].parse()
                if benchmark in weights:
                    self.stat_list[benchmark].weight = weights[benchmark]
            else:
                print(filepath, "does not exist")

        for benchmark in benchmark_list:
            res = re.match("([a-zA-Z0-9]*)_([a-zA-Z0-9]*)", benchmark)
#            res = re.match("([a-zA-Z0-9._]*)-([0-9]*)B", benchmark)
            acc_benchmark = res.group(1)
            if acc_benchmark not in self.acc_benchmark_list:
                self.acc_benchmark_list.append(acc_benchmark)
                self.stat_list[acc_benchmark] = Stat(self.desc, None)
            assert acc_benchmark != None
            self.stat_list[acc_benchmark].add_stat(self.stat_list[benchmark])
        for acc_benchmark in self.acc_benchmark_list:
            #print(acc_benchmark, self.stat_list[acc_benchmark].weight)
            self.stat_list[acc_benchmark].divide_weight()


