#!/bin/sh
# listplugins.sh - create an automake conformant list to build plugins
# Copyright (C) 1998-2002 Tim Janik

test -z "$1" && {
	echo "$0: No input files"
	exit 1
}

echo "## Generated data (by $0)"

#
if test "x$1" = "x--idl" ; then   # C++ plugins
#
shift
for file in "$@" ; do
	_name=`echo $file | sed 's/[^A-Za-z0-9]/_/g'`
	echo
	echo "##"
	echo "## Plugin $file"
	echo "##"
	# test works only for srcdir==builddir
	#test -e "$file.cc" || {
	#	echo "$file: missing source file: $file.cc" >&2
	#	exit 1
	#}
	echo "$_name""_la_SOURCES = $file.cc"
	echo "$_name""_la_LDFLAGS =" '-module $(plugin_ldflags)'
	echo "$_name""_la_LIBADD  =" '$(plugin_libs)'
	echo '$(srcdir)/'"$file.cc: $file.genidl.hh"
	echo "plugins_built_sources += $file.genidl.hh"
	echo "plugin_LTLIBRARIES    += $file.la"
done
#
else # not --idl option
#
FILES="$@"
LFILES=`echo $FILES | sort | uniq`
for file in $LFILES ; do
	echo "$file" | grep "\.c$" >/dev/null || continue
	name="`echo $file | sed 's/.c$//'`"
	_name=`echo $name | sed 's/[^A-Za-z0-9]/_/g'`
	cfile="$name.c"
	hfile="$name.h"
	efile=

	test -e "$cfile" || {
		echo "$cfile: No such file" >&2
		exit 1
	}

	grep -E "BSE_EXPORTS_BEGIN|BSE_DEFINE_EXPORTS" $cfile >/dev/null || {
		echo "$cfile: missing BSE_DEFINE_EXPORTS() directive" >&2
		exit 1
	}
	if grep -F BSE_EXPORT_AND_GENERATE_ENUMS $cfile >/dev/null ; then
	    test -e "$hfile" || {
		echo "$cfile: found BSE_EXPORT_AND_GENERATE_ENUMS() directive, but missing $hfile" >&2
		exit 1
	    }
	    efile="$name.enums"
	fi

	echo
	echo "##"
	echo "## Plugin $name"
	echo "##"
	test -n "$efile" && {
	    echo "$efile: $hfile "
	    echo "plugins_built_sources += $efile"
	    echo '$(srcdir)/'"$cfile: $efile"
	}
	echo "$_name""_la_SOURCES = $cfile"
	echo "$_name""_la_LDFLAGS =" '-module $(plugin_ldflags)'
	echo "$_name""_la_LIBADD  =" '$(plugin_libs)'
	# echo "$_name""_la_LDADD   =" '$(plugin_ldlibs)'
	test -e "$hfile" && {
	    echo "EXTRA_HEADERS      += $hfile"
	}
	echo "plugin_LTLIBRARIES += $name.la"
done
#
fi # not --idl option
#

echo
echo "## Generated data ends here"

# alles wird gut! ;)

exit 0
