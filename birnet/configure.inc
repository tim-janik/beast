dnl # Birnet                                       -*- Mode: shell-script; -*-
dnl # Copyright (C) 2006 Tim Janik
dnl #
dnl # GNU Lesser General Public License version 2 or any later version.

AC_DEFUN([AC_BIRNET_REQUIREMENTS],
[
    dnl # BIRNET versioning
    AC_DIVERT_BEFORE_HELP([
    BIRNET_MAJOR_VERSION=0
    BIRNET_MINOR_VERSION=0
    BIRNET_MICRO_VERSION=1
    BIRNET_INTERFACE_AGE=0
    BIRNET_BINARY_AGE=0
    BIRNET_VERSION="$BIRNET_MAJOR_VERSION.$BIRNET_MINOR_VERSION.$BIRNET_MICRO_VERSION"
    ])
    AC_SUBST(BIRNET_MAJOR_VERSION)
    AC_SUBST(BIRNET_MINOR_VERSION)
    AC_SUBST(BIRNET_MICRO_VERSION)
    AC_SUBST(BIRNET_INTERFACE_AGE)
    AC_SUBST(BIRNET_BINARY_AGE)
    AC_SUBST(BIRNET_VERSION)

    BIRNET_CFLAGS=""
    BIRNET_LIBS=""

    dnl # --- sizeof and presence of uint ---
    GLIB_SIZEOF([#include <sys/types.h>], uint, sys_typesh_uint)
    BIRNET_SIZEOF_SYS_TYPESH_UINT="$glib_cv_sizeof_sys_typesh_uint"
    AC_SUBST(BIRNET_SIZEOF_SYS_TYPESH_UINT) dnl # for birnetconfig.h

    dnl # --- OS/Win32 detection ---
    dnl # needs AC_CANONICAL_HOST
    test -z "$host" && {
	missing_macro="AC""_CANONICAL_HOST"
	AC_MSG_ERROR([configure failed to execute $missing_macro])
    }
    AC_MSG_CHECKING([for Win32])
    case "$host" in
	*-*-mingw*)
	    BIRNET_OS=WIN32
	    birnet_mingw=yes ;;
	*)
	    BIRNET_OS=UNIX
	    birnet_mingw=no ;;
    esac
    AC_MSG_RESULT([$birnet_mingw])
    AC_SUBST(BIRNET_OS)
    AC_DEFINE_UNQUOTED(BIRNET_OS_$BIRNET_OS, "1", [Win32 detection])

    dnl # --- sizeof threading structs ---
    GLIB_SIZEOF([#include <pthread.h>], pthread_mutex_t, pth_mutex_t)
    BIRNET_SIZEOF_PTH_MUTEX_T="$glib_cv_sizeof_pth_mutex_t"
    AC_SUBST(BIRNET_SIZEOF_PTH_MUTEX_T)
    GLIB_SIZEOF([#include <pthread.h>], pthread_cond_t, pth_cond_t)
    BIRNET_SIZEOF_PTH_COND_T="$glib_cv_sizeof_pth_cond_t"
    AC_SUBST(BIRNET_SIZEOF_PTH_COND_T)

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
      BIRNET_HAVE_MUTEXATTR_SETTYPE=1
      AC_MSG_RESULT(yes)
    ],[
      BIRNET_HAVE_MUTEXATTR_SETTYPE=0
      AC_MSG_RESULT(no)
    ])
    AC_SUBST(BIRNET_HAVE_MUTEXATTR_SETTYPE)

    dnl # --- require libz ---
    if test -z "$LIBZ"; then
        AC_CHECK_LIB(z, gzsetparams,
            [AC_CHECK_HEADER(zlib.h, LIBZ='-lz', LIBZ='')],
            LIBZ='')
    fi
    if test -z "$LIBZ"; then
        AC_MSG_ERROR([Compression library libz is missing, but required])
    fi
    AC_SUBST(LIBZ)

    dnl # --- check for GLib --- 
    DEPENDANCIES="glib-2.0 >= 2.6.4 gthread-2.0"
    dnl # define BIRNET_GLIB_CFLAGS and BIRNET_GLIB_LIBS:
    PKG_CHECK_MODULES(BIRNET_GLIB, $DEPENDANCIES,,
	AC_MSG_ERROR([[Failed to detect or find $DEPENDANCIES (consider adjusting \$PKG_CONFIG_PATH)]]))
    dnl # libtool doesn't automatically figure that -pthread implies -lpthread
    BIRNET_LIBS=`echo " $BIRNET_GLIB_LIBS" | sed 's/\(-pthread\b\)/-lpthread \1/' `
    BIRNET_LIBS="$BIRNET_LIBS $LIBZ"
    BIRNET_CFLAGS="$BIRNET_GLIB_CFLAGS"

    dnl # --- BIRNET_CFLAGS & BIRNET_LIBS ---
    AC_SUBST(BIRNET_CFLAGS)
    AC_SUBST(BIRNET_LIBS)
])
