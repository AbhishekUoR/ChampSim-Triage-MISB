#!/usr/bin/perl
if($#ARGV != 1){
    print "ERROR : Usage : perl get_hitrate.pl <dut> <coreid>\n";
    exit;
}

our $dut_stats_file = shift;
our $coreid = shift;

my $dut_hitrate = compute_hitrate($dut_stats_file, $coreid);

chomp($dut_hitrate);
print "$dut_hitrate\n";

sub compute_hitrate
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

#    return $num_hits;
    $hit_rate = 100*$num_hits/$num_accesses;
    if($hit_rate < 0.01) {
        $hit_rate = 0.01;
    }
    unless ( defined($hit_rate) ) {
        print "ERROR problem with $stats_file\n";
        return $hit_rate;
    }
    return $hit_rate;
}

