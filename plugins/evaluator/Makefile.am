# Evaluator Makefile

CFLAGS   += $(EXTRA_CFLAGS)
CPPFLAGS += $(EXTRA_CXXFLAGS)
INCLUDES += -I$(top_srcdir) -I. $(BSE_CFLAGS) -DG_LOG_DOMAIN=BSE_PLUGIN_NAME
INCLUDES += -DBSE_PLUGIN_FALLBACK="\"$(strip $(basename $(<F)))\""

# setup generation of C++ plugins from idl files
BSEPGEN = $(top_builddir)/sfi/bse-plugin-generator
%.gen-idl.h: %.idl $(BSEPGEN)
	$(BSEPGEN) --nostdinc -I$(top_builddir) $< > $@ || (rm $@ ; exit 1 )

# install plugins under:
plugindir = $(bseplugindir)

# all plugins want to link against:
plugin_libs = $(BSE_LIBS) -lm

#
noinst_PROGRAMS = evaluateexpr

EVALUATE_SOURCES = instruction.cc compiler.cc cpu.cc

evaluateexpr_SOURCES = evaluateexpr.cc $(EVALUATE_SOURCES)

evaluator_la_SOURCES = bseevaluator.cc $(EVALUATE_SOURCES)
evaluator_la_LDFLAGS = -module -avoid-version
evaluator_la_LIBADD  = $(plugin_libs)

$(srcdir)/bseevaluator.cc: bseevaluator.gen-idl.h

plugin_LTLIBRARIES = evaluator.la

EXTRA_DIST += $(strip	\
	bseevaluator.cc	\
)
