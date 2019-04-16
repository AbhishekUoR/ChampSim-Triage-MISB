#!/usr/bin/python3

import argparse, os

class Config:
    def __init__(self):
        parser = argparse.ArgumentParser(description=
            "This script is used to evaluate results from Champsim.")
        parser.add_argument('-d', '--desc', required = True, help="Descriptor JSON File Path.")
        parser.add_argument('-o', '--output', help="Results output directory.")

        args = parser.parse_args()

        self.desc_path = args.desc
#        if (args.output):
#            self.output_path = args.output
#        else:
#            self.output_path = os.path.dirname(self.desc_path)

