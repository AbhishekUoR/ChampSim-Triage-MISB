#!/usr/bin/perl
if($#ARGV != 3){
    print "ERROR : Usage : perl get_4core_speedup.pl <baseline> <dut> <sc_baseline> <mix>\n";
    exit;
}

our $baseline_dir = shift;
our $dut_dir = shift;
our $sc_baseline = shift;
our $mix = shift;

my $sub_trace_string = `sed -n '$mix''p' /u/akanksha/MyChampSim/ChampSim/sim_list/4core_workloads.txt`;
chomp ($sub_trace_string);
my @sub_trace_array = split / /, $sub_trace_string;

my $trace0=$sub_trace_array[0];
my $trace1=$sub_trace_array[1];
my $trace2=$sub_trace_array[2];
my $trace3=$sub_trace_array[3];

my $baseline_file="$baseline_dir/mix"."$mix.txt";
my $dut_file="$dut_dir/mix"."$mix.txt";
#    print $baseline_file $dut_file;

my $sc_file0="$sc_baseline/$trace0".".txt";
my $sc_file1="$sc_baseline/$trace1".".txt";
my $sc_file2="$sc_baseline/$trace2".".txt";
my $sc_file3="$sc_baseline/$trace3".".txt";

#    print "$sc_file0 $sc_file1 $sc_file2 $sc_file3\n";

my $core0_dut_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_percore_ipc.pl $dut_file 0`;
my $core1_dut_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_percore_ipc.pl $dut_file 1`;
my $core2_dut_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_percore_ipc.pl $dut_file 2`;
my $core3_dut_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_percore_ipc.pl $dut_file 3`;

my $core0_baseline_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_percore_ipc.pl $baseline_file 0`;
my $core1_baseline_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_percore_ipc.pl $baseline_file 1`;
my $core2_baseline_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_percore_ipc.pl $baseline_file 2`;
my $core3_baseline_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_percore_ipc.pl $baseline_file 3`;

my $core0_sc_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_ipc.pl $sc_file0`;
my $core1_sc_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_ipc.pl $sc_file1`;
my $core2_sc_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_ipc.pl $sc_file2`;
my $core3_sc_ipc=`perl /u/akanksha/MyChampSim/ChampSim/scripts/get_ipc.pl $sc_file3`;

#    print "$core0_sc_ipc $core1_sc_ipc $core2_sc_ipc $core3_sc_ipc\n";

   # echo "$core0_baseline_ipc $core0_sc_ipc" 
$a = ($core0_dut_ipc/$core0_sc_ipc);
$b = ($core1_dut_ipc/$core1_sc_ipc);
$c = ($core2_dut_ipc/$core2_sc_ipc);
$d = ($core3_dut_ipc/$core3_sc_ipc);

#print "$a $b $c $d\n";

$a = ($core0_baseline_ipc/$core0_sc_ipc);
$b = ($core1_baseline_ipc/$core1_sc_ipc);
$c = ($core2_baseline_ipc/$core2_sc_ipc);
$d = ($core3_baseline_ipc/$core3_sc_ipc);

#print "$a $b $c $d\n";


my $weighted_dut=($core0_dut_ipc/$core0_sc_ipc) + ($core1_dut_ipc/$core1_sc_ipc) + ($core2_dut_ipc/$core2_sc_ipc) + ($core3_dut_ipc/$core3_sc_ipc);
my $weighted_baseline=($core0_baseline_ipc/$core0_sc_ipc) + ($core1_baseline_ipc/$core1_sc_ipc) + ($core2_baseline_ipc/$core2_sc_ipc) + ($core3_baseline_ipc/$core3_sc_ipc);

my $weighted_speedup=100*(($weighted_dut/$weighted_baseline)-1);

print $weighted_speedup;
