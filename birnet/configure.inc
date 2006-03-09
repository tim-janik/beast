dnl # Birnet                                       -*- Mode: shell-script; -*-
dnl # Copyright (C) 2006 Tim Janik
dnl #
dnl # GNU Lesser General Public License version 2 or any later version.

AC_DEFUN([AC_BIRNET_REQUIREMENTS],
[
    BIRNET_CFLAGS=""
    BIRNET_LIBS=""

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

    dnl # --- check for GLib --- 
    DEPENDANCIES="glib-2.0 >= 2.4.11 gthread-2.0"
    dnl # define GLIB_CFLAGS and GLIB_LIBS:
    PKG_CHECK_MODULES(BIRNET_GLIB, $DEPENDANCIES,,
	AC_MSG_ERROR([[Failed to detect or find $DEPENDANCIES (consider adjusting \$PKG_CONFIG_PATH)]]))
    BIRNET_CFLAGS="$BIRNET_GLIB_CFLAGS"
    BIRNET_LIBS="$BIRNET_GLIB_LIBS"

    dnl # --- BIRNET_CFLAGS & BIRNET_LIBS ---
    AC_SUBST(BIRNET_CFLAGS)
    AC_SUBST(BIRNET_LIBS)
])
