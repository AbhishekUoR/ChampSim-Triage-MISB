#!/usr/bin/perl
if($#ARGV != 1){
    print "ERROR : Usage : perl opt_vs_sneakyopt.pl <baseline> <opt>\n";
    exit;
}

our $baseline_stats_file = shift;
our $dut_stats_file = shift;


my $num_accesses = 0;
my $num_accesses_line = `grep "OPTgen accesses" $dut_stats_file | tail -n 1`;
if ($num_accesses_line =~ m/OPTgen accesses:[\s\t]+([\d]+)/) {
    $num_accesses = $1;
}

my $baseline_miss_rate = compute_miss_rate($baseline_stats_file);
my $baseline_misses = $num_accesses * $baseline_miss_rate;
#my $baseline_tpa = compute_tpa($baseline_stats_file);
#print "$baseline_tpa\n";
#my $baseline_traffic = $num_accesses * $baseline_tpa;
my $baseline_traffic=`perl get_dram_traffic.pl $baseline_stats_file`;
#print "$baseline_traffic\n";

my $opt_misses = 0;
my $opt_line = `grep "OPTgen hits" $dut_stats_file | tail -n 1`;
#print $opt_line;
if ($opt_line =~ m/OPTgen hits:[\s\t]+([\d]+)/) {
    $opt_misses = $num_accesses - $1;
}
my $opt_traffic = 0;
my $opt_line = `grep "^Traffic:" $dut_stats_file | tail -n 1`;
if ($opt_line =~ m/Traffic:[\s\t]+([\d]+)/) {
    $opt_traffic = $1;
}
#print "$opt_traffic\n";

my $sneaky_opt_misses = 0;
my $sneaky_opt_line = `grep "OPTgen hits" $dut_stats_file | head -n 1`;
#print $sneaky_opt_line;
if ($sneaky_opt_line =~ m/OPTgen hits:[\s\t]+([\d]+)/) {
    $sneaky_opt_misses = $num_accesses - $1;
}
my $sneaky_opt_traffic = 0;
my $sneaky_opt_line = `grep "^Traffic:" $dut_stats_file | head -n 1`;
if ($sneaky_opt_line =~ m/Traffic:[\s\t]+([\d]+)/) {
    $sneaky_opt_traffic = $1;
}
#print "$sneaky_opt_traffic\n";

#print "$num_accesses: $baseline_misses $opt_misses $sneaky_opt_misses\n";

$opt_miss_reduction = 100*($baseline_misses - $opt_misses)/$baseline_misses;
$sneaky_opt_miss_reduction = 100*($baseline_misses - $sneaky_opt_misses)/$baseline_misses;
$opt_traffic_reduction = 100*($baseline_traffic - $opt_traffic)/$baseline_traffic;
$sneaky_opt_traffic_reduction = 100*($baseline_traffic - $sneaky_opt_traffic)/$baseline_traffic;
$sneaky_opt_vs_opt_miss = 100*($opt_misses - $sneaky_opt_misses)/$opt_misses;
$sneaky_opt_vs_opt_traffic = 100*($sneaky_opt_traffic - $opt_traffic)/$opt_traffic;

#print "$opt_miss_reduction, $sneaky_opt_miss_reduction\n";
print "$opt_traffic_reduction, $sneaky_opt_traffic_reduction\n";
#print "$sneaky_opt_vs_opt_miss, $sneaky_opt_vs_opt_traffic\n";

sub compute_miss_rate
{
    $stats_file = $_[0];   
    my $num_misses = 0;
    my $num_accesses = 0;
    my $roi = 0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/Region of Interest Statistics/)
        {
            $roi = 1;
        }
        if (($roi == 0) && ($line =~ m/LLC LOAD[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/))
        {
            $num_misses = $num_misses + $3;
            $num_accesses = $num_accesses + $1;
        }
        
        if (($roi == 0) && ($line =~ m/LLC RFO[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/)) 
        {
            $num_misses = $num_misses + $3;
            $num_accesses = $num_accesses + $1;
        }
    }

    #print "$num_accesses, $num_misses\n";
    $miss_rate = $num_misses/$num_accesses;
    if($miss_rate < 0.01) {
        $miss_rate = 0.01;
    }
    unless ( defined($miss_rate) ) {
        print "ERROR problem with $stats_file\n";
        return $miss_rate;
    }
 
    return $miss_rate;
}

sub compute_tpa
{
    $stats_file = $_[0];   
    my $traffic = 0;
    my $num_accesses = 0;
    my $tpa = 0.0;
    my $roi = 0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/Region of Interest Statistics/)
        {
            $roi = 1;
        }

        if (($roi == 0) && ($line =~ m/LLC LOAD[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/))
        {
            $traffic = $traffic + $3;
            $num_accesses = $num_accesses + $1;
        }

        if (($roi == 0) && ($line =~ m/LLC RFO[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/)) 
        {
            $traffic = $traffic + $3;
            $num_accesses = $num_accesses + $1;
        }

        if (($roi == 0) && ($line =~ m/LLC PREFETCH[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/)) 
        {
            $traffic = $traffic + $3;
        }


    }

    $tpa = 100*$traffic/$num_accesses;
    unless ( defined($tpa) ) {
        print "ERROR problem with $stats_file\n";
        return $tpa;
    }
    return $tpa;
}
