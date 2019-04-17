#!/usr/bin/python3

import os, sys, re, argparse
from config import Config
from desc import Descriptor
from output import CvsOutput, XlsxOutput

def init():
    global config, descriptor
    config = Config()
    descriptor = Descriptor(config.desc_path)

def launch(descriptor):
    if not os.path.exists(descriptor.stats_dir):
        os.mkdir(descriptor.stats_dir)
    for expr in descriptor.experiments:
        expr.launch(descriptor, descriptor.stats_dir)

def main():
    print("Initiate Launch Script")
    init()
    print("Launching Experiment...")
    launch(descriptor)

if __name__ == "__main__":
    main()
