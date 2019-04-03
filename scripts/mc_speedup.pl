#!/usr/bin/perl
if($#ARGV != 1){
    print "ERROR : Usage : perl speedup.pl <baseline> <dut>\n";
    exit;
}

our $baseline_stats_file = shift;
our $dut_stats_file = shift;

my $baseline_ipc = compute_av_ipc($baseline_stats_file);
my $dut_ipc = compute_av_ipc($dut_stats_file);

chomp($baseline_ipc);
chomp($dut_ipc);
#print "$baseline_ipc, $dut_ipc \n";
  
$speedup = 100*($dut_ipc - $baseline_ipc)/$baseline_ipc;
#$speedup = 0.00001 if $speedup < 0.00001;

print "$speedup \n";

sub compute_av_ipc
{
    $stats_file = $_[0];   
    my $av_ipc = 0.0;

    for(my $i=0; $i<4; $i++)
    {
        my $core_ipc = compute_ipc($stats_file, $i);
        $av_ipc = $av_ipc + $core_ipc;
    #    print "$i, $core_ipc \n";
    }
    $av_ipc = $av_ipc/4;
    return $av_ipc;
}

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
