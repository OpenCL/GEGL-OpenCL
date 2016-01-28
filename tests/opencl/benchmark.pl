#!/usr/bin/perl
use strict;
use warnings;
use XML::Twig;
use File::Basename;

# Usage
# perl benchmark.pl [operation-file] <cpu|accelerator|gpu|no> [iterations]

# Arguments
my $operation    =  $ARGV[0]; # 'stretch-contrast.xml'
my $cl_device    =  $ARGV[1]; # see above
my $it   =  $ARGV[2];

# Write to a null image so as to not bring up the gtk viewer
my $output = '/dev/null/a.png';
my $executable = 'gegl ';
my $op_name = fileparse($operation, '\..*');

# Read XML from file
my $twig = new XML::Twig;
$twig->parsefile($operation);

my $op_xml = $twig->root->sprint;

# If no cl_device is specified, assume OpenCL is disabled
$ENV{"GEGL_DEBUG_TIME"} = 1;
$ENV{"GEGL_USE_OPENCL"} = $cl_device;

get_avg_time();

# Process GEGL_DEBUG_TIME output for stats of interest
sub parse_debug_time {
    my $tot_time = 0;
    my $process_time = 0;
    my $op_time = 0;
    while (my $line = shift @_) {
        #print $line; #DEBUG

        # Regex that captures float variables
        my $float_var = qr/[0-9]+\.[0-9]+/;

        # Capture total time, process %, and operation %
        if ($line =~ /(${float_var})s/) {
            $tot_time = $1;
        } elsif ($line =~ /process\s+(${float_var})%/) {
            $process_time = $tot_time*$1/100;
        } elsif ($line =~ /$op_name\s+(${float_var})%/) {
            $op_time = $process_time*$1/100;
            last;
        }
    }
    #print "Intermediate Total = ", $tot_time, "s\n";
    #print "Intermediate Process = ", $process_time, "s\n";
    #print "Intermediate Operation = ", $op_time, "s\n"; #DEBUG

    return ($tot_time, $process_time, $op_time);
}

sub get_avg_time {
    my $avg_tot_time = 0;
    my $avg_process_time = 0;
    my $avg_op_time = 0;
    for(my $i = 0; $i < $it; ++$i) {
        my @timing = `$executable -x '$op_xml' -o $output`;
        my ($tot_time, $process_time, $op_time) = parse_debug_time(@timing);
        $avg_tot_time       += $tot_time;
        $avg_process_time   += $process_time;
        $avg_op_time        += $op_time;
    }

    print "Operation = ", $avg_op_time/$it, " seconds\n";

    return ($avg_tot_time, $avg_process_time, $avg_op_time);
}