# BEAST/BSE - Bedevilled Audio System / Bedevilled Sound Engine
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BEAST/BSE

# subdirs with source code to be compiled
SRC_SUBDIRS = sfi bse plugins bsw shell beast-gtk po po-bse

# other subdirs
SUBDIRS = . $(SRC_SUBDIRS) docs test

site site-upgrade site-update site-preview site-update-con site-update-img site-update-nav:
	$(MAKE) -C docs/ $(AM_MAKEFLAGS) $@

# require automake 1.4
AUTOMAKE_OPTIONS = 1.4

# extra dependancies
configure: acbeast.m4

# build make-paths.h
make-paths.h: $(top_builddir)/config.status Makefile
	cd $(top_builddir) \
	&& echo "/* Generated data from $< (by make $@) */" > xgen-pph \
	&& echo >> xgen-pph \
	&& echo "#define SFIDL_PATH_STDINC     \"${sfidlstdincdir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_PLUGINS      \"${bseplugindir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_LADSPA       \"${bseladspadir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_SCRIPTS      \"${bsescriptdir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_BINARIES     \"${bindir}\"" >> xgen-pph \
	&& echo "#define BST_PATH_DOCS         \"${beastdocsdir}\"" >> xgen-pph \
	&& echo "#define BST_PATH_IMAGES       \"${beastimagesdir}\"" >> xgen-pph \
	&& echo "#define BST_PATH_DATA_SAMPLES \"${beastsampledatadir}\"" >> xgen-pph \
	&& echo "#define BST_PATH_LOCALE       \"${beastlocaledir}\"" >> xgen-pph \
	&& echo >> xgen-pph \
	&& echo "/* Generated data ends here */" >> xgen-pph \
	&& (cp xgen-pph $@) \
	&& rm -f xgen-pph

#	&& (cmp -s xgen-pph $@ || cp xgen-pph $@) 

# always built make-paths.h and create links
stamp-h: make-paths.h

CLEANFILES += $(strip		\
	make-paths.h		\
	intltool-extract	\
	intltool-merge		\
	intltool-update		\
	)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = sfi.pc bse.pc
$(pkgconfig_DATA): config.status

EXTRA_DIST += $(strip		\
	TODO			\
	COPYING.GPL		\
	COPYING.LGPL		\
	acbeast.m4		\
	sfi.pc.in		\
	bse.pc.in		\
	topconfig.h		\
	intltool-extract.in     \
	intltool-merge.in       \
	intltool-update.in	\
)
