#!/bin/sh
# Run this to generate all the initial makefiles, etc.

PROJECT=BEAST
TEST_TYPE=-d
FILE=bse
AUTOMAKE=automake
AUTOMAKE_VERSION=1.4
ACLOCAL=aclocal
AUTOCONF=autoconf
AUTOCONF_VERSION=2.50
AUTOHEADER=autoheader
GETTEXTIZE=glib-gettextize
INTLTOOLIZE=intltoolize
LIBTOOLIZE=libtoolize
CONFIGURE_OPTIONS=--enable-devel-rules=yes

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
ORIGDIR=`pwd`
cd $srcdir
DIE=0

# check build directory
test $TEST_TYPE $FILE || {
	echo
	echo "$0 needs to be invoked in the toplevel directory of $PROJECT"
	DIE=1
}

# check for automake
if $AUTOMAKE --version 2>/dev/null | grep >/dev/null " $AUTOMAKE_VERSION\b" ; then
	:	# all fine
elif $AUTOMAKE$AUTOMAKE_VERSION --version 2>/dev/null | grep >/dev/null " $AUTOMAKE_VERSION\b" ; then
	AUTOMAKE=$AUTOMAKE$AUTOMAKE_VERSION
	ACLOCAL=$ACLOCAL$AUTOMAKE_VERSION
elif $AUTOMAKE-$AUTOMAKE_VERSION --version 2>/dev/null | grep >/dev/null " $AUTOMAKE_VERSION\b" ; then
	AUTOMAKE=$AUTOMAKE-$AUTOMAKE_VERSION
	ACLOCAL=$ACLOCAL-$AUTOMAKE_VERSION
else
	echo
	echo "You need to have $AUTOMAKE (version $AUTOMAKE_VERSION) installed to compile $PROJECT."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at http://ftp.gnu.org/gnu/automake"
	DIE=1
fi

# check for autoconf
if $AUTOCONF --version 2>/dev/null | grep >/dev/null " $AUTOCONF_VERSION\b" ; then
	:	# all fine
elif $AUTOCONF$AUTOCONF_VERSION --version 2>/dev/null | grep >/dev/null "GNU Autoconf\b" ; then
	AUTOCONF=$AUTOCONF$AUTOCONF_VERSION
	AUTOHEADER=$AUTOHEADER$AUTOCONF_VERSION
elif $AUTOCONF-$AUTOCONF_VERSION --version 2>/dev/null | grep >/dev/null "GNU Autoconf\b" ; then
	AUTOCONF=$AUTOCONF-$AUTOCONF_VERSION
	AUTOHEADER=$AUTOHEADER-$AUTOCONF_VERSION
else
	echo
	echo "You need to have $AUTOCONF (version $AUTOCONF_VERSION) installed to compile $PROJECT."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at http://ftp.gnu.org/gnu/autoconf"
	DIE=1
fi

# check for gettextize
$GETTEXTIZE --version >/dev/null 2>&1 || {
	echo
	echo "You need to have $GETTEXTIZE installed to compile $PROJECT."
	echo "Get the source tarball at http://ftp.gnu.org/gnu/"
	DIE=1
}

# check for intltoolize
$INTLTOOLIZE --version >/dev/null 2>&1 || {
	echo
	echo "You need to have $INTLTOOLIZE installed to compile $PROJECT."
	echo "Get the source tarball at http://ftp.gnu.org/gnu/"
	DIE=1
}

# check for libtool
$LIBTOOLIZE --version >/dev/null 2>&1 || {
	echo
	echo "You need to have $LIBTOOL installed to compile $PROJECT."
	echo "Get the source tarball at http://ftp.gnu.org/gnu/libtool"
	DIE=1
}

# sanity test aclocal
$ACLOCAL --version >/dev/null 2>&1 || {
	echo
	echo "Unable to run $ACLOCAL though $AUTOMAKE is available."
	echo "Please correct the above error first."
	DIE=1
}

# sanity test autoheader
$AUTOHEADER --version >/dev/null 2>&1 || {
	echo
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

$LIBTOOLIZE --force || exit $?

# run gettext
echo "Running gettextize...  Ignore non-fatal messages."
# We specify --force here, since otherwise things are
# not added reliably, but we don't want to overwrite intl
# while making dist.
echo "no" | $GETTEXTIZE --force || exit $?

# run intltool
echo "Running intltoolize..."
$INTLTOOLIZE --force --automake || exit $?

# run aclocal
$ACLOCAL $ACLOCAL_FLAGS	|| exit $?

# feature autoheader
$AUTOHEADER || exit $?

# invoke automake
case $CC in
*xlc | *xlc\ * | *lcc | *lcc\ *) am_opt=--include-deps;;
esac
$AUTOMAKE --add-missing $am_opt || exit $?

# invoke autoconf
$AUTOCONF || exit $?

# invoke configure
cd $ORIGDIR
$srcdir/configure $CONFIGURE_OPTIONS "$@" || exit $?
