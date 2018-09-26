# BEAST & BSE
include $(top_srcdir)/Makefile.decl

#
# Building of this plugin has been completely disabled because it never
# left pre-prototype stage, and it contains mallicious code that gets
# into the way of normal program operations (such as printf() statements
# that spew into the stdout channel).
#


## Evaluator Makefile
#
#AM_CPPFLAGS += -I$(top_srcdir) -I. $(BSE_CFLAGS) -DG_LOG_DOMAIN=BSE_PLUGIN_NAME
#AM_CPPFLAGS += -DBSE_PLUGIN_FALLBACK="\"$(strip $(basename $(<F)))\""
#
## install plugins under:
#plugindir = $(pkginstalldir)/plugins
#
## setup generation of C++ plugins from idl files
#SFIDL = ../../sfi/sfidl
#SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)
#%.genidl.hh: %.idl $(SFIDL)
#	$(SFIDL) --plugin $(SFIDL_INC) -I$(top_builddir) $< > $@ || (rm $@ ; exit 1 )
#CLEANFILES += $(plugins_built_sources)
#
#EVALUATE_SOURCES = instruction.cc compiler.cc cpu.cc
#
#bseevaluator_la_SOURCES = bseevaluator.cc $(EVALUATE_SOURCES)
#bseevaluator_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
#bseevaluator_la_LIBADD  = $(top_builddir)/bse/libbse-@MAJOR@.la $(BSE_LIBS) -lm
#plugins_built_sources += bseevaluator.genidl.hh
#EXTRA_HEADERS += bseevaluator.idl symbols.hh token.hh instruction.hh compiler.hh cpu.hh
#$(bseevaluator_la_OBJECTS): bseevaluator.genidl.hh
#
#plugin_LTLIBRARIES = bseevaluator.la
#
## test programs:
#noinst_PROGRAMS = evaluateexpr
#evaluateexpr_SOURCES = evaluateexpr.cc $(EVALUATE_SOURCES)
