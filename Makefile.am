# BEAST/BSE - Bedevilled Audio System / Bedevilled Sound Engine
# Copyright (C) 1998-2003 and Tim Janik
#
## Makefile.am for BEAST/BSE

SUBDIRS = . data sfi bse plugins bsw shell beast-gtk launchers library tools docs po tests test

# require automake 1.4
AUTOMAKE_OPTIONS = 1.4

site site-upgrade site-update site-preview site-update-con site-update-img site-update-nav:
	$(MAKE) -C docs/ $(AM_MAKEFLAGS) $@
test:
	$(MAKE) -C tests/ $(AM_MAKEFLAGS) $@

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
