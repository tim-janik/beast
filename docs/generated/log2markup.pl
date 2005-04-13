#!/usr/bin/perl -W
# Copyright (C) 2001,2003,2005 Tim Janik
# Copyright (C) 2002 Alper Ersoy
#
# This code is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
use strict;
use POSIX "strftime";

# parse options
my $TITLE = "TITLE";
while ($_ = $ARGV[0], defined $_ && /^-/) {
    shift;
    last if /^--$/;
    if (/^--title$/) { $TITLE = shift; }
}


# print HEADER
print <<END_HEADER;
\\input texinfo
\@c %**start of header
\@settitle $TITLE
\@footnotestyle end
\@c %**end of header

\@include texiutils.texi

\@docpackage{BEAST-\@value{BST_VERSION}}
\@docfont{tech}

\@unnumbered $TITLE
\@revision{Document Revised:}
END_HEADER

# print CONTENT
while (<>) {
    my $type = 0;
    my $file_name = $ARGV;

    # reset line numbering
    close (ARGV) if (eof);
    
    # escape special chars
    s/ ([{}@]) /\@$1/xg;
    
    # match statements
    if (m/^[0-9A-Za-z_]/x) { # change entry
      s/^( [0-9A-Za-z_] .* )$ /\@unnumberedsec \@clogentry{$1}/xg;
    } elsif (m/^ \s+ \* \s+ [^:]+ : /x) { # files
      $_ =~ s/^ (\s+ \* \s+) ([^:]+) : /$1\@clogitem{$2}:/xg;
      # escape commas inside argument lists
      $_ =~ s/ \@clogitem{ ([^}]+) }/'@clogitem{' . fix_commas($1) . '}'/xge;
    } elsif (1) { # ordinary lines
      my $line = $_;
      $_ = apply_markup ($line);
    }
    
    # fixup newlines
    s/$/\@*/x;
    s/^ \s* \@\* $//x;
    
    print $_;
}

# print FOOTER
print <<FOOTER;
\@*
\@revision{Document Revised:}
\@bye
FOOTER

sub fix_commas {
    my $t = shift;
    return unless defined $t;

    $t =~ s/,/\\\\,/g;

    return $t;
}

sub apply_markup {
    my $t = shift;

    die "input contains preserved keyword" if $t =~ m/MarkupIxIxQuote/;

    my $nthquote = 1;
    my $ithquotemark = "MarkupIxIxQuoteZL" . $nthquote;

    # replace URLs
    my %urlquotes;
    while ($t =~ s/( ((http|ftp|mailto):\/\/[^\s]+[^.,;:\s]) |
                     ((http|ftp|mailto):\/\/[^\s]+$)
                   )/ $ithquotemark /x) {
	$urlquotes{$ithquotemark} = $1;
	$nthquote++;
	$ithquotemark = "MarkupIxIxQuoteZL" . $nthquote;
    }
    
    # replace symbols
    my $SYMBOL_PREFIX  = "(Sfi|Gsl|Bse|Bst|Gxk|Gtk|Gdk|Gnome)"; # SYMBOL_GLIB instead of 'G'
    my $SYMBOL_PATTERN = "[*0-9A-Z?][*0-9A-Za-z_?]*\(\){0,1}";
    my $SYMBOL_GLIB    = "(G[*A-Z?]+[*a-z?][*0-9A-Za-z_?]*)";
    my %typequotes;
    while ($t =~ s/\b ( ($SYMBOL_PREFIX $SYMBOL_PATTERN) |
                        ($SYMBOL_GLIB)
                      )/ $ithquotemark /x) {
	$typequotes{$ithquotemark} = $1;
	$nthquote++;
	$ithquotemark = "MarkupIxIxQuoteZL" . $nthquote;
    }
    # replace functions
    my $FUNC_PREFIX    = "(sfi|gsl|bse|bst|gxk|gtk|gdk|gnome|g)";
    my $FUNC_PATTERN   = "[*0-9A-Za-z?][*0-9A-Za-z_?]*\(\){0,1}";
    my %funcquotes;
    while ($t =~ s/\b ( $FUNC_PREFIX _ $FUNC_PATTERN
                      )/ $ithquotemark /x) {
	$funcquotes{$ithquotemark} = $1;
	$nthquote++;
	$ithquotemark = "MarkupIxIxQuoteZL" . $nthquote;
    }
    # replace constants
    my $MACRO_PREFIX   = "(SFI|GSL|BSE|BST|GXK|GTK|GDK|GNOME|G)";
    my %constquotes;
    while ($t =~ s/\b ( $MACRO_PREFIX _ $FUNC_PATTERN
                      )/ $ithquotemark /x) {
	$constquotes{$ithquotemark} = $1;
	$nthquote++;
	$ithquotemark = "MarkupIxIxQuoteZL" . $nthquote;
    }

    # quote multiple dots
    $t =~ s/ ( \. \.+ ) /:MarkupIxIxQuote1$1MarkupIxIxQuote2:/gx;

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
    $t =~ s/ :MarkupIxIxQuote1 ( .* ) MarkupIxIxQuote2: /$1/gx;
    
    # escape commas inside argument lists
    $t =~ s/(\@ref[^{]+){([^}]+)}/$1 . '{' . fix_commas($2) . '}'/ge;

    # restore URLs
    while (my ($q, $v) = each %urlquotes) {
	$t =~ s/ $q / \@uref{$v} /x;
    }
    # restore symbols
    while (my ($q, $v) = each %typequotes) {
	$t =~ s/ $q / \@refType{$v} /x;
    }
    # restore functions
    while (my ($q, $v) = each %funcquotes) {
	$t =~ s/ $q / \@refFunction{$v} /x;
    }
    # restore constants
    while (my ($q, $v) = each %constquotes) {
	$t =~ s/ $q / \@refConstant{$v} /x;
    }

    return $t;
}

# vim: ts=8 sw=4
