#!/usr/bin/perl -w

my $gen_externs = 0;
my $gen_array = 0;

# parse options
while ($_ = $ARGV[0], /^-/) {
    shift;
    last if /^--$/;
    if (/^--externs$/) { $gen_externs = 1 }
    elsif (/^--array$/) { $gen_array = 1 }
}

print "\n/**\n ** Generated data (by mkbuiltinlist.pl";
if ($gen_externs) {
    print " --externs";
}
if ($gen_array) {
    print " --array";
}
print ")\n **/\n";

my %bdefs;

while (<>) {
    my $name = 0;
    my $file = $ARGV;
    my $line = $.;

    # read lines until comment end is matched
    while (m@/\*([^*]|\*[^/])*$@x) {
        my $new = <>;

        (defined ($new) && ($file eq $ARGV)) or die "$0: $file:$.: Unmatched comment (line $line)\n";
        $_ .= $new;
    }
    
    # strip comments
    s@/\*([^*]|\*[^/])*\*/@@gx;
    
    # discard non BSE_EXPORTS_* lines
    if (/BSE_EXPORTS_BEGIN/) {
    } else {
	next;
    }
    
    # read up to next paren to finish arg list
    while (!m@\)@) {
	my $new = <>;
	
	(defined ($new) && ($file eq $ARGV)) or die "$0: $file:$.: Unmatched argument brace (line $line)\n";
	$_ .= $new;
    }
    
    # parse builtin name
#    if (/\(\s*(\w+)\s*\)/) {
#	$name = $1;
#    } else {
#	die "$0: $file:$.: Plugin name unrecognized, invalid syntax?\n";
#    }
    
    # check for doubles
#    if (defined $bdefs{$name}) {
#	my ($name, $ofile, $oline) = @{$bdefs{$name}};
#	
#	die "$0: $file:$.: Plugin name `$name' already used in $ofile:$oline\n";
#    }

    # set name from file name
    $name = $file;
    $name =~ s/\.[^.]+$//;
    $name =~ s/[-_.]/_/g;
    
    $bdefs{$name} = [ $name, $file, $line ];
    
    close(ARGV) if (eof);
}

my %tmp_hash = ();

foreach $elem (values %bdefs) {
    my ($name, $file, $line) = @{$elem};
    
    if ($gen_externs && !defined $tmp_hash{$file}) {
	$tmp_hash{$file} = 1;
	print "\n/* --- $file --- */\n";
    }
    
    if ($gen_externs) {
	print "extern BSE_EXPORT_IMPL_F ($name);\n";
    }
    if ($gen_array) {
	print "  BSE_EXPORT_IMPL_L ($name),\n";
    }
}


print "\n/**\n ** Generated data ends here\n **/\n";
