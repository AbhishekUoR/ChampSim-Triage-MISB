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

    my $core0_opt_line = `grep \"Core 0\" $stats_file`;
    my $core1_opt_line = `grep \"Core 1\" $stats_file`;
    my $core2_opt_line = `grep \"Core 2\" $stats_file`;
    my $core3_opt_line = `grep \"Core 3\" $stats_file`;

    my @core0_opt = split(/\,/, $core0_opt_line);
    my @core1_opt = split(/\,/, $core1_opt_line);
    my @core2_opt = split(/\,/, $core2_opt_line);
    my @core3_opt = split(/\,/, $core3_opt_line);

#    print "$#core0_opt $#core1_opt $#core2_opt $#core3_opt\n";
    for my $i (1 .. $#core0_opt) {
#        print "$core0_opt[$i] $core1_opt[$i] $core2_opt[$i] $core3_opt[$i]\n";
        if(($core0_opt[$i] + $core1_opt[$i] + $core2_opt[$i] + $core3_opt[$i]) >= 85)
        {
            $total = $total + 1;
#            print "$line";
            $core0 = $core0 + $core0_opt[$i];
            $core1 = $core1 + $core1_opt[$i];
            $core2 = $core2 + $core2_opt[$i];
            $core3 = $core3 + $core3_opt[$i];
        }
    }

    $core0 = 100*($core0/$total)/87;
    $core1 = 100*($core1/$total)/87;
    $core2 = 100*($core2/$total)/87;
    $core3 = 100*($core3/$total)/87;

    print "$core0, $core1, $core2, $core3\n"
}

