# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BSE Plugins

SUBDIRS = icons

INCLUDES += -I$(top_srcdir) $(BSE_CFLAGS) -DG_LOG_DOMAIN=BSE_PLUGIN_NAME
INCLUDES += -DBSE_PLUGIN_FALLBACK="\"$(strip $(basename $(<F)))\""
mkenums = $(top_srcdir)/bse/mkenums.pl

# install plugins under:
plugindir = $(bseplugindir)

# all plugins want to link against:
plugin_libs = $(BSE_LIBS) -lm

# create plugin list
Makefile.plugins: $(srcdir)/listplugins.sh *.c Makefile.am
	$(srcdir)/listplugins.sh *.c >$@

EXTRA_DIST += $(strip \
        $(EXTRA_HEADERS) \
        $(plugins_built_sources) \
	listplugins.sh \
	TODO \
)

# rules to auto-build built sources, we need an
# oldest-source-stamp for this that is never touched
COPYING: $(plugins_built_sources)
$(OBJECTS): COPYING # this is our oldest-source-stamp

# *.enums generation rule
%.enums: %.h
	cd $(srcdir) \
	&& $(PERL) $(mkenums) \
	  --hprod "\n#include\t<bse/bsetype.h>\n" \
	  --iprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"\n" \
	  --eprod "/* @EnumName@\n */\n" \
	  --eprod "#define BSE_TYPE_@ENUMSHORT@\t    (BSE_TYPE_ID (@EnumName@))\n" \
	  --vhead "static Bse@Type@Value @enum_name@_values[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	  --vtail "  { 0, NULL, NULL }\n};\n" \
	  --vtail "static BseType BSE_TYPE_ID (@EnumName@);" \
	    $< > xgen-enums \
	&& $(PERL) $(mkenums) \
	  --hprod "\n#include\t<bse/bseexports.h>\n" \
	  --hprod "\nBSE_EXPORT_STATIC_ENUMS = {" \
	  --eprod "  { &BSE_TYPE_ID (@EnumName@), \"@EnumName@\",\n" \
	  --eprod "    BSE_TYPE_@TYPE@, @enum_name@_values }," \
	  --tprod "  { NULL, },\n};" \
	    $< >> xgen-enums \
	&& cp xgen-enums $(@F) \
	&& rm -f xgen-enums


## include generated plugin Makefile
include Makefile.plugins
## Makefile.plugins contains a generated make rule list of the *.c
## files in this directory, e.g. it'd list a given foo.c as:
##
##foo.enums: foo.h $(mkenums)
##plugins_built_sources += foo.enums
##foo_la_SOURCES = foo.c
##foo_la_LDFLAGS = -module -avoid-version
##foo_la_LIBADD  = $(plugin_libs)
##EXTRA_HEADERS += foo.h
##
##plugin_LTLIBRARIES += foo.la
