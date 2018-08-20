# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I.
DEFS        += -DBINDIR=\"$(beastbindir)\"
AM_CXXFLAGS += $(LAUNCHER_CFLAGS) -DG_DISABLE_CONST_RETURNS

#
# programs to build
#
beastbin_PROGRAMS = beast
beast_SOURCES        = suidmain.c beaststart.c
EXTRA_HEADERS       += suidmain.h

#
# make beast wrapper suid to allow renicing
#
install-exec-hook:	# runs after install-binPROGRAMS
	true \
	&& ( chown root $(DESTDIR)$(beastbindir)/beast   \
	&&   chmod 4755 $(DESTDIR)$(beastbindir)/beast ) \
	|| ( echo -e "***\n*** WARNING: $(DESTDIR)$(beastbindir)/beast needs to be installed as root to allow renicing \\_(o.o)_/\n***" && sleep 1 )
