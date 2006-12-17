dnl # Birnet
dnl # GNU Lesser General Public License version 2 or any later version.

## Portability defines that help interoperate with classic and modern autoconfs
ifdef([AC_TR_SH],[
define([GLIB_TR_SH],[AC_TR_SH([$1])])
define([GLIB_TR_CPP],[AC_TR_CPP([$1])])
], [
define([GLIB_TR_SH],
       [patsubst(translit([[$1]], [*+], [pp]), [[^a-zA-Z0-9_]], [_])])
define([GLIB_TR_CPP],
       [patsubst(translit([[$1]],
  	                  [*abcdefghijklmnopqrstuvwxyz],
 			  [PABCDEFGHIJKLMNOPQRSTUVWXYZ]),
		 [[^A-Z0-9_]], [_])])
])

# AC_DIVERT_BEFORE_HELP(STUFF)
# ---------------------------------
# Put STUFF early enough so that they are available for $ac_help expansion.
# Handle both classic (<= v2.13) and modern autoconf
AC_DEFUN([AC_DIVERT_BEFORE_HELP],
[ifdef([m4_divert_text], [m4_divert_text([NOTICE],[$1])],
       [ifdef([AC_DIVERT], [AC_DIVERT([NOTICE],[$1])],
              [AC_DIVERT_PUSH(AC_DIVERSION_NOTICE)dnl
$1
AC_DIVERT_POP()])])])


dnl GLIB_SIZEOF (INCLUDES, TYPE, ALIAS [, CROSS-SIZE])
AC_DEFUN([GLIB_SIZEOF],
[pushdef([glib_Sizeof], GLIB_TR_SH([glib_cv_sizeof_$3]))dnl
AC_CACHE_CHECK([size of $2], glib_Sizeof,
[AC_TRY_RUN([#include <stdio.h>
#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif
$1
main()
{
  FILE *f=fopen("conftestval", "w");
  if (!f) exit(1);
  fprintf(f, "%d\n", sizeof($2));
  exit(0);
}],
  [glib_Sizeof=`cat conftestval`  dnl''
],
  [glib_Sizeof=0],
  ifelse([$4], [], [], [glib_Sizeof=$4]))])
AC_DEFINE_UNQUOTED(GLIB_TR_CPP(glib_sizeof_$3), [$[]glib_Sizeof], [Size of $3])
popdef([glib_Sizeof])dnl
])


dnl MC_IF_VAR_EQ(environment-variable, value [, equals-action] [, else-action])
AC_DEFUN([MC_IF_VAR_EQ], [
	case "$[$1]" in
	"[$2]"[)]
		[$3]
		;;
	*[)]
		[$4]
		;;
	esac
])


dnl MC_STR_CONTAINS(src-string, sub-string [, contains-action] [, else-action])
AC_DEFUN([MC_STR_CONTAINS], [
	case "[$1]" in
	*"[$2]"*[)]
		[$3]
		;;
	*[)]
		[$4]
		;;
	esac
])

dnl MC_EVAR_ADD(environment-variable, check-string, add-string)
AC_DEFUN([MC_EVAR_ADD], [
	MC_STR_CONTAINS($[$1], [$2], [$1]="$[$1]", [$1]="$[$1] [$3]")
])
dnl MC_EVAR_SUPPLEMENT(environment-variable, check-string, add-string)
AC_DEFUN([MC_EVAR_SUPPLEMENT], [
	MC_STR_CONTAINS($[$1], [$2], [$1]="$[$1] [$3]", [$1]="$[$1]")
])


dnl MC_CHECK_VERSION() extracts up to 6 decimal numbers out of given-version
dnl and required-version, using any non-number letters as delimiters. it then
dnl compares each of those 6 numbers in order 1..6 to each other, requirering
dnl all of the 6 given-version numbers to be greater than, or at least equal
dnl to the corresponding number of required-version.
dnl MC_CHECK_VERSION(given-version, required-version [, match-action] [, else-action])
AC_DEFUN([MC_CHECK_VERSION], [
[eval `echo "$1:0:0:0:0:0:0" | sed -e 's/^[^0-9]*//' -e 's/[^0-9]\+/:/g' \
 -e 's/\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):\(.*\)/ac_v1=\1 ac_v2=\2 ac_v3=\3 ac_v4=\4 ac_v5=\5 ac_v6=\6/' \
`]
[eval `echo "$2:0:0:0:0:0:0" | sed -e 's/^[^0-9]*//' -e 's/[^0-9]\+/:/g' \
 -e 's/\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):\(.*\)/ac_r1=\1 ac_r2=\2 ac_r3=\3 ac_r4=\4 ac_r5=\5 ac_r6=\6/' \
`]
ac_vm=[`expr \( $ac_v1 \> $ac_r1 \) \| \( \( $ac_v1 \= $ac_r1 \) \& \(		\
	      \( $ac_v2 \> $ac_r2 \) \| \( \( $ac_v2 \= $ac_r2 \) \& \(		\
	       \( $ac_v3 \> $ac_r3 \) \| \( \( $ac_v3 \= $ac_r3 \) \& \(	\
	        \( $ac_v4 \> $ac_r4 \) \| \( \( $ac_v4 \= $ac_r4 \) \& \(	\
	         \( $ac_v5 \> $ac_r5 \) \| \( \( $ac_v5 \= $ac_r5 \) \& \(	\
	          \( $ac_v6 \>= $ac_r6 \)					\
		 \) \)	\
		\) \)	\
	       \) \)	\
	      \) \)	\
	     \) \)	`]
case $ac_vm in
[1)]
	[$3]
	;;
*[)]
	[$4]
	;;
esac
])

dnl MC_ASSERT_NONEMPTY(variable, program, srcpackage)
AC_DEFUN([MC_ASSERT_NONEMPTY], [
    case "x$[$1]"y in
    xy)
	AC_MSG_ERROR([failed to find $2 which is required for a functional build. $3])
	;;
    esac
])

dnl Find program
dnl MC_ASSERT_PROG(variable, program, srcpackage)
AC_DEFUN([MC_ASSERT_PROG], [
    AC_PATH_PROG([$1], [$2], no)
    case "x$[$1]" in
    xno)
	AC_MSG_ERROR([failed to find $2 which is required for a functional build. $3])
	;;
    esac
])
dnl MC_ASSERT_PROGS(variable, programs, srcpackage)
AC_DEFUN([MC_ASSERT_PROGS], [
    AC_PATH_PROGS([$1], [$2], no)
    case "x$[$1]" in
    xno)
	AC_MSG_ERROR([failed to find any of ($2) which is required for a functional build. $3])
	;;
    esac
])

dnl MC_PKG_CONFIG_REQUIRE(package, version, clfgas-var, libs-var)
dnl Find package through $PKG_CONFIG
AC_DEFUN([MC_PKG_CONFIG_REQUIRE], [
    mc_PACKAGE="[$1]"
    mc_VERSION="[$2]"
    AC_MSG_CHECKING([for $mc_PACKAGE - version >= $mc_VERSION])
    if $PKG_CONFIG --atleast-version="$mc_VERSION" $mc_PACKAGE 2>/dev/null ; then
      mc_VERSION=`$PKG_CONFIG --modversion $mc_PACKAGE`
      AC_MSG_RESULT([yes ($mc_VERSION)])
    else
      AC_MSG_RESULT([no])
      AC_MSG_ERROR([pkg-config failed to find "$mc_PACKAGE" v"$mc_VERSION"])
    fi
    [$3]=`$PKG_CONFIG $mc_PACKAGE --cflags`
    [$4]=`$PKG_CONFIG $mc_PACKAGE --libs`
    unset mc_PACKAGE
    unset mc_VERSION
])

dnl Check whether cc accepts a certain option
dnl MC_PROG_CC_SUPPORTS_OPTION(OPTIONS, ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND])
AC_DEFUN([MC_PROG_CC_SUPPORTS_OPTION], [
AC_MSG_CHECKING([whether ${CC-cc} supports $1])
echo >conftest.c;
if ${CC-cc} [$1] -c $CFLAGS conftest.c >/dev/null 2>&1 ; then
    AC_MSG_RESULT(yes)
    [$2]
else
    AC_MSG_RESULT(no)
    [$3]
fi
rm -fr conftest*
])dnl

dnl # Setup CC with default CFLAGS value.
AC_DEFUN([MC_PROG_CC_WITH_CFLAGS], [
	MC_IF_VAR_EQ(CFLAGS, "", CFLAGS="-g")
	CFLAGS_saved="$CFLAGS"
	unset CFLAGS
	dnl Checks for compiler characteristics, CFLAGS.
	AC_PROG_CC
	MC_STR_CONTAINS($CFLAGS, -g, CFLAGS_include_g=yes)
	MC_STR_CONTAINS($CFLAGS, -O, CFLAGS_include_O=yes)
	CFLAGS="$CFLAGS_saved"

	dnl Setup CFLAGS for debugging.
	MC_IF_VAR_EQ(enable_debug, yes,
		MC_IF_VAR_EQ(CFLAGS_include_g, yes,
			MC_EVAR_ADD(CFLAGS, -g, -g)
		)
		dnl Reading assembler Code
		MC_IF_VAR_EQ(GCC, yes,
			dnl MC_EVAR_ADD(CFLAGS, -fvolatile-global, -fvolatile-global)
			dnl MC_EVAR_ADD(CFLAGS, -fverbose-asm, -fverbose-asm)
		)
	)

	dnl Further setup CFLAGS for GCC.
	MC_IF_VAR_EQ(GCC, yes,
		dnl Debugging
		MC_EVAR_SUPPLEMENT(CFLAGS, -g, -ggdb3)
		
		dnl Sane Behaviour
		MC_EVAR_ADD(CFLAGS, -fno-cond-mismatch, -fno-cond-mismatch)

		dnl Warnings.
		MC_EVAR_ADD(CFLAGS, -Wall, -Wall)
		MC_EVAR_ADD(CFLAGS, -Wmissing-prototypes, -Wmissing-prototypes)
		MC_EVAR_ADD(CFLAGS, -Wmissing-declarations, -Wmissing-declarations)
		MC_EVAR_ADD(CFLAGS, -Wno-cast-qual, -Wno-cast-qual)
		dnl MC_EVAR_ADD(CFLAGS, -Winline, -Winline)
		MC_IF_VAR_EQ(enable_pedantic_ansi, yes,
		    MC_EVAR_ADD(CFLAGS, -ansi, -ansi)
		    MC_EVAR_ADD(CFLAGS, -pedantic, -pedantic)
		)
		dnl avoid lots of bogus warnings with string pointers
		MC_PROG_CC_SUPPORTS_OPTION(-Wno-pointer-sign,
		  MC_EVAR_ADD(CFLAGS, -Wno-pointer-sign, -Wno-pointer-sign))
		dnl problematic, triggers warnings in glibc headers
		MC_EVAR_ADD(CFLAGS, -Wpointer-arith, -Wpointer-arith)
		dnl problematic, warns on prototype arguments:
		dnl MC_EVAR_ADD(CFLAGS, -Wshadow, -Wshadow)
		dnl problematic, glibc breakage:
		MC_EVAR_ADD(CFLAGS, -Wredundant-decls, -Wredundant-decls)
		dnl instrument function workarounds
		MC_STR_CONTAINS($CC $CFLAGS, -finstrument-functions,
		                [mc_opt_warn_no_return=-Wno-missing-noreturn],
		                [mc_opt_warn_no_return=-Wmissing-noreturn])
  		MC_PROG_CC_SUPPORTS_OPTION($mc_opt_warn_no_return,
		      MC_EVAR_ADD(CFLAGS, $mc_opt_warn_no_return, $mc_opt_warn_no_return))

		dnl Optimizations
		MC_EVAR_ADD(CFLAGS, -pipe, -pipe)
		MC_EVAR_ADD(CFLAGS, -O, -O2)
		MC_PROG_CC_SUPPORTS_OPTION(-ftracer,
		    MC_EVAR_ADD(CFLAGS, -ftracer, -ftracer))
		MC_EVAR_ADD(CFLAGS, -finline-functions, -finline-functions) dnl -O3 stuff as of gcc-3.3
		MC_PROG_CC_SUPPORTS_OPTION(-fno-keep-static-consts,
		    MC_EVAR_ADD(CFLAGS, -fno-keep-static-consts, -fno-keep-static-consts))
		dnl MC_EVAR_ADD(CFLAGS, -freg-struct-return, -freg-struct-return) dnl buggy with gcc-3.2

		dnl Fun options
		dnl MC_EVAR_ADD(CFLAGS, -Q, -Q)	dnl report each compiled function
		dnl MC_EVAR_ADD(CFLAGS, -ftime-report, -ftime-report)
		dnl MC_EVAR_ADD(CFLAGS, -fmem-report, -fmem-report)
	,	
		MC_IF_VAR_EQ(CFLAGS_include_O, yes,
			MC_EVAR_ADD(CFLAGS, -O, -O2)
		)
	)
])

dnl # MC_PROG_CC_SPECIAL_FLAGS([VARNAME], [FLAG_LIST])
AC_DEFUN([MC_PROG_CC_SPECIAL_FLAGS], [
	for flag in [$2] ; do
	    MC_PROG_CC_SUPPORTS_OPTION($flag, MC_EVAR_ADD([$1], $flag, $flag))
	done
	AC_MSG_CHECKING([[$1]])
	AC_MSG_RESULT($[$1])
])

dnl # Setup CXX with default CXXFLAGS value.
AC_DEFUN([MC_PROG_CXX_WITH_CXXFLAGS], [
	MC_IF_VAR_EQ(CXXFLAGS, "", CXXFLAGS="-g")
	CXXFLAGS_saved="$CXXFLAGS"
	unset CXXFLAGS
	dnl Checks for compiler characteristics, CXXFLAGS.
	AC_PROG_CXX
	MC_STR_CONTAINS($CXXFLAGS, -g, CXXFLAGS_include_g=yes)
	MC_STR_CONTAINS($CXXFLAGS, -O, CXXFLAGS_include_O=yes)
	CXXFLAGS="$CXXFLAGS_saved"

	dnl Setup CXXFLAGS for debugging.
	MC_IF_VAR_EQ(enable_debug, yes,
		MC_IF_VAR_EQ(CXXFLAGS_include_g, yes,
			MC_EVAR_ADD(CXXFLAGS, -g, -g)
		)
		dnl Reading assembler Code
		MC_IF_VAR_EQ(GCC, yes,
			dnl MC_EVAR_ADD(CXXFLAGS, -fvolatile-global, -fvolatile-global)
			dnl MC_EVAR_ADD(CXXFLAGS, -fverbose-asm, -fverbose-asm)
		)
	)

	dnl Further setup CXXFLAGS for GXX.
	MC_IF_VAR_EQ(GXX, yes,
		dnl # enable many useful warnings
		MC_EVAR_ADD(CXXFLAGS, -Wall, -Wall)
		MC_EVAR_ADD(CXXFLAGS, -Wdeprecated, -Wdeprecated)
		MC_EVAR_ADD(CXXFLAGS, -Wno-cast-qual, -Wno-cast-qual)
		dnl # MC_EVAR_ADD(CXXFLAGS, -Wmissing-prototypes, -Wmissing-prototypes)
		dnl # MC_EVAR_ADD(CXXFLAGS, -Winline, -Winline)
		
		dnl # avoid bogus offsetof()-usage warnings
		dnl MC_PROG_CC_SUPPORTS_OPTION(-Wno-invalid-offsetof,
		dnl   MC_EVAR_ADD(CXXFLAGS, -Wno-invalid-offsetof, -Wno-invalid-offsetof))

		dnl Optimizations
		MC_EVAR_ADD(CXXFLAGS, -pipe, -pipe)
		MC_EVAR_ADD(CXXFLAGS, -O, -O2)
		MC_PROG_CC_SUPPORTS_OPTION(-ftracer,
		    MC_EVAR_ADD(CXXFLAGS, -ftracer, -ftracer))
		MC_EVAR_ADD(CXXFLAGS, -finline-functions, -finline-functions) dnl -O3 stuff as of gcc-3.3
		MC_PROG_CC_SUPPORTS_OPTION(-fno-keep-static-consts,
		    MC_EVAR_ADD(CXXFLAGS, -fno-keep-static-consts, -fno-keep-static-consts))
		dnl MC_EVAR_ADD(CXXFLAGS, -freg-struct-return, -freg-struct-return) dnl buggy with gcc-3.2
		dnl -funroll-loops gives problems with -O and templates (see Rep-CppBug_1.C)

		dnl figure current screen width from ncurses to make g++
		dnl format errors for screensizes!=80 correctly
		gxx_columns=0
		AC_CHECK_PROG(TPUT, tput, yes)
		if test "$TPUT" = "yes"; then
		    gxx_columns=`tput cols`
		fi
		if test "$gxx_columns" -gt 1 ; then
		    MC_PROG_CC_SUPPORTS_OPTION(-fmessage-length=$gxx_columns,
			MC_EVAR_ADD(CXXFLAGS, -fmessage-length=, -fmessage-length=$gxx_columns))
		fi
		dnl

	,	
		MC_IF_VAR_EQ(CXXFLAGS_include_O, yes,
			MC_EVAR_ADD(CXXFLAGS, -O, -O2)
		)
	)
])
