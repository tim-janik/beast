#!/usr/bin/perl -w
# Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

# figure whether all { have a corresponding }
sub brace_level {
    my $line = shift;
    my $level = 0;
    my $i = 0;
    my $c;

    # eat up strings
    $line =~ s/\"([^\"])*\"//g;

    # eat up chars
    $line =~ s/\'([^\'])*\'//g;

    # eat up non braces
    $line =~ s/([^\}\{])//g;

    # count { } levels
    do {
	$c = substr ($line, $i++, 1);

	if (ord ($c) == ord ('{')) {
	    $level++
	} elsif (ord ($c) == ord ('}')) {
	    $level--;
	}
    } while (ord ($c));

    # return brace level (0 means braces match)

    return $level;
}

my $gen_externs = 0;
my $gen_interns = 0;
my $gen_export_proto = 0;
my $gen_array = 0;

# parse options
while ($_ = $ARGV[0], /^-/) {
    shift;
    last if /^--$/;
    if (/^--externs$/) { $gen_externs = 1 }
    if (/^--interns$/) { $gen_interns = 1 }
    if (/^--export-proto$/) { $gen_export_proto = 1 }
    elsif (/^--array$/) { $gen_array = 1 }
}

print "\n/*\n * Generated data (by mktypes.pl";
if ($gen_externs) {
    print " --externs";
}
if ($gen_interns) {
    print " --interns";
}
if ($gen_export_proto) {
    print " --export-proto";
}
if ($gen_array) {
    print " --array";
}
print ")\n */\n";


my %adefs;   # hash, indexed with {$uc_type}, value = [ $type, $uc_type, $uc_parent, $uc_iface, $file ]

while (<>) {
    my $type = 0;
    my $file = $ARGV;

    if (eof) {
	close (ARGV);          # reset line numbering
    }

    # read lines until comment end is matched
    while (m@/\*([^*]|\*[^/*])*\**$@x) {
        my $new = <>;

        (defined ($new) && ($file eq $ARGV)) or die "$0: $file:$.: Unmatched comment\n";
        $_ .= $new;
    }
    # strip comments
    s@/\*([^*]|\*[^/*])*\**\*/@@gx;

    # discard non BSE_BUILTIN_TYPE lines
    if (!m@^BSE_BUILTIN_TYPE@x) {
	next;
    }

    # read up to next ) to finish arg list
    while (!m@\)@) {
	my $new = <>;

        (defined ($new) && ($file eq $ARGV)) or die "$0: Unmatched argument brace in $file\n";
	$_ .= $new;
    }

    # parse type name
    if (/\(\s*(\w+)\s*\)/) {
	$type = $1;
    } else {
	die "$0: can't figure type name from BSE_BUILTIN_TYPE() in $file\n";
    }


    if ($gen_array) {
	my $uc_type = $type;
	my $uc_parent = 0;
	my $uc_iface = 0;

	# read up to next { to begin function block
	while (!m@\{@) {
	    my $new = <>;

	    (defined ($new) && ($file eq $ARGV)) or die "$0: Unmatched curly brace in $file\n";
	    $_ .= $new;
	}
	
	# read lines until } is matched
	while (brace_level ($_) != 0) {
	    my $new = <>;

	    (defined ($new) && ($file eq $ARGV)) or die "$0: Unmatched block braces in $file\n";
	    $_ .= $new;
	}
	# ok! we now have the complete function body in $_

	# parse parent type
	if (/_type_register_(static|abstract)\s*\(\s*(\w+)\s*,/) {
	    $uc_parent = $2;
	} else {
	    die "$0: can't figure parent type of $type from $file\n";
	}

	# parse parent type
	if (/bse_type_add_interface/) {
	    if (/bse_type_add_interface\s*\([^\),]*,\s*(\w+)\s*,/) {
		$uc_iface = $1;
	    } else {
		die "$0: can't figure parent type of $type from $file\n";
	    }
	}

	$uc_type =~ s/^Bse//;
	$uc_type =~ s/([^A-Z])([A-Z])/$1_$2/g;
	$uc_type =~ s/([A-Z][A-Z])([A-Z][0-9a-z])/$1_$2/g;
	$uc_type = uc($uc_type);
	$uc_type = "BSE_TYPE_" . $uc_type;

	$adefs{$uc_type} = [ $type, $uc_type, $uc_parent, $uc_iface, $file ];
    }

    if ($gen_externs || $gen_interns) {
	print "\n/* --- $file --- */\n";
    }

    if ($gen_externs) {
	print "extern GType bse_type_builtin_id_$type;\n";
    }

    if ($gen_export_proto) {
	print "extern \"C\" BSE_BUILTIN_PROTO ($type);\n";
    }
    
    if ($gen_interns) {
	print "GType bse_type_builtin_id_$type = 0;\n";
    }
}


# print out an array entry
sub foreach_entry {
    my $entry = shift;
    my @entry;
    my ($type, $uc_type, $uc_parent, $uc_iface, $file) = @{$entry};

    # delete this entry, so we don't get called twice for it
    undef $adefs{$uc_type};

    # need to print out the parent type first?
    # print STDERR "$type has parent: $uc_parent: " . $adefs{$uc_parent} . "\n";
    if ($adefs{$uc_parent}) {
	foreach_entry ($adefs{$uc_parent});
    }

    # need to print out an interface type first?
    if ($adefs{$uc_iface}) {
	foreach_entry ($adefs{$uc_iface});
    }

    print "  { &bse_type_builtin_id_$type, bse_type_builtin_register_$type },\n";
}

if ($gen_array) {
    while (($key,$value) = each %adefs) {
	undef $key;      # dunno how else to silence -w, probably need more perl experience ;)

	if (defined ($value)) {
	    foreach_entry ($value);
	}
    }
}


print "\n/*\n * Generated data ends here\n */\n";
