# BEAST - Bedevilled Audio System
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
# Makefile.am for BEAST
 
INCLUDES = -I$(top_srcdir) $(BEAST_CFLAGS) -DGTK_DISABLE_COMPAT_H \
	-DBEASTDATADIR=\"@beastdatadir@\"

bin_PROGRAMS = beast

progs_LDADD = $(top_builddir)/bse/libbse.la $(LDFLAGS) $(BEAST_LIBS)

beast_LDADD = $(progs_LDADD)

# *.h files
beast_headers = $(strip \
	bstapp.h \
	bstdefs.h \
	bstfiledialog.h \
	bstinstrumentview.h \
	bstitemview.h \
	bstmaster.h \
	bstmenus.h \
	bstparam.h \
	bstparamview.h \
	bstpatterneditor.h \
	bstpatterndialog.h \
	bstpatternview.h \
	bstprocedure.h \
	bstsamplerepo.h \
	bstsongshell.h \
	bstsongrouter.h \
	bststatusbar.h \
	bstsupershell.h \
)
# *.c files to be compiled
beast_sources = $(strip \
	bstapp.c \
	bstfiledialog.c \
	bstinstrumentview.c \
	bstitemview.c \
	bstmain.c \
	bstmaster.c \
	bstmenus.c \
	bstparam.c \
	bstparamview.c \
	bstpatterneditor.c \
	bstpatterndialog.c \
	bstpatternview.c \
	bstprocedure.c \
	bstsamplerepo.c \
	bstsongshell.c \
	bstsongrouter.c \
	bststatusbar.c \
	bstsupershell.c \
)
# included *.c files, xpms etc...
beast_extra = \
	bstkeytables.c \
	beast.glr

beast_SOURCES = $(beast_sources)

EXTRA_DIST += TODO $(beast_headers) $(beast_extra)

# dependancy for TAGS file from etags
all: @MISSING_ETAGS@ @BIRNET_Y@ TAGS

old_sources = \
	bstdialogs.c \
	bstinstrumenteditor.c \
	bstmenus.c \
	bstmessagebox.c \
	bstpatternview.c \
	bstrc.c \
	bstrcargs.c \
	bstshell.c \
	beast.c
