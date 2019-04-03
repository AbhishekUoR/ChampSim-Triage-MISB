#!/usr/bin/perl
if($#ARGV != 1){
    print "ERROR : Usage : perl get_ipc.pl <dut> <core id>\n";
    exit;
}

our $dut_stats_file = shift;
our $coreid = shift;

my $dut_ipc = compute_ipc($dut_stats_file, $coreid);

chomp($dut_ipc);
print "$dut_ipc\n";

sub compute_ipc
{
    $stats_file = $_[0];   
    $core = $_[1];

    #print "$stats_file $core\n";

    my $ipc = 0.0;
    my $roi = 0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/Region of Interest Statistics/)
        {
            $roi = 1;
        }
    
        if (($roi == 1) && ($line =~ m/CPU ([\d]+) cumulative IPC: ([\d\.]+) instructions: [\d]+ cycles: ([\d]+)/)) {
            if ($core == $1)        
            {
                $ipc = 0.0 + $2;
                last;
            }
        }
    }
    unless ( defined($ipc) ) {
        print "ERROR problem with $stats_file\n";
        return $ipc;
    }
    return $ipc;
}
