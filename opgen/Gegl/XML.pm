#!/usr/bin/perl -w

# Copyright 2000-2001 Jay Cox <jaycox@gimp.org>

# this code may be used under the terms of the GPL

package Gegl::XML;

use XML::XQL;
use XML::XQL::DOM;
use XML::DOM::ValParser;

use Gegl::XML::PointOp;


sub parse_file
  {
    my $filename = shift;
    my ($query, @result);
    my @ops;

    $parser = new XML::DOM::ValParser;
    # TODO: set_sgml_search_path(...); # Tell the parser where to find our DTD

    { # Parse xml file with our error handler temporarly installed.
      local $XML::Checker::FAIL = \&xml_error_handler;
      $doc = $parser->parsefile ($filename);
    }

    $query = new XML::XQL::Query (Expr => "PointOp");
    @result = $query->solve ($doc);

#    print "found ",  @result+0, " PointOp[s]!\n\n";
    @ops = ();
    my $i = 0;
    print "Warning, found no PointOps\n" unless @result;
    foreach (@result)
      {
	$ops[$i] =  Gegl::XML::PointOp::parse($_);
	$ops[$i]->{filename} = $filename;
	$i++;
      };
    return @ops;
  }


# General XML utility routines
sub xml_error_handler {
  my $code = shift;
  return if $code == 300; # 300 is a useless warning (unused ID)
  die XML::Checker::error_string ($code, @_) if $code < 300;
  XML::Checker::print_error ($code, @_);
}

sub query_multiple
  {
    my ($xml, $search) = @_;
    my ($query);
    $query = new XML::XQL::Query (Expr => $search);
    return $query->solve ($xml);
  }

sub query_single
  {
    my (@tmp);
    @tmp = query_multiple(@_);
    return $tmp[0] if (@tmp == 1);
    if (@tmp > 1)
      {
	print "query_single:: Error, ", @tmp+0," matches for ",$_[1], " found\n";
	return $tmp[0];
      };
  }

1; # package return code
