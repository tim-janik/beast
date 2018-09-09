# Beast & Bse                                    -*- Mode: shell-script; -*-

m4_include(acbeast.m4)

AC_PREREQ(2.57)

# == Package & Version ==
# VERSION: MAJOR.MINOR.MICRO-RC
# * MAJOR:       Increment on major API breaks, *forces* ABI break, resets MINOR, MICRO and ABI_VERSION
# * MINOR:       Increment for significant changes or ABI breaks, resets MICRO
# * MICRO:       Increment with every release, increments must *not* break ABI
# * -RC:         Pre-release suffix, may be empty or contain -beta.[0-9]+
AC_INIT([beast], [m4_esyscmd_s([./git-version.sh -s])])
die() { AC_MSG_ERROR([$1]) ; exit -1 ; }
# * ABI_VERSION: Record the latest version since ABI compatibility has been kept
# * ABI changes: Increment MAJOR or MINOR and reset ABI_VERSION to the current VERSION
[ABI_VERSION=`echo $PACKAGE_VERSION | sed 's/^\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\1.\2.0/'`]
AC_CONFIG_SRCDIR([bse/bseserver.cc])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_CONFIG_HEADER(config/config.h)
AC_CANONICAL_TARGET                                     # architecture information
AM_INIT_AUTOMAKE(1.9 no-dist no-define foreign subdir-objects)
# version handling
BST_VERSION="$PACKAGE_VERSION"
[MAJOR=`echo $PACKAGE_VERSION | sed 's/^\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\1/'`]
[MINOR=`echo $PACKAGE_VERSION | sed 's/^\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\2/'`]
[MICRO=`echo $PACKAGE_VERSION | sed 's/^\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\3/'`]
AC_DEFINE_UNQUOTED(BST_ARCH_NAME, "$target", [Architecture Description])
AC_DEFINE_UNQUOTED(BST_VERSION, "$BST_VERSION", [Beast Version])
AC_DEFINE_UNQUOTED(BEAST_EXEC_POSTFIX, "-$MAJOR.$MINOR.$MICRO", [Beast executable postfix])
# For automake, we need to easily provide MAJOR.MINOR.MICRO and MAJOR_MINOR_MICRO:
AC_SUBST(MAJOR)
AC_SUBST(MINOR)
AC_SUBST(MICRO)
# calculate ABI age
ABI_MAJOR=[`echo "$ABI_VERSION" | sed 's/^\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\1/'`]
ABI_MINOR=[`echo "$ABI_VERSION" | sed 's/^\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\2/'`]
ABI_MICRO=[`echo "$ABI_VERSION" | sed 's/^\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\3/'`]
test "$ABI_MAJOR" = "$MAJOR" || {
  AC_MSG_ERROR([ABI_VERSION must match MAJOR version: $ABI_MAJOR == $MAJOR])
}
test "$ABI_MINOR" -le "$MINOR" || {
  AC_MSG_ERROR([ABI_VERSION must predate MINOR version: $ABI_MINOR <= $MINOR])
}
test "$ABI_MICRO" -eq 0 || {
  AC_MSG_ERROR([ABI_VERSION must not indicate ABI break in MICRO releases: $ABI_MAJOR.$ABI_MINOR.$ABI_MICRO == $ABI_MAJOR.$ABI_MINOR])
}
ABI_AGE=$(($MINOR - $ABI_MINOR))
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# Note, this package provides backwards binary compatibility only within
# MINOR.MICRO increments, and only since ABI_VERSION.
# Interface additions occour with pretty much every release.
# For libtool this means:
# - the library name needs to encode MAJOR
# - the '-release X.Y' option cannot be used, because it's ignored for naming .a, .la, .so files.
# - lt_current=MINOR  # disallow forward compatible builds, i.e. enforce runtime-lib >= build-time-lib
# - lt_age=ABI_AGE    # allow backward compatibility between ABI_VERSION .. VERSION
# - lt_revision=MICRO # pick the latest SO revision for unchanged MAJOR.MINOR,
#                     # see also: https://autotools.io/libtool/version.html
# On Linux this yields:
# library:      libfoo-$MAJOR.so.$ABI_MINOR.$ABI_AGE.$MICRO
# SONAME:       libfoo-$MAJOR.so.$ABI_MINOR
lt_current=$MINOR ; lt_age=$ABI_AGE ; lt_revision=$MICRO
LT_VERSION_INFO="$lt_current:$lt_revision:$lt_age"
AC_SUBST(LT_VERSION_INFO)

# == Argument parsing ==
AC_ARG_ENABLE(debug, [  --enable-debug=[no/min/yes/asan/tsan/lsan/ubsan] turn on debugging, default=minimum],,enable_debug=min)
AC_ARG_WITH(pkgroot, [  --with-pkgroot[[=DIR]]    create package installation directory inside DIR],,with_pkgroot=no)
# package library defaults
enable_static=no ; enable_shared=yes

# == System info ==
AC_MSG_CHECKING([for uname -s...]) ; uname_S=`uname -s 2>/dev/null || echo none` ; AC_MSG_RESULT([$uname_S])
AC_MSG_CHECKING([for uname -m...]) ; uname_M=`uname -m 2>/dev/null || echo none` ; AC_MSG_RESULT([$uname_M])
AC_MSG_CHECKING([for uname -o...]) ; uname_O=`uname -o 2>/dev/null || echo none` ; AC_MSG_RESULT([$uname_O])
AC_MSG_CHECKING([for uname -r...]) ; uname_R=`uname -r 2>/dev/null || echo none` ; AC_MSG_RESULT([$uname_R])
AC_MSG_CHECKING([for uname -p...]) ; uname_P=`uname -p 2>/dev/null || echo none` ; AC_MSG_RESULT([$uname_P])
AC_MSG_CHECKING([for uname -v...]) ; uname_V=`uname -v 2>/dev/null || echo none` ; AC_MSG_RESULT([$uname_V])
AC_SUBST(uname_S)
AC_SUBST(uname_M)
AC_SUBST(uname_O)
AC_SUBST(uname_R)
AC_SUBST(uname_P)
AC_SUBST(uname_V)

# == Check srcdir == builddir ==
echo "checking for srcdir==builddir... (#buildapi-variable-no-builddir)" # see https://github.com/cgwalters/build-api
TEST_SRCDIR=`cd $srcdir && pwd -P` || die "failed to cd or pwd"
TEST_BUILDDIR=`cd . && pwd -P`	   || die "failed to cd or pwd"
test "$TEST_SRCDIR" == "$TEST_BUILDDIR" || {
  AC_MSG_ERROR([builds require builddir == srcdir (use e.g. git-worktree instead)])
}

# Define package directories per major/minor version
beastlocaledir="\${beastdir}/locale"
beastbindir="\${beastdir}/bin"
bselibdir="\${beastdir}/lib"
beastsharedir="\${beastdir}/share"
beastdatadir="\${beastdir}"
beastdocdir="\${beastdir}/doc"
bseincludedir="\${beastdir}/include"
includerootdir="\${beastdir}/include"
# Provide directories for C++ and move Beast packagedir into libdir if under /usr
bak_prefix="$prefix"; bak_exec_prefix="$exec_prefix"
test "x$prefix" != xNONE || prefix="$ac_default_prefix" ; test "x$exec_prefix" != xNONE || exec_prefix="$prefix"
case "$with_pkgroot" in
  no)			beastdir="$libdir/beast-$MAJOR-$MINOR" ;;
  */*)			beastdir="$with_pkgroot/beast-$MAJOR-$MINOR" ;;
  *)			AC_MSG_ERROR([invalid pkgroot path: $with_pkgroot]) ;;
esac
bak_libdir="$libdir"
bak_includerootdir="$includerootdir"; bak_bseincludedir="$bseincludedir"; bak_beastdir="$beastdir"
libdir=`eval echo "${libdir}"`
includerootdir=`eval echo "${includerootdir}"`; bseincludedir=`eval echo "${bseincludedir}"`; beastdir=`eval echo "${beastdir}"`
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_BEASTDIR, ["`eval echo "${beastdir}"`"], [Beast program directory])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_INCLUDEROOTDIR, ["`eval echo "${includerootdir}"`"], [Directory for libbse header files])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_BSEINCLUDEDIR, ["`eval echo "${bseincludedir}"`"], [Directory for libbse header files])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_BINDIR, ["`eval echo "${beastbindir}"`"], [Directory to install and execute Beast])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_LOCALEBASE, ["`eval echo ${beastlocaledir}`"], [Directory for locale specific message catalogs])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_LADSPA, ["`eval echo ${libdir}/ladspa`"], [Searchpath for LADSPA plugins])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_BSELIBDIR, ["`eval echo ${bselibdir}`"], [Directory to install plugins and drivers])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_DATADIR, ["`eval echo ${beastdatadir}`"], [Directory to install samples, effects, scripts, etc])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_DOCDIR, ["`eval echo ${beastdocdir}`"], [Directory to install documentation])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_USER_DATA, ["~/beast"], [Searchpath for user owned Beast resources])
includerootdir="$bak_includerootdir"; bseincludedir="$bak_bseincludedir"; beastdir="$bak_beastdir"
libdir="$bak_libdir"
prefix="$bak_prefix"; exec_prefix="$bak_exec_prefix"
# Substitute directories in Makefiles
AC_SUBST(beastlocaledir)
AC_SUBST(includerootdir)
AC_SUBST(bseincludedir)
AC_SUBST(beastbindir)
AC_SUBST(bselibdir)
AC_SUBST(beastsharedir)
AC_SUBST(beastdatadir)
AC_SUBST(beastdocdir)
AC_SUBST(beastdir)

# == Check for git ==
AC_MSG_CHECKING([for git repository])
DOTGIT=`git rev-parse --git-dir 2>/dev/null` && INGIT= || INGIT='#'
AC_SUBST(INGIT)
result=$(test -z "$INGIT" && echo yes || echo no)
AC_MSG_RESULT($result)

# == Compiler, linker, libtool ==
saved_CFLAGS="$CFLAGS" ; saved_CXXFLAGS="$CFLAGS" # the AC_PROG_C* macros mess up CFLAGS and CXXFLAGS
AC_PROG_CC             # note, this must preceede AC_CHECK_LIB, AC_TRY_RUN, etc to preserve CFLAGS etc
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_LANG_CPLUSPLUS	# use CXX for compilation tests
AC_PROG_LD
AC_LIB_PROG_LD
AC_LIB_PROG_LD_GNU
CFLAGS="$saved_CFLAGS" ; CXXFLAGS="$saved_CXXFLAGS"
AM_PROG_LIBTOOL
# AM_PROG_LIBTOOL sets up lt_cv_objdir and objdir
AC_SUBST(objdir)
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_OBJDIR, [".libs"], [Directory used for libtool objects])
AC_LIBTOOL_DLOPEN

# == C++ Dialect ==
CXX="$CXX -std=gnu++14"
test -z "$CXXCPP" || CXXCPP="$CXXCPP -std=gnu++14"
AC_MSG_CHECKING([compiler version])
cc_version_line1=`${CXX} --version | head -n1`
AC_MSG_RESULT([$cc_version_line1])

# == Compiler flags ==
# disable invalid "can't wrap" optimizations for signed / pointers
no_invalid_flags="-fno-strict-overflow"
# disable invalid optimizations of pointer casts
no_invalid_flags="$no_invalid_flags -fno-strict-aliasing"
# needs gcc >= 5, clang >= 3.4
c_warnings="-Wall -Wdeprecated -Werror=format-security -Wredundant-decls -Wpointer-arith -Wmissing-declarations -Werror-implicit-function-declaration"
c_warnings="$c_warnings -Werror=incompatible-pointer-types" # -Wdate-time -Wconversion -Wshadow
cxx_warnings="-Wsign-promo -Woverloaded-virtual" # -Wnon-virtual-dtor  -Wempty-body -Wignored-qualifiers -Wunreachable-code -Wtype-limits
c_only_flags="-Wmissing-prototypes -Wnested-externs -Wno-pointer-sign"
gcc_c_only_flags="-fno-cond-mismatch -fdevirtualize-speculatively"
compiler_flags="-pipe"
optimization_flags="-O3 -funroll-loops -ftree-vectorize $no_invalid_flags"
FAST_MATH_FLAGS="-ffast-math"
gcc_flags="-rdynamic -finline-functions -ftracer -ftree-loop-distribution -ftree-loop-ivcanon -ftree-loop-im -fno-delete-null-pointer-checks"
gcc_fast_math="-fivopts"
clang_fix_flags="-Wno-tautological-compare -Wno-constant-logical-operand"
x86_flags="-minline-all-stringops"
x86_flags="$x86_flags -mmmx -msse -msse2"	# Intel since 2001, AMD since 2003
x86_flags="$x86_flags -msse3"			# Intel since 2004, AMD since 2007
#x86_flags="$x86_flags -mssse3"			# Intel since 2006, AMD since 2011
x86_64_flags="-mcx16"				# for CMPXCHG16B, in AMD64 since 2005
#x86_64_flags="$x86_64_flags -msse4a"		# AMD since 2007
#x86_64_flags="$x86_64_flags -msse4.1 -msse4.2"	# Intel since 2008, AMD since 2011
#x86_64_flags="$x86_64_flags -mavx"		# Intel since 2011, AMD since 2011
#x86_64_flags="$x86_64_flags -mavx2"		# Intel since 2013, AMD since 2015
AC_MSG_CHECKING([compiler flavour])
if   ${CXX} --version 2>&1 | grep -q -F 'clang' ; then # clang
    AC_MSG_RESULT([clang++])
    AM_CFLAGS="$c_warnings $c_only_flags $compiler_flags $optimization_flags $clang_fix_flags"
    AM_CXXFLAGS="$c_warnings $cxx_warnings $compiler_flags $optimization_flags $clang_fix_flags"
    # reduce clang's bogus -pthread warning (llvm#7798, llvm#38772)
    AM_LDFLAGS="$AM_LDFLAGS -Wno-unused-command-line-argument"
    AM_CFLAGS="$AM_CFLAGS -Wno-unused-command-line-argument"
    AM_CXXFLAGS="$AM_CXXFLAGS -Wno-unused-command-line-argument"
    MC_ASSERT_VERSION($CXX --version, 4.0.1)
elif ${CXX} --version 2>&1 | grep -q -E 'Free Software Foundation' ; then # gcc
    AC_MSG_RESULT([g++])
    AM_CFLAGS="$c_warnings $c_only_flags $gcc_c_only_flags $compiler_flags $optimization_flags $gcc_flags"
    AM_CXXFLAGS="$c_warnings $cxx_warnings $compiler_flags $optimization_flags $gcc_flags"
    FAST_MATH_FLAGS="$FAST_MATH_FLAGS $gcc_fast_math"
    MC_ASSERT_VERSION($CXX --version, 5.2.1)
else
  AC_MSG_RESULT([none])
  AC_MSG_ERROR([unknown C++ compiler])
fi
AC_SUBST(FAST_MATH_FLAGS)
if test "$uname_M" = x86_64 ; then	# AMD64
    AM_CFLAGS="$AM_CFLAGS $x86_flags $x86_64_flags"
    AM_CXXFLAGS="$AM_CXXFLAGS $x86_flags $x86_64_flags"
elif echo "$uname_M" | grep -q '^i.*86$' ; then
    AM_CFLAGS="$AM_CFLAGS $x86_flags"
    AM_CXXFLAGS="$AM_CXXFLAGS $x86_flags"
fi

# == Debug flags ==
case "_$enable_debug" in
  _ubsan)
    debug_flags="-DG_ENABLE_DEBUG -ggdb3 -fverbose-asm -fno-omit-frame-pointer -fno-inline -fsanitize=undefined"
    AM_LDFLAGS="$AM_LDFLAGS -lubsan" ;;
  _asan)
    debug_flags="-DG_ENABLE_DEBUG -ggdb3 -fverbose-asm -fno-omit-frame-pointer -fno-inline -fsanitize=address"
    AM_LDFLAGS="$AM_LDFLAGS -lasan" ;;
  _tsan)
    debug_flags="-DG_ENABLE_DEBUG -ggdb3 -fverbose-asm -fno-omit-frame-pointer -fno-inline -fsanitize=thread"
    AM_LDFLAGS="$AM_LDFLAGS -ltsan" ;;
  _lsan)
    debug_flags="-DG_ENABLE_DEBUG -ggdb3 -fverbose-asm -fno-omit-frame-pointer -fno-inline -fsanitize=leak"
    AM_LDFLAGS="$AM_LDFLAGS -llsan" ;;
  _yes)
    debug_flags="-DG_ENABLE_DEBUG -ggdb3 -fverbose-asm -fno-omit-frame-pointer" ;;
  _min*)
    debug_flags="-DG_ENABLE_DEBUG -g" ;;
  _*)
    debug_flags="-DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS" ;; # -DG_DISABLE_ASSERT
esac
AM_CFLAGS="$AM_CFLAGS $debug_flags"
AM_CXXFLAGS="$AM_CXXFLAGS $debug_flags"
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(AM_CPPFLAGS)

# == SSE Build Conditionals ==
lacking_mmx_sse=`echo "$AM_CXXFLAGS" | grep 'mmx.*sse' -q ; echo $?` # sh truth values: 1 (without sse) or 0 (with sse)
AM_CONDITIONAL(WITH_MMX_SSE, test 0 == $lacking_mmx_sse)
AC_DEFINE_UNQUOTED(BSE_WITH_MMX_SSE, [(0 == $lacking_mmx_sse)], [wether MMX and SSE are supported])

# C & C++ idioms
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDC

# == Linker flags ==
AM_LDFLAGS="$AM_LDFLAGS -lrt -pthread"
ld_help_text=`${LD-ld} --help 2>&1 | tr '\n' ' '`
check_ld_flags="-export-dynamic -O1 -Bsymbolic-functions --as-needed --no-undefined"
for flag in $check_ld_flags ; do
  echo " $ld_help_text" | grep -q -e "$flag" && AM_LDFLAGS="$AM_LDFLAGS -Wl,$flag"
done
echo " $ld_help_text" | grep -q -e "--hash-style" && AM_LDFLAGS="$AM_LDFLAGS -Wl,--hash-style=both"
AC_SUBST(AM_LDFLAGS)

# == Utilities and build tools ==
AC_PROG_MAKE_SET()
AM_SILENT_RULES([yes])
AC_PROG_LN_S
AC_PROG_INSTALL
MC_ASSERT_PROG(PKG_CONFIG, pkg-config)
MC_ASSERT_PROG(PANDOC, pandoc)
MC_ASSERT_PROG(DOXYGEN, doxygen)
MC_ASSERT_PROG(DOT, dot)
MC_ASSERT_PROG(PYTHON2, python-2.7 python2.7 python2)
MC_ASSERT_VERSION($PYTHON --version, 2.7)
MC_ASSERT_PROG(XMLLINT, xmllint)
MC_ASSERT_VERSION($XMLLINT --version, 20901)

# == Headers ==
AC_CHECK_HEADERS_ONCE( zlib.h mad.h execinfo.h alsa/asoundlib.h sys/soundcard.h soundcard.h X11/XKBlib.h X11/extensions/XKBgeom.h )

# == pkg-config Libraries ==
SFI_REQUIRES="
        glib-2.0        	>= 2.32.3
	gobject-2.0		>= 2.32.3
	gthread-2.0		>= 2.32.3
	gmodule-no-export-2.0	>= 2.32.3
"
# defines SFI_CFLAGS and SFI_LIBS
PKG_CHECK_MODULES(SFI, [ $SFI_REQUIRES ])
# lib dependencies
LIBBSE_REQUIRES="
	ogg			>= 1.2.2
	vorbis			>= 1.3.2
	vorbisfile		>= 1.3.2
	vorbisenc		>= 1.3.2
	mad			>= 0.14.2
	flac			>= 1.2.1
	fluidsynth		>= 1.0.6
	alsa			>= 1.0.5
        pangoft2        	>= 1.30.0
	gtk+-2.0		>= 2.12.12
	libart-2.0		>= 2.3.8
	libgnomecanvas-2.0	>= 2.4.0
"
LIBBSE_REQUIRES=`echo $LIBBSE_REQUIRES` # strip whitespaces
# define BSE_CFLAGS and BSE_LIBS
PKG_CHECK_MODULES(BSE, [ $LIBBSE_REQUIRES ])
# for calling pthread_ functions, '-lpthread' is needed in addition to '-pthread'
dnl # FIXME: BSE_LIBS=`printf %s "$BSE_LIBS" | sed 's/\(-pthread\b\)/\1 -lpthread/' `
BSE_CFLAGS="$BSE_CFLAGS ${PTHREAD_CFLAGS:--D_REENTRANT}"
BSE_LIBS="$BSE_LIBS ${PTHREAD_LIBS:--lpthread}"

# == fluidsynth version check ==
FLUID_VERSION=`$PKG_CONFIG --modversion fluidsynth`
AC_SUBST(FLUID_VERSION)

# == libz ==
AC_CHECK_LIB(z, gzsetparams, [ LIBZ_LIBS='' ; test "$ac_cv_header_zlib_h" = yes && LIBZ_LIBS='-lz' ])
if test -z "$LIBZ_LIBS"; then
    AC_MSG_ERROR([Compression library libz is missing, but required])
else
  BSE_LIBS="$BSE_LIBS $LIBZ_LIBS"
fi
AC_SUBST(LIBZ_LIBS)

# == functions ==
AC_CHECK_FUNCS(getpwnam_r getpwnam)
AC_CHECK_FUNCS(timegm)

# == NPM ==
AC_ARG_WITH([npm], [  --without-npm           Disable npm based Beast package builds])
with_usable_npm=no
# only attempt npm builds if ebeast/ files are present in the current build tree
if test -r ebeast/package.json.in ; then
    if test "_$with_npm" != "_no" ; then
	AC_PATH_PROG([NPM], [npm])
	if test "_$NPM" != "_" ; then
	    AC_MSG_CHECKING([for npm version])
	    NPM_VERSION=`$NPM --version`
	    AC_MSG_RESULT([$NPM_VERSION])
	    MC_CHECK_VERSION([$NPM_VERSION], 1.4.21, with_usable_npm=yes)
	fi
    fi
fi
AM_CONDITIONAL(WITH_NPM, [test "_$with_usable_npm" = "_yes"])

# == pthread_spinlock_t initializer ==
AC_LANG_PUSH([C])
saveed_LDFLAGS="$LDFLAGS" ; LDFLAGS="-pthread $LDFLAGS"
AC_TRY_RUN(
  [
    #include <stdio.h>
    #include <string.h>
    #include <assert.h>
    #include <pthread.h>
    struct Spin { pthread_spinlock_t dummy1, s1, dummy2, s2, dummy3; };
    int
    main (int argc, char *argv[])
    {
      struct Spin spin;
      memset (&spin, 0xffffffff, sizeof (spin));
      if (pthread_spin_init (&spin.s1, 0) == 0 &&
          pthread_spin_init (&spin.s2, 0) == 0 &&
	  sizeof (pthread_spinlock_t) == 4 && // current implementation assumption
	  spin.s1 == spin.s2) // check for location-independent initialization
	 {
	   FILE *f = fopen ("conftest_spinlock_initializer", "w");
	   assert (f);
	   fprintf (f, "/*{*/ ");
	   fprintf (f, "0x%04x,", *(int*) &spin.s1);
	   fprintf (f, " /*}*/\n");
	   fclose (f);
	 }
	 return 0;
    } ],
  BSE_SPINLOCK_INITIALIZER=`cat conftest_spinlock_initializer` && rm -f conftest_spinlock_initializer ,
  BSE_SPINLOCK_INITIALIZER='' )
LDFLAGS="$saveed_LDFLAGS"
AC_LANG_POP([C])
test -z "$BSE_SPINLOCK_INITIALIZER" && {
  AC_MSG_FAILURE([spinlock initializer check failed to execute])
}
AC_SUBST(BSE_SPINLOCK_INITIALIZER)

dnl #
dnl # Define package requirements.
dnl #
AC_DEFUN([MC_I18N_REQUIREMENTS],
[
  # LINGUAS
  ALL_LINGUAS=`cat "$srcdir/po/LINGUAS" | grep -v '^#' | xargs echo -n `
  AC_SUBST(ALL_LINGUAS)
  AC_SUBST([CONFIG_STATUS_DEPENDENCIES], ['$(top_srcdir)/po/LINGUAS'])

  # for simplicity, Beast and Bse translations are currently in the same domain
  BST_GETTEXT_DOMAIN="beast-bse-$MAJOR.$MINOR.$MICRO"
  AC_SUBST(BST_GETTEXT_DOMAIN)
  AC_DEFINE_UNQUOTED(BST_GETTEXT_DOMAIN, "$BST_GETTEXT_DOMAIN", [Versioned BEAST gettext domain])
  GETTEXT_PACKAGE=$BST_GETTEXT_DOMAIN
  AC_SUBST(GETTEXT_PACKAGE)

  # check for proper gettext support
  AM_GNU_GETTEXT([external])
  test -z "$ac_cv_path_XGETTEXT" && {
    AC_MSG_ERROR(["Failed to detect gettext, required for internationalization." \
      "GNU gettext is available from: http://www.gnu.org/software/gettext/"])
  }

  # check for gettext utilities
  AC_PATH_PROG(ICONV, iconv, iconv)
  AC_PATH_PROG(MSGFMT, msgfmt, msgfmt)
  AC_PATH_PROG(MSGMERGE, msgmerge, msgmerge)
  AC_PATH_PROG(XGETTEXT, xgettext, xgettext)
  AC_SUBST(ac_aux_dir) dnl # needed to ship various intlfix scripts
  INTLMERGE='$(top_builddir)/$(ac_aux_dir)/intltool-merge'
  AC_SUBST(INTLMERGE) dnl # AC_PATH_PROG(INTLMERGE, intltool-merge, intltool-merge)
])
AC_DEFUN([MC_BSE_REQUIREMENTS],
[
    dnl # require Rapicorn
    dnl # PKG_CHECK_MODULES(RAPICORN, rapicorn >= 17.0.0)

    dnl # --- Ogg/Vorbis check ---
    PKG_CHECK_MODULES(OGGVORBIS, ogg >= 1.2.2 vorbis >= 1.3.2 vorbisfile >= 1.3.2 vorbisenc >= 1.3.2)
    AC_MSG_CHECKING([for Vorbisfile versions with buggy pcm_seek])
    vorbisfile_badseek=0 # pcm_seek bug near EOF for small files in vorbisfile <= 1.3.4
    PKG_CHECK_EXISTS(vorbisfile <= 1.3.4, vorbisfile_badseek=1)
    AC_MSG_RESULT([$(test $vorbisfile_badseek = 1 && echo yes || echo no)])
    AC_DEFINE_UNQUOTED(VORBISFILE_BADSEEK, [$vorbisfile_badseek], [buggy Vorbisfile pcm_seek])

    dnl # --- libmad MPEG decoder check ---
    dnl # libmad doesn't come with a mad.pc file (just its debian package)
    dnl # PKG_CHECK_MODULES(MAD, mad >= 0.14.2,
    dnl #     BSE_HAVE_LIBMAD=1
    dnl #     ,
    dnl #     BSE_HAVE_LIBMAD=0
    dnl #     AC_MSG_WARN([failed to detect libmad (MPEG I-III audio decoder) or headers])
    dnl # )
    MAD_LIBS=
    mad_tests=0
    AC_CHECK_LIB(mad, mad_stream_errorstr,
        [AC_CHECK_HEADER(mad.h, mad_tests=`expr $mad_tests + 1`)]
        ,
        mad_tests=0, -lm)
    if test $mad_tests = 1 ; then
	BSE_HAVE_LIBMAD=1
        MAD_LIBS="-lmad -lm"
    else
        AC_MSG_WARN([failed to detect libmad (MPEG I-III audio decoder) or headers])
	BSE_HAVE_LIBMAD=0
    fi
    AC_DEFINE_UNQUOTED(BSE_HAVE_LIBMAD, $BSE_HAVE_LIBMAD, [Whether bse links against libmad])

    dnl # --- check codec requirements ---
    AC_MSG_CHECKING([the required GSL-Loader library set])
    if echo " $OGGVORBIS_LIBS" | grep ".*-lvorbis.*" >/dev/null ; then
      AC_MSG_RESULT([complete])
    else
      AC_MSG_RESULT([])
      AC_MSG_ERROR([Ogg/Vorbis is missing, but required])
    fi

    dnl # --- figure stuff for bse.pc ---
    BSE_PC_REQUIRE=
    dnl # except for debian, libmad doesn't ship a mad.pc file
    dnl #if test $BSE_HAVE_LIBMAD = 1 ; then
    dnl #	BSE_PC_REQUIRE=mad
    dnl #fi
    BSE_PC_CFLAGS=
    BSE_PC_LIBS="$OGGVORBIS_LIBS"
    AC_SUBST(BSE_PC_REQUIRE)
    AC_SUBST(BSE_PC_CFLAGS)
    AC_SUBST(BSE_PC_LIBS)

    # == sizeof ==
    AC_CHECK_SIZEOF(uint, 0, [#include <sys/types.h>])
    BSE_SIZEOF_SYS_TYPESH_UINT="$ac_cv_sizeof_uint"
    AC_SUBST(BSE_SIZEOF_SYS_TYPESH_UINT)

    dnl # --- sizeof types ---
    GLIB_SIZEOF([#include <pthread.h>], pthread_mutex_t, pth_mutex_t)
    SFI_SIZEOF_PTH_MUTEX_T="$glib_cv_sizeof_pth_mutex_t"
    AC_SUBST(SFI_SIZEOF_PTH_MUTEX_T)
    GLIB_SIZEOF([#include <pthread.h>], pthread_cond_t, pth_cond_t)
    SFI_SIZEOF_PTH_COND_T="$glib_cv_sizeof_pth_cond_t"
    AC_SUBST(SFI_SIZEOF_PTH_COND_T)

    dnl # --- pthread_mutexattr_settype ---
    AC_MSG_CHECKING([for pthread_mutexattr_settype()])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      #define _XOPEN_SOURCE   600
      #include <pthread.h>
    ]], [[
      int (*attr_settype) (pthread_mutexattr_t *__attr, int __kind) = pthread_mutexattr_settype;
      int val = PTHREAD_MUTEX_RECURSIVE;
      attr_settype = 0; val = 0;
    ]])],[
      SFI_HAVE_MUTEXATTR_SETTYPE=1
      AC_MSG_RESULT(yes)
    ],[
      SFI_HAVE_MUTEXATTR_SETTYPE=0
      AC_MSG_RESULT(no)
    ])
    AC_SUBST(SFI_HAVE_MUTEXATTR_SETTYPE)

    dnl # --- math.h glibc extensions
    AC_CHECK_LIB(m, sincos,
      AC_DEFINE(HAVE_SINCOS, 1, [Define if you have the sincos function in <math.h>]))
])
AC_DEFUN([MC_ASOUND_CHECK],
[
    saved_cflags="$CFLAGS"
    saved_cxxflags="$CXXFLAGS"
    saved_ldflags="$LDFLAGS"
    dnl # automatically search for libasound under the BSE prefix
    CFLAGS="$CFLAGS $BSE_CFLAGS"
    CXXFLAGS="$CFLAGS $BSE_CFLAGS"
    LDFLAGS="$LDFLAGS $BSE_LIBS"
    AC_LANG_SAVE

    dnl --- find asoundlib.h ---
    found_asoundlibh=0
    AC_CHECK_HEADERS(alsa/asoundlib.h, found_asoundlibh=1)
    if test $found_asoundlibh = 0 ; then
	AC_MSG_ERROR([failed to detect asoundlib.h, ALSA not present?])
    fi

    dnl --- check for ALSA >= 1.0.5 ---
    AC_MSG_CHECKING([for ALSA with version >= 1.0])
    found_alsa1=0
    AC_TRY_COMPILE([#include <alsa/asoundlib.h>],[
            #if SND_LIB_MAJOR >= 1
		/* ALSA >= 1.0 present */
	    #else
		#error ALSA version <= 1.0
	    #endif
    ], [ found_alsa1=1; AC_MSG_RESULT(present); ], [ AC_MSG_RESULT(not present); ])
    if test $found_alsa1 = 0; then
	AC_MSG_ERROR([ALSA version 1.0.5 is required to build this package])
    fi

    dnl --- check for -lasound ---
    found_libasound=0
    dnl snd_asoundlib_version() was introduced in ALSA 1.0.5
    CFLAGS=""  # kept in saved_cflags
    LDFLAGS="" # kept in saved_ldflags
    AC_CHECK_LIB(asound, snd_asoundlib_version, ALSA_LIBS="-lasound"; found_libasound=1)
    if test $found_libasound = 0; then
	AC_MSG_ERROR([ALSA version 1.0.5 is required to build this package])
    fi
    AC_SUBST(ALSA_LIBS)

    AC_LANG_RESTORE
    CFLAGS="$saved_cflags"
    CXXFLAGS="$saved_cxxflags"
    LDFLAGS="$saved_ldflags"
])
AC_DEFUN([MC_GXK_REQUIREMENTS],
[
    dnl # required libraries, defines LIB*_CFLAGS and LIB*_LIBS:
    PKG_CHECK_MODULES(LIBPANGO, pangoft2 >= 1.4.0)
    PKG_CHECK_MODULES(LIBGTK, gtk+-2.0 >= 2.12.12)
    PKG_CHECK_MODULES(LIBART, libart-2.0 >= 2.3.8)
    PKG_CHECK_MODULES(LIBGNOMECANVAS, libgnomecanvas-2.0 >= 2.4.0)
    GXK_CFLAGS="$LIBPANGO_CFLAGS $LIBGTK_CFLAGS $LIBART_CFLAGS $LIBGNOMECANVAS_CFLAGS"
    GXK_LIBS="$LIBPANGO_LIBS $LIBGTK_LIBS $LIBART_LIBS $LIBGNOMECANVAS_LIBS"
    AC_SUBST(GXK_CFLAGS)
    AC_SUBST(GXK_LIBS)
])
AC_DEFUN([MC_BEAST_REQUIREMENTS],
[
    dnl # start where GXK left off
    BEAST_CFLAGS="$GXK_CFLAGS"
    BEAST_LIBS="$GXK_LIBS"
    AC_SUBST(BEAST_CFLAGS)
    AC_SUBST(BEAST_LIBS)

    dnl # Feature XKB extension if present
    cflags=$CFLAGS
    ldflags=$LDFLAGS
    CFLAGS=$BEAST_CFLAGS
    LDFLAGS=$BEAST_LIBS
    have_xkb=
    AC_CHECK_LIB(X11, XkbOpenDisplay, have_xkb=1$have_xkb)
    AC_CHECK_LIB(X11, XkbGetKeyboard, have_xkb=2$have_xkb)
    AC_CHECK_HEADER(X11/XKBlib.h, have_xkb=3$have_xkb)
    AC_CHECK_HEADER(X11/extensions/XKBgeom.h, have_xkb=4$have_xkb, , X11/Xlib.h)
    MC_IF_VAR_EQ(have_xkb, 4321,
	AC_DEFINE_UNQUOTED(BST_WITH_XKB, 1, [XKB present]),
    )
    CFLAGS=$cflags
    LDFLAGS=$ldflags
])
AC_DEFUN([MC_SUIDMAIN_REQUIREMENTS],
[
    dnl # figure everything required by the suid wrapper
    AC_CHECK_FUNCS(seteuid)
    AC_CHECK_FUNCS(setreuid)
])
AC_DEFUN([MC_BSE_SNDDEV_CHECK],
[
    dnl # OSS-Lite driver check
    AC_CHECK_HEADERS(sys/soundcard.h soundcard.h, have_oss_header=yes, have_oss_header=no)
    AC_ARG_ENABLE(osspcm,
	[  --enable-osspcm=DEVICE  force DEVICE as OSS PCM device name],
	[AC_MSG_WARN([Ignoring detection of PCM device])],
	[enable_osspcm=""])
    AC_ARG_ENABLE([ossmidi],
	[  --enable-ossmidi=DEVICE force DEVICE as OSS MIDI device name],
	[AC_MSG_WARN([Ignoring detection of MIDI device])],
	[enable_ossmidi=""])

    dnl # === OSS PCM device ===
    AC_MSG_CHECKING([for available OSS audio devices])
    result=
    if test -n "$enable_osspcm" ; then
	dev="$enable_osspcm"
    else
	for dev in /dev/dsp /dev/sound \
	    /dev/dsp0 /dev/dsp1 /dev/dsp2 /dev/dsp3 ; do
	  if test -c $dev ; then
	      break;
	  else
	      dev=""
	  fi
        done
    fi
    if test -n "$dev" ; then
	bse_found_pcmdev=yes
	AC_DEFINE_UNQUOTED(BSE_PCM_DEVICE_CONF_OSS, "$dev", [Device used for PCM I/O])
	result="PCM:$dev "
    else
	bse_found_pcmdev=no
	AC_DEFINE_UNQUOTED(BSE_PCM_DEVICE_CONF_OSS, "/dev/dsp", [Device used for PCM I/O])
	result="PCM:none (assuming PCM:/dev/dsp)"
    fi
    AC_MSG_RESULT($result)

    dnl # === OSS MIDI device ===
    AC_MSG_CHECKING([for available OSS midi devices])
    if test -n "$enable_ossmidi" ; then
        dev="$enable_ossmidi"
    else
	for dev in /dev/midi \
	    /dev/midi0  /dev/midi1  /dev/midi2  /dev/midi3 \
	    /dev/rmidi0 /dev/rmidi1 /dev/rmidi2 /dev/rmidi3 ; do
	  if test -c $dev ; then
	      break;
	  else
	      dev=""
	  fi
        done
    fi
    if test -n "$dev" ; then
	bse_found_mididev=yes
        AC_DEFINE_UNQUOTED(BSE_MIDI_DEVICE_CONF_OSS, "$dev", [Device used for MID I/O])
        result="MIDI:$dev "
    else
	bse_found_mididev=no
        AC_DEFINE_UNQUOTED(BSE_MIDI_DEVICE_CONF_OSS, "/dev/midi", [Device used for MID I/O])
        result="MIDI:none (assuming MIDI:/dev/midi)"
    fi
    AC_MSG_RESULT($result)

    dnl # == Complain about mising devices ===
    if test "$bse_found_pcmdev $bse_found_mididev" \!= "yes yes"; then
	AC_MSG_WARN([failed to detect proper sound device files])
    fi
])
AC_DEFUN([MC_DOC_REQUIREMENTS],
[
])

# find installation utilities
AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database)
AC_PATH_PROG(UPDATE_DESKTOP_DATABASE, update-desktop-database)

dnl # MC_PYTHON_REQUIREMENTS() - python checks
AC_DEFUN([MC_PYTHON_REQUIREMENTS],
[
  PYTHON_REQUIRED=2.7
  AM_PATH_PYTHON($PYTHON_REQUIRED)
  dnl # AC_REQUIRE([AC_PYTHON_DEVEL]) # AC_PYTHON_DEVEL is usually provided by autoconf-archive
  AC_PYTHON_DEVEL([>= $PYTHON_REQUIRED])
  if test -z "$PYTHON" || $PYTHON -c "import sys; sys.exit (sys.hexversion >= 0x2070500)" 2>/dev/null ; then
    AC_MSG_ERROR([failed to detect python (version >= 2.7.5)])
  fi
])

AC_CHECK_PROGS(PERL, perl5 perl)
if test -z "$PERL" || $PERL -e 'exit ($] >= 5.002)' >/dev/null ; then
    AC_MSG_WARN([failed to detect perl (version >= 5.002), disabling dependent build rules])
    PERLRULE='#'
else
    PERLRULE="$SRCEQBUILDRULE"
fi
AC_SUBST(PERLRULE)
AM_CONDITIONAL(WITH_PERLRULE, [test "$PERLRULE" != "#"])

# Check requirement sets
MC_I18N_REQUIREMENTS
MC_BSE_REQUIREMENTS
MC_ASOUND_CHECK
MC_BSE_SNDDEV_CHECK
MC_PYTHON_REQUIREMENTS
MC_GXK_REQUIREMENTS
MC_BEAST_REQUIREMENTS
MC_SUIDMAIN_REQUIREMENTS
MC_DOC_REQUIREMENTS

# create output files
AC_CONFIG_FILES([
Makefile
data/Makefile
data/beast.applications
sfi/Makefile
sfi/sysconfig.h
sfi/tests/Makefile
bse/bseconfig.h
bse/Makefile
bse/icons/Makefile
bse/tests/Makefile
plugins/Makefile
plugins/icons/Makefile
plugins/freeverb/Makefile
drivers/Makefile
beast-gtk/Makefile
beast-gtk/gxk/Makefile
beast-gtk/icons/Makefile
launchers/Makefile
tools/Makefile
tools/scripts/Makefile
po/Makefile
docs/Makefile
docs/images/Makefile
])

AC_OUTPUT

# == Configuration summary ==
printexpandvar()
{
	_x="$1";
	while echo " $_x" | fgrep -q '${'; do
		_x=$(eval echo "$_x")
	done
	echo -n "$_x"
}

AM_COND_IF(WITH_MMX_SSE, [summary_mmx_sse=yes], [summary_mmx_sse=no])
AM_COND_IF(WITH_NPM, [summary_npm=yes], [summary_npm=no])
if test x$BSE_HAVE_LIBMAD = x1; then
  summary_libmad=yes
else
  summary_libmad=no
fi

cat <<EOF

Autoconfiguration complete.

	Building package:		$PACKAGE-$VERSION

	Install Prefix:			`printexpandvar ${prefix}`
	Binaries:			`printexpandvar ${bindir}`
	Beast Program Directory:	`printexpandvar ${beastdir}`

	MMX/SSE optimizations:		${summary_mmx_sse}
	NPM available:			${summary_npm}
	MP3 support using libmad:	${summary_libmad}

EOF
