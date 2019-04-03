#!/usr/bin/perl
if($#ARGV != 0){
    print "ERROR : Usage : perl get_misses.pl <dut>\n";
    exit;
}

our $dut_stats_file = shift;

my $dut_misses = compute_num_misses($dut_stats_file);

chomp($dut_misses);
print "$dut_misses";

sub compute_num_misses
{
    $stats_file = $_[0];   
    my $num_misses = 0;
    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;
        if ($line =~ m/LLC LOAD[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/) 
        {
            $num_misses = $num_misses + $3;
        }
        
        if ($line =~ m/LLC RFO[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/) 
        {
            $num_misses = $num_misses + $3;
        }
    }
    unless ( defined($num_misses) ) {
        print "ERROR problem with $stats_file\n";
        return $num_misses;
    }
    return $num_misses;
}

