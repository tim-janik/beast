# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BEAST icons

icon_pattern = *.c *.xcf
icon_list = 

# create icon list
Makefile.icons: $(icon_pattern) Makefile.am
	cd $(srcdir) \
	&& echo "## Generated data (by make $@)" > $@ \
	&& for f in $(icon_pattern) ; do \
	    echo "icon_list += $$f" >>$@ ; \
	done \
	&& echo "## Generated data ends here" >> $@

EXTRA_DIST += $(strip \
	$(icon_list) \
)

## include generated icon list Makefile
include Makefile.icons

## Makefile.plugins contains a generated make rule list of the *.c
## files in this directory, e.g. it'd list a given foo.c as:
##
##foo_la_SOURCES = foo.c
##foo_la_LDFLAGS = -module -avoid-version
##foo_la_LIBADD  = $(plugin_libs)
##
##plugin_LTLIBRARIES += foo.la
