#!/usr/bin/perl -W
# Stupid doc scanner
# Copyright (C) 2001,2003 Tim Janik
# Copyright (C) 2002 Alper Ersoy
#
# This code is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
use strict;
use POSIX "strftime";

my $pname = "Foo-Manual";
my $pblurb = "Official Foo Manual";
my $package = "FooFactory";

my @seealso = ();

# supported documentation tags (quoted from gtk-doc/doc/gnome.txt)
#   @name   a parameter.
#   %name   a constant.
#   name()  reference to a function, or a macro which works like a function
#           (this will be turned into a hypertext link if the function is
#           documented anywhere).
#   #name   reference to any other type of declaration, e.g. a struct, enum,
#           union, or macro (this will also be turned into a link if possible).
#

# parse options
while ($_ = $ARGV[0], defined $_ && /^-/) {
    shift;
    last if /^--$/;
    if (/^--name$/) { $pname = shift; }
    elsif (/^--blurb$/) { $pblurb = shift; }
    elsif (/^--package$/) { $package = shift; }
    elsif (/^--seealso$/) { push @seealso, shift; }
}

# docs
my @records = ();
my %declhash = ();

while (<>) {
    my $type = 0;
    my $file_name = $ARGV;

    # reset line numbering
    close (ARGV) if (eof);

    # read lines until comment end is matched
    while (m@/\*([^*]|\*[^/*])*\**$@x) {
        my $new = <>;
        (defined ($new) && ($file_name eq $ARGV)) or die "$file_name:$.: Unmatched comment\n";
        $_ .= $new;
    }

    # read lines until function decl is complete
    while (m@^\w[^(]*\([^{;]*$@x) {
        my $new = <>;
        (defined ($new) && ($file_name eq $ARGV)) or die "$file_name:$.: Unmatched function declaration\n";
        $_ .= $new;
    }

    # match docu comment
    # if (m@^/\*\*\s+(([^*]|\*[^/*]|\*\*[^/])*)\*\*/@) {
    if (m@^/\*\*\s+(([^*]|\*+[^*/])*)\*+/@) {
	my @lines = split ('\n', $1);
	my $line;
	my $rec = { name => 'Unnamed',
		    var_names => [], var_blurbs => [], var_types => [],
		    var_hash => {}, found_decl => 0,
		    text => '', returns => [] };
	my $first_line = 1;

	# print STDERR "found doc comment in $file_name:$.:\n";
	for my $line (@lines) {
	    $line =~ s/^\s*\**\s*//;
	    if ($first_line) {
		$line =~ s/^\s*//;
		$line =~ s/\s*$//;
		$line =~ s/\+/::/;
		$rec->{name} = $line;
		$declhash{$line} = $rec;
	    } elsif ($line =~ m/@([A-Za-z._][A-Za-z0-9._-]*)\s*:\s*(.*)\s*$/) {
		my $name = $1;
		my $blurb = $2;

		if ($name =~ m/^returns$/i) {
		    my $returns = $rec->{returns};
		    push @$returns, $blurb;
		} else {
		    my $names = $rec->{var_names};
		    my $blurbs = $rec->{var_blurbs};
		    my $var_hash = $rec->{var_hash};
		    $var_hash->{$name} = $blurb;
		    push @$names, $name;
		    push @$blurbs, $blurb;
		}
	    } else {
		$line =~ s/^\s*//;
		$line =~ s/\s*$//;
		$rec->{text} .= $line;
		$rec->{text} .= ' ' if (length ($rec->{text}));
	    }
	    $first_line = 0;
	}
	push @records, $rec;
    }


    # try to match function decls that we know about
    # if (m@([A-Za-z._][A-Za-z0-9._-]*)\s*\(([A-Za-z0-9\s,*_-]*)\)\s*\{@) {
    if (m@([A-Za-z._][A-Za-z0-9._-]*)\s*\(([A-Za-z0-9\s,*_-]*)\)\s*[{]@) {
	my $name = $1;
	my $declargs = $2;
	my $rec = $declhash{$name};

        if (defined $rec) {  # have docu record for this decl
	    # print STDERR "ARGDECL: $name: $declargs\n";
	    my @args = split (',', $declargs);
	    $rec->{var_names} = [];
	    $rec->{var_types} = [];
	    $rec->{var_blurbs} = [];
	    $rec->{found_decl} = 1;
	    for my $arg (@args) {
		$arg =~ s/^\s*//;
		$arg =~ s/\s*$//;
		my $t = $arg;

		$t =~ s/^(.*\W)\w*$/$1/;
		$arg = substr ($arg, length $t);
		$t =~ s/\s*\*\s*/*/g;
		$t =~ s/\s\s/ /g;
		# print STDERR "found: $name: $t -- $arg\n";
		if (length $arg) {   # for $t=="void", $arg will be ""
		    my $names = $rec->{var_names};
		    my $types = $rec->{var_types};
		    my $blurbs = $rec->{var_blurbs};
		    my $hash = $rec->{var_hash};
		    my $blurb = $hash->{$arg};
		    if (defined $blurb) {
			delete $hash->{$arg};
		    } else {
			$blurb = '';
			print STDERR "NOTE: missing description for \`$name(ARG: $arg)'\n";
		    }
		    push @$names, $arg;
		    push @$types, $t;
		    push @$blurbs, $blurb;
		}
	    }
	}
    }
}

sub tags_print_syntax {
    my $rec       = shift;
    my $prefix    = shift || '';
    my @var_names = @{$rec->{var_names}};

    my $anchor = ($prefix eq '@item ' ? '@anchor{' . $rec->{name} . '}' : ' ');

    print $prefix, $anchor, '@refFunctionNoLink{' . $rec->{name} . '}(',
	join(', ', map { $_ = "\@refParameter{$_}" } @var_names),
	');';
}
sub tags_highlight {
    my $t = shift;

    my $ident = "[*0-9A-Za-z?][*0-9A-Za-z_?]*";

    # Protect explicit newline indicators @*
    $t =~ s/ \s \@ \* \s/ :scandocs_pl_QUOTE1:newline:scandocs_pl_QUOTE2:/gx;

    # A variable (parameter)
    $t =~ s/ \@ ( $ident ) /\@refParameter{$1}/gx;

    # Restore explicit newline indicators
    $t =~ s/:scandocs_pl_QUOTE1:newline:scandocs_pl_QUOTE2:/\@*/gx;

    # A type
    $t =~ s/ \# ( $ident ) /\@refType{$1}/gx;

    # A function name
    $t =~ s/    ( $ident \( [-+*=!^\$%&\/?\\~;:,.|<>A-Za-z0-9\s_]* \) ) /\@refFunction{$1}/gx;

    # quote multiple dots
    die "input contains preserved keyword" if $t =~ m/scandocs_pl_QUOTE/;
    $t =~ s/ ( \. \.+ ) /:scandocs_pl_QUOTE1$1scandocs_pl_QUOTE2:/gx;

    # markup numeric constants automagically
    my $expo = "([eE][+-]?[0-9]+)";
    if ($t =~ s/( # float:
		  ( \b [0-9]+ \.   [0-9]+ $expo ? \b ) |
		  ( \b [0-9]+ \. ?        $expo   \b ) |
		  ( \b [0-9]+ \.                  \B ) |
		  ( \B        \.   [0-9]+ $expo ? \b ) |
		  # integer:
		  ( \b              [0-9]+ [LlUu]* \b ) |
		  ( \b 0 [xX] [A-Fa-f0-9]+         \b )
		)/\@refConstant{$1}/gx ) {
	# print STDERR "CONSTANT: \$1\n";
    }

    # unquote dots
    $t =~ s/ :scandocs_pl_QUOTE1 ( .* ) scandocs_pl_QUOTE2: /$1/gx;

    # A constant
    $t =~ s/  % ( $ident ) /\@refConstant{$1}/gx;

    $t =~ s/(\@ref[^{]+){([^}]+)}/$1 . '{' . fix_commas($2) . '}'/ge;

    return $t;
}
sub tags_print_description {
    my $rec = shift;
    my $var_types = $rec->{var_types};
    my $var_names = $rec->{var_names};
    my $var_blurbs = $rec->{var_blurbs};
    my $returns = $rec->{returns};

    if (@{$var_names} or @{$returns}) {
	print "\@multitable \@columnfractions .3 .3 .3\n";

	for (my $i = 0; $i <= $#$var_names; $i++) {
	    my $t = $$var_types[$i] || "";
	    $t =~ s/\t/ /g;
	    # $t =~ s/\s/\\ /g;

	    print "\@item\n";
	    print "\@refType{$t}\n" if $t;

	    print "\@tab\n";
	    printf ("\@refParameter{%s};\n", $$var_names[$i]) if $$var_names[$i];

	    printf ("\@tab\n%s\n", tags_highlight ($$var_blurbs[$i]));
	}

	for my $r (@$returns) {
	    printf ("\@item\n\@refReturns\n\@tab\n\@tab\n%s\n", tags_highlight ($r));
	}

	print "\@end multitable\n\n";
    } else {
	print "\n";
    }

    if ($rec->{text}) {
	print tags_highlight($rec->{text}) . "\n\n";
    } else {
	print "@*\n\n";
    }
}
sub sort_items {
    my $list = shift;

    return unless ref($list) eq 'ARRAY';

    @{$list} = sort { ${$a}{name} cmp ${$b}{name} } @{$list};
}
sub fix_commas {
    my $t = shift;
    return unless defined $t;

    $t =~ s/,/\\\\,/g;

    return $t;
}

sort_items(\@records);

my %test_hash = ();
my @dups = ();

my $tname = lc($pname);
$tname =~ tr/A-Za-z0-9-/_/c;

# \@settitle $pname - $pblurb - $package

print <<END_HEADER;
\\input texinfo
\@c %**start of header
\@settitle $pname
\@footnotestyle end
\@c %**end of header

\@include texiutils.texi

\@docpackage{$package}
\@docfont{tech}

\@unnumbered NAME
$pname - $pblurb

\@revision{Document Revised:}

\@unnumbered SYNOPSIS
\@printplainindex fn

\@unnumbered DESCRIPTION
\@ftable \@asis
END_HEADER

for my $rec (@records) {
    # Remove the following line when @ftable is capable of findexing when
    # outputting to XML
    tags_print_syntax($rec, '@item ');
    tags_print_syntax($rec, ' @findex ');
    print "\n";
    tags_print_description ($rec);

    push (@dups, $rec->{name}) if (defined $test_hash{$rec->{name}});
    $test_hash{$rec->{name}} = 1;
}

print "\@end ftable\n";

# Link to external documents
if (@seealso) {
    print "\@unnumbered SEE ALSO\n";
    print join(', ', map { $_ = "\@uref{$_}" } @seealso), "\n";
}

print <<FOOTER;
\@*
\@revision{Document Revised:}
\@bye
FOOTER

# provide feedback
for my $rec (@records) {
    my $var_hash = $rec->{var_hash};
    next if (!$rec->{found_decl});
    for my $a (keys %$var_hash) {
	print STDERR "NOTE: couldn't find declaration for \`".$rec->{name}."($a)'\n";
    }
}
for (@dups) {
    print STDERR "WARNING: duplicate description for \`$_'\n";
}

sub array_find_str {
    my $a = shift;
    my $s = shift;
    my $i;
    for ($i = 0; $i <= $#$a; $i++) {
	if ($$a[$i] eq $s) {
	    return $i;
	}
    }
    return -1;
}

# vim: ts=8 sw=4
