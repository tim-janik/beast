# BEAST - Bedevilled Audio System
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BEAST
 

INCLUDES = -I$(top_srcdir) $(BEAST_CFLAGS) -DGTK_DISABLE_COMPAT_H


#
# programs to compile and install
#
bin_PROGRAMS = beast

# we specify the logging domain here
DEFS += $(strip \
	-DG_LOG_DOMAIN=\"BEAST\" \
)

# installation directory for BEAST program data
beast_pdatadir = $(beastpdatadir)

# link programs against BSE and other neccessary libraries
progs_LDADD = $(top_builddir)/bse/libbse.la $(LDFLAGS) $(BEAST_LIBS)
beast_LDADD = $(progs_LDADD)


#
# setup source file variables
#
# BEAST header files that don't get installed
beast_headers = $(strip \
	bstapp.h \
	bstdefs.h \
	bstfiledialog.h \
	bstinstrumentview.h \
	bstitemview.h \
	bstkeytables.h \
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
	bstxkb.h \
)
# BEAST c sources to build the program from
beast_sources = $(strip \
	bstapp.c \
	bstfiledialog.c \
	bstinstrumentview.c \
	bstitemview.c \
	bstkeytables.c \
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
	bstxkb.c \
)
# BEAST c sources that get included (don't have own .lo rules)
beast_extra_sources = $(strip \
	bstkeytable-base.c \
	bstkeytable-us.c \
	bstkeytable-de.c \
)
# BEAST program data files
beast_pdata_DATA = $(strip \
	beast.glr \
)
# BEAST built sources that don't get installed
beast_built_sources = $(strip \
	bstconfigpaths.h \
)

#
# setup BEAST sources and their dependancies
#
beast_SOURCES = $(beast_sources)
EXTRA_DIST += $(beast_headers) $(beast_extra_sources)
MAINTAINERCLEANFILES += $(beast_built_sources)
EXTRA_DIST += $(beast_pdata_DATA) $(beast_built_sources)

#
# rules to generate built sources
#
gen_source = gen-source.tmp
CLEANFILES += $(gen_source)
COPYING: $(beast_built_sources)
$(OBJECTS): COPYING # this is our oldest-source-stamp
bstconfigpaths.h: $(top_srcdir)/configure Makefile
	cd $(srcdir) \
	&& echo "/* Generated data from $< (by make $@) */" > $(gen_source) \
	&& echo >> $(gen_source) \
	&& echo "#define BST_DATA_DIR \"${beastdatadir}\"" >> $(gen_source) \
	&& echo "#define BST_PDATA_DIR \"${beastpdatadir}\"" >> $(gen_source) \
	&& echo >> $(gen_source) \
	&& echo "/* Generated data ends here */" >> $(gen_source) \
	&& mv $(gen_source) $@


#
# private (birnet) dependancy to make TAGS file after each build
#
all: @MISSING_ETAGS@ @BIRNET_Y@ TAGS


#
# auxillary files
#
EXTRA_DIST += \
	TODO
