#!/usr/bin/perl
if($#ARGV != 0){
    print "ERROR : Usage : perl get_misses.pl <dut>\n";
    exit;
}

our $dut_stats_file = shift;

compute_average_allocation($dut_stats_file);

sub compute_average_allocation
{
    $stats_file = $_[0];   
    my $total = 0;
    my $core0 = 0;
    my $core1 = 0;
    my $core2 = 0;
    my $core3 = 0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;
        if ($line =~ m/([\d\.]+),[\s\t]+([\d\.]+),[\s\t]+([\d\.]+),[\s\t]+([\d\.]+),/) 
        {
            if(($1+$2+$3+$4) >= 95)
            {
                $total = $total + 1;
            print "$line";
                $core0 = $core0 + $1;
                $core1 = $core1 + $2;
                $core2 = $core2 + $3;
                $core3 = $core3 + $4;
            }
        }
    }

    $core0 = $core0/$total;
    $core1 = $core1/$total;
    $core2 = $core2/$total;
    $core3 = $core3/$total;

    print "$core0, $core1, $core2, $core3\n"
}

