##!/usr/bin/python3

import xlsxwriter

class CvsOutput:
    def __init__(self, config, desc):
        self.config = config
        self.desc = desc

    def write(self, output_path):
        print("Output csv to: ", output_path)
        print("Output Stats: ", self.desc.stats)
        # Example: ipc.csv
        # , gcc, mcf, astar, average
        # noprefetch, 1.0, 2.0, 3.0, 2.0
        # misb, 1.0, 2.0, 3.0, 2.0
        for item in self.desc.stats:
            output_filepath = output_path + "/" + item + ".csv"
            with open(output_filepath, 'w') as output_file:
                output_file.write(',')
                #for benchmark in self.desc.experiments[0].stats.stat_list:
                #for benchmark in self.desc.benchmark_list:
                for benchmark in self.desc.experiments[0].stats.acc_benchmark_list:
                    output_file.write(benchmark)
                    output_file.write(',')
                output_file.write('\n')
                for experiment in self.desc.experiments:
                    output_file.write(experiment.name)
                    output_file.write(',')
                    #for benchmark in experiment.stats.stat_list:
                    #for benchmark in self.desc.benchmark_list:
                    for benchmark in self.desc.experiments[0].stats.acc_benchmark_list:
                        stat = experiment.stats.stat_list[benchmark]
                        if stat.error:
                            # just put 0 as error mark
                            res = 0
                        else:
                            res = stat.stat[item]
                        #print(item, experiment.name, benchmark, res)
                        output_file.write(str(res))
                        output_file.write(',')
                    output_file.write('\n')

class XlsxOutput:
    def __init__(self, config, desc):
        self.config = config
        self.desc = desc

    def write(self, output_path):
        print("Output xlsx to: ", output_path+"/result.xlsx")
        print("Output Stats: ", self.desc.stats)

        book = xlsxwriter.Workbook(output_path+"/result.xlsx")
        # TODO Make this configurable

        for item in self.desc.stats:
            worksheet = book.add_worksheet(item)
            row = col = 1
            for benchmark in self.desc.experiments[0].stats.acc_benchmark_list:
                col += 1
                worksheet.write(row, col, benchmark)
            for experiment in self.desc.experiments:
                row += 1
                col = 1
                worksheet.write(row, col, experiment.name)
                for benchmark in self.desc.experiments[0].stats.acc_benchmark_list:
                    col += 1
                    stat = experiment.stats.stat_list[benchmark]
                    if stat.error:
                        res = 0
                    else:
                        res = stat.stat[item]
                    #print(item, experiment.name, benchmark, res)
                    worksheet.write(row, col, res)
        book.close()

