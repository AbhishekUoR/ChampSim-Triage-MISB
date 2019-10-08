#!/lusr/bin/python3

import os, random

irregular_list = [ "astar_163B", "mcf_46B", "omnetpp_340B", "soplex_66B", "sphinx3_2520B", "gcc_13B", "xalancbmk_748B" ]

all_list = []

def read_list():
    with open("crc2_list.txt") as crc_list_file:
        for line in crc_list_file:
            all_list.append(line[:-1])

def generate(filename, bench_list, num_cores):
    with open(filename, 'w') as mix_file:
        for i in range(30):
            for core in range(num_cores):
                index = random.randint(0, len(bench_list)-1)
                mix_file.write(bench_list[index])
                mix_file.write(' ')
            mix_file.write('\n')

if __name__ == "__main__":
    read_list()
#    generate("16core_workloads.txt", all_list, 16)
#    generate("16core_irregular.txt", irregular_list, 16)
#    generate("8core_irregular.txt", irregular_list, 8)
#    generate("2core_workloads.txt", all_list, 2)
    generate("2core_irregular.txt", irregular_list, 2)
