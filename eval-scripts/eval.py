#!/usr/bin/python3

import os, sys, re, argparse
from config import Config
from desc import Descriptor
from output import CvsOutput, XlsxOutput

def init():
    global config, descriptor
    config = Config()
    descriptor = Descriptor(config.desc_path)

def evaluate(descriptor):
    for expr in descriptor.experiments:
        expr.evaluate(descriptor)

def write_output():
    cvs_output = CvsOutput(config, descriptor)
    cvs_output.write(descriptor.stats_dir)
    xlsx_output = XlsxOutput(config, descriptor)
    xlsx_output.write(descriptor.stats_dir)

def plot():
    #Implement
    None


def main():
    print("Initiate Eval Script")
    init()
    evaluate(descriptor)

    print("Writing Output...")
    write_output()
    print("Plotting chart...(Not Implemented Yet.)")
    plot()

if __name__ == "__main__":
    main()
