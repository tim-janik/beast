# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
#
## Makefile.am for BSE Plugins

SUBDIRS = icons

CFLAGS   += -funroll-loops $(EXTRA_CFLAGS)
INCLUDES += -I$(top_srcdir) $(BSE_CFLAGS) -DG_LOG_DOMAIN=BSE_PLUGIN_NAME
INCLUDES += -DBSE_PLUGIN_FALLBACK="\"$(strip $(basename $(<F)))\""
GLIB_MKENUMS = glib-mkenums

# install plugins under:
plugindir = $(bseplugindir)

# all plugins want to link against:
plugin_libs = $(BSE_LIBS) -lm
# plugin_ldlibs = $(top_builddir)/bse/libbse.la $(top_builddir)/blib/libblib.la

# create plugin list
UNPORTED = @STRIP_BEGIN@	\
	bsefirfilter.[hc]	\
	bseloader-wave.[hc]	\
	davbassfilter.[hc]	\
	davcanyondelay.[hc]	\
	davchorus.[hc]		\
	davguitar.[hc]		\
	davorgan.[hc]		\
	davsyndrum.[hc]		\
@STRIP_END@
PLUGIN_CFILES = @STRIP_BEGIN@	\
	bseadder.[hc]		\
	bsecompressor.[hc]	\
	bseconstant.[hc]	\
	bsegenosc.[hc]		\
	bsemixer.[hc]		\
	bsemult.[hc]		\
	bsenoise.[hc]		\
	bsesimpleadsr.[hc]	\
	bsesimpleosc.[hc]	\
	bsesnooper.[hc]		\
	bsewaveosc.[hc]		\
	davxtalstrings.[hc]	\
	fldbsepattern.[hc]	\
@STRIP_END@
EXTRA_CFILES = @STRIP_BEGIN@	\
	bsesimpleadsr-aux.c	\
	bsewaveosc-aux.c	\
@STRIP_END@
Makefile.plugins: $(srcdir)/listplugins.sh $(PLUGIN_CFILES) Makefile.am
	$(srcdir)/listplugins.sh $(PLUGIN_CFILES) >$@

EXTRA_DIST += $(strip \
        $(EXTRA_HEADERS) \
        $(EXTRA_CFILES) \
        $(plugins_built_sources) \
	listplugins.sh \
	TODO \
)

# rules to auto-build built sources, we need an
# oldest-source-stamp for this that is never touched
COPYING: $(plugins_built_sources)
$(OBJECTS): COPYING # this is our oldest-source-stamp

# *.enums generation rule
missing_perl = @MISSING_PERL5@ yes

if IF_HAVE_PERL5
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
	    $< > xgen-enums \
	&& $(GLIB_MKENUMS) \
	  --fhead "\n#include\t<bse/bseexports.h>\n" \
	  --fhead "\nBSE_EXPORT_STATIC_ENUMS = {" \
	  --eprod "  { &BSE_TYPE_ID (@EnumName@), \"@EnumName@\",\n" \
	  --eprod "    G_TYPE_@TYPE@, @enum_name@_values }," \
	  --ftail "  { NULL, },\n};" \
	    $< >> xgen-enums \
	&& cp xgen-enums $(@F) \
	&& rm -f xgen-enums
endif

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
