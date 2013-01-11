#!/usr/bin/perl

foreach $f (<*.cl>) {
  print $f . "\n";
  system("./cltostring.pl $f");
}
