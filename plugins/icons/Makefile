# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#



SHELL = /bin/sh

srcdir = .
top_srcdir = ../..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/beast
pkglibdir = $(libdir)/beast
pkgincludedir = $(includedir)/beast

top_builddir = ../..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = i586-pc-linux-gnu
host_triplet = i586-pc-linux-gnu
AS = @AS@
AWK = mawk
BEAST_CFLAGS = -I/usr/local/include -DNEED_GNOMESUPPORT_H -I/usr/local/lib/gnome-libs/include -I/usr/X11R6/include -I/usr/local/lib/glib/include
BEAST_LIBS = -lgle -rdynamic -L/usr/local/lib -L/usr/X11R6/lib -lgnomeui -lart_lgpl -lgdk_imlib -lSM -lICE -lgtk -lgdk -lgmodule -lXext -lX11 -lgnome -lgnomesupport -lesd -lasound -laudiofile -lm -ldb -lglib -ldl
BIRNET_N = #
BIRNET_Y = 
BSE_BINARY_AGE = 0
BSE_CFLAGS = -I/usr/local/lib/glib/include -I/usr/local/include
BSE_INTERFACE_AGE = 0
BSE_LIBS = -L/usr/local/lib -rdynamic -lgmodule -lglib -ldl
BSE_MAJOR_VERSION = 0
BSE_MICRO_VERSION = 0
BSE_MINOR_VERSION = 2
BSE_VERSION = 0.2.0
CC = gcc
CFLAGS = -DG_ENABLE_DEBUG -g -ggdb3 -fno-strict-aliasing -fno-cond-mismatch -ffor-scope -Wno-cast-qual -Wall -Wmissing-prototypes -Winline -Wmissing-noreturn -O6 -pipe -fstrength-reduce -fexpensive-optimizations -finline-functions -frerun-cse-after-loop -freg-struct-return -funroll-loops -frerun-loop-opt -fgcse -fno-keep-static-consts
CPP = gcc -E
CPPFLAGS = 
CXXCPP = c++ -E
DLLTOOL = @DLLTOOL@
ETAGS = etags
GLE_LIBS = 
GLIB_CFLAGS = -I/usr/local/lib/glib/include -I/usr/local/include
GLIB_CONFIG = /usr/local/bin/glib-config
GLIB_LIBS = -L/usr/local/lib -rdynamic -lgmodule -lglib -ldl
GTK_CFLAGS = -I/usr/X11R6/include -I/usr/local/lib/glib/include -I/usr/local/include
GTK_CONFIG = /usr/local/bin/gtk-config
GTK_LIBS = -L/usr/local/lib -L/usr/X11R6/lib -lgtk -lgdk -rdynamic -lgmodule -lglib -ldl -lXext -lX11 -lm
INDENT = indent
LD = /usr/bin/ld
LDFLAGS = 
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LN_S = ln -s
LT_AGE = 0
LT_CURRENT = 0
LT_RELEASE = 0.2
LT_REVISION = 0
MAINT = 
MAKEINFO = makeinfo
MISSING_AWK = 
MISSING_ETAGS = 
MISSING_INDENT = 
MISSING_PERL5 = 
NM = /usr/bin/nm -B
OBJDUMP = @OBJDUMP@
PACKAGE = beast
PERL = perl
RANLIB = ranlib
STRIP_BEGIN = $(strip
STRIP_END = )
VERSION = 0.2.0
beastdatadir = ${prefix}/share/beast/v0.2
beastpdatadir = ${prefix}/share/beast/v0.2.0
bsealsadevices = "/dev/snd/pcmC0D0", "/dev/snd/pcmC0D1"
bsedatadir = ${prefix}/share/bse
bselibdir = ${exec_prefix}/lib/bse/v0.2
bseossdevice = "/dev/dsp"

icon_pattern = *.c *.xcf
icon_list =  amp.c any.c mic.c sine.c speaker.c testos.c amp.xcf mic.xcf sine.xcf speaker.xcf

EXTRA_DIST =  $(strip 	$(icon_list) 	TODO )

mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../../BEASTconfig.h
CONFIG_CLEAN_FILES = 
DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP_ENV = --best
all: all-redirect
.SUFFIXES:
$(srcdir)/Makefile.in:  Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) $(srcdir)/Makefile.icons
	cd $(top_srcdir) && $(AUTOMAKE) --gnu plugins/icons/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

tags: TAGS
TAGS:


distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = plugins/icons

distdir: $(DISTFILES)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(top_distdir) && pwd`; \
	distdir=`cd $(distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --gnu plugins/icons/Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done
info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am:
install-exec: install-exec-am

install-data-am:
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am:
uninstall: uninstall-am
all-am: Makefile
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs:


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-generic mostlyclean-am

clean: clean-am

distclean-am:  distclean-generic clean-am
	-rm -f libtool

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: tags distdir info-am info dvi-am dvi check check-am \
installcheck-am installcheck install-exec-am install-exec \
install-data-am install-data install-am install uninstall-am uninstall \
all-redirect all-am all installdirs mostlyclean-generic \
distclean-generic clean-generic maintainer-clean-generic clean \
mostlyclean distclean maintainer-clean


# create icon list
Makefile.icons: $(icon_pattern) Makefile.am
	cd $(srcdir) \
	&& echo "## Generated data (by make $@)" > $@ \
	&& for f in $(icon_pattern) ; do \
	    echo "icon_list += $$f" >>$@ ; \
	done \
	&& echo "## Generated data ends here" >> $@

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
