#!/usr/bin/perl -w
# TOYPROF - Poor man's profiling toy
# Copyright (C) 2001 Tim Janik
#
# This code is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

my $unknown = "???";
my $refs_file = 0;
my @sortcolumns = ();
my $merge = undef;
my $float_precision = 9;
my $addr2line = "addr2line";

# parse options
while ($_ = $ARGV[0], defined $_ && /^-/) {
    shift;
    last if /^--$/;
    if (/^--sort$/) { push @sortcolumns, shift; }
    elsif (/^--merge$/) { $merge = shift; }
}
@sortcolumns = ( "-0", "-1", "-2" ) if ($#sortcolumns < 0);

# construct tmpfile name
my $tmpfile = "$0";
$tmpfile =~ s@[/.]@_@g;
$tmpfile = "/tmp/$tmpfile-tmp.$$";

# read profile data
my %files;     # executable files with symbol adresses
my %meta;
my $head_text = "";
my $tail_text = "";
my $is_tail = 0;
my @rows;
while (<>) {
    if (s/^TOYPROFMETA:\s*([a-zA-Z0-9_]+)\s*=\s*(.*?)\s*$/$2/ ||
	s/^TOYPROFMETA:\s*([a-zA-Z0-9_]+)\/(.)\/\s*=(.*?)$/$2/) {
	if (defined $3) {
	    my @vals = split ($2, $3);
	    $meta{$1} = \@vals;
	} else {
	    $meta{$1} = $2;
	}
	$is_tail = 1;
	next;
    }
    if (s/^TOYPROFDATA:\s*//) {
	s/\s*$//;
	my @row = split /\s+/;     # store profile data in row array
	push @rows, \@row;         # push into array of row arrays

	# match symbol references
	for (@row) {
	    if (m/^([^?:\s]+):(0[xX][0-9A-Fa-f]+)$/) {
		(my $exe, my $addr) = ($1, $2);
		my $addresses = $files{$exe};
		if (!defined $addresses) {
		    $files{$exe} = { $addr => 0 };  # anonymous hash reference (man perlref)
		} else {
		    $addresses->{$addr} = 0;        # assign hash reference
		}
		# print "ref: $exe,$addr\n";
	    }
	}
	$is_tail = 1;
	next;
    }
    if ($is_tail) {
	$tail_text = $tail_text . $_;
    } else {
	$head_text = $head_text . $_;
    }
}

# for each executable invoke addr2line on its symbol references
my %symbols = ();
while (my ($exe, $alist) = each %files) {
    # write adress list into temporary file
    my @foo = keys %{$alist};
    open TMP, ">$tmpfile-addrs";
    print TMP join ("\n", @foo) . "\n";
    close TMP;
    my $syms = `$addr2line -f -e $exe < $tmpfile-addrs`;
    my $toggle = 1;    # addr2line puts out "function\nfile:line\n"
    my $i = 0;
    for (split (/\n/, $syms)) {
	if ($toggle) {
	    my $symref = "$exe:".$foo[$i++];

	    s/.*\?.*/$unknown/;         # replace mismatches
#	    print "$symref = $_ \n";
	    $symbols{$symref} = $_;
	}
	$toggle = !$toggle;
    }
}
unlink "$tmpfile-addrs";

# replace fields with symbol references
for (@rows) {
    my $row = $_;
    my $i = 0;
    for (@$row) {
	if (m/^([^?:\s]+):(0[xX][0-9A-Fa-f]+)$/) {
	    $$row[$i] = $symbols{$_};
	}
	$i++;
    }
}

sub is_num {
    $arg = shift;
    if ($arg =~ m/^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/) {  # match C float (perlfaq4)
	return 1;
    } else {
	return 0;
    }
}

sub is_float {
    $arg = shift;
    return is_num ($arg) && $arg =~ m/(\.|[eE])/;
}

# sort facility
sub rowsort {
    my $rows = shift;
    my $scol = shift;
    
    return sort {
	my $r;
	for $v (@$scol) {
	    my $d; my $c; $_ = $v;
	    if ($_ =~ s/^-//) { $d = $$a[$_]; $c = $$b[$_]; } else { $c = $$a[$_]; $d = $$b[$_]; }
	    
	    if (is_num ($c) && is_num ($d)) {
		#=~ m/^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/ &&
		#$d =~ m/^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/) {   # match C float (perlfaq4)
		$r = $c <=> $d;
	    } else {
		$r = $c cmp $d;
	    }
	    last if ($r != 0);
	}
	$r;
    } @$rows;
}

# merge rows
if (defined $merge) {
    @rows = rowsort (\@rows, [ $merge ] );
    my @new = ();
    my $last;
    my $last_field = "field is not " . $rows[0]->[$merge];
    
    for (@rows) {
	my $row = $_;
	my $field = $$row[$merge];
	
	if ($last_field ne $field) {
	    push @new, $row;
	    $last = $row;
	} else {
	    for (my $i = 0; $i <= $#$row; $i++) {
		next if ($i == $merge);
		if (is_num ($$last[$i]) && is_num ($$row[$i])) {
		    $$last[$i] += $$row[$i];
		} else {
		    $$last[$i] = "<merged>";
		}
	    }
	}
	$last_field = $field;
    }
    @rows = @new;
}

# sort rows
@rows = rowsort (\@rows, \@sortcolumns);

# support column titles
$tmp = $meta{"columns"};
if (ref $tmp) {
    unshift @rows, $tmp;
}

# format numbers and collect column widths
sub MAX { my $a = shift; my $b = shift; return $a > $b ? $a : $b; }
my %cwidth;
for (@rows) {
    my $row = $_;
    my $i = 0;
    
    for ($i = 0; $i <= $#{$row}; $i++) {
	my $num = $$row[$i];
	if (is_float ($num)) {
	    $$row[$i] = sprintf ("%." . $float_precision . "f", $num);
	}
	my $w = $cwidth[$i];
	$w = 0 if (!defined $w);
	$cwidth[$i] = MAX ($w, length ($$row[$i]));
    }
}

# row formatting
sub space { my $i = MAX (shift, 0); my $str = ""; while ($i--) { $str = $str . " "; } return $str; }
sub format_row {
    $row = shift;
    my $str = "";
    my $i = 0;

    for (@$row[0..$#$row]) {
	if (m/^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/) {        # match C float (perlfaq4)
	    $str = $str . space ($cwidth[$i] - length ($_)) . $_ . " | ";
	} else { # non-numeric field
	    $str = $str . $_ . space ($cwidth[$i] - length ($_)) . " | ";
	}
	$i++;
    }
    $str =~ s/\s*\|*\s*$//;
    return $str;
}

# print non-profiler data
print $head_text;

# print meta data
$tmp = $meta{"total_time"};
if (defined $tmp && !ref $tmp) {
    print "Total time accounted: $tmp\n";
}
$tmp = $meta{"device"};
if (defined $tmp && !ref $tmp) {
    print "Profiler timing device: $tmp\n";
}

# print out formatted rows
for (@rows) {
    my $row = $_;

    print format_row ($row) . "\n";
}

# print non-profiler data
print $tail_text;
