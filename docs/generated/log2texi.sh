#!/bin/bash

SYMBOL_PATTERN='[*0-9A-Z?][*0-9A-Za-z_?]*\(()\)\{0,1\}'
  FUNC_PATTERN='[*0-9A-Za-z?][*0-9A-Za-z_?]*\(()\)\{0,1\}'
   SYMBOL_GLIB='\(G[*A-Z?]\+[*a-z?][*0-9A-Za-z_?]*\)'
 SYMBOL_PREFIX='\(Sfi\|Gsl\|Bse\|Bst\|Gxk\|Gtk\|Gdk\|Gnome\)' # SYMBOL_GLIB instead of 'G'
   FUNC_PREFIX='\(sfi\|gsl\|bse\|bst\|gxk\|gtk\|gdk\|gnome\|g\)'
  MACRO_PREFIX='\(SFI\|GSL\|BSE\|BST\|GXK\|GTK\|GDK\|GNOME\|G\)'
CONST1_EXPR='\b\([0-9]\+\.[0-9]\+\([eE][+-]\?[0-9]\+\)\?\)\b'
CONST2_EXPR='\b\([0-9]\+\.\?\([eE][+-]\?[0-9]\+\)\)\b'
CONST3_EXPR='\b\([0-9]\+\.\)\B'
CONST4_EXPR='\B\(\.[0-9]\+\([eE][+-]\?[0-9]\+\)\?\)\b'
CONST5_EXPR='\b\([0-9]\+[LlUu]\+\)\b'
CONST6_EXPR='\b\([0-9]\+\)\b'
CONST7_EXPR='\b\(0[xX][A-Fa-f0-9]\+\)\b'

export SYMBOL_PATTERN FUNC_PATTERN SYMBOL_PREFIX FUNC_PREFIX MACRO_PREFIX

function ChangeLog2texi ()
{
    # file name markup: file names may be pretty much everything except
    # - names starting with an @
    # - names containing ( or ) or , or : or ' or spaces
    # - names containing @ or { or } (due to a makeinfo bug)
    # file start: [-!-&+.-9;-?A-z|~*]
    # file rest:  [-!-&+.-9;-?A-z|~*]
    sed \
	-e 's/[{}@]/@&/g' \
        -e '/^[ 	]\+\* [^:]\+:/ { ' \
          -e ':NextFile;' \
          -e 's/^\([^:]*\*[^:]*\) \([-!-&+.-9;-?A-z|~*][-!-&+.-9;-?A-z|~*]*\)/\1 @logentry{\2}/;' \
          -e 'tNextFile;' \
        -e '}' \
	-e 's/<\([^@]*@[^>]*\)>/mailto:\1:end-mailto/g' \
	-e 's/mailto:\([^@]*@[^ ]*\):end-mailto/<@uref{mailto:\/\/\1}\>/g' \
	-e 's/\(ftp:\/\/[^ 	]*[^.;,]\)/@uref{\1}/g' \
        -e 's/[^">]\(ftp:\/\/[^ 	]*$\)/@uref{\1}/g' \
	-e 's/[^">]\(http:\/\/[^ 	]*$\)/@uref{\1}/g' \
	-e 's/\(http:\/\/[^ 	]*[^.;,]\)/@uref{\1}/g' \
	-e '/^[ 	]\+[^*]/s/^[ 	]\+/  /' \
	-e '/^[		]\+\* [^:]\+$/s/^[ 	]\+//' \
	-e '/^[12][-0-9]\{9\}/s/^\(.*\)$/@unnumberedsec @code{\1}/' \
	-e '/^[A-z]\{3\}/s/^\(.*\)$/@unnumberedsec @code{\1}/' \
	-e 's/\(\b'"$SYMBOL_PREFIX"''"$SYMBOL_PATTERN"'\)/@reference_type{\1}/g' \
	-e 's/\(\b'"$SYMBOL_GLIB"'\)/@reference_type{\1}/g' \
	-e 's/\(\b'"$FUNC_PREFIX"'_'"$FUNC_PATTERN"'\)/@reference_function{\1}/g' \
	-e 's/\(\b'"$MACRO_PREFIX"'_'"$FUNC_PATTERN"'\)/@reference_constant{\1}/g' \
	-e '/^[ 	]\+/ { ' \
	  -e 's/'"$CONST1_EXPR"'/@reference_constant{\1}/g;' \
	  -e 's/'"$CONST2_EXPR"'/@reference_constant{\1}/g;' \
	  -e 's/'"$CONST3_EXPR"'/@reference_constant{\1}/g;' \
	  -e 's/'"$CONST4_EXPR"'/@reference_constant{\1}/g;' \
	  -e 's/'"$CONST5_EXPR"'/@reference_constant{\1}/g;' \
	  -e 's/'"$CONST6_EXPR"'/@reference_constant{\1}/g;' \
	  -e 's/'"$CONST7_EXPR"'/@reference_constant{\1}/g;' \
        -e '}' \
	-e 's/$/@*/g' \
	-e 's/^[ 	]*@\*$//g' \
	-e '/^@[a-z]/ s/@\*$//g' \
    | perl -e '$_ = join("", <>); s/@\*\n^$/\n/gm; print'
}

function print_template ()
{
  cat <<EOT
\input texinfo
@c %**start of header
@settitle @@TITLE@@
@c %**end of header

@include teximacros.texi

@unnumbered @@TITLE@@

@@CONTENT@@

@bye
EOT
}

function apply_template ()
{
  sed -e "s/@@TITLE@@/$2/
  /@@CONTENT@@/ {
    r $1
    d
  }"
}

if ! [ $# -ge 2 ]; then
  echo "Usage: $0 <log-file> <log-title>" >&2
  exit 1
fi

filename=$1
title="ChangeLog for $2"

tmpfile=`mktemp -q /tmp/$0.$$.XXXXXX`
if [ $? -ne 0 ]; then
  echo "$0: Can't create temp file, exiting..."
  exit 1
fi

ChangeLog2texi <$filename >$tmpfile

print_template | apply_template "$tmpfile" "$title"

# Cleanup
rm -f $tmpfile
