#!/usr/bin/perl
if($#ARGV != 1){
    print "ERROR : Usage : perl cost_reduction.pl <baseline> <dut>\n";
    exit;
}

our $baseline_stats_file = shift;
our $dut_stats_file = shift;

my $baseline_cost = compute_cost($baseline_stats_file);
my $dut_cost = compute_cost($dut_stats_file);

chomp($baseline_cost);
chomp($dut_cost);
#print "$baseline_cost, $dut_cost \n";
  
$cost_reduction = 100*($baseline_cost - $dut_cost)/$baseline_cost;
print "$cost_reduction \n";

sub compute_cost
{
    $stats_file = $_[0];   
    my $cost = 0;
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

