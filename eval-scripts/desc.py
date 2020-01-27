#!/usr/bin/python3

import os, json
from experiment import Experiment

class Descriptor:
    def __init__(self, path):
        self.path = path
        with open(path) as desc_file:
            self.json_object = json.load(desc_file)
        self.eval_all = self.json_object['eval_all_benchmarks']
        self.stats = self.json_object['stats']
        self.stats_dir = self.json_object['stats_dir']
        self.benchmark_list = self.json_object['benchmark_list']
        self.warmup_insns = self.json_object['warmup_insns']
        self.champsim_dir = self.json_object['champsim_dir']
        self.trace_dir = self.json_object['trace_dir']
        self.weights_file = self.json_object['weights_file']
        self.simulation_insns = self.json_object['simulation_insns']
        self.experiments = [Experiment(exp) for exp in
                self.json_object['experiments']]

