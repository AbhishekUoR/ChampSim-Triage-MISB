#!/usr/bin/perl
if($#ARGV != 1){
    print "ERROR : Usage : perl miss_reduction.pl <baseline> <dut>\n";
    exit;
}

our $baseline_stats_file = shift;
our $dut_stats_file = shift;

my $baseline_misses = compute_misses($baseline_stats_file);
my $dut_misses = compute_misses($dut_stats_file);

chomp($baseline_misses);
chomp($dut_misses);
#print "$baseline_misses, $dut_misses \n";
  
$miss_reduction = 100*($baseline_misses - $dut_misses)/$baseline_misses;
print "$miss_reduction \n";

sub compute_misses
{
    $stats_file = $_[0];   
    my $num_misses = 0;
    for(my $i=0; $i<4; $i++)
    {
        my $core_misses = compute_core_misses($stats_file, $i);
        $num_misses = $num_misses + $core_misses;
    #    print "$i, $core_ipc \n";
    }
    $av_ipc = $av_ipc/4;

    return $num_misses;
}

sub compute_core_misses
{
    $stats_file = $_[0];   
    $core = $_[1];
    my $num_hits = 0;
    my $num_accesses = 0;
    my $roi = 0;
    my $core_stats = 0;

    foreach (qx[cat $stats_file 2>/dev/null]) {
        $line = $_;

        if ($line =~ m/Region of Interest Statistics/)
        {
            $roi = 1;
        }
    
        if ($line =~ m/CPU ([\d]+)/)
        {
            if($1 == $core)
            {
                $core_stats = 1;
            }
            else
            {
                $core_stats = 0;
            }
        }
    
        if (($roi == 1) && ($core_stats == 1) && ($line =~ m/LLC LOAD[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/))
        {
            $num_hits = $num_hits + $2;
            $num_accesses = $num_accesses + $1;
        }
        
        if (($roi == 1) && ($core_stats == 1) && ($line =~ m/LLC RFO[\s\t]+ACCESS:[\s\t]+([\d]+)[\s\t]+HIT:[\s\t]+([\d]+)[\s\t]+MISS:[\s\t]+([\d]+)/))
        {
            $num_hits = $num_hits + $2;
            $num_accesses = $num_accesses + $1;
        }
    }

    return ($num_accesses - $num_hits);
}


