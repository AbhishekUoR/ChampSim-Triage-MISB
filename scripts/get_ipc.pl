#!/usr/bin/perl
if($#ARGV != 0){
    print "ERROR : Usage : perl get_ipc.pl <dut>\n";
    exit;
}

our $dut_stats_file = shift;

my $dut_ipc = compute_ipc($dut_stats_file);

chomp($dut_ipc);
print "$dut_ipc";

sub compute_ipc
{
    $stats_file = $_[0];   
    my $ipc = 0.0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/CPU 0 cumulative IPC: ([\d\.]+) instructions: [\d]+ cycles: ([\d]+)/) {
            $ipc = 0.0 + $1;
            last;
        }
    }
    unless ( defined($ipc) ) {
        print "ERROR problem with $stats_file\n";
        return $ipc;
    }
    return $ipc;
}
