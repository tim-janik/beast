# launchers - program starters
# Copyright (C) 2004 Tim Janik
#

INCLUDES = -I$(top_srcdir) -I$(top_builddir) -I. $(LAUNCHER_CFLAGS) -DG_DISABLE_CONST_RETURNS

DEFS += $(strip                         		\
        -DBINDIR=\"$(bindir)\" 				\
)

#
# programs to build
#
bin_PROGRAMS = beast bsesh
beast_SOURCES = suidmain.c beaststart.c
bsesh_SOURCES = suidmain.c bseshstart.c
EXTRA_DIST += suidmain.h

#
# make bsesh wrapper suid to allow renicing
#
install-exec-hook:	# runs after install-binPROGRAMS
	true \
	&& ( chown root $(DESTDIR)$(bindir)/beast $(DESTDIR)$(bindir)/bsesh   \
	&&   chmod 4755 $(DESTDIR)$(bindir)/beast $(DESTDIR)$(bindir)/bsesh ) \
	|| ( echo "*** WARNING *** beast and bsesh need to be installed as root to allow renicing ***" && sleep 5 )
