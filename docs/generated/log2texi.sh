#!/bin/bash

SYMBOL_PATTERN='[*0-9A-Z?][*0-9A-Za-z_?]*\(()\)\{0,1\}'
FUNC_PATTERN='[*0-9A-Za-z?][*0-9A-Za-z_?]*\(()\)\{0,1\}'
SYMBOL_PREFIX='\(Gsl\|Bse\|Bsw\|Bst\|Gtk\|Gdk\|Gnome\)'
FUNC_PREFIX='\(gsl\|bse\|bsw\|bst\|gtk\|gnome\)'
MACRO_PREFIX='\(GSL\|BSE\|BSW\|BST\|GTK\|Gdk\|GNOME\)'

export SYMBOL_PATTERN FUNC_PATTERN SYMBOL_PREFIX FUNC_PREFIX MACRO_PREFIX

function ChangeLog2texi ()
{
    sed \
	-e 's/[{}@]/@&/g' \
	-e 's/<\([^@]*@[^>]*\)>/mailto:\1:end-mailto/g' \
	-e 's/mailto:\([^@]*@[^ ]*\):end-mailto/<@uref{mailto:\/\/\1}\>/g' \
	-e 's/\(ftp:\/\/[^ 	]*[^.;,]\)/@uref{\1}/g' \
        -e 's/[^">]\(ftp:\/\/[^ 	]*$\)/@uref{\1}/g' \
	-e 's/[^">]\(http:\/\/[^ 	]*$\)/@uref{\1}/g' \
	-e 's/\(http:\/\/[^ 	]*[^.;,]\)/@uref{\1}/g' \
	-e '/^[ 	]\+\* [^:]\+:/s/[ 	]*\(\* \)\([^:(<>)]*\)/\1@file{\2}/' \
	-e '/^[ 	]\+[^*]/s/^[ 	]\+/  /' \
	-e '/^[		]\+\* [^:]\+$/s/^[ 	]\+//' \
	-e '/^[12][-0-9]\{9\}/s/^\(.*\)$/@unnumberedsec \1/' \
	-e '/^[A-z]\{3\}/s/^\(.*\)$/@unnumberedsec \1/' \
	-e 's/\(\b'"$SYMBOL_PREFIX"''"$SYMBOL_PATTERN"'\)/@reference_parameter{\1}/g' \
	-e 's/\(\b'"$FUNC_PREFIX"'_'"$FUNC_PATTERN"'\)/@reference_function{\1}/g' \
	-e 's/\(\b'"$MACRO_PREFIX"'_'"$FUNC_PATTERN"'\)/@reference_function{\1}/g' \
	-e 's/$/@*/g' \
	-e 's/^[ 	]*@\*$//g' \
	-e '/^@[a-z]/ s/@\*$//g' | perl -e '$_ = join("", <>); s/@\*\n^$/\n/gm; print'
}

function print_template ()
{
  cat <<EOT
\input texinfo
@c %**start of header
@settitle @@TITLE@@
@c %**end of header

@include teximacros.texi

@docfont{mono}

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

print_template | apply_template <(ChangeLog2texi <$filename) "$title"
