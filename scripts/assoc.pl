#!/usr/bin/perl
if($#ARGV != 0){
    print "ERROR : Usage : perl assoc.pl <dut>\n";
    exit;
}

our $dut_stats_file = shift;

my $assoc = compute_assoc($dut_stats_file);

chomp($dut_traffic);

print "\n";

sub compute_assoc
{
    $stats_file = $_[0];   
    my $assoc = 0.0;
    my $total_assoc = 0.0;
    my $total_trigger = 0.0;
    my $roi = 0;
    my $cpu = 0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/Region of Interest Statistics/)
        {
            $roi = 1;
        }

        if (($roi == 1) && ($line =~ m/CPU ([\d]+)/)) {
            $cpu = $1;
        }

        if (($roi == 1) && ($line =~ m/total_assoc=([\d]+)/)) {
#            print "$line.\n";
            $total_assoc = $1;
            $assoc = $total_assoc / $total_trigger;
            print "$assoc ";
        }
        if (($roi == 1) && ($line =~ m/trigger_count=([\d]+)/)) {
#            print "$line.\n";
            $total_trigger = $1;
        }
    }
    unless ( defined($assoc) ) {
        print "ERROR problem with $stats_file\n";
        return $assoc;
    }
    return $assoc;
}
