#!/usr/bin/env bash
# birnet/examples/mkhost.sh
# Copyright (C) 2007 Tim Janik

ARG0=$(basename $0)

# check directory
test -r ./birnet/birnet.hh || { echo "$ARG0: script must be invoked from a birnet parent dir like birnet/.." >&2 ; exit 1; }

create_target() {
	if test -e "$1" ; then
		echo "$ARG0: skipping existing file: $1" >&1
		cat >/dev/null
	else
		echo "$ARG0: creating $1..."
		cat >"$1"
	fi
}
create_link() {
	if test -e "$1" ; then
		echo "$ARG0: skipping existing file: $1" >&1
	else
		echo "$ARG0: linking $1..."
		ln -s "$2" "$1"
	fi
}

###
# create a minimal set of build files required to host a birnet/ subdir
###

# ChangeLog
create_link ChangeLog birnet/ChangeLog

# COPYING
create_link COPYING ./birnet/COPYING.LGPL

# AUTHORS
create_target AUTHORS <<-__EOFmkhost
	${USER:-nobody}
__EOFmkhost

# NEWS
create_target NEWS <<-__EOFmkhost
	Version 0.0.0 news:
	
	* Added birnet/ library
	* Setup ChangeLog, AUTHORS, COPYING, NEWS and README
	* Added build files autogen.sh, configure.in, Makefile.am and Makefile.decl
__EOFmkhost

# README
create_target README <<-__EOFmkhost
	This is an example project, it was created by a Birnet script to compile 
	the birnet/ subdirectory.
__EOFmkhost

#########################################
#########################################
###                 #####################
###  Makefile.decl  START >>> >>> >>> >>>
###                 #####################
#########################################
#########################################
create_target Makefile.decl <<\__EOFmkhost # need correct TAB interpretation for makefiles
# Makefile.decl
# provide declarations and initializations for all Makefiles

INCLUDES=
EXTRA_DIST=
CLEANFILES=

# === slowcheck ===
# recursive rule supported by all Makefiles to run time consuming checks
.PHONY: slowcheck slowcheck-recursive slowcheck-SLOWTESTS
slowcheck: all slowcheck-recursive slowcheck-SLOWTESTS
slowcheck-recursive:
	@for subdir in $(SUBDIRS) ; do				\
	  test "$$subdir" = '.' ||				\
	    $(MAKE) -C "$$subdir" $(AM_MAKEFLAGS) slowcheck ||	\
	    exit 1 ;						\
	done
slowcheck-SLOWTESTS:
	@for tst in $(SLOWTESTS) ; do				\
	  ./$$tst --test-slow && echo "PASS: $$tst" || exit 1 ;	\
	done
	@MESSAGETEXT="All $(words $(SLOWTESTS)) slow tests passed"	\
	&& [ 0 -lt $(words $(SLOWTESTS)) ]				\
	&& echo $$MESSAGETEXT | sed 's/./=/g' && echo $$MESSAGETEXT	\
	&& echo $$MESSAGETEXT | sed 's/./=/g' || true
SLOWTESTS=

# === perf ===
# recursive rule supported by all Makefiles to run performance tests
.PHONY: perf perf-recursive perf-PERFTESTS
perf: all perf-recursive perf-PERFTESTS
perf-recursive:
	@for subdir in $(SUBDIRS) ; do				\
	  test "$$subdir" = '.' ||				\
	    $(MAKE) -C "$$subdir" $(AM_MAKEFLAGS) perf ||	\
	    exit 1 ;						\
	done
perf-PERFTESTS:
	@for tst in $(PERFTESTS) ; do				\
	  ./$$tst --test-perf && echo "PASS: $$tst" || exit 1 ;	\
	done
	@MESSAGETEXT="All $(words $(PERFTESTS)) perf tests passed"	\
	&& [ 0 -lt $(words $(PERFTESTS)) ]				\
	&& echo $$MESSAGETEXT | sed 's/./=/g' && echo $$MESSAGETEXT	\
	&& echo $$MESSAGETEXT | sed 's/./=/g' || true
PERFTESTS=

# === ALLTESTS ===
TESTS=
ALLTESTS = $(TESTS) $(SLOWTESTS) $(PERFTESTS) # used in noinst_PROGRAMS

# === report ===
.PHONY: report
report: all
	@export REPORTFILE="$(REPORTFILE)"			\
	&& [ -z "$$REPORTFILE" ]				\
	&& export REPORTFILE="$(shell pwd)/report.out" ;	\
	( :							\
	  && echo -n "#TREPSTART: " && date --iso-8601=seconds	\
	  && $(MAKE) $(AM_MAKEFLAGS) check slowcheck perf	\
	  && echo -n "#TREPDONE: "  && date --iso-8601=seconds	\
	) 2>&1 | tee "$$REPORTFILE"				\
	&& test "$${PIPESTATUS[*]}" = "0 0"
__EOFmkhost
#########################################
#########################################
###                 #####################
###  Makefile.decl  END   <<< <<< <<< <<<
###                 #####################
#########################################
#########################################
	
# Makefile.am
create_target Makefile.am <<-\__EOFmkhost
	# Makefile.am (toplevel)
	include $(top_srcdir)/Makefile.decl
	
	SUBDIRS = . birnet
	
	# require automake 1.9
	AUTOMAKE_OPTIONS = 1.9
	
	# extra dependencies
	configure: birnet/acbirnet.m4 birnet/configure.inc
__EOFmkhost
	
# configure.in
create_target configure.in <<-\__EOFmkhost
	dnl # include Birnet macros
	builtin(include, birnet/acbirnet.m4)dnl
	
	# Initialize and configure
	PACKAGE_VERSION=0.0.0
	AC_INIT
	AC_CONFIG_SRCDIR([birnet/birnet.hh])
	AM_CONFIG_HEADER(configure.h)
	AC_PREREQ(2.57)
	AC_CANONICAL_TARGET
	AM_INIT_AUTOMAKE(birnet, $PACKAGE_VERSION, no-define)
	AC_PROG_MAKE_SET
	
	ADDON_CFLAGS="-g -DG_ENABLE_DEBUG" # DEBUG defaults
	test -z "$CFLAGS"   && CFLAGS="$ADDON_CFLAGS"   || CFLAGS="$CFLAGS $ADDON_CFLAGS"
	test -z "$CXXFLAGS" && CXXFLAGS="$ADDON_CFLAGS" || CXXFLAGS="$CXXFLAGS $ADDON_CFLAGS"
	
	# check compilers and their behaviour, setup CFLAGS
	MC_PROG_CC_WITH_CFLAGS
	MC_PROG_CXX_WITH_CXXFLAGS
	AC_PROG_CPP
	AC_PROG_CXX
	AC_PROG_CXXCPP
	AC_C_CONST
	AC_C_INLINE
	AC_HEADER_STDC
	AC_PROG_INSTALL
	AC_PROG_LN_S
	AM_PROG_LIBTOOL
	
	dnl # Include Birnet configure macro
	dnl # Copy the next 3 lines as is, to avoid autoconf/automake bugs.
	builtin(include, birnet/configure.inc)dnl 
	#include(birnet/configure.inc)
	AC_DEFUN([DUMMY_DEFS], [ AC_SUBST(BIRNET_CFLAGS) AC_SUBST(BIRNET_LIBS) ])
	dnl # Birnet configure macro included
	
	# Configure Birnet
	AC_BIRNET_REQUIREMENTS
	
	# create output files
	AC_CONFIG_FILES([
	Makefile
	birnet/Makefile
	birnet/tests/Makefile
	])
	AC_OUTPUT
__EOFmkhost

# autogen.sh
create_target autogen.sh <<-\__EOFmkhost
	#!/bin/sh
	# Run build all tools including configure
	
	TEST_TYPE=-f
	FILE=birnet/birnet.hh
	AUTOMAKE=automake
	AUTOMAKE_POSTFIX=1.9
	AUTOMAKE_VERSION=1.9
	AUTOMAKE_MAXVERSION=1.9
	ACLOCAL=aclocal
	AUTOCONF=autoconf
	AUTOCONF_POSTFIX=2.50
	AUTOCONF_VERSION=2.57
	AUTOHEADER=autoheader
	LIBTOOLIZE=libtoolize
	LIBTOOLIZE_VERSION=1.5.0
	CONFIGURE_OPTIONS=--enable-devel-rules=yes
	
	srcdir=`dirname $0`
	test -z "$srcdir" && srcdir=.
	ORIGDIR=`pwd`
	cd $srcdir
	DIE=0
	
	# check build directory
	test $TEST_TYPE $FILE || {
		echo
		echo "$0: must be invoked in the toplevel directory"
		DIE=1
	}
	
	# check_version(given,required) compare two versions in up to 6 decimal numbers
	check_version()
	{
	  # number pattern
	  N="\([^:]*\)"
	  # remove leading non-numbers, seperate by :
	     GIVEN=`echo "$1:0:0:0:0:0:0" | sed -e 's/^[^0-9]*//' -e 's/[^0-9]\+/:/g'`
	  REQUIRED=`echo "$2:0:0:0:0:0:0" | sed -e 's/^[^0-9]*//' -e 's/[^0-9]\+/:/g'`
	  # extract 6 numbers from $GIVEN into ac_v?
	  eval `echo "$GIVEN"    | sed "s/^$N:$N:$N:$N:$N:$N.*$/ac_v1=\1 ac_v2=\2 ac_v3=\3 ac_v4=\4 ac_v5=\5 ac_v6=\6/" `
	  # extract 6 numbers from $REQUIRED into ac_r?
	  eval `echo "$REQUIRED" | sed "s/^$N:$N:$N:$N:$N:$N.*$/ac_r1=\1 ac_r2=\2 ac_r3=\3 ac_r4=\4 ac_r5=\5 ac_r6=\6/" `
	  # do the actual comparison (yielding 1 on success)
	  ac_vm=`expr \( $ac_v1 \> $ac_r1 \) \| \( \( $ac_v1 \= $ac_r1 \) \& \(          \
	               \( $ac_v2 \> $ac_r2 \) \| \( \( $ac_v2 \= $ac_r2 \) \& \(         \
	                \( $ac_v3 \> $ac_r3 \) \| \( \( $ac_v3 \= $ac_r3 \) \& \(        \
	                 \( $ac_v4 \> $ac_r4 \) \| \( \( $ac_v4 \= $ac_r4 \) \& \(       \
	                  \( $ac_v5 \> $ac_r5 \) \| \( \( $ac_v5 \= $ac_r5 \) \& \(      \
	                   \( $ac_v6 \>= $ac_r6 \)                                       \
	                  \) \)  \
	                 \) \)   \
	                \) \)    \
	               \) \)     \
	              \) \)      `
	  #echo "Given:    ac_v1=$ac_v1 ac_v2=$ac_v2 ac_v3=$ac_v3 ac_v4=$ac_v4 ac_v5=$ac_v5 ac_v6=$ac_v6"
	  #echo "Required: ac_r1=$ac_r1 ac_r2=$ac_r2 ac_r3=$ac_r3 ac_r4=$ac_r4 ac_r5=$ac_r5 ac_r6=$ac_r6"
	  #echo "Result:   $ac_vm"
	  test $ac_vm = 1
	}
	
	# check for automake
	if check_version "`$AUTOMAKE --version 2>/dev/null | sed 1q`" $AUTOMAKE_VERSION ; then
		:	# all fine
	elif check_version "`$AUTOMAKE$AUTOMAKE_POSTFIX --version 2>/dev/null | sed 1q`" $AUTOMAKE_VERSION ; then
		AUTOMAKE=$AUTOMAKE$AUTOMAKE_POSTFIX
		ACLOCAL=$ACLOCAL$AUTOMAKE_POSTFIX
	elif check_version "`$AUTOMAKE-$AUTOMAKE_POSTFIX --version 2>/dev/null | sed 1q`" $AUTOMAKE_VERSION ; then
		AUTOMAKE=$AUTOMAKE-$AUTOMAKE_POSTFIX
		ACLOCAL=$ACLOCAL-$AUTOMAKE_POSTFIX
	else
		echo "You need to have $AUTOMAKE (version >= $AUTOMAKE_VERSION) installed for compilation."
		echo "Download the appropriate package for your distribution,"
		echo "or get the source tarball at http://ftp.gnu.org/gnu/automake"
		DIE=1
	fi
	# due to automake release incompatibilities, check max-version
	check_version $AUTOMAKE_MAXVERSION.9999 "`$AUTOMAKE --version 2>/dev/null | sed 1q`" || {
		echo "You need to have $AUTOMAKE (version <= $AUTOMAKE_MAXVERSION) installed for compilation."
		echo "Download the appropriate package for your distribution,"
		echo "or get the source tarball at http://ftp.gnu.org/gnu/automake"
		DIE=1
	}
	# check for autoconf
	if check_version "`$AUTOCONF --version 2>/dev/null | sed 1q`" $AUTOCONF_VERSION ; then
		:	# all fine
	elif check_version "`$AUTOCONF$AUTOCONF_POSTFIX --version 2>/dev/null | sed 1q`" $AUTOCONF_VERSION ; then
		AUTOCONF=$AUTOCONF$AUTOCONF_POSTFIX
		AUTOHEADER=$AUTOHEADER$AUTOCONF_POSTFIX
	elif check_version "`$AUTOCONF-$AUTOCONF_POSTFIX --version 2>/dev/null | sed 1q`" $AUTOCONF_VERSION ; then
		AUTOCONF=$AUTOCONF-$AUTOCONF_POSTFIX
		AUTOHEADER=$AUTOHEADER-$AUTOCONF_POSTFIX
	else
		echo "You need to have $AUTOCONF (version >= $AUTOCONF_VERSION) installed for compilation."
		echo "Download the appropriate package for your distribution,"
		echo "or get the source tarball at http://ftp.gnu.org/gnu/autoconf"
		DIE=1
	fi
	# check for libtool
	check_version "`$LIBTOOLIZE --version 2>/dev/null | sed 1q`" $LIBTOOLIZE_VERSION || {
		echo "You need to have $LIBTOOLIZE (version >= $LIBTOOLIZE_VERSION) installed for compilation."
		echo "Get the source tarball at http://ftp.gnu.org/gnu/libtool"
		DIE=1
	}
	# sanity test aclocal
	$ACLOCAL --version >/dev/null 2>&1 || {
		echo "Unable to run $ACLOCAL though $AUTOMAKE is available."
		echo "Please correct the above error first."
		DIE=1
	}
	# sanity test autoheader
	$AUTOHEADER --version >/dev/null 2>&1 || {
		echo "Unable to run $AUTOHEADER though $AUTOCONF is available."
		echo "Please correct the above error first."
		DIE=1
	}
	
	# bail out as scheduled
	test "$DIE" -gt 0 && exit 1
	
	# sanity check $ACLOCAL_FLAGS
	if test -z "$ACLOCAL_FLAGS"; then
		acdir=`$ACLOCAL --print-ac-dir`
	        m4tests="glib-2.0.m4"
		for file in $m4tests ; do
			[ ! -f "$acdir/$file" ] && {
				echo "WARNING: failed to find $file in $acdir"
				echo "         If this file is installed in /some/dir, you may need to set"
				echo "         the ACLOCAL_FLAGS environment variable to \"-I /some/dir\","
				echo "         or install $acdir/$file."
			}
		done
	fi
	
	echo "Cleaning configure cache..."
	rm -rf autom4te.cache/
	
	echo "Running: $LIBTOOLIZE"
	$LIBTOOLIZE --force || exit $?
	
	echo "Running: $ACLOCAL $ACLOCAL_FLAGS"
	$ACLOCAL $ACLOCAL_FLAGS	|| exit $?
	
	echo "Running: $AUTOHEADER"
	$AUTOHEADER || exit $?
	
	echo "Running: $AUTOMAKE"
	case $CC in
	*xlc | *xlc\ * | *lcc | *lcc\ *) am_opt=--include-deps;;
	esac
	$AUTOMAKE --force-missing --add-missing $am_opt || exit $?
	
	echo "Running: $AUTOCONF"
	$AUTOCONF || exit $?
	
	cd $ORIGDIR
	echo "Running: $srcdir/configure $CONFIGURE_OPTIONS $@"
	$srcdir/configure --enable-maintainer-mode $CONFIGURE_OPTIONS "$@" || exit $?
__EOFmkhost
chmod +x autogen.sh

# Done
echo "$ARG0: Host project setup for birnet/ completed:"
echo
head NEWS
echo "[...]"
