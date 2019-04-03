#!/usr/bin/perl
if($#ARGV != 2){
    print "ERROR : Usage : perl pick_best_config.pl <baseline> <sc_baseline> <mix>\n";
    exit;
}

our $baseline_dir = shift;
our $sc_baseline = shift;
our $mix = shift;

my @config_array = ("hawkeye_final_1B", "hawkeye_nodp_1B", "hawkeye_alldp_1B", "hawkeye_nodp_core0_1B", "hawkeye_nodp_core1_1B", "hawkeye_nodp_core2_1B", "hawkeye_nodp_core3_1B", "hawkeye_alldp_core0_1B", "hawkeye_alldp_core3_1B");

our $best_config = $config_array[0];
our $best_config_speedup = 0;

foreach my $config (@config_array) 
{ 
    my $dut_dir="$baseline_dir/../$config/";
    my $weighted_speedup=`perl get_4core_speedup.pl $baseline_dir $dut_dir $sc_baseline $mix`;

    #print "$config, $weighted_speedup\n";

    if ($weighted_speedup > $best_config_speedup) 
    {
        $best_config = $config;
        $best_config_speedup = $weighted_speedup;
    }
}

#print "$best_config, $best_config_speedup\n";
print "$best_config_speedup";
