#!/usr/bin/perl

use File::Basename;

#input file
$path = $ARGV[0];

open(INPUT_FILE, ${path})
     or die "Couldn't open ${path} for reading.";

open(OUTPUT_FILE, ">${path}.h");

($name,$dir,$ext) = fileparse($path,'\..*');

$name =~ s/-/_/g;
$name =~ s/:/_/g;

print OUTPUT_FILE "static const char* ${name}_cl_source =\n";

while(<INPUT_FILE>)
{
        ~s/\n//;
        printf( OUTPUT_FILE "\"%-78s\\n\"\n", $_);
}
print OUTPUT_FILE ";\n";

close(INPUT_FILE);
close(OUTPUT_FILE);
