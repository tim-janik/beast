# launchers - program starters
# Copyright (C) 2004 Tim Janik
#

INCLUDES = -I$(top_srcdir) -I$(top_builddir) -I. $(LAUNCHER_CFLAGS) -DG_DISABLE_CONST_RETURNS

#
# programs to build
#
#bin_PROGRAMS = beast bsesh
#beast_SOURCES = suidmain.c
#bsesh_SOURCES = suidmain.c
# SUIDMAIN_EXECUTABLE=\"$(bindir)/@BEAST_BINARY@\"
# SUIDMAIN_EXECUTABLE=\"$(bindir)/@BSESH_BINARY@\"

#
# make bsesh wrapper suid to allow renicing
#
install-exec-local: # install-binPROGRAMS
	true \
	&& ( chown root $(bindir)/beast $(bindir)/bsesh   \
	&&   chmod 4755 $(bindir)/beast $(bindir)/bsesh ) \
	|| ( echo "*** WARNING *** beast and bsesh need to be installed as root to allow renicing ***" && sleep 5 )

