#!/usr/bin/perl -w

use Cwd 'abs_path';

my $gen_preprocess = 0;
my $gen_externs = 0;
my $gen_funcs = 0;
my $funcname = 0;

# parse options
while ($_ = $ARGV[0], defined $_ && /^-/) {
    shift;
    last if /^--$/;
    if (/^--preprocess$/) { $gen_preprocess = 1 }
    elsif (/^--externs$/) { $gen_externs = 1 }
    elsif (/^--funcname$/) { $funcname = shift }
    elsif (/^--functions$/) { $gen_funcs = 1 }
}

if (@ARGV < 1) {
    die "$0: missing file name\n";
}

sub ncanon {
    my $name = shift;
    $name =~ s/[^a-zA-Z0-9]/_/g;
    return $name;
}

sub func_name {
    my $name = shift;
    return "bse__builtin_init_". ncanon ($name) ."";
}

if ($gen_externs) {
    for (@ARGV) {
	print "BseExportNode* ". func_name ($_) ." (void);\n";
    }
    exit 0;
} elsif ($gen_funcs) {
    for (@ARGV) {
	print "  ". func_name ($_) .",\n";
    }
    exit 0;
} elsif (!$gen_preprocess) {
    exit 0;
}

if (@ARGV > 1) {
    die "$0: too many file names\n";
}

my $var_pattern = "HELP|OPTIONS|AUTHORS|LICENSE";
my %var_defs = ();
my %proc_defs = ();

print "\n/*\n * Generated data (by mkcproc.pl";
print ")\n */\n";

sub has_semicolon {
    my $line = shift;

    # eat up strings
    $line =~ s/\"([^\"])*\"//g;
    # eat up chars
    $line =~ s/\'([^\'])*\'//g;

    return m/;/;
}

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
	# print STDERR "no assignment for $var\n" ;
	return;
    }
    print "#line $vline \"$vfile\"\n";
    print "  $assignment = $val;\n";
}

sub get_variable {
    my $var = shift;
    my $fallback = shift;
    my $val;
    my $vfile;
    my $vline;
    if (defined $proc_defs{$var}) {
	( $val, $vfile, $vline ) = @{$proc_defs{$var}};
    } elsif (defined $var_defs{$var}) {
	( $val, $vfile, $vline ) = @{$var_defs{$var}};
    } else {
	$val = $fallback;
    }
    return "$val";
}
sub get_variable_file {
    my $var = shift;
    my $fallback = shift;
    my $val;
    my $vfile;
    my $vline;
    if (defined $proc_defs{$var}) {
	( $val, $vfile, $vline ) = @{$proc_defs{$var}};
    } elsif (defined $var_defs{$var}) {
	( $val, $vfile, $vline ) = @{$var_defs{$var}};
    } else {
	$vfile = $fallback;
    }
    return "$vfile";
}
sub get_variable_line {
    my $var = shift;
    my $fallback = shift;
    my $val;
    my $vfile;
    my $vline;
    if (defined $proc_defs{$var}) {
	( $val, $vfile, $vline ) = @{$proc_defs{$var}};
    } elsif (defined $var_defs{$var}) {
	( $val, $vfile, $vline ) = @{$var_defs{$var}};
    } else {
	$vline = $fallback;
    }
    return "$vline";
}

my $proc_name;
my $proc_method;
my $proc_category;
my $match_contents = 1;
my $externs = "";
my $last_node = "NULL";
my $line_jump = 1;
my $file = "";

while (<>) {
    my $type = 0;
    my $line = $.;
    $file = $ARGV;
    $afile = abs_path ($file);
    
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
	    $proc_category = "\"" . "/Methods/" . $type . "/" . $subcat . "\"";
	} else {
	    die "$file:$.: Invalid METHOD/PROCEDURE directive\n";
	}

	print "/* --- $proc_name --- */\n";
	print "static void\n";
	print ncanon ($proc_name) . "_setup (BseProcedureClass *proc, ";
	print "GParamSpec **in_pspecs, GParamSpec **out_pspecs) {\n";
	print "#line $line \"$file\"\n$rest\n" if (defined $rest);

	$match_contents = 1; $line_jump = 1; next;
    }

    # find BODY directive
    if ($match_contents && m@[^\)]*BODY\s*\(@) {
	if (!defined $proc_name || !defined $proc_method) {
	    die "$file:$.: BODY() without PROCEDURE() or METHOD()\n";
	}
	while (!m@\)@) {
	    my $new = <>;
	    (defined ($new) && ($file eq $ARGV)) or die "$file:$.: Invalid expression\n";
	    $_ .= $new; $line_jump = 1;
	}
	if (!m@^(.*)BODY\s*\( ([^\)]*) \)\s*@x) {
	    die "$file:$.: Invalid BODY() directive\n";
	}

	$externs .= "static void\n";
	$externs .= "__enode_". ncanon ($proc_name) ."__fill_strings (BseExportStrings *es)\n";
        $externs .= "{\n";
	$externs .= "  es->blurb = ". get_variable ("HELP", "NULL") .";\n";
	$externs .= "  es->file = \"". get_variable_file ("HELP", "") ."\";\n";
	$externs .= "  es->line = ". get_variable_line ("HELP", "0") .";\n";
	$externs .= "  es->authors = ". get_variable ("AUTHORS", "NULL") .";\n";
	$externs .= "  es->license = ". get_variable ("LICENSE", "NULL") .";\n";
        $externs .= "}\n";
	$externs .= "static BseExportNodeProc __enode_". ncanon ($proc_name) ." = {\n";
	$externs .= "  { $last_node, BSE_EXPORT_NODE_PROC,\n";
	$externs .= "    $proc_method, \n";
	$externs .= "    ". get_variable ("OPTIONS", "NULL") .",\n";
	$externs .= "    $proc_category,\n";
	$externs .= "    NULL,\n"; # pixmaps
	$externs .= "    __enode_". ncanon ($proc_name) ."__fill_strings,\n";
        $externs .= "  },\n";
	$externs .= "  0, ";                               # private_id
	$externs .= ncanon ($proc_name) . "_setup, ";      # init
	$externs .= ncanon ($proc_name) . "_exec, ";       # exec
	$externs .= "\n};\n";
	$last_node = "(BseExportNode*) &__enode_". ncanon ($proc_name);

	print "#line $line \"$file\"\n$1 }\n";
	print "static BseErrorType\n";
	print "#line $line \"$file\"\n";
	print ncanon ($proc_name) . "_exec (" . $2 . ")\n";
	
	undef %proc_defs;
	undef $proc_name;
	
	$match_contents = 0; $line_jump = 1; next;
    }
    
    # read variables lines
    if ($match_contents && m@[^;]*\b($var_pattern)\s*=@) {
	my $var = $1;
	while (!has_semicolon ($_)) {
	    my $new = <>;
	    (defined ($new) && ($file eq $ARGV)) or die "$file:$.: Invalid expression\n";
	    $_ .= $new; $line_jump = 1;
	}
	$value = $_;
	$value =~ s/^.*$var[^=]*=\s*//;
	$value =~ s/;\s*$//;
	if (defined $proc_name) {
	    $proc_defs{$var} = [ $value, $afile, $line ];
	} else {
	    $var_defs{$var} = [ $value, $afile, $line ];
	}

	$match_contents = 1; $line_jump = 1; next;
    }

    # warn verbose
    if (m@\b($var_pattern|BODY|PROCEDURE|METHOD)@) {
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
print "\n";
print "/* --- Export to BSE --- */\n";
print $externs;
my $func = func_name ($file);
if ($funcname) {
    $func = func_name ($funcname);
}
print "BseExportNode* $func (void);\n";
print "BseExportNode* $func (void)\n{\n  return $last_node;\n}\n";


print "\n/*\n * Generated data ends here\n */\n";
