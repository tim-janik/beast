# BEAST/BSE - Bedevilled Audio System / Bedevilled Sound Engine
# Copyright (C) 1998-2003 and Tim Janik
#
## Makefile.am for BEAST/BSE

SUBDIRS = . data sfi bse plugins bsw shell beast-gtk library docs po po-bse test

# require automake 1.4
AUTOMAKE_OPTIONS = 1.4

site site-upgrade site-update site-preview site-update-con site-update-img site-update-nav:
	$(MAKE) -C docs/ $(AM_MAKEFLAGS) $@

# extra dependancies
configure: acbeast.m4

# build make-paths.h
make-paths.h: $(top_builddir)/config.status Makefile
	cd $(top_builddir) \
	&& echo "/* Generated data from $< (by make $@) */" > xgen-pph \
	&& echo >> xgen-pph \
	&& echo "#define SFIDL_PATH_STDINC        \"${sfidlstdincdir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_BINARIES        \"${bindir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_LADSPA          \"${bseladspapath}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_PLUGINS         \"${bseplugindir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_DEMOS           \"${bsedemodir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_SAMPLES         \"${bsesampledir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_EFFECTS         \"${bseeffectdir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_INSTRUMENTS     \"${bseinstrumentdir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_SCRIPTS         \"${bsescriptdir}\"" >> xgen-pph \
	&& echo "#define BSE_PATH_USER_DATA(dir)  \"${bseuserdatadir}\" dir" >> xgen-pph \
	&& echo "#define BST_PATH_DOCS            \"${beastdocsdir}\"" >> xgen-pph \
	&& echo "#define BST_PATH_IMAGES          \"${beastimagesdir}\"" >> xgen-pph \
	&& echo "#define BST_PATH_LOCALE          \"${beastlocaledir}\"" >> xgen-pph \
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

EXTRA_DIST += $(strip		\
	TODO			\
	COPYING.GPL		\
	COPYING.LGPL		\
	acbeast.m4		\
 	po-helper.sh.in		\
	topconfig.h		\
	intltool-extract.in     \
	intltool-merge.in       \
	intltool-update.in	\
)
