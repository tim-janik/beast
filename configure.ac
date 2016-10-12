dnl # Beast & Bse                                    -*- Mode: shell-script; -*-

# include various m4 macros
m4_include(acbeast.m4)dnl # include special macros
m4_include(ld-symbolic.m4)dnl # include special macros
m4_include(autotools/glib-gettext.m4)


#
# Configure script for BEAST/BSE
#
AC_PREREQ(2.57)
# VERSION: MAJOR.MINOR.MICRO-RC:
# * MAJOR:           Increment on major API breaks, *forces* ABI break, resets MINOR, MICRO and ABI_VERSION
# * MINOR:           Increment for significant changes or ABI breaks, resets MICRO
# * MICRO:           Increment with every release, increments must *not* break ABI
# * ABI_VERSION:     Record the latest version since ABI compatibility has been kept
# * ABI changes:     Increment MAJOR or MINOR and reset ABI_VERSION to the current VERSION
# * -RC:             Release candidate, may contain -rc[0-9]+ or nothing
AC_INIT([beast], [0.10.1]) # defines PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_STRING
:;   ABI_VERSION=[0.10.0]
BST_VERSION_HINT=alpha
AC_CONFIG_SRCDIR([bse/bseconfig.h.in])
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_MACRO_DIR([autotools])
AC_CANONICAL_TARGET					# architecture information
AM_INIT_AUTOMAKE(1.9 dist-xz tar-pax no-dist-gzip no-define foreign)
AM_CONFIG_HEADER(configure.h)
# version handling
BST_VERSION="$PACKAGE_VERSION"
[MAJOR=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\1/'`]
[MINOR=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\2/'`]
[MICRO=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\3/'`]
AC_SUBST(BEAST_VERSION)
AC_DEFINE_UNQUOTED(BST_ARCH_NAME, "$target", [Architecture Description])
AC_DEFINE_UNQUOTED(BST_VERSION, "$BST_VERSION", [Beast Version])
AC_DEFINE_UNQUOTED(BST_VERSION_HINT, "$BST_VERSION_HINT", [Version classification])
AC_DEFINE_UNQUOTED(BSE_MAJOR_VERSION, $MAJOR, [Beast/Bse Major Version])
AC_DEFINE_UNQUOTED(BSE_MINOR_VERSION, $MINOR, [Beast/Bse Minor Version])
AC_DEFINE_UNQUOTED(BSE_MICRO_VERSION, $MICRO, [Beast/Bse Micro Version])
AC_DEFINE_UNQUOTED(BEAST_EXEC_POSTFIX, "-$MAJOR.$MINOR.$MICRO", [Beast executable postfix])
# For automake, we need to easily provide MAJOR.MINOR.MICRO and MAJOR_MINOR_MICRO:
AC_SUBST(MAJOR)
AC_SUBST(MINOR)
AC_SUBST(MICRO)
# calculate ABI age
ABI_MAJOR=[`echo "$ABI_VERSION" | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\1/'`]
ABI_MINOR=[`echo "$ABI_VERSION" | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\2/'`]
ABI_MICRO=[`echo "$ABI_VERSION" | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\3/'`]
test "$ABI_MAJOR" = "$MAJOR" || {
  AC_MSG_ERROR([ABI_VERSION must match MAJOR version: $ABI_MAJOR == $MAJOR])
}
test "$ABI_MINOR" -le "$MINOR" || {
  AC_MSG_ERROR([ABI_VERSION must predate MINOR version: $ABI_MINOR <= $MINOR])
}
test "$ABI_MICRO" -eq 0 || {
  AC_MSG_ERROR([ABI_VERSION must not indicate ABI break in MICRO releases: $ABI_MINOR.$ABI_MICRO <= $MINOR.0])
}
ABI_AGE=$(($MINOR - $ABI_MINOR))
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# Note, Beast provides backwards binary compatibility only within
# MINOR.MICRO increments, and only since ABI_VERSION.
# Interface additions occour with pretty much every release.
# For libtool this means:
# - the library name needs to encode MAJOR
# - the '-release X.Y' option cannot be used, because it's ignored for naming .a, .la, .so files.
# - lt_current=MINOR  # disallow forward compatible builds, i.e. enforce runtime-lib >= build-time-lib
# - lt_age=ABI_AGE    # allow backward compatibility between ABI_VERSION .. VERSION
# - lt_revision=MICRO # pick the latest SO revision for unchanged MAJOR.MINOR,
#                     # see also: https://autotools.io/libtool/version.html
lt_current=$MINOR ; lt_age=$ABI_AGE ; lt_revision=$MICRO
LT_VERSION_INFO="$lt_current:$lt_revision:$lt_age"
AC_SUBST(LT_VERSION_INFO)

# Define package directories per major/minor version
bseincludedir="\${includedir}/bse-$MAJOR"
bselibdir="\${libdir}/bse-$MAJOR.$MINOR.$MICRO"
beastexecdir="\${libexecdir}/beast-$MAJOR.$MINOR.$MICRO"
datadir="\${datarootdir}/beast"
docdir="${datarootdir}/doc/beast" # "beast-$MAJOR"
# Substitute directories in Makefiles
AC_SUBST(bseincludedir)
AC_SUBST(bselibdir)
AC_SUBST(beastexecdir)
AC_SUBST(datadir)
AC_SUBST(docdir)
# Provide directories for C++
bak_prefix="$prefix"; bak_exec_prefix="$exec_prefix"; bak_libdir="$libdir"; bak_libexecdir="$libexecdir"
bak_datarootdir="$datarootdir"; bak_datadir="$datadir"; bak_includedir="$includedir"
test "x$prefix" = xNONE && prefix="$ac_default_prefix" ; test "x$exec_prefix" = xNONE && exec_prefix="$prefix"
libdir=`eval echo "${libdir}"` ; libexecdir=`eval echo "${libexecdir}"` ; datarootdir=`eval echo "${datarootdir}"` ;
datadir=`eval echo "${datadir}"`; bseincludedir=`eval echo "${bseincludedir}"`
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_BSEINCLUDEDIR, ["`eval echo "${bseincludedir}"`"], [Sfidl standard include directory])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_BINDIR, ["`eval echo "${bindir}"`"], [Directory to install and execute programs])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_LOCALEBASE, ["`eval echo ${localedir}`"], [Base directory for locale specific message catalogs])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_LADSPA, ["`eval echo ${libdir}/ladspa`"], [Searchpath for LADSPA plugins])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_BSELIBDIR, ["`eval echo ${bselibdir}`"], [Directory to install plugins and drivers])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_BEASTEXECDIR, ["`eval echo ${beastexecdir}`"], [Directory under whic to install Beast executable files])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_DATADIR, ["`eval echo ${datadir}`"], [Directory to install samples, effects, scripts, etc])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_DOCDIR, ["`eval echo ${docdir}`"], [Directory to install documentation])
AC_DEFINE_UNQUOTED(CONFIGURE_INSTALLPATH_USER_DATA, ["~/beast"], [Searchpath for user owned Beast resources])
prefix="$bak_prefix"; exec_prefix="$bak_exec_prefix"; libdir="$bak_libdir"; libexecdir="$bak_libexecdir"
datarootdir="$bak_datarootdir"; datadir="$bak_datadir"; includedir="$bak_includedir"

# Build tree checks
AC_MSG_CHECKING([for git repository])
test -f "${srcdir}/.git/index" && INGIT= || INGIT='#'
AC_SUBST(INGIT)
result=$(test -z "$INGIT" && echo yes || echo no)
AC_MSG_RESULT($result)

# We have subdirectories.
AC_PROG_MAKE_SET
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# declare --enable-* args and collect ac_help strings
AC_ARG_ENABLE(debug,           [  --enable-debug=no/minimum/yes turn on debugging, default=yes],,enable_debug=yes)
AC_ARG_ENABLE(devel-mode,      [  --enable-devel-mode=no/yes    turn on development mode, default=auto],,enable_devel_mode=auto)

dnl # Foreign defaults:
MC_IF_VAR_EQ(enable_static, yes, , enable_static=no)
MC_IF_VAR_EQ(enable_shared, no, , enable_shared=yes)

# compiler, linker & libtool
saved_CFLAGS="$CFLAGS"
AC_PROG_CC
CFLAGS="$saved_CFLAGS" 		# AC_PROG_CC_G called internally consistently breaks $CFLAGS
saved_CXXFLAGS="$CFLAGS"
AC_PROG_CPP
CXXFLAGS="$saved_CXXFLAGS"	# AC_PROG_CXX_G called internally consistently breaks $CXXFLAGS
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_LD
AC_LANG_CPLUSPLUS	# use CXX for compilation tests
AC_LIB_PROG_LD
AC_LIB_PROG_LD_GNU
AM_PROG_LIBTOOL
AC_LIBTOOL_DLOPEN
ACX_PROG_LD_GNU_SYMBOLIC
# SYMBOLIC_LDFLAGS is included in DYNAMIC_LIST_CPP_NEW_LDFLAGS
ACX_PROG_LD_GNU_DYNAMIC_LIST_CPP_NEW
MC_EVAR_ADD(AM_LDFLAGS, $DYNAMIC_LIST_CPP_NEW_LDFLAGS)
MC_EVAR_ADD(AM_LDFLAGS, -pthread)
MC_EVAR_ADD(AM_LDFLAGS, -lpthread)
MC_EVAR_ADD(AM_LDFLAGS, -lrt)
MC_EVAR_ADD(AM_LDFLAGS, [-Wl,--no-undefined])
if ${LD} --help 2>&1 | grep -q -- '-export-dynamic\b' ; then
  MC_EVAR_ADD(AM_LDFLAGS, [-Wl,-export-dynamic])
fi

# == C++ Dialect ==
CXX="$CXX -std=gnu++11"
test -z "$CXXCPP" || CXXCPP="$CXXCPP -std=gnu++11"

# AM_LDFLAGS, optimize linker to use faster ELF symbol hashing if we have compiler and linker support
AC_MSG_CHECKING([whether CC and LD support gnu.hash sections])
if ${CC-cc} -dumpspecs | grep -q -- '--hash-style=\(both\|gnu\).*--as-needed' && \
   ${LD-ld} --help     | grep -q -- '--hash-style=.*\(both\|gnu\)' ; then
  MC_EVAR_ADD(AM_LDFLAGS, [-Wl,--hash-style=both -Wl,--as-needed -Wl,-O1])
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST(AM_LDFLAGS)

# AM_CFLAGS & AM_CXXFLAGS
MC_CXX_TRY_OPTION(AM_CXXFLAGS,  -std=gnu++0x)
BOTHCCXXFLAGS=
if test "x$enable_debug" = "xyes"; then
  MC_EVAR_ADD(BOTHCCXXFLAGS,      -DG_ENABLE_DEBUG)
  MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -g3)
  MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -fno-omit-frame-pointer)
  MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -fverbose-asm)
  dnl # MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Q -ftime-report -fmem-report)
elif test "x$enable_debug" = "xno"; then
  MC_EVAR_ADD(BOTHCCXXFLAGS, [-DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS]) # -DG_DISABLE_ASSERT
else
  : # minimum debugging
fi
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Wall)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Wdeprecated)
MC_CC_TRY_OPTION(AM_CFLAGS,     -Wmissing-prototypes)
MC_CC_TRY_OPTION(AM_CFLAGS,     -Wno-pointer-sign)
MC_CC_TRY_OPTION(AM_CFLAGS,     -Wnested-externs)
MC_CC_TRY_OPTION(AM_CFLAGS,     -fno-cond-mismatch)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Wmissing-declarations)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Wpointer-arith)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Wredundant-decls)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Werror=format-security)
MC_CXX_TRY_OPTION(AM_CXXFLAGS,  -Wsign-promo)
MC_CXX_TRY_OPTION(AM_CXXFLAGS,  -Woverloaded-virtual)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -rdynamic)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -pipe)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -O2)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -ftracer)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -mcx16)
dnl # MC_CXX_TRY_OPTION(AM_CXXFLAGS,  -Wno-invalid-offsetof)
MC_EVAR_ADD(AM_CFLAGS,   "$BOTHCCXXFLAGS")
MC_EVAR_ADD(AM_CXXFLAGS, "$BOTHCCXXFLAGS")
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(AM_CPPFLAGS)

# == CPU Optimizations ==
# MMX and SSE instruction sets have been introduced way more than a decade ago:
# Intel Pentium 3: CMOV MMX SSE (1999)
# VIA C3 Nehemiah: CMOV MMX SSE (2003)
# Athlon Palomino: CMOV MMX SSE 3DNow (2001)
SIMD_FLAGS=
MC_CC_TRY_OPTION(SIMD_FLAGS, -mmmx)
MC_CC_TRY_OPTION(SIMD_FLAGS, -msse)
# Note, -mfpmath=sse can harm FPU-algorithm performance
# Note, -funroll-loops can harm SSE loop performance
#MC_CC_TRY_OPTION(SIMD_FLAGS, -funroll-loops)
#AC_SUBST(SIMD_FLAGS)
# Expect the target architecture to support MMX/SSE if the compiler does it.
MC_EVAR_ADD(AM_CFLAGS,   "$SIMD_FLAGS")
MC_EVAR_ADD(AM_CXXFLAGS, "$SIMD_FLAGS")
# Newer vectorization instruction sets, should be optionally supported
# SSE >= 2, AVX
SSEAVX_FLAGS=
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse2)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse3)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -mssse3)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse4)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse4.1)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse4.2)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse4a)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -mavx)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -mavx2)
AC_SUBST(SSEAVX_FLAGS)

# == OPTIMIZE_FAST ==
# Some critical code paths should be optimized to run as fast as possible
OPTIMIZE_FAST=
MC_CC_TRY_OPTION(OPTIMIZE_FAST, -O3)
MC_CC_TRY_OPTION(OPTIMIZE_FAST, -ftree-vectorize)
MC_CC_TRY_OPTION(OPTIMIZE_FAST, -finline-functions)
MC_CC_TRY_OPTION(OPTIMIZE_FAST, -minline-all-stringops)
AC_SUBST(OPTIMIZE_FAST)

# == Bse Plugin FLAGS ==
# * -funroll-loops significantly boosts FPU performance
# * -mfpmath=sse harms FPU-algorithm performance
# * -funroll-loops harms SSE loop performance
MC_CXX_TRY_OPTION(FAST_MATH_FLAGS, -ftree-loop-ivcanon)
MC_CXX_TRY_OPTION(FAST_MATH_FLAGS, -ftree-loop-linear)
MC_CXX_TRY_OPTION(FAST_MATH_FLAGS, -ftree-loop-im)
MC_CXX_TRY_OPTION(FAST_MATH_FLAGS, -ffast-math)
MC_CXX_TRY_OPTION(FAST_MATH_FLAGS, -fivopts)
AC_SUBST(FAST_MATH_FLAGS)
MC_CXX_TRY_OPTION(FPU_FLAGS, -funroll-loops)
AC_SUBST(FPU_FLAGS)
SIMD_FLAGS="$SIMD_FLAGS"
AC_SUBST(SIMD_FLAGS)

# == SSE Build Conditionals ==
lacking_mmx_sse=`echo "$SIMD_FLAGS" | grep 'mmx.*sse' -q ; echo $?` # sh truth values: 1 (without sse) or 0 (with sse)
AM_CONDITIONAL(WITH_MMX_SSE, test 0 == $lacking_mmx_sse)
AC_DEFINE_UNQUOTED(BSE_WITH_MMX_SSE, [(0 == $lacking_mmx_sse)], [wether MMX and SSE are supported])

# C & C++ idioms
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDC

# Checks for programs
AC_PROG_INSTALL
AC_PROG_LN_S

# require pkg-config
MC_ASSERT_PROG(PKG_CONFIG, pkg-config, [pkg-config can be found at http://www.freedesktop.org/software/pkgconfig/])

# == Libraries ==
# libz
AC_CHECK_LIB(z, gzsetparams, [AC_CHECK_HEADER(zlib.h, LIBZ_LIBS='-lz', LIBZ='')], LIBZ='')
if test -z "$LIBZ_LIBS"; then
    AC_MSG_ERROR([Compression library libz is missing, but required])
fi
AC_SUBST(LIBZ_LIBS)

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
  AM_GLIB_GNU_GETTEXT([external])
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
AC_DEFUN([MC_SFI_REQUIREMENTS],
[
    dnl # check for GLib libs
    PKG_CHECK_MODULES(GLIB_GOMT, glib-2.0 >= 2.32.3 gobject-2.0 gmodule-no-export-2.0 gthread-2.0)
    AC_SUBST(GLIB_GOMT_CFLAGS)
    AC_SUBST(GLIB_GOMT_LIBS)
    SFI_CPPFLAGS="$GLIB_GOMT_CFLAGS"
    SFI_LIBS="$GLIB_GOMT_LIBS"
    AC_SUBST(SFI_CPPFLAGS)
    AC_SUBST(SFI_LIBS)

    AC_CHECK_FUNCS(getpwnam_r getpwnam)
    AC_CHECK_FUNCS(timegm)
])
AC_DEFUN([MC_BSE_REQUIREMENTS],
[
    dnl # require Rapicorn
    PKG_CHECK_MODULES(RAPICORN, rapicorn >= 16.0.1)
    MC_ASSERT_PROG(AIDACC, aidacc, [aidacc gets shipped with Rapicorn])
    MC_ASSERT_PROG(AIDACC, rapidres, [rapidres gets shipped with Rapicorn])

    dnl # --- Ogg/Vorbis check ---
    PKG_CHECK_MODULES(OGGVORBIS, ogg >= 1.2.2 vorbis >= 1.3.2 vorbisfile >= 1.3.2 vorbisenc >= 1.3.2)
    AC_MSG_CHECKING([for Vorbisfile versions with buggy pcm_seek])
    vorbisfile_badseek=0 # pcm_seek bug near EOF for small files in vorbisfile <= 1.3.4
    PKG_CHECK_EXISTS(vorbisfile <= 1.3.4, vorbisfile_badseek=1)
    AC_MSG_RESULT([$(test $vorbisfile_badseek = 1 && echo yes || echo no)])
    AC_DEFINE_UNQUOTED(VORBISFILE_BADSEEK, [$vorbisfile_badseek], [buggy Vorbisfile pcm_seek])

    dnl # --- FLAC check ---
    PKG_CHECK_MODULES(FLAC, flac)

    dnl # old FLAC versions use "-I/usr/.../include/FLAC" as pkg-config --cflags
    dnl # new FLAC versions use "-I/usr/.../include" as pkg-config --cflags
    dnl #
    dnl # to support both, we strip /FLAC from FLAC_CFLAGS and use #include <FLAC/foo.h>
    FLAC_CFLAGS=`echo $FLAC_CLFAGS | $SED "s/FLAC$//"`

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

    dnl # --- complete CFLAGS/LIBS setup ---
    BSE_CFLAGS="$MAD_CFLAGS $FLAC_CFLAGS $SFI_CPPFLAGS"
    BSE_LIBS="$OGGVORBIS_LIBS $MAD_LIBS $FLAC_LIBS $SFI_LIBS"
    AC_SUBST(BSE_CFLAGS)
    AC_SUBST(BSE_LIBS)
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
AC_DEFUN([MC_BSESCM_REQUIREMENTS],
[
    BSESCM_CFLAGS="$BSE_CFLAGS"
    BSESCM_LIBS="$BSE_LIBS"

    dnl # --- libguile check ---
    guile_tests=0
    GUILE_VERSION=none
    REQUIRED_VERSION=1.6
    AC_PATH_PROG(GUILE_CONFIG, guile-config)
    if test -n "$GUILE_CONFIG"; then
	guile_tests=`expr $guile_tests + 1`
	GUILE_VERSION=`$GUILE_CONFIG info guileversion`
	GUILE_CFLAGS=`$GUILE_CONFIG compile`
	GUILE_LDFLAGS=`$GUILE_CONFIG link`
	MC_CHECK_VERSION($GUILE_VERSION, $REQUIRED_VERSION, [guile_tests=`expr $guile_tests + 1`])
	dnl # check headers seperately, since some distributions ship on-devel guile packages with guile-config
	AC_CHECK_HEADER(guile/gh.h, guile_tests=`expr $guile_tests + 1`)
	AC_CHECK_HEADER(libguile/properties.h, guile_tests=`expr $guile_tests + 1`) dnl # guile-1.6 headerfile
    fi
    if test $guile_tests = 4 ; then
	BSESCM_CFLAGS="$GUILE_CFLAGS $BSESCM_CFLAGS"
	BSESCM_LIBS="$GUILE_LDFLAGS $BSESCM_LIBS" 
    else
	EMSG="failed to detect guile headers and library v$REQUIRED_VERSION"
	EMSG="$EMSG (available from http://www.gnu.org/software/guile/guile.html)"
	EMSG="$EMSG - detected library version: $GUILE_VERSION"
	AC_MSG_ERROR([$EMSG])
    fi

    AC_SUBST(BSESCM_CFLAGS)
    AC_SUBST(BSESCM_LIBS)
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

# == ENABLE_DEVEL_MODE ==
# check for development build rules, which require acess to .git/
AC_MSG_CHECKING(for ENABLE_DEVEL_MODE)
[test x"$enable_devel_mode" = xauto -a -d "$srcdir/.git/" && enable_devel_mode=yes]
test x"$enable_devel_mode" = xyes || enable_devel_mode=no
AC_MSG_RESULT([$enable_devel_mode])
AM_CONDITIONAL(ENABLE_DEVEL_MODE, test x"$enable_devel_mode" = xyes)
if test x"$enable_devel_mode" = xyes ; then
  git show HEAD:configure.ac 2>&1 | grep -q 'AC'_'INIT.*\bbeast\b.*,' || {
    AC_MSG_ERROR([development build rules require a beast.git repository])
  }
  MC_ASSERT_PROG(XMLLINT, xmllint, [See: http://xmlsoft.org/xmllint.html])
  DEFINE__FILE_DIR__='-D__FILE_DIR__=\"${abs_srcdir}\"' # full debugging paths for devel-mode
else
  DEFINE__FILE_DIR__='-D__FILE_DIR__=\"beast/${subdir}\"'
fi
AC_SUBST(DEFINE__FILE_DIR__)

dnl # MC_PYTHON_REQUIREMENTS() - python checks
AC_DEFUN([MC_PYTHON_REQUIREMENTS],
[
  PYTHON_REQUIRED=2.7
  AM_PATH_PYTHON($PYTHON_REQUIRED)
  dnl # AC_REQUIRE([AC_PYTHON_DEVEL]) # AC_PYTHON_DEVEL is usually provided by autoconf-archive
  AC_PYTHON_DEVEL([>= $PYTHON_REQUIRED])
  AC_MSG_CHECKING([for working Python extension compilation])
  PYTHON_CPPFLAGS="$PYTHON_CPPFLAGS -fno-strict-aliasing" # needed by Python headers
  saved_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $PYTHON_CPPFLAGS -Wall -Werror"     # verify sane Python module builds
  AC_TRY_COMPILE([#include <Python.h>], [Py_InitModule(0,0);], py_build=ok, py_build=failed)
  CPPFLAGS="$saved_CPPFLAGS"
  AC_MSG_RESULT($py_build)
  if test "$py_build" != "ok" ; then
    AC_MSG_ERROR([failed to build extension against Python (>=$PYTHON_REQUIRED) headers])
  fi
  if test -z "$PYTHON" || $PYTHON -c "import sys; sys.exit (sys.hexversion >= 0x2070500)" 2>/dev/null ; then
    AC_MSG_ERROR([failed to detect python (version >= 2.7.5)])
  fi
])

AC_MSG_CHECKING(whether source dir equals build dir)
TEST_SRCDIR=`cd $srcdir ; /bin/pwd`
TEST_BUILDDIR=`cd . ; /bin/pwd`
if test "$TEST_SRCDIR" = "$TEST_BUILDDIR" ; then
    AC_MSG_RESULT(yes)
    SRCEQBUILDRULE=''
else
    AC_MSG_RESULT(no)
    AC_MSG_WARN([disabling some build rules because srcdir != builddir])
    SRCEQBUILDRULE='#'
fi
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
MC_SFI_REQUIREMENTS
MC_BSE_REQUIREMENTS
MC_ASOUND_CHECK
MC_BSESCM_REQUIREMENTS
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
sfi/tests/Makefile
bse/bseconfig.h
bse/Makefile
bse/icons/Makefile
bse/res/Makefile
bse/pybse/Makefile
bse/tests/Makefile
plugins/Makefile
plugins/icons/Makefile
plugins/freeverb/Makefile
drivers/Makefile
shell/Makefile
pybeast/Makefile
beast-gtk/Makefile
beast-gtk/gxk/Makefile
beast-gtk/res/Makefile
beast-gtk/icons/Makefile
launchers/Makefile
library/Makefile
library/effects/Makefile
library/instruments/Makefile
library/samples/Makefile
library/scripts/Makefile
library/demo/Makefile
library/keys/Makefile
library/skins/Makefile
library/skins/images/Makefile
tools/Makefile
tools/scripts/Makefile
po/Makefile
tests/Makefile
tests/latency/Makefile
tests/scripts/Makefile
tests/bse/Makefile
tests/audio/Makefile
tests/filecheck/Makefile
docs/Makefile
docs/images/Makefile
docs/imports/Makefile
])

AC_OUTPUT
