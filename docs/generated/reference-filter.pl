#!/usr/bin/perl -w
# Stupid index scanner :b
# Copyright (C) 2003 Alper Ersoy
#
# This code is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
use strict;

sub make_xref {
    my $index   = shift;
    my $item    = shift;
    my $prefix  = shift;
    my $postfix = shift;

    unless (ref $index eq 'HASH') {
	print STDERR "WARNING: item index unusable\n";
	return $prefix . $item . $postfix;
    }

    # Get rid of unwanted characters (non-alphanumeric)
    my $item_tmp = $item;
    $item_tmp =~ y/A-Za-z0-9_-//cd;

    my $url = $index->{$item_tmp};

    unless (defined $url) {
	return $prefix . $item . $postfix;
    }

    return 
	"<uref><urefurl>$url</urefurl><urefreplacement>$prefix$item$postfix</urefreplacement></uref>";
}

my @index_files;
my %item_index;

while ($_ = $ARGV[0], defined $_ && /^-/) {
    shift;
    last if /^--$/;
    if (/^--index$/) { push @index_files, shift; }
}

# Read the index files into a hash table
foreach my $index_file (@index_files) {
    die "Failed to read $index_file" unless -r $index_file;
    open(INDEX, "<$index_file");
    while (<INDEX>) {
	chomp;
	my ($function, $url) = split(/\s+/, $_, 2);
	$item_index{$function} = $url;
    }
    close(INDEX);
}

while (<>) {
    # Match functions
    s{(<reference-function>)([^<]+)(</reference-function>)}
    {make_xref(\%item_index, $2, $1, $3)}ge;

    # Match structs
    s{(<reference-type>)([^<]+)(</reference-type>)}
    {make_xref(\%item_index, $2, $1, $3)}ge;

    # Move no-link functions to normal ones
    s{<(/?)reference-function-nolink>}
    {<$1reference-function>}g;

    print;
}

# vim: ts=8 sw=4
