#!/usr/bin/perl -w

# Information about the current enumeration

my $flags;			# Is enumeration a bitmask
my $seenbitshift;			# Have we seen bitshift operators?
my $prefix;			# Prefix for this enumeration
my $enumname;			# Name for this enumeration
my $enumindex = 0;			# Global enum counter
my $firstenum = 1;		# Is this the first enumeration in file?
my @entries;			# [ $name, $val ] for each entry

sub parse_options {
    my $opts = shift;
    my @opts;

    for $opt (split /\s*,\s*/, $opts) {
#        print "xxxOPT \"$opt\"\n";
        my ($key,$val) = $opt =~ /^(\w+)(?:=(.+))?/;
	defined $val or $val = 1;
	push @opts, $key, $val;
    }
    @opts;
}
sub parse_entries {
    my $file = shift;
    
    while (<$file>) {
	
	# strip comments w/o options
	s@/\*(?!\s*<)
	    ([^*]+|\*(?!/))*
		\*/@@gx;

	# strip newlines
	s/\n//;
	
	# skip empty lines
	next if m@^\s*$@;
	
#	print "xxx $_\n";
	
	# Handle include files
	if (/^\#include\s*<([^>]*)>/ ) {
            my $file= "../$1";
	    open NEWFILE, $file or die "Cannot open include file $file: $!\n";
	    
	    # Read lines until we have no open comments
	    while (m@/\*
		   ([^*]|\*(?!/))*$
		   @x) {
		my $new;
		defined ($new = <>) || die "Unmatched comment in $ARGV";
		$_ .= $new;
	    }
	    
	    # strip comments w/o options
	    s@/\*(?!\s*<)
		([^*]+|\*(?!/))*
		    \*/@@gx;
	
	    if (parse_entries (\*NEWFILE)) {
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
              (?:/\*\s*<                 # options
                (([^*]|\*(?!/))*)
               >\s*\*/)?,?
              \s*$
             @x) {
            my ($name, $value, $options) = ($1,$2,$3);

#	    print "xxx \"$name\" \"$value\" \"$otions\"\n";

	    if (!defined $flags && defined $value && $value =~ /<</) {
		$seenbitshift = 1;
	    }

	    if (defined $options) {
		my %options = parse_options($options);
		if (!defined $options{skip}) {
		    push @entries, [ $name, $options{nick} ];
		}
	    } else {
		push @entries, [ $name ];
	    }
	} else {
	    print STDERR "Can't understand: $_\n";
	}
    }
    return 0;
}


my $gen_externs = 0;
my $gen_interns = 0;
my $gen_list = 0;
my $gen_arrays = 0;
my $gen_includes = 0;


# parse options
while ($_ = $ARGV[0], /^-/) {
    shift;
    last if /^--$/;
    if (/^--includes$/) { $gen_includes = 1 }
    elsif (/^--externs$/)  { $gen_externs = 1 }
    elsif (/^--interns$/)  { $gen_interns = 1 }
    elsif (/^--arrays$/)  { $gen_arrays = 1 }
    elsif (/^--list$/)  { $gen_list = 1 }
}

print "\n/**\n ** Generated data (by mkenums.pl";
if ($gen_includes) {
    print " --includes";
}
if ($gen_externs) {
    print " --externs";
}
if ($gen_interns) {
    print " --interns";
}
if ($gen_arrays) {
    print " --arrays";
}
if ($gen_list) {
    print " --list";
}
print ")\n **/\n";

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
    s@/\*(?!\s*<)
        ([^*]+|\*(?!/))*
            \*/@@gx;

#       print "xxx $_\n";

    if (m@^\s*typedef\s+enum\s*
           ({)?\s*
           (?:/\*\s*<
             (([^*]|\*(?!/))*)
            >\s*\*/)?
         @x) {
	if (defined $2) {
	    my %options = parse_options ($2);
	    next if defined $options{skip};
	    $prefix = $options{prefix};
	    $flags = $options{flags};
	} else {
	    $prefix = undef;
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
	parse_entries (\*ARGV);

	# figure out if this was a flags or enums enumeration

	if (!defined $flags) {
	    $flags = $seenbitshift;
	}

	# Autogenerate a prefix

	if (!defined $prefix) {
	    for (@entries) {
		my $nick = $_->[1];
		if (!defined $nick) {
		    my $name = $_->[0];
		    if (defined $prefix) {
			my $tmp = ~ ($name ^ $prefix);
			($tmp) = $tmp =~ /(^\xff*)/;
			$prefix = $prefix & $tmp;
		    } else {
			$prefix = $name;
		    }
		}
	    }
	    if (!defined $prefix) {
		$prefix = "";
	    } else {
		# Trim so that it ends in an underscore
		$prefix =~ s/_[^_]*$/_/;
	    }
	} else {
	    # canonicalize user defined prefixes
	    $prefix = uc($prefix);
	    $prefix =~ s/-/_/g;
	    $prefix =~ s/(.*)([^_])$/$1$2_/;
	}
	
	for $entry (@entries) {
	    my ($name,$nick) = @{$entry};
            if (!defined $nick) {
 	        ($nick = $name) =~ s/^$prefix//;
	        $nick =~ tr/_/-/;
	        $nick = lc($nick);
	        @{$entry} = ($name, $nick);
            }
	}
	
	# Spit out the output
	
	($valuename = $enumname) =~ s/([A-Z][a-z])/_$1/g;
	$valuename =~ s/([a-z])([A-Z])/$1_$2/g;
	$valuename = lc($valuename);
	$valuename =~ s/_(.*)/$1/;
	
	($enumtype = $enumname) =~ s/^Bse//;
	$enumtype =~ s/([^A-Z])([A-Z])/$1_$2/g;
	$enumtype =~ s/([A-Z][A-Z])([A-Z][0-9a-z])/$1_$2/g;
	$enumtype = uc($enumtype);

	if ($firstenum) {
	    print "\n/* --- $ARGV --- */\n";
	    $firstenum = 0;
	    
	    if ($gen_includes) {
		print "#include\t\"$ARGV\"\n";
	    }
	}
	
	if ($gen_externs) {
	    print "#define BSE_TYPE_$enumtype\t(BSE_TYPE_ID ($enumname))\n";
            print "extern BseType bse_type_builtin_id_$enumname;\n";
	}

	if ($gen_interns) {
            print "BseType bse_type_builtin_id_$enumname = 0;\n";
	}

	if ($gen_arrays) {
	    print "/* $enumname\n */\n";

	    print "static Bse".($flags ? "Flags" : "Enum")."Value ${valuename}_values[] = {\n";
	    for (@entries) {
		my ($name,$nick) = @{$_};
		print qq(  { $name, "$name", "$nick" },\n);
	    }
	    print "  { 0, NULL, NULL }\n};\n";

	    print "\n";
	}

	if ($gen_list) {
	    print "  { \"$enumname\",";
	    print " BSE_TYPE_".($flags ? "FLAGS" : "ENUM").",";
	    print " &bse_type_builtin_id_$enumname,";
	    print " ${valuename}_values },\n";
	}
    }
}


print "\n/**\n ** Generated data ends here\n **/\n";
