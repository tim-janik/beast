#!/usr/bin/perl -w

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

print "\n/**\n ** Generated data (by mkcproc.pl";
print ")\n **/\n";

sub ncanon {
    my $name = shift;
    $name =~ s/[^a-zA-Z0-9]/_/g;
    return $name;
}

sub has_semicolon {
    my $line = shift;

    # eat up strings
    $line =~ s/\"([^\"])*\"//g;
    # eat up chars
    $line =~ s/\'([^\'])*\'//g;

    return m/;/;
}

my $var_pattern = "HELP|AUTHOR|CRIGHTS|DATE";
my %var_defs = ();
my %proc_defs = ();
my $line_jump = 1;
my $proc_name;
my $proc_method;
my $proc_category;
my $match_contents = 1;
my $externs = "";

sub print_assignment {
    my $assignment = shift;
    my $var = shift;
    my $val;
    my $vfile;
    my $vline;
    if (defined $proc_defs{$var}) {
	( $val, $vfile, $vline ) = @{$proc_defs{$var}};
    } elsif (defined $var_defs{$var}) {
	( $val, $vfile, $vline ) = @{$var_defs{$var}};
    } else {
	return;
    }
    print "#line $vline \"$vfile\"\n";
    print "  $assignment = $val\n";
}

while (<>) {
    my $type = 0;
    my $file = $ARGV;
    my $line = $.;
    
    if (eof) {
	close (ARGV);          # reset line numbering
    }

    # read lines until comment end is matched
    while (m@/\*([^*]|\*[^/*])*\**$@x) {
        my $new = <>;
        (defined ($new) && ($file eq $ARGV)) or die "$file:$.: Unmatched comment\n";
        $_ .= $new; $line_jump = 1;
    }
    # strip comments
    if (s@/\*([^*]|\*[^/*])*\**\*/@@gx) {
	$line_jump = 1;
    }

    # find PROCEDURE() directive
    if (m@^\s*(METHOD|PROCEDURE)\s*\(@) {
	my $rest;
	if (defined $proc_name) {
	    die "$file:$.: METHOD/PROCEDURE within METHOD/PROCEDURE\n";
	}
        while (!m@\)@) {
	    my $new = <>;
	    (defined ($new) && ($file eq $ARGV)) or die "$file:$.: Invalid expression\n";
	    $_ .= $new; $line_jump = 1;
	}
	if (m@.*PROCEDURE\s*\( ([a-zA-Z0-9_-]*)\s* , \s*"([^\"\)]*)" \);?(.*)$@x) {
	    $rest = $3;
	    $proc_name = $1;
	    $proc_category = $2;
	    if ($proc_category =~ m@^/.*@ || $proc_category =~ m@.*/$@) {
		die "$file:$.: Procedure category contains extraneous slahses: \"$proc_category\"";
	    }
	    $proc_category = "\"" . "/Proc/" . $proc_category . "\"";
	    $proc_method = "\"" . $proc_name . "\"";
	} elsif (m@.*METHOD\s*\( ([a-zA-Z0-9_-]*)\s* , \s*([a-zA-Z0-9_-]*) , \s*"([^\"\)]*)" \);?(.*)$@x ||
		 m@.*METHOD\s*\( ([a-zA-Z0-9_-]*)\s* , \s*([a-zA-Z0-9_-]*) \);?(.*)$@x) {
	    my $type = $1;
	    my $subcat;
	    $proc_name = $2;
	    if (defined $4) {
		$rest = $4;
		$subcat = $3;
		if ($subcat =~ m@^/.*@ || $subcat =~ m@.*/$@) {
		    die "$file:$.: Method category contains extraneous slahses: \"$subcat\"";
		}
	    } else {
		$rest = $3;
		$subcat = $proc_name;
		$subcat =~ s/^(.)/\U$1/;
		$subcat =~ s/-(.)/ \U$1/g;
		$subcat = "General/" . $subcat;
	    }
	    $proc_method = "\"" . $type . "+" . $proc_name . "\"";
	    $proc_category = "\"" . "/Method/" . $type . "/" . $subcat . "\"";
	} else {
	    die "$file:$.: Invalid METHOD/PROCEDURE directive\n";
	}

	print "/* --- $proc_name --- */\n";
	print "static GType type_id_" . ncanon ($proc_name) . " = 0;\n";
	print "static void\n";
	print ncanon ($proc_name) . "_setup (BseProcedureClass *proc, ";
	print "GParamSpec **in_pspecs, GParamSpec **out_pspecs) {\n";
	print "#line $line \"$file\"\n$rest\n" if (defined $rest);

	$externs .= "  { ";
	$externs .= "&type_id_" . ncanon ($proc_name) . ", ";
	$externs .= $proc_method . ", ";                 # name
	$externs .= "NULL, ";                            # FIXME: type blurb
	$externs .= "0, ";                               # private_id
	$externs .= ncanon ($proc_name) . "_setup, ";    # init
	$externs .= ncanon ($proc_name) . "_exec, ";     # exec
	$externs .= "NULL, ";                            # unload
	$externs .= $proc_category . ", ";               # category
	$externs .= "{ 0, }, ";                          # FIXME: pixdata
	$externs .= "},\n";

	$match_contents = 1; $line_jump = 1; next;
    }

    # find BODY directive
    if ($match_contents && m@[^\)]*BODY\s*\(@) {
	if (!defined $proc_name) {
	    die "$file:$.: BODY() without PROCEDURE()\n";
	}
	while (!m@\)@) {
	    my $new = <>;
	    (defined ($new) && ($file eq $ARGV)) or die "$file:$.: Invalid expression\n";
	    $_ .= $new; $line_jump = 1;
	}
	if (!m@^(.*)BODY\s*\( ([^\)]*) \)\s*@x) {
	    die "$file:$.: Invalid BODY() directive\n";
	}
	
	print_assignment ("proc->help", "HELP");
	print_assignment ("proc->author", "AUTHOR");
	print_assignment ("proc->copyright", "CRIGHTS");
	print_assignment ("proc->date", "DATE");
	print "#line $line \"$file\"\n$1 }\n";
	print "static BseErrorType\n";
	print "#line $line \"$file\"\n";
	print ncanon ($proc_name) . "_exec (" . $2 . ")\n";

	undef %proc_defs;
	undef $proc_name;
	
	$match_contents = 0; $line_jump = 1; next;
    }
    
    # read variables lines
    if ($match_contents && m@[^;]*($var_pattern)\s*=@) {
	my $var = $1;
	while (!has_semicolon ($_)) {
	    my $new = <>;
	    (defined ($new) && ($file eq $ARGV)) or die "$file:$.: Invalid expression\n";
	    $_ .= $new; $line_jump = 1;
	}
	$value = $_;
	$value =~ s/^.*$var[^=]*=\s*//;
	if (defined $proc_name) {
	    $proc_defs{$var} = [ $value, $file, $line ];
	} else {
	    $var_defs{$var} = [ $value, $file, $line ];
	}

	$match_contents = 1; $line_jump = 1; next;
    }

    # warn verbose
    if (m@($var_pattern|BODY|PROCEDURE|METHOD)@) {
	print STDERR "$file:$.: warning: reserved word `$1' remains unmatched\n";
    }

    # rewrite parameter assignments
    if ($match_contents && m@^\s*(IN|OUT)\s*=@) {
	$_ =~ s/^\s*IN\s/  \*\(in_pspecs++\) /;
	$_ =~ s/^\s*OUT\s/  \*\(out_pspecs++\) /;
    }

    # normal line dump
    if ($line_jump) {
	print "#line $line \"$file\"\n";
	$line_jump = 0;
    }
    print $_;
}


# BSE export stuff
{
    print "\n";
    print "/* --- Export to BSE --- */\n";
    print "BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);\n";
    print "BSE_EXPORT_PROCEDURES = {\n";
    print $externs;
    print "  { NULL, },\n";
    print "};\n";
    print "BSE_EXPORTS_END;\n";
}


print "\n/**\n ** Generated data ends here\n **/\n";
