# BEAST/BSE - Bedevilled Audio System / Bedevilled Sound Engine
# Copyright (C) 1998-2005 Tim Janik
#
## Makefile.am for BEAST/BSE

SUBDIRS = . data sfi bse plugins shell beast-gtk launchers library tools po tests test doxer docs web

# require automake 1.4
AUTOMAKE_OPTIONS = 1.4

site site-preview site-update site-update-all site-update-pages site-update-galleries:
	@$(MAKE) -C web/ $(AM_MAKEFLAGS) $@

test:
	@$(MAKE) -C tests/ $(AM_MAKEFLAGS) $@

.PHONY: test
        
# extra dependancies
configure: acbeast.m4

CLEANFILES += $(strip		\
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
