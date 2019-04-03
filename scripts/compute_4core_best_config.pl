#!/usr/bin/perl
if($#ARGV != 2){
    print "ERROR : Usage : perl pick_best_config.pl <baseline> <hawkeye baseline> <sc_baseline>\n";
    exit;
}

our $baseline_dir = shift;
our $hawkeye_dir = shift;
our $sc_baseline = shift;

my $weight=(1/100);
my $speedup_average=1;

for (my $i=1; $i <= 100; $i++) 
{
    #print "perl pick_best_config.pl $baseline_dir $hawkeye_baseline_dir $sc_baseline $i\n";
#    my $best_speedup = `perl pick_best_config.pl $baseline_dir $sc_baseline $i`;
    my $best_speedup = `perl predict_best_config_v2_percore.pl $baseline_dir $hawkeye_dir $sc_baseline $i`;

    print "$i, $best_speedup\n";

    $speedup_average = $speedup_average * (100+$best_speedup) ** $weight;
}

printf "Average: %.6f\n", ($speedup_average);
