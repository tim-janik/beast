#!/usr/bin/perl -w

use strict;
use vars qw(
  $config
  @doc_struct
  $doc_header
  $doc_footer
  $doc_input
  $doc_output
  $doc_tag
  $tag_handle
);

use Text::Wrap;

# parse options
while ($_ = $ARGV[0], defined $_ && /^-/) {
    shift;
    last if /^--$/;
    if (/^--config$/) { $config = shift; next; }
}

dang("Provide a config file with --config option!") unless ($config);
dang("Config file unreadable!") unless (-r $config);

do $config;
dang("Config file error: $@") if $@;

if (defined $doc_input and defined $doc_output) {
  if ($doc_input ne "stdin" and $doc_output ne "stdout") {
	if (-r $doc_input and -r $doc_output) {
	  unless ((stat($doc_input))[9] > (stat($doc_output))[9]) {
		# print STDERR "Output is up to date\n";
		exit;
	  }
	}
  }
}

my ($parsed, $doc_vars) = parse_document($doc_input);
dang("Document seems to be empty!") unless $parsed;

my $document;

$document .= expand_variables(readfile($doc_header), $doc_vars, 0) if $doc_header;
$document .= expand_document($parsed, $doc_vars, $_) foreach (@doc_struct);
$document .= expand_variables(readfile($doc_footer), $doc_vars, $#$parsed) if $doc_footer;

# Output
if (defined $doc_output and $doc_output ne 'stdout') {
  open(OUT, ">$doc_output") or dang("Can't write to $doc_output!");
  print OUT $document;
  close(OUT);
} else {
  print $document;
}

exit;

sub parse_document {
  my $document = shift;

  if (defined $document and $document ne 'stdin') {
	close(STDIN);
	open(STDIN, "<$doc_input") or dang("Can't read from $document!");
  }

  my ($current_tag, @stack, @parsed, %doc_vars, $n);

  $n = 0;

  while (<STDIN>) {
	chomp;

	if (s/^$tag_handle$doc_tag-(.*)$tag_handle$/$1/ or eof) {
	  if ($current_tag) {
		# Don't lose the last line
		push(@stack, $_) if eof;

		unless ($current_tag eq 'variable-definitions') {
		  my $fresh = join(' ', @stack);

		  push (@parsed, {
			text  => $fresh,
			tag   => $current_tag,
			order => $n
		  });

		  $n++;

		} else {
		  foreach (@stack) {
			my ($var, $value) = split(/\s*=\s*/, $_, 2);

			$doc_vars{$var}{$n} = $value;
		  }
		}

		# Empty the stack
		@stack = ();
	  }

	  # Save the tag processed
	  $current_tag = lc($_);
	} else {
	  # Skip if we are not in a tag
	  next unless $current_tag;

	  # Skip white-space only lines
	  next if /^\s*$/;

	  # Save line to the stack
	  push(@stack, $_);
	}
  }

  return (\@parsed, \%doc_vars);
}

sub expand_document {
  my @parsed = @{shift @_};
  my %docvar = %{shift @_};
  my %struct = %{shift @_};

  my $max = scalar @parsed;

  return unless (@parsed and %struct);
  
  my (
	@document, @processed, %freeze, %ids,
	%order, @nest, $last_tag, %counter
  );

  # do repetation and redundancy adjustments
  @processed = @parsed;
  @parsed = ();

  foreach (@processed) {
	my $tag   = ${$_}{'tag'};
	my $text  = ${$_}{'text'};
	my $order = ${$_}{'order'};

	# skip items that have not defined in the %struct
	next if (not defined $struct{$tag});

	# skip items that are repeating but not allowed to repeat
	next if (defined $freeze{$tag} and $freeze{$tag} eq $text and not $struct{$tag}{'repeat'});

	$freeze{$tag} = $text;
	$text = expand_inline($text, $struct{'_inline'});
	$text = expand_variables($text, \%docvar, $order);

	push(@parsed, {
	  'tag'   => $tag,
	  'text'  => $text,
	  'order' => $order
	});
  }

  %freeze = ();

  for (my $n = 0; $n <= $#parsed; $n++) {
	my $tag        = ${$parsed[$n]}{'tag'};
	my $text       = ${$parsed[$n]}{'text'};
	my $real_order = ${$parsed[$n]}{'order'};

	# save the order in which tags start to appear
	$order{$tag} = $n if (not defined $order{$tag});

	# increment the tag counter
	$counter{$tag}++;
	$ids{$tag}++;

    # skip items that have a limit which is already reached
	next if (defined $struct{$tag}{'olimit'} and $struct{$tag}{'olimit'} < $ids{$tag});
	next if (defined $struct{$tag}{'limit'}  and $struct{$tag}{'limit'}  < $counter{$tag});

	# handle nesting tags
	if (defined $freeze{$tag}) {
	  while (my $level = pop(@nest)) {

		# if a nesting tag should appear before the last inserted tag
		# it should be added before the last item
		if ($order{$tag} < $order{$last_tag}) {
		  push(@document, ${$level}{'text'});
		} else {
		  splice(@document, -1, 0, ${$level}{'text'});
		}

		delete $freeze{$tag};
		last if (${$level}{'tag'} eq $tag);
	  }

	  if (scalar(@nest) eq 0) {
		%freeze = ();
	  }
	}

	# handle tag starts
	if (defined $struct{$tag}{'start'}) {
	  my $line = $struct{$tag}{'start'};

	  $line = expand_variables($line, \%docvar, $real_order);

	  $line =~ s/%s\b/$text/g;
	  $line =~ s/%(([0-9]+)?-?([0-9]+)?)?p\b/&wrap(' ' x ($2 or 0), ' ' x ($3 or 0), $text)/ge;
	  $line =~ s/%(([a-zA-Z]+)?-?([0-9]+)?)?c\b/&countf($tag, \%counter, \%struct, $2, $3)/ge;
	  $line =~ s/%i\b/$tag . $ids{$tag}/ge;

	  push(@document, $line);
	  $last_tag = $tag;
	}

	# handle tag ends
	if (defined $struct{$tag}{'end'}) {
	  my $line = $struct{$tag}{'end'};

	  $line = expand_variables($line, \%docvar, $real_order);

	  $line =~ s/%s\b/$text/g;
	  $line =~ s/%(([0-9]+)?-?([0-9]+)?)?p\b/&wrap(' ' x ($2 or 0), ' ' x ($3 or 0), $text)/ge;
	  $line =~ s/%(([a-zA-Z]+)?-?([0-9]+)?)?c\b/&countf($tag, \%counter, \%struct, $2, $3)/ge;
	  $line =~ s/%i\b/$tag . $ids{$tag}/ge;

	  $freeze{$tag} = $text;

	  # if a tag has an end, it means it should nest
	  push(@nest, {
		text => $line,
		tag  => $tag
	  });
	}

	# handle counter resets
	if (defined $struct{$tag}{'reset'}) {
	  my $reset = $struct{$tag}{'reset'};
	  $counter{$reset} = 0;
	}
  }

  # add all remaining tags that should nest
  while (my $level = pop(@nest)) {
	push(@document, ${$level}{'text'});
  }

  # add header and footer
  unshift(@document, expand_variables(readfile($struct{'_header'}), \%docvar, 0)) if $struct{'_header'};
  unshift(@document, expand_variables($struct{'_prepend'}, \%docvar, 0))          if $struct{'_prepend'};
  push   (@document, expand_variables($struct{'_append'}, \%docvar, $max))           if $struct{'_append'};
  push   (@document, expand_variables(readfile($struct{'_footer'}), \%docvar, $max)) if $struct{'_footer'};

  return join('', @document);
}

sub expand_variables {
  my $line = shift;
  my $vars = shift;
  my $n    = shift;

  return $line unless $line =~ /#([A-Za-z0-9._]+)#/;

  return $line unless defined $vars;
  return $line unless $vars =~ /HASH/;

  my %vars = %{$vars};
  return $line unless %vars;

  my (%tmp_vars, $closest);

  foreach (keys %vars) {
	my $var = $_;

	foreach (sort {$a <=> $b} keys %{$vars{$var}}) {
	  $closest = $_ if $_ <= $n;
	}

	next unless defined $closest;

	$tmp_vars{$var} = $vars{$var}{$closest};
  }

  eval '$line =~ s/#([A-Za-z0-9._]+)#/$tmp_vars{$1}/g;';

  return $line;
}

sub expand_inline {
  my $line   = shift;
  my $struct = shift;

  return $line unless defined $struct;
  return $line unless $struct =~ /HASH/;

  my %struct = %{$struct};
  return $line unless %struct;

  my %rx;

  my $at = "\b";

  $line =~ s/\\@/$at/g;

  %rx = (
	nl     => $tag_handle . "NL" . $tag_handle . " ?",
	url    => $tag_handle . "URL ([^ ]+) ([^$tag_handle]*)" . $tag_handle,
	mail   => $tag_handle . "MAIL ([^ ]+) ([^$tag_handle]*)" . $tag_handle,
	strong => $tag_handle . "STRONG ([^$tag_handle]+)" . $tag_handle,
	emph   => $tag_handle . "EMPH ([^$tag_handle]+)" . $tag_handle,
	ul     => $tag_handle . "UL ([^$tag_handle]+)" . $tag_handle,
  );

  while (my ($key, $regex) = each(%rx)) {
	my $substitute = $struct{$key};
	next unless defined $substitute;

	eval "\$line =~ s{\$regex}{$substitute}g;";
  }

  $line =~ s/$at/@/g;

  return $line;
}

sub dang {
  # exit with a message, similar to die()ing
  my $message = shift;
  print STDERR $message . "\n" if $message;
  exit 1;
}

sub readfile {
  my $file = shift;

  return unless defined $file;
  dang("File $file unreadable!") unless (-r $file);

  my @file;

  open(FILE, "<$file");
  push(@file, $_) while (<FILE>);
  close(FILE);

  return join('', @file);
}

sub int_to_alpha {
  my $num = shift;
  return unless defined $num;

  $num = $num % 25;

  return chr($num + 97);
}

sub int_to_Alpha {
  my $num = shift;
  return unless defined $num;

  $num = $num % 25;

  return chr($num + 64);
}

sub int_to_roman {
  return shift;
}

sub int_to_Roman {
  return shift;
}

sub countf {
  my $tag     = shift;
  my %counter = %{shift @_};
  my %struct  = %{shift @_};
  my $ptag    = shift;
  my $width   = shift;

  $tag = $ptag if defined $ptag;
  $width = '' unless defined $width;

  my $count = $counter{$tag};

  if (defined $struct{$tag}{'counter'}) {
	if      ($struct{$tag}{'counter'} eq 'alpha') {
	  $count = int_to_alpha($count);
	} elsif ($struct{$tag}{'counter'} eq 'Alpha') {
	  $count = int_to_Alpha($count);
	} elsif ($struct{$tag}{'counter'} eq 'roman') {
	  $count = int_to_roman($count);
	} elsif ($struct{$tag}{'counter'} eq 'Roman') {
	  $count = int_to_Roman($count);
	}
  }

  return sprintf('%-' . $width . 's', $count);
}

# vim: ts=4 sw=2
