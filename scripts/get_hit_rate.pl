#!/usr/bin/perl
if($#ARGV != 0){
    print "ERROR : Usage : perl get_hitrate.pl <dut>\n";
    exit;
}

our $dut_stats_file = shift;

my $dut_hitrate = compute_hitrate($dut_stats_file);

chomp($dut_hitrate);
print "$dut_hitrate";

sub compute_hitrate
{
    $stats_file = $_[0];   
    my $num_hits = 0;
    my $num_accesses = 0;
    my $roi = 0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/Region of Interest Statistics/)
        {
            $roi = 1;
        }
        if (($roi == 1) && ($line =~ m/LLC LOAD[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/))
        {
            $num_hits = $num_hits + $2;
            $num_accesses = $num_accesses + $1;
        }
        
        if (($roi == 1) && ($line =~ m/LLC RFO[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/)) 
        {
            $num_hits = $num_hits + $2;
            $num_accesses = $num_accesses + $1;
        }
    }

    $hit_rate = 100*$num_hits/$num_accesses;
    if ($hit_rate == 0) {
        $hit_rate = 0.01;
    }
    unless ( defined($hit_rate) ) {
        print "ERROR problem with $stats_file\n";
        return $hit_rate;
    }
    return $hit_rate;
}

