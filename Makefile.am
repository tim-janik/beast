# BEAST/BSE - Bedevilled Audio System / Bedevilled Sound Engine
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BEAST/BSE

# subdirs with source code to be compiled
SRC_SUBDIRS = bse plugins bsw bsw-shell beast-gtk

# other subdirs
SUBDIRS = $(SRC_SUBDIRS) docs test

# require automake 1.4
AUTOMAKE_OPTIONS = 1.4

# extra dependancies
configure: acbeast.m4

# build PKG_paths.h
PKG_paths.h: $(top_srcdir)/configure Makefile
	cd $(top_builddir) \
	&& echo "/* Generated data from $< (by make $@) */" > xgen-pph \
	&& echo >> xgen-pph \
	&& echo "#define BSE_PATH_PLUGINS      \"${bseplugindir}\"" >> xgen-pph \
	&& echo "#define BSW_PATH_SCRIPTS      \"${bswscriptdir}\"" >> xgen-pph \
	&& echo "#define BSW_PATH_BINARIES     \"${bindir}\"" >> xgen-pph \
	&& echo "#define BST_PATH_DOCS         \"${beastdocsdir}\"" >> xgen-pph \
	&& echo "#define BST_PATH_IMAGES       \"${beastimagesdir}\"" >> xgen-pph \
	&& echo "#define BST_PATH_DATA_SAMPLES \"${beastsampledatadir}\"" >> xgen-pph \
	&& echo >> xgen-pph \
	&& echo "/* Generated data ends here */" >> xgen-pph \
	&& (cp xgen-pph $@) \
	&& rm -f xgen-pph

#	&& (cmp -s xgen-pph $@ || cp xgen-pph $@) 

# always built PKG_paths.h and create gsl link
stamp-h: PKG_paths.h gsl

# create builddir gsl link
gsl:
	ln -s bse gsl
# dist ship the srcdir gsl link
EXTRA_DIST += gsl

MAINTAINERCLEANFILES += PKG_paths.h gsl

EXTRA_DIST += \
	PKG_paths.h \
	acbeast.m4
