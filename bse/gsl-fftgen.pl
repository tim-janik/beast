#!/usr/bin/perl -w
# GSL-GENFFT - Power2 FFT C Code Generator
# Copyright (C) 2001 Tim Janik
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
    
# TODO:
# - optimize L case more
# - join 2 or 3 stages

my $indent = "  ";
my $PI = 3.1415926535897932384626433832795029;
my $ieee_type = "float";
my $tmp_ieee_type = "double"; # don't change, decreases precision _and_ performance

# arguments
my $max_fft_size = 0;
my $gen_macros = 1;
my $max_unroll = 256;
my $min_split = 2048;
my $min_compress = 512;
my $fft2loop = 0;
my $single_stage = 0;
my $negate_sign = 0;
my $Wtest = 0;

#
# parse options
#
while ($_ = $ARGV[0], defined $_ && /^-/) {
    shift;
    last if (/^--$/);
    if (/^--fft2loop$/) { $fft2loop = 1 }
    elsif (/^--analysis$/) { $negate_sign = 0 }
    elsif (/^--synthesis$/) { $negate_sign = 1 }
    elsif (/^--Wtest$/) { $Wtest = 1 }
    elsif (/^--double$/) { $ieee_type = "double" }
    elsif (/^--skip-macros$/) { $gen_macros = 0 }
    elsif (/^--max-unroll$/) { $max_unroll = shift }
    elsif (/^--split-at$/) { $min_split = shift }
    elsif (/^--min-compress$/) { $min_compress = shift }
    elsif (/^--single-stage$/) { $single_stage = shift }
}
# parse arguments
my @arguments = 0;
if (defined $ARGV[0]) {
    $max_fft_size = $ARGV[0];
    shift;
}
while (defined $ARGV[0]) {
    push @arguments, $ARGV[0];
    shift;
}

#
# check arguments and provide help
#
if ($max_fft_size && ($max_fft_size < 2 || $max_fft_size & ($max_fft_size - 1))) {
    print(STDERR "usage: gsl-genfft [options] <fftsize>\n".
	  "Produces C code to calculate an FFT of specified <size> which has\n".
	  "to be a power of 2.\n".
	  "  --max-unroll <size>   max fft size to unroll loops and bodies\n".
	  "  --min-compress <size> min fft size to unroll only loop bodies\n".
	  "  --split-at <size>     max fft size for which recursive ffts are joined\n".
	  "  --fft2loop            force bitreversing fft2 part into a loop\n".
	  "");
    #     "                                                                              |\n"
    exit 0;
}


#
# utility functions
#
sub bitreverse {
    my $h = shift;   # fft_size;
    my $n = shift;
    my $r = 0;

    while ($n) {
	$h >>= 1;
	$r |= $h if ($n & 1);
	$n >>= 1;
    }
    return $r;
}
sub Wexponent {
    my $fft_size = shift;
    my $n_points = shift;
    my $nth = shift;      # nth coefficient of this <n_points> stage

    my $r = $nth * $fft_size / $n_points;

    return $r;
}
sub Wreal {
    my $n = shift;   # fft_size
    my $k = shift;   # power

    my $x = cos ($PI * (2. * $k / $n));

    return $x == 0 ? 0 : $x; # -0.0 == 0
}
sub Wimag {
    my $n = shift;   # fft_size
    my $k = shift;

    my $x = sin ($PI * (2. * $k / $n));
    $x = -$x if ($negate_sign);

    return $x == 0 ? 0 : $x; # -0.0 == 0
}


#
# unrolled fft generation
#
sub butterfly {
    my $type = shift;
    my $offset1 = shift;
    my $offset2 = shift;
    my $Wre = shift;
    my $Wim = shift;
    my $var = shift;
    
    # define BUTTERFLY_XY(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,Wre,Wim,T1re,T1im,T2re,T2im)
    printf($indent."BUTTERFLY_%s (%s[%s], %s[%s + 1],\n".
	   $indent."              %s[%s], %s[%s + 1],\n".
	   $indent."              %s[%s], %s[%s + 1],\n".
	   $indent."              %s[%s], %s[%s + 1],\n".
	   $indent."              %s, %s);\n",
	   $type,
	   $var, $offset1, $var, $offset1,
	   $var, $offset2, $var, $offset2,
	   $var, $offset1, $var, $offset1,
	   $var, $offset2, $var, $offset2,
	   $Wre, $Wim);
}
sub butterfly_auto {
  my $fft_size = shift;
  my $index1 = shift;
  my $index2 = $index1 + shift;
  my $wk = shift;
  my $ofs = shift;
  my $inplace = shift;
  my $ib = $inplace ? "Y" : "X";
  my $rfact = Wreal ($fft_size, $wk);
  my $ifact = Wimag ($fft_size, $wk);
  my $index1r = !$inplace ? bitreverse ($fft_size, $index1 >> 1) << 1 : $index1;
  my $index2r = !$inplace ? bitreverse ($fft_size, $index2 >> 1) << 1 : $index2;
  my $btype = "XY";
  my $optimize_10 = $wk == 0;
  my $optimize_0x = $wk == $fft_size / 4;

  if (abs ($rfact - 1.0) < 0.0000000000000005 &&
      abs ($ifact - 0.0) < 0.0000000000000005) {
      $btype = "10";
      # print STDERR "10: ". $wk ."\n";
      $optimize_10 = 0 if ($optimize_10);
  } elsif (abs ($rfact - 0.0) < 0.0000000000000005 &&
	   abs ($ifact - 1.0) < 0.0000000000000005) {
      $btype = "01";
      # print STDERR "01: ". $wk ."\n";
      $optimize_0x = 0 if ($optimize_0x);
  } elsif (abs ($rfact - 0.0) < 0.0000000000000005 &&
	   abs ($ifact + 1.0) < 0.0000000000000005) {
      $btype = "0m";
      # print STDERR "0m: ". $wk ."\n";
      $optimize_0x = 0 if ($optimize_0x);
  } elsif (abs ($rfact - $ifact) < 0.0000000000000005) {
      $btype = "XX";  # grr, this actually slows down the athlon (keep "XY")
  } elsif (abs ($rfact + $ifact) < 0.0000000000000005) {
      $btype = "yY";  # grr, this actually slows down the athlon (keep "XY")
  }

  if ($optimize_10 || $optimize_0x) {
      die("optimization error for fft$fft_size: 10=". $optimize_10 .", 0x=". $optimize_0x .
	  ", Wre=". $rfact ." Wim=". $ifact .", wk=". $wk);
  }
  
  # BUTTERFLY_XY (X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,Wre,Wim,T1re,T1im,T2re,T2im);
  printf($indent."BUTTERFLY_%s (%s[%s%u],               /* W%u */\n".
	 $indent."              %s[%s%u + 1],\n".
	 $indent."              %s[%s%u],\n".
	 $indent."              %s[%s%u + 1],\n".
	 $indent."              Y[%s%u],\n".
	 $indent."              Y[%s%u + 1],\n".
	 $indent."              Y[%s%u],\n".
	 $indent."              Y[%s%u + 1],\n".
	 $indent."              (%s) %+.15f, (%s) %+.15f);\n",
	 $btype,
	 $ib, $ofs, $index1r, $wk, $ib, $ofs, $index1r, $ib, $ofs, $index2r, $ib, $ofs, $index2r,
	 $ofs, $index1, $ofs, $index1, $ofs, $index2, $ofs, $index2,
	 $tmp_ieee_type, $rfact, $tmp_ieee_type, $ifact);
}
sub unroll_stage {
    my $fft_size = shift;
    my $n_points = shift;
    my $n_rep = shift;     # repetitions
    my $unroll_outer = shift;
    
    if ($unroll_outer && $n_points == 2) {
	for (my $i = 0; $i < $n_points >> 1; $i++) {
	    for (my $j = 0; $j < $n_rep; $j++) {
		my $offset1 = ($j * $n_points + $i) * 2;
		my $offset2 = $offset1 + $n_points;
		my $offset1r = bitreverse ($fft_size, $offset1 >> 1) << 1;
		my $offset2r = bitreverse ($fft_size, $offset2 >> 1) << 1;
		my $scale = !$negate_sign ? "__1, __0" : sprintf "1.0 / (%s) %u", $tmp_ieee_type, $fft_size;
		printf($indent."BUTTERFLY_10%s (X[%s], X[%s + 1],\n".
		       $indent."                X[%s], X[%s + 1],\n".
		       $indent."                Y[%s], Y[%s + 1],\n".
		       $indent."                Y[%s], Y[%s + 1],\n".
		       $indent."                %s);\n",
		       $negate_sign ? "scale" : "",
		       $offset1r, $offset1r,
		       $offset2r, $offset2r,
		       $offset1, $offset1,
		       $offset2, $offset2,
		       $scale);
	    }
	}
    } elsif ($unroll_outer) {
	for (my $i = 0; $i < $n_points >> 1; $i++) {
	    for (my $j = 0; $j < $n_rep; $j++) {
		butterfly_auto ($fft_size,
				($j * $n_points + $i) * 2,
				$n_points,
				Wexponent ($fft_size, $n_points, $i),
				"", $n_points != 2);
	    }
	}
    } else {
	die "cannot skip outer loop unrolling for fft2" if $n_points == 2;
	
	printf "%sfor (block = 0; block < %u; block += %u) {\n", $indent, $n_points * 2 * $n_rep, $n_points * 2;
	$indent .= "  ";
	for (my $i = 0; $i < $n_points >> 1; $i++) {
	    butterfly_auto ($fft_size,
			    $i * 2,
			    $n_points,
			    Wexponent ($fft_size, $n_points, $i),
			    "block + ", $n_points != 2);
	}
	$indent =~ s/\ \ $//;
	printf "%s}\n", $indent;
    }
}
sub table_stage {
    my $fft_size = shift;
    my $n_points = shift;
    my $n_rep = shift;     # repetitions
    
    die "cannot loop over coefficient table for fft2" if $n_points == 2;

    printf "%s{\n", $indent;
    $indent .= "  ";
    printf "%sstatic const %s Wconst%u[] = {\n", $indent, $tmp_ieee_type, $n_points;
    for (my $i = 1; $i < $n_points >> 2; $i++) {
	my $wk = Wexponent ($fft_size, $n_points, $i);
	printf "%s  %+.15f, %+.15f,\n", $indent, Wreal ($fft_size, $wk), Wimag ($fft_size, $wk);
    }
    printf "%s};\n", $indent;
    printf "%sconst %s *W = Wconst%u - 2;\n", $indent, $tmp_ieee_type, $n_points;
    printf "%s%s *Z = Y + %u;\n", $indent, $ieee_type, $n_points >> 1;

    # first half loops
    printf("%sfor (offset = 0; offset < %u; offset += %u) {\n",
	   $indent, $n_points * 2 * $n_rep, $n_points * 2);
    $indent .= "  ";
    $ofs2 = sprintf "offset + %u", $n_points;
    butterfly ("10", "offset", $ofs2, "__1", "__0", "Y");
    butterfly ($negate_sign ? "0m" : "01", "offset", $ofs2, "__0", "__1", "Z");
    $indent =~ s/\ \ $//;
    printf "%s}\n", $indent;
    printf "%sfor (butterfly = 2; butterfly < %u; butterfly += 2) {\n", $indent, $n_points >> 1;
    printf "%s  Wre = W[butterfly]; Wim = W[butterfly + 1];\n", $indent;
    printf "%s  for (block = 0; block < %u; block += %u) {\n", $indent, $n_points * 2 * $n_rep, $n_points * 2;
    $indent .= "    ";
    printf "%soffset = butterfly + block;\n", $indent;
    $ofs2 = sprintf "offset + %u", $n_points;
    butterfly ("XY", "offset", $ofs2, "Wre", "Wim", "Y");
    butterfly ($negate_sign ? "yX" : "Yx", "offset", $ofs2, "Wre", "Wim", "Z");
    $indent =~ s/\ \ \ \ $//;
    printf "%s  }\n", $indent;
    printf "%s}\n", $indent;

    # second half loops
    if (0) {
	printf("%sfor (offset = %u; offset < %u + %u; offset += %u) {\n",
	       $indent, $n_points >> 1, $n_points >> 1, $n_points * 2 * $n_rep, $n_points * 2);
	$indent .= "  ";
	$ofs2 = sprintf "offset + %u", $n_points;
	butterfly ($negate_sign ? "0m" : "01", "offset", $ofs2, "__0", "__1", "Y");
	$indent =~ s/\ \ $//;
	printf "%s}\n", $indent;
	printf "%sW -= %u;\n", $indent, $n_points >> 1;
	printf "%sfor (butterfly = %u + 2; butterfly < %u; butterfly += 2) {\n", $indent, $n_points >> 1, $n_points;
	printf "%s  Wre = W[butterfly]; Wim = W[butterfly + 1];\n", $indent;
	printf "%s  for (block = 0; block < %u; block += %u) {\n", $indent, $n_points * 2 * $n_rep, $n_points * 2;
	$indent .= "    ";
	printf "%soffset = butterfly + block;\n", $indent;
	$ofs2 = sprintf "offset + %u", $n_points;
	butterfly ($negate_sign ? "yX" : "Yx", "offset", $ofs2, "Wre", "Wim", "Y");
	$indent =~ s/\ \ \ \ $//;
	printf "%s  }\n", $indent;
	printf "%s}\n", $indent;
    }
    
    $indent =~ s/\ \ $//;
    printf "%s}\n", $indent;
}
sub bitreverse_fft2 {
    # mul_result = gsl_complex (c1.re * c2.re - c1.im * c2.im, c1.re * c2.im + c1.im * c2.re);
    printf "
static inline void
bitreverse_fft2analysis (const unsigned int n,
                         const %-6s        *X,
                         %-6s              *Y)
{
  const unsigned int n2 = n >> 1, n1 = n + n2, max = n >> 2;
  unsigned int i, r;
  
  BUTTERFLY_10 (X[0], X[1],
		X[n], X[n + 1],
		Y[0], Y[1],
		Y[2], Y[3],
		__1, __0);
  BUTTERFLY_10 (X[n2], X[n2 + 1],
		X[n1], X[n1 + 1],
		Y[4], Y[5],
		Y[6], Y[7],
		__1, __0);
  for (i = 1, r = 0; i < max; i++)
    {
      unsigned int k, j = n >> 1;

      while (r >= j)
	{
	  r -= j;
	  j >>= 1;
	}
      r |= j;

      k = r >> 1;
      j = i << 3;
      BUTTERFLY_10 (X[k], X[k + 1],
		    X[k + n], X[k + n + 1],
		    Y[j], Y[j + 1],
		    Y[j + 2], Y[j + 3],
		    __1, __0);
      k += n2;
      j += 4;
      BUTTERFLY_10 (X[k], X[k + 1],
		    X[k + n], X[k + n + 1],
		    Y[j], Y[j + 1],
		    Y[j + 2], Y[j + 3],
		    __1, __0);
    }
}
static inline void
bitreverse_fft2synthesis (const unsigned int n,
                          const %-6s        *X,
                          %-6s              *Y)
{
  const unsigned int n2 = n >> 1, n1 = n + n2, max = n >> 2;
  unsigned int i, r;
  %s scale = n;

  scale = 1.0 / scale;
  BUTTERFLY_10scale (X[0], X[1],
		     X[n], X[n + 1],
		     Y[0], Y[1],
		     Y[2], Y[3],
		     scale);
  BUTTERFLY_10scale (X[n2], X[n2 + 1],
		     X[n1], X[n1 + 1],
		     Y[4], Y[5],
		     Y[6], Y[7],
		     scale);
  for (i = 1, r = 0; i < max; i++)
    {
      unsigned int k, j = n >> 1;

      while (r >= j)
	{
	  r -= j;
	  j >>= 1;
	}
      r |= j;

      k = r >> 1;
      j = i << 3;
      BUTTERFLY_10scale (X[k], X[k + 1],
			 X[k + n], X[k + n + 1],
			 Y[j], Y[j + 1],
			 Y[j + 2], Y[j + 3],
			 scale);
      k += n2;
      j += 4;
      BUTTERFLY_10scale (X[k], X[k + 1],
			 X[k + n], X[k + n + 1],
			 Y[j], Y[j + 1],
			 Y[j + 2], Y[j + 3],
			 scale);
    }
}
", $ieee_type, $ieee_type, $ieee_type, $ieee_type, $tmp_ieee_type;

    # testing:
    # define BUTTERFLY_10(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,Wre,Wim,T1re,T1im,T2re,T2im) \
    #  { int a = &X1re-X, b = &X2re-X, c = &Y1re-Y; \
    #    printf ("bufferfly: [%u,%u] -> [%u,%u]\n", (a)>>1, (b)>>1, (c)>>1, (c+2)>>1); \
    #    if (((a)|(b)|(c))&1) perror("****eek, extraneous bits!");}
    # int main (int argc, char *argv[]) {
    #   bitreverse_fft2a (atoi (argv[1]), 0, 0);
    #     return 0;
    # }
}

#
# fft generation routines
#
sub gen_fft_loop_unoptimized {
    my $fft_size = shift;
    my $n_points = shift;
    my $n_rep = shift;     # repetitions
    my $ofs2;
    
    die "invalid fft size: " . $n_points if ($n_points < 4);

    # incremental coefficient W for this fft
    my $theta = $PI / ($n_points >> 1);
    $theta = -$theta if ($negate_sign);
    my $re = sin (0.5 * $theta);
    my $im = sin ($theta);
    $re = $re * $re * -2.;
    
    # init variables, start loops
    printf "%sWre = 1.0; Wim = 0.0;\n", $indent;
    printf "%sfor (butterfly = 0; butterfly < %u; butterfly += 2) {\n", $indent, $n_points;
    $indent .= "  ";
    printf "%sfor (block = 0; block < %u; block += %u) {\n", $indent, $n_rep * $n_points << 1, $n_points << 1;
    $indent .= "  ";

    # do the butterfly
    printf "%soffset = butterfly + block;\n", $indent;
    $ofs2 = sprintf "offset + %u", $n_points;
    butterfly ("XY", "offset", $ofs2, "Wre", "Wim", "Y");

    # close inner loop, update W
    $indent =~ s/\ \ $//;
    printf "%s}\n", $indent;
    printf("%sWMULTIPLY (Wre, Wim, %+.15f, %+.15f);\n",
	   $indent, $re, $im);

    # close outer loop, done
    $indent =~ s/\ \ $//;
    printf "%s}\n", $indent;
}
sub gen_fft_loop_o10o0x {
    my $fft_size = shift;
    my $n_points = shift;
    my $n_rep = shift;     # repetitions
    my $ofs2;
    
    die "invalid fft size: " . $n_points if ($n_points < 4);

    # coefficient {1,0} loop
    printf "%sfor (offset = 0; offset < %u; offset += %u) {\n", $indent, $n_rep * $n_points << 1, $n_points << 1;
    $indent .= "  ";
    $ofs2 = sprintf "offset + %u", $n_points;
    butterfly ("10", "offset", $ofs2, "__1", "__0", "Y");
    $indent =~ s/\ \ $//;
    printf "%s}\n", $indent;

    # incremental coefficient W for this fft
    my $theta = $PI / ($n_points >> 1);
    $theta = -$theta if ($negate_sign);
    my $re = sin (0.5 * $theta);
    my $im = sin ($theta);
    $re = $re * $re * -2.;
    
    # loop first half
    if (2 < $n_points >> 1) {
	printf "%sWre = %+.15f; Wim = %+.15f;\n", $indent, $re + 1.0, $im;
	printf "%sfor (butterfly = 2; butterfly < %u; butterfly += 2) {\n", $indent, $n_points >> 1;
	$indent .= "  ";
	printf "%sfor (block = 0; block < %u; block += %u) {\n", $indent, $n_rep * $n_points << 1, $n_points << 1;
	$indent .= "  ";
	# do the butterfly
	printf "%soffset = butterfly + block;\n", $indent;
	$ofs2 = sprintf "offset + %u", $n_points;
	butterfly ("XY", "offset", $ofs2, "Wre", "Wim", "Y");
	# close inner loop, update W
	$indent =~ s/\ \ $//;
	printf "%s}\n", $indent;
	printf("%sWMULTIPLY (Wre, Wim, %+.15f, %+.15f);\n",
	       $indent, $re, $im);
	# close outer loop, done
	$indent =~ s/\ \ $//;
	printf "%s}\n", $indent;
    }
    
    # coefficient {0,1} loop
    printf "%sfor (offset = %u; offset < %u; offset += %u) {\n", $indent, $n_points >> 1, $n_rep * $n_points << 1, $n_points << 1;
    $indent .= "  ";
    $ofs2 = sprintf "offset + %u", $n_points;
    butterfly ($negate_sign ? "0m" : "01", "offset", $ofs2, "__0", "__1", "Y");
    $indent =~ s/\ \ $//;
    printf "%s}\n", $indent;

    # loop second half
    if (($n_points >> 1) + 2 < $n_points) {
	if ($negate_sign) {
            printf "%sWre = %+.15f; Wim = %+.15f;\n", $indent, $im, -$re - 1.0;
	} else {
	    printf "%sWre = %+.15f; Wim = %+.15f;\n", $indent, -$im, $re + 1.0;
	}
	printf "%sfor (butterfly = %u; butterfly < %u; butterfly += 2) {\n", $indent, ($n_points >> 1) + 2, $n_points;
	$indent .= "  ";
	printf "%sfor (block = 0; block < %u; block += %u) {\n", $indent, $n_rep * $n_points << 1, $n_points << 1;
	$indent .= "  ";
	# do the butterfly
	printf "%soffset = butterfly + block;\n", $indent;
	$ofs2 = sprintf "offset + %u", $n_points;
	butterfly ("XY", "offset", $ofs2, "Wre", "Wim", "Y");
	# close inner loop, update W
	$indent =~ s/\ \ $//;
	printf "%s}\n", $indent;
	printf("%sWMULTIPLY (Wre, Wim, %+.15f, %+.15f);\n",
	       $indent, $re, $im);
	# close outer loop, done
	$indent =~ s/\ \ $//;
	printf "%s}\n", $indent;
    }
}
sub fft_loop_macros {
    # mul_result = gsl_complex (c1.re * c2.re - c1.im * c2.im, c1.re * c2.im + c1.im * c2.re);
    print "
#define WMULTIPLY(Wre,Wim,Dre,Dim) { \\
  register $tmp_ieee_type T1re, T1im, T2re, T2im; \\
  T1re = Wre * Dre;  \\
  T1im = Wim * Dre;  \\
  T2re = Wim * Dim;  \\
  T2im = Wre * Dim;  \\
  T1re -= T2re;      \\
  T1im += T2im;      \\
  Wre += T1re;       \\
  Wim += T1im;       \\
}";
}
sub butterfly_macros {
    # mul_result = gsl_complex (c1.re * c2.re - c1.im * c2.im, c1.re * c2.im + c1.im * c2.re);
    # add_result = gsl_complex (c1.re + c2.re, c1.im + c2.im);
    print "
#define BUTTERFLY_XY(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,Wre,Wim) { \\
  register $tmp_ieee_type T1re, T1im, T2re, T2im; \\
  T1re = X2re * Wre;  \\
  T1im = X2im * Wre;  \\
  T2re = X2im * Wim;  \\
  T2im = X2re * Wim;  \\
  T1re -= T2re;       \\
  T1im += T2im;       \\
  T2re = X1re - T1re; \\
  T2im = X1im - T1im; \\
  Y1re = X1re + T1re; \\
  Y1im = X1im + T1im; \\
  Y2re = T2re;        \\
  Y2im = T2im;        \\
}
#define BUTTERFLY_Yx(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,Wre,Wim) { \\
  register $tmp_ieee_type T1re, T1im, T2re, T2im; \\
  T1re = X2re * Wim;  \\
  T1im = X2im * Wim;  \\
  T2re = X2im * Wre;  \\
  T2im = X2re * Wre;  \\
  T1re += T2re;       \\
  T1im -= T2im;       \\
  T2re = X1re + T1re; \\
  T2im = X1im + T1im; \\
  Y1re = X1re - T1re; \\
  Y1im = X1im - T1im; \\
  Y2re = T2re;        \\
  Y2im = T2im;        \\
}
#define BUTTERFLY_yX(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,Wre,Wim) { \\
  register $tmp_ieee_type T1re, T1im, T2re, T2im; \\
  T1re = X2re * Wim;  \\
  T1im = X2im * Wim;  \\
  T2re = X2im * Wre;  \\
  T2im = X2re * Wre;  \\
  T1re += T2re;       \\
  T1im -= T2im;       \\
  T2re = X1re - T1re; \\
  T2im = X1im - T1im; \\
  Y1re = X1re + T1re; \\
  Y1im = X1im + T1im; \\
  Y2re = T2re;        \\
  Y2im = T2im;        \\
}
#define BUTTERFLY_10(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,_1,_2) { \\
  register $tmp_ieee_type T2re, T2im; \\
  T2re = X1re - X2re; \\
  T2im = X1im - X2im; \\
  Y1re = X1re + X2re; \\
  Y1im = X1im + X2im; \\
  Y2re = T2re;        \\
  Y2im = T2im;        \\
}
#define BUTTERFLY_01(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,_1,_2) { \\
  register $tmp_ieee_type T2re, T2im; \\
  T2re = X1re + X2im; \\
  T2im = X1im - X2re; \\
  Y1re = X1re - X2im; \\
  Y1im = X1im + X2re; \\
  Y2re = T2re;        \\
  Y2im = T2im;        \\
}
#define BUTTERFLY_0m(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,_1,_2) { \\
  register $tmp_ieee_type T2re, T2im; \\
  T2re = X1re - X2im; \\
  T2im = X1im + X2re; \\
  Y1re = X1re + X2im; \\
  Y1im = X1im - X2re; \\
  Y2re = T2re;        \\
  Y2im = T2im;        \\
}
#define BUTTERFLY_XX(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,Wre,_2) { \\
  register $tmp_ieee_type T1re, T1im, T2re, T2im; \\
  T1re = X2re * Wre;  \\
  T1im = X2im * Wre;  \\
  T2re = T1im; \\
  T2im = T1re; \\
  T1re -= T2re;       \\
  T1im += T2im;       \\
  T2re = X1re - T1re; \\
  T2im = X1im - T1im; \\
  Y1re = X1re + T1re; \\
  Y1im = X1im + T1im; \\
  Y2re = T2re;        \\
  Y2im = T2im;        \\
}
#define BUTTERFLY_yY(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,Wre,_2) { \\
  register $tmp_ieee_type T1re, T1im, T2re, T2im; \\
  T1re = X2re * Wre;  \\
  T1im = X2im * Wre;  \\
  T2re = T1im;  \\
  T2im = T1re;  \\
  T1re += T2re;       \\
  T1im -= T2im;       \\
  T2re = X1re - T1re; \\
  T2im = X1im - T1im; \\
  Y1re = X1re + T1re; \\
  Y1im = X1im + T1im; \\
  Y2re = T2re;        \\
  Y2im = T2im;        \\
}
#define BUTTERFLY_10scale(X1re,X1im,X2re,X2im,Y1re,Y1im,Y2re,Y2im,S) { \\
  register $tmp_ieee_type T2re, T2im; \\
  T2re = X1re - X2re; \\
  T2im = X1im - X2im; \\
  Y1re = X1re + X2re; \\
  Y1im = X1im + X2im; \\
  Y2re = T2re * S;    \\
  Y2im = T2im * S;    \\
  Y1re *= S;          \\
  Y1im *= S;          \\
}
";
}


#
# stage generation
#
sub gen_stage {
    my $fft_size = shift;
    my $n_points = shift;
    my $kind = shift;
    my $times = $fft_size / $n_points;
    
    if ($kind eq 'S') {
	printf "\n%s/* skipping %u times fft%u */\n", $indent, $times, $n_points;
	return;
    }
    if ($n_points == 2) {   # input stage needs special handling
	if ($kind eq 'L') {
	    printf "\n%s/* perform fft2 and bitreverse input */\n", $indent;
	    printf "%sbitreverse_fft2%s (%u, X, Y);\n", $indent, $negate_sign ? "synthesis" : "analysis", $fft_size;
	} elsif ($kind eq 'F') {
	    printf "\n%s/* perform %u times fft2 */\n", $indent, $times;
	    unroll_stage ($fft_size, $n_points, $times, 1);
	} else {
	    die "need one of 'L' or 'F' for input stage";
	}
    } else {
	if ($kind eq 'L') {
            printf "\n%s/* perform %u times fft%u */\n", $indent, $times, $n_points;
	    gen_fft_loop_o10o0x ($fft_size, $n_points, $times);
	} elsif ($kind eq 'F') {
            printf "\n%s/* perform %u times fft%u */\n", $indent, $times, $n_points;
	    unroll_stage ($fft_size, $n_points, $times, 1);
	} elsif ($kind eq 'R') {
            printf "\n%s/* perform %u times fft%u */\n", $indent, $times, $n_points;
	    unroll_stage ($fft_size, $n_points, $times, $times == 1);
	} elsif ($kind eq 'T') {
            printf "\n%s/* perform %u times fft%u */\n", $indent, $times, $n_points;
	    table_stage ($fft_size, $n_points, $times);
	} elsif ($kind eq 'X') {
            printf "\n%s/* perform %u times fft%u */\n", $indent, $times, $n_points;
	    for (my $i = 0; $i < $times; $i++) {
		if ($i) {
		    printf($indent."gsl_power2_fft%u%s_skip2 (X + %u, Y + %u);\n",
			   $n_points, $negate_sign ? "synthesis" : "analysis", $n_points * $i << 1, $n_points * $i << 1);
		} else {
		    printf($indent."gsl_power2_fft%u%s_skip2 (X, Y);\n",
			   $n_points, $negate_sign ? "synthesis" : "analysis");
		}
	    }
	} else {
	    die "unknown kind: $kind";
	}
    }
}

#
# test output
#
if ($Wtest) {
    my $fft_size = $max_fft_size;
    
    for (my $i = 0; $i < $fft_size >> 1; $i++) {
	my $wk = Wexponent ($fft_size, $fft_size, $i);
	printf "$fft_size: %4u:  %+.15f, %+.15f,\n", $i, Wreal ($fft_size, $wk), Wimag ($fft_size, $wk);
    }
    exit 0;
}
if (0) {
    # reversal test
    for (my $i = 0; $i < $fft_size; $i++) {
	printf STDERR "%-3u <-> %3u   %s\n", $i, bitreverse ($fft_size, $i), $i % 4 ? "" : "XXX";
    }
}


#
# main output
#
if ($gen_macros) {
    butterfly_macros ();
    fft_loop_macros ();
    bitreverse_fft2 ();
}
exit 0 if (!$max_fft_size);

my $max_stages = 0;
for ($i = $max_fft_size >> 1; $i; $i >>= 1) {
    $max_stages++;
}
die "missing stage specifications ($max_stages stages, $#arguments specs)" if ($max_stages > $#arguments);
die "too many stage specifications ($max_stages stages, $#arguments specs)" if ($max_stages < $#arguments);
print "/**\n";
printf(" ** Generated data (by gsl-genfft $max_fft_size");
for (my $i = 1; $i < @arguments; $i++) {
    printf " %s", uc ($arguments[$i]);
}
printf ")\n";
print " **/\n";
{
    my $fft_size = $max_fft_size;
    my $skip2 = uc ($arguments[1]) eq 'S';

    printf STDERR "FFT-%-5u: ", $fft_size;
    
    printf "static void\n";
    printf("gsl_power2_fft%u%s%s (const %s *X, %s *Y)\n{\n",
	   $fft_size, $negate_sign ? "synthesis" : "analysis",
	   $skip2 ? "_skip2" : "",
	   $ieee_type, $ieee_type);
    printf "%sregister unsigned int butterfly, block, offset;\n", $indent;
    printf "%sregister %s Wre, Wim;\n\n", $indent, $tmp_ieee_type, $tmp_ieee_type;
    printf "%sbutterfly = block = offset = 0, Wre = Wim = 0.0; /* silence compiler */\n", $indent;
    
    my $seen_rule = 0;
    for (my $i = 1; $i < @arguments && 1 << $i <= $fft_size; $i++) {
	my $stage = uc ($arguments[$i]);
	printf STDERR "%u-%s ", 1 << $i, $stage;
	die "X follows non Skip" if ($stage eq "X" && $seen_rule);
	gen_stage ($fft_size, 1 << $i, $stage);
	$seen_rule |= (1 << $i) > 2 && !($stage eq "S");
    }
    printf "}\n";
    printf STDERR "\n";
}
print "\n/**\n";
print " ** Generated data ends here\n";
print " **/\n";
