#!/usr/bin/perl -w

# Information about the current enumeration

my $flags;			# Is enumeration a bitmask
my $seenbitshift;		# Have we seen bitshift operators?
my $enum_prefix;		# Prefix for this enumeration
my $enumname;			# Name for this enumeration
my $enumshort;			# Name for this enumeration
my $enumindex = 0;		# Global enum counter
my $firstenum = 1;		# Is this the first enumeration in file?
my @entries;			# [ $name, $val ] for each entry

# substitutions:
# @EnumName@                    PrefixTheEEnum
# @enum_name@                   prefix_the_eenum
# @ENUMNAME@                    PREFIX_THE_EENUM
# @ENUMSHORT@                   THE_EENUM
# @VALUENAME@                   PREFIX_THE_VVALUE
# @valuenick@                   vvalue
# @Type@                        Enum or Flags
# @TYPE@                        ENUM or FLAGS


sub parse_trigraph {
    my $opts = shift;
    my @opts;

    for $opt (split /\s*,\s*/, $opts) {
	$opt =~ s/^\s*//;
	$opt =~ s/\s*$//;
#	print STDERR "xxxOPT \"$opt\"\n";
#       my ($key,$val) = $opt =~ /\s*(\w+)(?:=(\S+))?/;
        my ($key,$val) = $opt =~ /(\w+)(?:=(.+))?/;
#	print STDERR "KEY=\"$key\" VAL=\"$val\"\n";
	defined $val or $val = 1;
	push @opts, $key, $val;
    }
    @opts;
}
sub parse_entries {
    my $file = shift;
    my $file_name = shift;
    
    while (<$file>) {
	
	# Read lines until we have no open comments
	while (m@/\*
	       ([^*]|\*(?!/))*$
	       @x) {
	    my $new;
	    defined ($new = <>) || die "Unmatched comment in $ARGV";
	    $_ .= $new;
	}

	# strip comments w/o options
	s@/\*(?!<)
	    ([^*]+|\*(?!/))*
		\*/@@gx;

	# strip newlines
	s/\n//;
	
	# skip empty lines
	next if m@^\s*$@;
	
#	print STDERR "xxx $_\n";
	
	# Handle include files
	if (/^\#include\s*<([^>]*)>/ ) {
            my $file= "../$1";
	    open NEWFILE, $file or die "Cannot open include file $file: $!\n";
	    
	    # Read lines until we have no open comments
	    while (m@/\*
		   ([^*]|\*(?!/))*$
		   @x) {
		my $new;
		defined ($new = <>) || die "Unmatched comment in $file_name";
		$_ .= $new;
	    }
	    
	    # strip comments w/o options
	    s@/\*(?!<)
		([^*]+|\*(?!/))*
		    \*/@@gx;
	
	    if (parse_entries (\*NEWFILE, $NEWFILE)) {
		return 1;
	    } else {
		next;
	    }
	}
	
	if (/^\s*\}\s*(\w+)/) {
	    $enumname = $1;
	    $enumindex++;
	    return 1;
	}
	
        if (m@^\s*
              (\w+)\s*                   # name
              (?:=(                      # value
                   (?:[^,/]|/(?!\*))*
                  ))?,?\s*
              (?:/\*<                    # options
                (([^*]|\*(?!/))*)
               >\s*\*/)?,?
              \s*$
             @x) {
            my ($name, $value, $options) = ($1,$2,$3);

#	    print STDERR "xxx \"$name\" \"$value\" \"$otions\"\n";

	    if (!defined $flags && defined $value && $value =~ /<</) {
		$seenbitshift = 1;
	    }

	    if (defined $options) {
		my %options = parse_trigraph($options);
		if (!defined $options{skip}) {
		    push @entries, [ $name, $options{nick} ];
		}
	    } else {
		push @entries, [ $name ];
	    }
	} elsif (m@^\s*\#@) {
	    # ignore preprocessor directives
	} else {
	    print STDERR "$0: $file_name:$.: Failed to parse `$_'\n";
	}
    }

    return 0;
}


my $iprod = "";
my $hprod = "";
my $tprod = "";
my $eprod = "";
my $vprod_head = "";
my $vprod = "";
my $vprod_tail = "";

# parse options
while ($_ = $ARGV[0], /^-/) {
    shift;
    last if /^--$/;
    if (/^--iprod$/)  { $iprod = $iprod . shift }
    elsif (/^--eprod$/)  { $eprod = $eprod . shift }
    elsif (/^--vhead$/)  { $vprod_head = $vprod_head . shift }
    elsif (/^--vprod$/)  { $vprod = $vprod . shift }
    elsif (/^--vtail$/)  { $vprod_tail = $vprod_tail . shift }
    elsif (/^--hprod$/)  { $hprod = $hprod . shift }
    elsif (/^--tprod$/)  { $tprod = $tprod . shift }
}

print "\n/**\n ** Generated data (by mkenums.pl";
print ")\n **/\n";

if (length($hprod)) {
    my $prod = $hprod;

    $prod =~ s/\@filename\@/$ARGV/g;
    $prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
    $prod =~ s/\\v/\v/g; $prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
		
    print "$prod\n";
}

while (<>) {
    if (eof) {
	close (ARGV);		# reset line numbering
	$firstenum = 1;		# Flag to print filename at next enum
    }

    # Read lines until we have no open comments
    while (m@/\*
           ([^*]|\*(?!/))*$
           @x) {
        my $new;
        defined ($new = <>) || die "Unmatched comment in $ARGV";
        $_ .= $new;
    }

    # strip comments w/o options
    s@/\*(?!<)
        ([^*]+|\*(?!/))*
            \*/@@gx;

#    print STDERR "xxx $_\n";

    if (m@^\s*typedef\s+enum\s*
           ({)?\s*
           (?:/\*<
             (([^*]|\*(?!/))*)
            >\s*\*/)?
         @x) {
	if (defined $2) {
	    my %options = parse_trigraph ($2);
	    next if defined $options{skip};
	    $enum_prefix = $options{prefix};
	    $flags = $options{flags};
	} else {
	    $enum_prefix = undef;
	    $flags = undef;
	}
	# Didn't have trailing '{' look on next lines
	if (!defined $1) {
	    while (<>) {
		if (s/^\s*\{//) {
		    last;
		}
	    }
	}

	$seenbitshift = 0;
	@entries = ();

	# Now parse the entries
	parse_entries (\*ARGV, $ARGV);

	# figure out if this was a flags or enums enumeration

	if (!defined $flags) {
	    $flags = $seenbitshift;
	}

	# Autogenerate a prefix

	if (!defined $enum_prefix) {
	    for (@entries) {
		my $nick = $_->[1];
		if (!defined $nick) {
		    my $name = $_->[0];
		    if (defined $enum_prefix) {
			my $tmp = ~ ($name ^ $enum_prefix);
			($tmp) = $tmp =~ /(^\xff*)/;
			$enum_prefix = $enum_prefix & $tmp;
		    } else {
			$enum_prefix = $name;
		    }
		}
	    }
	    if (!defined $enum_prefix) {
		$enum_prefix = "";
	    } else {
		# Trim so that it ends in an underscore
		$enum_prefix =~ s/_[^_]*$/_/;
	    }
	} else {
	    # canonicalize user defined prefixes
	    $enum_prefix = uc($enum_prefix);
	    $enum_prefix =~ s/-/_/g;
	    $enum_prefix =~ s/(.*)([^_])$/$1$2_/;
	}
	
	for $entry (@entries) {
	    my ($name,$nick) = @{$entry};
            if (!defined $nick) {
 	        ($nick = $name) =~ s/^$enum_prefix//;
	        $nick =~ tr/_/-/;
	        $nick = lc($nick);
	        @{$entry} = ($name, $nick);
            }
	}
	

	# Spit out the output
	
	# enumname is e.g. GMatchType
	$enspace = $enumname;
	$enspace =~ s/^([A-Z][a-z]*).*$/$1/;
	
	$enumshort = $enumname;
	$enumshort  =~ s/^[A-Z][a-z]*//;
	$enumshort =~ s/([^A-Z])([A-Z])/$1_$2/g;
	$enumshort =~ s/([A-Z][A-Z])([A-Z][0-9a-z])/$1_$2/g;
	$enumshort = uc($enumshort);

	$enumlong = uc($enspace) . "_" . $enumshort;
	$enumsym = lc($enspace) . "_" . lc($enumshort);

	if ($firstenum) {
	    $firstenum = 0;
	    
	    if (length($iprod)) {
		my $prod = $iprod;

		$prod =~ s/\@filename\@/$ARGV/g;
		$prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
		$prod =~ s/\\v/\v/g; $prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
		
		print "$prod\n";
	    }
	}
	
	if (length($eprod)) {
	    my $prod = $eprod;

	    $prod =~ s/\@enum_name\@/$enumsym/g;
	    $prod =~ s/\@EnumName\@/$enumname/g;
	    $prod =~ s/\@ENUMSHORT\@/$enumshort/g;
	    $prod =~ s/\@ENUMNAME\@/$enumlong/g;
	    if ($flags) { $prod =~ s/\@type\@/flags/g; } else { $prod =~ s/\@type\@/enum/g; }
	    if ($flags) { $prod =~ s/\@Type\@/Flags/g; } else { $prod =~ s/\@Type\@/Enum/g; }
	    if ($flags) { $prod =~ s/\@TYPE\@/FLAGS/g; } else { $prod =~ s/\@TYPE\@/ENUM/g; }
	    $prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
	    $prod =~ s/\\v/\v/g; $prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;

	    print "$prod\n";
	}

	if (length($vprod_head)) {
	    my $prod = $vprod_head;

	    $prod =~ s/\@enum_name\@/$enumsym/g;
            $prod =~ s/\@EnumName\@/$enumname/g;
            $prod =~ s/\@ENUMSHORT\@/$enumshort/g;
            $prod =~ s/\@ENUMNAME\@/$enumlong/g;
	    if ($flags) { $prod =~ s/\@type\@/flags/g; } else { $prod =~ s/\@type\@/enum/g; }
	    if ($flags) { $prod =~ s/\@Type\@/Flags/g; } else { $prod =~ s/\@Type\@/Enum/g; }
	    if ($flags) { $prod =~ s/\@TYPE\@/FLAGS/g; } else { $prod =~ s/\@TYPE\@/ENUM/g; }
            $prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
            $prod =~ s/\\v/\v/g; $prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
	    
            print "$prod\n";
	}

	if (length($vprod)) {
	    my $prod = $vprod;
	    
	    $prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
	    $prod =~ s/\\v/\v/g; $prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
	    for (@entries) {
		my ($name,$nick) = @{$_};
		my $tmp_prod = $vprod;

		$tmp_prod =~ s/\@VALUENAME\@/$name/g;
		$tmp_prod =~ s/\@valuenick\@/$nick/g;
		if ($flags) { $tmp_prod =~ s/\@type\@/flags/g; } else { $tmp_prod =~ s/\@type\@/enum/g; }
		if ($flags) { $tmp_prod =~ s/\@Type\@/Flags/g; } else { $tmp_prod =~ s/\@Type\@/Enum/g; }
		if ($flags) { $tmp_prod =~ s/\@TYPE\@/FLAGS/g; } else { $tmp_prod =~ s/\@TYPE\@/ENUM/g; }

		print "$tmp_prod\n";
	    }
	}

	if (length($vprod_tail)) {
	    my $prod = $vprod_tail;

	    $prod =~ s/\@enum_name\@/$enumsym/g;
            $prod =~ s/\@EnumName\@/$enumname/g;
            $prod =~ s/\@ENUMSHORT\@/$enumshort/g;
            $prod =~ s/\@ENUMNAME\@/$enumlong/g;
	    if ($flags) { $prod =~ s/\@type\@/flags/g; } else { $prod =~ s/\@type\@/enum/g; }
	    if ($flags) { $prod =~ s/\@Type\@/Flags/g; } else { $prod =~ s/\@Type\@/Enum/g; }
	    if ($flags) { $prod =~ s/\@TYPE\@/FLAGS/g; } else { $prod =~ s/\@TYPE\@/ENUM/g; }
            $prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
            $prod =~ s/\\v/\v/g; $prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
	    
            print "$prod\n";
	}
    }
}

if (length($tprod)) {
    my $prod = $tprod;

    $prod =~ s/\@filename\@/$ARGV/g;
    $prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
    $prod =~ s/\\v/\v/g; $prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
		
    print "$prod\n";
}

print "\n/**\n ** Generated data ends here\n **/\n";
