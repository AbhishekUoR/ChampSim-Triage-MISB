#!/usr/bin/perl
if($#ARGV != 3){
    print "ERROR : Usage : perl pick_best_config.pl <baseline> <hawkeye_baseline> <sc_baseline> <mix> \n";
    exit;
}

our $baseline_dir = shift;
our $hawkeye_baseline_dir = shift;
our $sc_baseline = shift;
our $mix = shift;

my $sub_trace_string = `sed -n '$mix''p' /u/akanksha/MyChampSim/ChampSim/sim_list/4core_workloads.txt`;
chomp ($sub_trace_string);
my @sub_trace_array = split / /, $sub_trace_string;

my $trace0=$sub_trace_array[0];
my $trace1=$sub_trace_array[1];
my $trace2=$sub_trace_array[2];
my $trace3=$sub_trace_array[3];

#my @config_array = ("hawkeye_final_1B", "hawkeye_nodp_1B", "hawkeye_alldp_1B");
my @config_array = ("hawkeye_final_1B", "hawkeye_nodp_1B", "hawkeye_alldp_1B", "hawkeye_nodp_core0_1B", "hawkeye_nodp_core1_1B", "hawkeye_nodp_core2_1B", "hawkeye_nodp_core3_1B", "hawkeye_alldp_core0_1B", "hawkeye_alldp_core3_1B");
my @config_useful = (0, 0, 0, 0, 0, 0, 0, 0);
my @config_hitrate = (0, 0, 0, 0, 0, 0, 0, 0);
my @config_bw = (0, 0, 0, 0, 0, 0, 0, 0);
my @config_speedup = (0, 0, 0, 0, 0, 0, 0, 0);
my @config_predicted_speedup = (0, 0, 0, 0, 0, 0, 0, 0);

our $hawkeye_baseline_file="$hawkeye_baseline_dir/mix"."$mix.txt";
our $baseline_used_bw = `perl get_used_dram_bw.pl $hawkeye_baseline_file`;
chomp($baseline_used_bw);
our $baseline_hits = 0;
our $bw_bound = 0;
if($baseline_used_bw > 50) {
        $bw_bound = 1;
}

for (my $i=0; $i < 4; $i++) 
{
    $baseline_hits = $baseline_hits + compute_hits($hawkeye_baseline_file, $i);
}

#print "$baseline_hits $baseline_used_bw \n";

our $counter = 0;
our $dram_latency = get_latency($mix);
#our $dram_latency = 35+1.88*$baseline_used_bw;
#print "$baseline_used_bw Baseline Latency: $dram_latency\n";

foreach my $config (@config_array) 
{
    my $dut_file="$baseline_dir/../$config/mix"."$mix.txt";
    my $dut_used_bw = `perl get_used_dram_bw.pl $dut_file`;
    chomp($dut_used_bw);
    my $dut_hits = 0;
    my @percore_predicted_improvement = (0, 0, 0, 0);
    my $predicted_improvement = 0;
    #my $dut_dram_latency = 35+1.88*$dut_used_bw;
    my $dut_dram_latency = $dram_latency;

    for (my $i=0; $i < 4; $i++) 
    {
        #my $core_dram_latency = get_percore_latency($mix, $i);   
        my $core_hitrate = compute_hits($dut_file, $i);
        my $core_baseline_hitrate = compute_hits($hawkeye_baseline_file, $i);
        $percore_predicted_improvement[$i] = (20*$core_baseline_hitrate + $dram_latency*(1-$core_baseline_hitrate))/(20*$core_hitrate + ($dram_latency+3*($dut_used_bw - $baseline_used_bw)*abs($dut_used_bw - $baseline_used_bw))*(1-$core_hitrate));
        #$percore_predicted_improvement[$i] = (20*$core_baseline_hitrate + $dram_latency*(1-$core_baseline_hitrate))/(20*$core_hitrate + (($dram_latency*(100+(($dut_used_bw - $baseline_used_bw)*abs($dut_used_bw - $baseline_used_bw)))/100)*(1-$core_hitrate)));
        #$percore_predicted_improvement[$i] = (20*$core_baseline_hitrate + $dram_latency*(1-$core_baseline_hitrate))/(20*$core_hitrate + ($dram_latency)*(1-$core_hitrate));
        #print "$config, $i, $core_hitrate, $percore_predicted_improvement[$i] $dut_used_bw $dut_dram_latency\n";
        $predicted_improvement += $percore_predicted_improvement[$i];
        $dut_hits = $dut_hits + compute_hits($dut_file, $i);
    #$a = (20*$core_baseline_hitrate + $dram_latency*(1-$core_baseline_hitrate));
    #$b = (20*$core_hitrate + ($dram_latency+2*($dut_used_bw - $baseline_used_bw)*abs($dut_used_bw - $baseline_used_bw))*(1-$core_hitrate));
        print "    $core_hitrate $core_baseline_hitrate $dut_dram_latency $percore_predicted_improvement[$i]\n";
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

    $config_speedup[$counter] = `perl get_4core_speedup.pl $baseline_dir $baseline_dir/../$config $sc_baseline $mix`;
    $config_predicted_speedup[$counter] = $predicted_improvement;

    #print "Old: $config \t$config_useful[$counter] \t$config_hitrate[$counter] \t$config_bw[$counter] \t$config_slope[$counter] \t-- \t$config_speedup[$counter]\n";

    my $aggressive_latency = ($dram_latency+2*($dut_used_bw - $baseline_used_bw)*abs($dut_used_bw - $baseline_used_bw));
    print "$config_array[$counter]: $predicted_improvement \t$dut_used_bw \t$dut_dram_latency \t$aggressive_latency \t$config_hitrate[$counter] -- \t$config_speedup[$counter]\n";
    $counter++;
}

our $counter = 0;
our $best_speedup = 0;
our $best_speedup_config = $config_array[0];

#The main algorithm
foreach my $config (@config_array) 
{
    if($config_speedup[$counter] > $best_speedup) {
        $best_speedup = $config_speedup[$counter];
        $best_speedup_config = $config;
    }
    $counter++; 
}

our $best_config = "hawkeye_final_1B";
our $max_predicted_speedup = 0;
$counter = 0;
my $bw_delta = 0;

foreach my $config (@config_array) 
{
    if($config_predicted_speedup[$counter] > $max_predicted_speedup) {
        $best_config = $config;
        $max_predicted_speedup = $config_predicted_speedup[$counter];
        $bw_delta = $config_bw[$counter];
    }
    $counter++;
}

my $best_config_speedup = `perl get_4core_speedup.pl $baseline_dir $baseline_dir/../$best_config $sc_baseline $mix`;
print "New: $mix, $best_config $best_config_speedup -- $best_speedup_config $best_speedup\n";
#print "$bw_delta\n";
#print "$best_config_speedup";
print "$best_speedup";

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

sub get_percore_latency
{
    $mix_no = $_[0];
    $core = $_[1];
    $file = "/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC/hawkeye_simple_prefetch_percorelatency/mix"."$mix_no".".txt";
    #$line = `grep "latency" $file`;
    $latency = 200;
    foreach (qx[grep "latency" $file 2>/dev/null]) 
    {
        $line = $_;
        if ($line =~ m/Average latency Core ([\d]+): ([\d\.]+)/)
        {   
            $line_core = $1;
            if($line_core == $core)
            {
                $latency = $2;
                $last;
            }
        }
    }

    #print "$latency\n";

    return $latency;
}


sub get_latency
{
    $mix_no = $_[0];
    $file = "/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC/hawkeye_simple_prefetch_hawkeyegen/mix"."$mix_no".".txt";
    $line = `grep "latency" $file`;
    $latency = 200;
    if ($line =~ m/Average latency: ([\d\.]+)/)
    {   
        $latency = $1;
    }
    else
    {
        print "Error\n";
        exit(0);
    }

    #print "$latency\n";

    return $latency;
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
        $hit_rate = $num_hits/$num_accesses;
    }
    if($hit_rate < 0.01) {
        $hit_rate = 0.01;
    }
    unless ( defined($hit_rate) ) {
        print "ERROR problem with $stats_file\n";
        return $$num_hits;
    }
    return $hit_rate;
}
