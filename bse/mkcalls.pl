#!/usr/bin/perl -w

my $magic_id = "MAKE_CALLS_FOR";
my $prod1 = "";
my $tail1 = "";
my $head1 = "";
my $prod2 = "";
my $tail2 = "";
my $head2 = "";

my $values = "";

# parse options
while ($_ = $ARGV[0], /^-/) {
    shift;
    last if /^--$/;
    if (/^--magic$/) { $magic_id = shift }
    elsif (/^--prod1$/) { $prod1 = $prod1 . shift }
    elsif (/^--tail1$/) { $tail1 = $tail1 . shift }
    elsif (/^--head1$/) { $head1 = $head1 . shift }
    elsif (/^--prod2$/) { $prod2 = $prod2 . shift }
    elsif (/^--tail2$/) { $tail2 = $tail2 . shift }
    elsif (/^--head2$/) { $head2 = $head2 . shift }
}

print "\n/*\n * Generated data (by mkcalls.pl";
print ")\n */\n\n";

while (<>) {
    my $file = $ARGV;

    # eat stuff until magic id
    next if (!m@ ^ $magic_id \s* = \s* { \s* $ @x);

    # ok, found id
    for ($_ = <>; !m@ } @x; $_ = <>) {
	# strip cruft, i.e. whitespaces and comments
	$_ =~ s/\n//;
	$_ =~ s@/\*.*\*/@@g;
	$_ =~ s/^\s*//;
	$_ =~ s/\s*$//;
	$values = $values . $_;
    }
}

if (length($head1)) {
    my $prod = $head1;

    $prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
    $prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
    print "$prod\n";
}

if (length($prod1)) {
    my $n = 1;

    for $value (split /\s*,\s*/, $values) {
	my $prod = $prod1;
	
	$prod =~ s/\@value\@/$value/g;
	$prod =~ s/\@n\@/$n/g;
	$prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
	$prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
	print "$prod\n";
	$n++;
    }
}

if (length($tail1)) {
    my $prod = $tail1;

    $prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
    $prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
    print "$prod\n";
}

if (length($head2)) {
    my $prod = $head2;

    $prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
    $prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
    print "$prod\n";
}

if (length($prod2)) {
    my $n = 1;

    for $value (split /\s*,\s*/, $values) {
	my $prod = $prod2;
	
	$prod =~ s/\@value\@/$value/g;
	$prod =~ s/\@n\@/$n/g;
	$prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
	$prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
	print "$prod\n";
	$n++;
    }
}

if (length($tail2)) {
    my $prod = $tail2;

    $prod =~ s/\\a/\a/g; $prod =~ s/\\b/\b/g; $prod =~ s/\\t/\t/g; $prod =~ s/\\n/\n/g;
    $prod =~ s/\\f/\f/g; $prod =~ s/\\r/\r/g;
    print "$prod\n";
}

print "\n/*\n * Generated data ends here\n */\n";
