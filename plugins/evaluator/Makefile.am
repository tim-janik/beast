# Evaluator Makefile

INCLUDES += -I$(top_srcdir) -I. $(BSE_CFLAGS) -DG_LOG_DOMAIN=BSE_PLUGIN_NAME
INCLUDES += -DBSE_PLUGIN_FALLBACK="\"$(strip $(basename $(<F)))\""

# install plugins under:
plugindir = $(bseplugindir)

# setup generation of C++ plugins from idl files
BSEPGEN = $(top_builddir)/sfi/bse-plugin-generator
%.gen-idl.h: %.idl $(BSEPGEN)
	$(BSEPGEN) --nostdinc -I$(top_builddir) $< > $@ || (rm $@ ; exit 1 )
CLEANFILES += $(plugins_built_sources)

EVALUATE_SOURCES = instruction.cc compiler.cc cpu.cc

bseevaluator_la_SOURCES = bseevaluator.cc $(EVALUATE_SOURCES)
bseevaluator_la_LDFLAGS = -module -avoid-version
bseevaluator_la_LIBADD  = $(BSE_LIBS) -lm
$(srcdir)/bseevaluator.cc: bseevaluator.gen-idl.h
plugins_built_sources += bseevaluator.gen-idl.h
EXTRA_HEADERS += bseevaluator.idl symbols.h token.h instruction.h compiler.h cpu.h

plugin_LTLIBRARIES = bseevaluator.la

# test programs:
noinst_PROGRAMS = evaluateexpr
evaluateexpr_SOURCES = evaluateexpr.cc $(EVALUATE_SOURCES)

EXTRA_DIST += $(strip		\
	bseevaluator.cc		\
	$(EXTRA_HEADERS)	\
)
