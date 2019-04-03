#!/usr/bin/perl
if($#ARGV != 1){
    print "ERROR : Usage : perl speedup.pl <baseline> <dut>\n";
    exit;
}

our $baseline_stats_file = shift;
our $dut_stats_file = shift;

my $baseline_traffic = compute_traffic($baseline_stats_file);
my $dut_traffic = stms_traffic($dut_stats_file);

chomp($baseline_traffic);
chomp($dut_traffic);
#print "$baseline_traffic, $dut_traffic \n";
  
$traffic_increase = 100*($dut_traffic - $baseline_traffic)/$baseline_traffic;
#$speedup = 0.00001 if $speedup < 0.00001;

print "$traffic_increase \n";
sub compute_traffic
{
    $stats_file = $_[0];   
    my $traffic = 0.0;
    my $roi = 1;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/Region of Interest Statistics/)
        {
            $roi = 0;
        }
    
        if (($roi == 1) && ($line =~ m/LLC TOTAL[\s\t]+ACCESS:[\s\t]+[\d]+[\s\t]+HIT:[\s\t]+[\d]+[\s\t]+MISS:[\s\t]+([\d]+)/)) {
    #        print "$line\n";
            $traffic = $traffic + $1;
        }
    }
    unless ( defined($traffic) ) {
        print "ERROR problem with $stats_file\n";
        return $traffic;
    }
    return $traffic;
}

sub stms_traffic
{
    $stats_file = $_[0];   
    my $traffic = 0.0;
    my $roi = 1;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/Region of Interest Statistics/)
        {
            $roi = 0;
        }
        #STMS_nb_offchip_index_table_writes=116713 
        if (($roi == 1) && ($line =~ m/^Domino_nb_offchip/)) {
    #        print "$line\n";
            if( $line =~ m/=([\d]+)/) {
    #            print "$1\n";
                $traffic = $traffic + $1;
            }
        }
    }
    unless ( defined($traffic) ) {
        print "ERROR problem with $stats_file\n";
        return $traffic;
    }
    return $traffic;
}
