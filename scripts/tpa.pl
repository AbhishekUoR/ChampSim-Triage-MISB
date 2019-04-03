#!/usr/bin/perl
if($#ARGV != 0){
    print "ERROR : Usage : perl speedup.pl <dut>\n";
    exit;
}

our $dut_stats_file = shift;
my $dut_tpa = compute_tpa($dut_stats_file);

chomp($dut_tpa);
print "$dut_tpa \n";

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

        if (($roi == 1) && ($line =~ m/LLC TOTAL[\s\t]+ACCESS:[\s\t]+[\d]+[\s\t]+HIT:[\s\t]+[\d]+[\s\t]+MISS:[\s\t]+([\d]+)/)) 
        {
            $traffic = $traffic + $1
        }
        if (($roi == 1) && ($line =~ m/LLC LOAD[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/))
        {
            $num_accesses = $num_accesses + $1;
        }

        if (($roi == 1) && ($line =~ m/LLC RFO[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/)) 
        {
            $num_accesses = $num_accesses + $1;
        }

    }

    $tpa = 100*$traffic/$num_accesses;
    unless ( defined($tpa) ) {
        print "ERROR problem with $stats_file\n";
        return $tpa;
    }
    return $tpa;
}
