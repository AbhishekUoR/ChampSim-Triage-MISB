#!/usr/bin/perl
if($#ARGV != 1){
    print "ERROR : Usage : perl get_hitrate.pl <dut> <coreid>\n";
    exit;
}

our $dut_stats_file = shift;
our $coreid = shift;

my $dut_hitrate = compute_hitrate($dut_stats_file, $coreid);

chomp($dut_hitrate);
print "$dut_hitrate";

sub compute_hitrate
{
    $stats_file = $_[0];   
    $core = $_[1];
    my $hit_rate = 0.0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/OPTgen hit rate for core ([\d]+): ([\d\.]+)/)
        {
            if($1 == $core) {
                $hit_rate = $2;
                break;
            }
        }
    }

    unless ( defined($hit_rate) ) {
        print "ERROR problem with $stats_file\n";
        return $hit_rate;
    }
    return $hit_rate;
}

