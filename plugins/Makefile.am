# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 1998-2002 Tim Janik
#
## Makefile.am for BSE Plugins

SUBDIRS = icons . freeverb

CFLAGS   += $(EXTRA_CFLAGS)
INCLUDES += -I$(top_srcdir) -I. $(BSE_CFLAGS) -DG_LOG_DOMAIN=BSE_PLUGIN_NAME
INCLUDES += -DBSE_PLUGIN_FALLBACK="\"$(strip $(basename $(<F)))\""
GLIB_MKENUMS = glib-mkenums

# install plugins under:
plugindir = $(bseplugindir)

# all plugins want to link against:
plugin_libs = $(BSE_LIBS) -lm
# plugin_ldlibs = $(top_builddir)/bse/libbse.la $(top_builddir)/blib/libblib.la

# create plugin list
UNPORTED = $(strip		\
	bsefirfilter.[hc]	\
	bseloader-wave.[hc]	\
	davbassfilter.[hc]	\
	davchorus.[hc]		\
	davguitar.[hc]		\
)
PLUGIN_CFILES = $(strip		\
	bseadder.[hc]		\
	bseatandistort.[hc]	\
	bseiirfilter.[hc]	\
	bsemixer.[hc]		\
	bsemult.[hc]		\
	bsenoise.[hc]		\
	bsesimpleadsr.[hc]	\
	bsesequencer.[hc]	\
	davorgan.[hc]		\
	davxtalstrings.[hc]	\
	davsyndrum.[hc]		\
	davcanyondelay.[hc]	\
)
EXTRA_CFILES = $(strip		\
	bsesimpleadsr-aux.c	\
)

# *.enums generation rule
%.enums: %.h
	cd $(srcdir) \
	&& $(GLIB_MKENUMS) \
	  --fhead "\n#include\t<bse/bsetype.h>\n" \
	  --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"\n" \
	  --eprod "/* @EnumName@\n */\n" \
	  --eprod "#define BSE_TYPE_@ENUMSHORT@\t    (BSE_TYPE_ID (@EnumName@))\n" \
	  --vhead "static G@Type@Value @enum_name@_values[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	  --vtail "  { 0, NULL, NULL }\n};\n" \
	  --vtail "static GType BSE_TYPE_ID (@EnumName@);" \
	    $< > xgen-enums-$(@F) \
	&& $(GLIB_MKENUMS) \
	  --fhead "\n#include\t<bse/bseexports.h>\n" \
	  --fhead "\nBSE_EXPORT_STATIC_ENUMS = {" \
	  --eprod "  { &BSE_TYPE_ID (@EnumName@), \"@EnumName@\",\n" \
	  --eprod "    G_TYPE_@TYPE@, @enum_name@_values }," \
	  --ftail "  { NULL, },\n};" \
	    $< >> xgen-enums-$(@F) \
	&& cp xgen-enums-$(@F) $(@F) \
	&& rm -f xgen-enums-$(@F)
CLEANFILES += $(plugins_built_sources) xgen-enums-*

#
# C++ Plugins
#
plugin_idl_files = $(strip	\
	davchorus.idl		\
)
# setup generation of C++ plugins from idl files
BSEPGEN = ../sfi/bse-plugin-generator
%.gen-idl.h: %.idl $(BSEPGEN)
	$(BSEPGEN) --nostdinc -I$(top_builddir) $< > $@ || (rm $@ ; exit 1 )

plugins_built_sources =
Makefile.plugins: $(srcdir)/listplugins.sh $(PLUGIN_CFILES) Makefile.am
	$(srcdir)/listplugins.sh $(PLUGIN_CFILES) >$@
	$(srcdir)/listplugins.sh --idl $(plugin_idl_files:.idl=) >>$@

## include generated plugin Makefile
include Makefile.plugins
## Makefile.plugins contains a generated make rule list of the *.c
## files in this directory, e.g. it'd list a given foo.c as:
##
##foo.enums: foo.h
##plugins_built_sources += foo.enums
##foo_la_SOURCES = foo.c
##foo_la_LDFLAGS = -module -avoid-version
##foo_la_LIBADD  = $(plugin_libs)
##EXTRA_HEADERS += foo.h
##
##plugin_LTLIBRARIES += foo.la

EXTRA_DIST += $(strip \
        $(EXTRA_HEADERS) \
        $(EXTRA_CFILES) \
        $(plugins_built_sources) \
	listplugins.sh \
)

