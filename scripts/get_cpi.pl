#!/usr/bin/perl
if($#ARGV != 0){
    print "ERROR : Usage : perl get_cpi.pl <dut>\n";
    exit;
}

our $dut_stats_file = shift;

my $dut_cpi = compute_cpi($dut_stats_file);

chomp($dut_cpi);
print "$dut_cpi";

sub compute_cpi
{
    $stats_file = $_[0];   
    my $cpi = 0.0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/CPU 0 cumulative IPC: [\d\.]+ instructions: [\d]+ cycles: ([\d]+)/) {
            $cpi = 0.0 + $1;
            last;
        }
    }
    unless ( defined($cpi) ) {
        print "ERROR problem with $stats_file\n";
        return $cpi;
    }
    return $cpi;
}
