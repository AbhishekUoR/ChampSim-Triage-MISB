#!/usr/bin/perl
if($#ARGV != 3){
    print "ERROR : Usage : perl pick_best_config.pl <baseline> <hawkeye_baseline> <sc_baseline> <mix>\n";
    exit;
}

our $baseline_dir = shift;
our $hawkeye_baseline_dir = shift;
our $sc_baseline = shift;
our $mix = shift;

#print "$baseline_dir $sc_baseline $mix\n";
my $sub_trace_string = `sed -n '$mix''p' /u/akanksha/MyChampSim/ChampSim/sim_list/4core_workloads.txt`;
chomp ($sub_trace_string);
my @sub_trace_array = split / /, $sub_trace_string;

my $trace0=$sub_trace_array[0];
my $trace1=$sub_trace_array[1];
my $trace2=$sub_trace_array[2];
my $trace3=$sub_trace_array[3];

my @config_array = ("hawkeye_final_1B", "hawkeye_nodp_1B", "hawkeye_alldp_1B", "hawkeye_nodp_core0_1B", "hawkeye_nodp_core1_1B", "hawkeye_nodp_core2_1B", "hawkeye_nodp_core3_1B", "hawkeye_alldp_core0_1B", "hawkeye_alldp_core3_1B");
my @config_useful = (0, 0, 0, 0, 0, 0, 0, 0);
my @config_hitrate = (0, 0, 0, 0, 0, 0, 0, 0);
my @config_bw = (0, 0, 0, 0, 0, 0, 0, 0);
my @config_slope = (0, 0, 0, 0, 0, 0, 0, 0);
my @config_speedup = (0, 0, 0, 0, 0, 0, 0, 0);

our $hawkeye_baseline_file="$hawkeye_baseline_dir/mix"."$mix.txt";
our $baseline_used_bw = `perl get_used_dram_bw.pl $hawkeye_baseline_file`;
our $baseline_hits = 0;
our $bw_bound = 0;
if($baseline_used_bw > 50) {
        $bw_bound = 1;
}

for (my $i=0; $i < 4; $i++) 
{
    $baseline_hits = $baseline_hits + compute_hits($hawkeye_baseline_file, $i);
}

print "$baseline_hits $baseline_used_bw \n";

our $counter = 0;

foreach my $config (@config_array) 
{
    my $dut_file="$baseline_dir/../$config/mix"."$mix.txt";
#    print "$dut_file \n";
    my $dut_used_bw = `perl get_used_dram_bw.pl $dut_file`;
    my $dut_hits = 0;
    for (my $i=0; $i < 4; $i++) 
    {
        $dut_hits = $dut_hits + compute_hits($dut_file, $i);
    }

    if($dut_used_bw == 0) {
        $counter++;
        next;
    }
    my $useful = 0;
    if($bw_bound == 1) #BW-bound
    {
        if($dut_used_bw < $baseline_used_bw) {
            $useful = 1;
        }
    }   
    else
    {
        if($dut_hits > $baseline_hits) {
            $useful = 1;
        }
    }
    $config_useful[$counter] = $useful;
    $config_hitrate[$counter] = ($dut_hits - $baseline_hits);
    $config_bw[$counter] = ($dut_used_bw - $baseline_used_bw);
    if($dut_hits != $baseline_hits) {
        $config_slope[$counter] = 1000000 * (($dut_used_bw - $baseline_used_bw)/($dut_hits - $baseline_hits));
    }
    else {
        $config_slope[$counter] = 10000;
    }

    $config_speedup[$counter] = `perl get_4core_speedup.pl $baseline_dir $baseline_dir/../$config $sc_baseline $mix`;

#    print "$config $useful ".($dut_hits - $baseline_hits)." ".($dut_used_bw - $baseline_used_bw)."\n";
    print "$config \t$config_useful[$counter] \t$config_hitrate[$counter] \t$config_bw[$counter] \t$config_slope[$counter] \t-- \t$config_speedup[$counter]\n";
    $counter++;
}

our $best_config_bw = $config_array[0];
our $best_config_hitrate = $config_array[0];
our $highest_traffic_reduction = $config_bw[0];
our $highest_miss_reduction = $config_hitrate[0];
our $counter = 0;
our $best_speedup = 0;
our $best_speedup_config = $config_array[0];

#print "$best_config_bw $best_config_hitrate \n";
#print "Choosing $bw_bound\n";
#The main algorithm
foreach my $config (@config_array) 
{
    if($config_speedup[$counter] > $best_speedup) {
        $best_speedup = $config_speedup[$counter];
        $best_speedup_config = $config;
    }
    if($config_useful[$counter] == 1) 
    {
#        print "$config $config_slope[$counter]\n";
        #vals are negative - lower is better
        if($config_bw[$counter] < $highest_traffic_reduction)
        {
            $highest_traffic_reduction = $config_bw[$counter];
            $best_config_bw = $config;
        }
    #    print "$config_bw[$counter] $highest_traffic_reduction $best_config_bw\n"; 
    
        if($config_hitrate[$counter] > $highest_miss_reduction) 
        {
            $highest_miss_reduction = $config_hitrate[$counter];
            $best_config_hitrate = $config;
        } 
#        print "$config_hitrate[$counter] $highest_miss_reduction $best_config_hitrate\n"; 
    }

    $counter++; 
}

our $best_config = "hawkeye_final_1B";
our $best_config_speedup = 0;
#print "$best_config_bw $best_config_hitrate \n";

if($bw_bound == 1) {
    $best_config = $best_config_bw;
}
else {
    $best_config = $best_config_hitrate;
}

#print "$mix, $best_config\n";
my $best_config_speedup = `perl get_4core_speedup.pl $baseline_dir $baseline_dir/../$best_config $sc_baseline $mix`;
#print "$mix, $best_config $best_config_speedup -- $best_speedup_config $best_speedup\n";
print "$best_config_speedup";


sub compute_dram_traffic
{
    $stats_file = $_[0];   
    my $dram_traffic = 0;
    my $roi = 1;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/Region of Interest Statistics/)
        {
            $roi = 0;
        }
        if (($roi == 1) && ($line =~ m/LLC TOTAL[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/))
        {
            $dram_traffic = $dram_traffic + $3;
        }
    }

    unless ( defined($dram_traffic) ) {
        print "ERROR problem with $stats_file\n";
        return $dram_traffic;
    }
    return $dram_traffic;
}

sub compute_hits
{
    $stats_file = $_[0];   
    $core = $_[1];
    my $num_hits = 0;
    my $num_accesses = 0;
    my $roi = 0;
    my $core_stats = 0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/Region of Interest Statistics/)
        {
            $roi = 1;
        }
    
        if ($line =~ m/CPU ([\d]+)/)
        {
            if($1 == $core)
            {
                $core_stats = 1;
            }
            else
            {
                $core_stats = 0;
            }
        }
    
        if (($roi == 1) && ($core_stats == 1) && ($line =~ m/LLC LOAD[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/))
        {
            $num_hits = $num_hits + $2;
            $num_accesses = $num_accesses + $1;
        }
        
        if (($roi == 1) && ($core_stats == 1) && ($line =~ m/LLC RFO[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/))
        {
            $num_hits = $num_hits + $2;
            $num_accesses = $num_accesses + $1;
        }
    }

    if($num_accesses == 0) {
        $hit_rate = 0;
    }
    else {
        $hit_rate = 100*$num_hits/$num_accesses;
    }
    if($hit_rate < 0.01) {
        $hit_rate = 0.01;
    }
    unless ( defined($hit_rate) ) {
        print "ERROR problem with $stats_file\n";
        return $$num_hits;
    }
    return $num_hits;
}
