#!/usr/bin/perl
if($#ARGV != 0){
    print "ERROR : Usage : perl get_dram_traffic.pl <dut>\n";
    exit;
}

our $dut_stats_file = shift;

my $dut_dram_traffic = compute_dram_traffic($dut_stats_file);

chomp($dut_dram_traffic);
print "$dut_dram_traffic\n";

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

