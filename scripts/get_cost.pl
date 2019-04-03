#!/usr/bin/perl
if($#ARGV != 0){
    print "ERROR : Usage : perl get_cost.pl <dut>\n";
    exit;
}

our $dut_stats_file = shift;

my $dut_cost = compute_cost($dut_stats_file);

chomp($dut_cost);
print "$dut_cost";

sub compute_cost
{
    $stats_file = $_[0];   
    my $cost = 0.0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/Total cost: ([\d]+)/) {
            $cost = 0.0 + $1;
            last;
        }
    }
    unless ( defined($cost) ) {
        print "ERROR problem with $stats_file\n";
        return $cost;
    }
    return $cost;
}
