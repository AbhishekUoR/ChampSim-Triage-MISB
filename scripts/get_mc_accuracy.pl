#!/usr/bin/perl
if($#ARGV != 2){
    print "ERROR : Usage : perl get_mc_accuracy.pl <folder> <mixnum> <id>\n";
    exit;
}

our $dut_dir = shift;
our $mix = shift;
our $benchmark_id = shift;

my $sub_trace_string = `sed -n '$mix''p' /u/akanksha/MyChampSim/ChampSim/sim_list/4core_ml_number.txt`;
chomp ($sub_trace_string);
my @sub_trace_array = split / /, $sub_trace_string;

my $trace0=$sub_trace_array[0];
my $trace1=$sub_trace_array[1];
my $trace2=$sub_trace_array[2];
my $trace3=$sub_trace_array[3];

my $core = 8;

for (my $i=0; $i < 4; $i++) {
    if($sub_trace_array[$i] eq $benchmark_id)
    {
        $core = $i;
        last;
    }
}


my $file="$dut_dir"."/"."mix"."$mix".".txt";

my $accuracy_line=`grep "Reuse Accuracy for core $core" $file`;
chomp($accuracy_lines);

my $accuracy;
if ($accuracy_line =~ m/Reuse Accuracy for core [\d]: ([\d\.]+)/)
{
    $accuracy = $1;
}

print "$accuracy\n";
