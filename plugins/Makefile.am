# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = icons . freeverb # evaluator

if WITH_SPECTMORPH
SUBDIRS += spectmorph
endif

AM_CPPFLAGS += -I$(top_srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN="\"$(strip $(basename $(<F)))\"" -DBSE_PLUGIN_COMPILATION
AM_CXXFLAGS += $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE

# == IDL C++ Plugins ==
idl_files = $(strip			\
	artscompressor.idl		\
	bseamplifier.idl		\
	bsebalance.idl			\
	bsenoise.idl			\
	bsequantizer.idl		\
	bsesummation.idl		\
	standardsaturator.idl		\
	standardguspatchenvelope.idl	\
	bsecontribsampleandhold.idl	\
	davbassfilter.idl		\
	davchorus.idl			\
	davorgan.idl			\
)

doc_sources = $(idl_files) $(SOURCES) $(cxx_plugin_headers) $(cxx_plugin_sources) $(cxx_plugin_extra)

# == doc_sources.lst ==
doc_sources.lst: Makefile
	$(AM_V_GEN)
	$(Q) echo $(patsubst %, plugins/%, $(doc_sources)) > $@


# setup generation of C++ plugins from idl files
SFIDL = ../sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)
%.genidl.hh: %.idl # $(SFIDL)
	$(SFIDL) --plugin  --macro $(<F) $(SFIDL_INC) -I$(top_builddir) $< > $@ || (rm $@ ; exit 1 )

# == C++ Plugins ==
cxx_plugin_headers = bseadder.hh bseatandistort.hh bsesimpleadsr.hh bsemixer.hh bsemult.hh bsesequencer.hh bseiirfilter.hh	\
		     davcanyondelay.hh davsyndrum.hh davxtalstrings.hh
cxx_plugin_sources = bseadder.cc bseatandistort.cc bsesimpleadsr.cc bsemixer.cc bsemult.cc bsesequencer.cc bseiirfilter.cc	\
		     davcanyondelay.cc davsyndrum.cc davxtalstrings.cc
cxx_plugin_extra   = bsesimpleadsr-aux.cc
cxxplugins_FPU_la_SOURCES  = $(cxx_plugin_sources)
cxxplugins_FPU_la_LDFLAGS  = $(AM_LDFLAGS) -module $(plugins_ldflags)
cxxplugins_FPU_la_LIBADD   = $(plugins_libs)
cxxplugins_FPU_la_CFLAGS   = $(FPU_PLUGIN_CFLAGS)
cxxplugins_FPU_la_CXXFLAGS = $(FPU_PLUGIN_CXXFLAGS)

# old/unported plugin list
UNPORTED = $(strip		\
	bsefirfilter.[hc]	\
	davguitar.[hc]		\
)

# === plugin build rule setup ===
plugins_ldflags 	  = -avoid-version
plugins_libs    	  = ../bse/libbse-@MAJOR@.la $(LIBBSE_LIBS)
plugins_built_sources 	  =
CLEANFILES 		 += $(plugins_built_sources)
# .FPU plugins
plugin_FPU_LTLIBRARIES	  = cxxplugins.FPU.la
plugin_FPUdir 	  	  = $(bselibdir)/plugins
FPU_PLUGIN_CFLAGS	  = $(AM_CFLAGS) $(PLUGIN_FLAGS) $(FPU_FLAGS)
FPU_PLUGIN_CXXFLAGS	  = $(AM_CXXFLAGS) $(PLUGIN_FLAGS) $(FPU_FLAGS)

# == SIMD bseblockutils ==
if WITH_MMX_SSE
plugin_FPU_LTLIBRARIES	     += bseblockutils.SSE.la
bseblockutils_SSE_la_SOURCES  = bseblockutils.cc
bseblockutils_SSE_la_LDFLAGS  = $(AM_LDFLAGS) -module $(plugins_ldflags)
bseblockutils_SSE_la_LIBADD   = $(plugins_libs)
endif

# == C++ Plugin Library bseplugins ==
plugin_FPU_LTLIBRARIES	  += bseplugins.FPU.la
bseplugins_FPU_la_SOURCES  =
bseplugins_FPU_la_CXXFLAGS = $(FPU_PLUGIN_CXXFLAGS)
bseplugins_FPU_la_LDFLAGS  = -module $(plugins_ldflags)
bseplugins_FPU_la_LIBADD   = $(plugins_libs)

# == artscompressor ==
bseplugins_FPU_la_SOURCES	+= artscompressor.cc
plugins_built_sources		+= artscompressor.genidl.hh
$(srcdir)/artscompressor.cc: artscompressor.genidl.hh

# == bseamplifier ==
bseplugins_FPU_la_SOURCES	+= bseamplifier.cc
plugins_built_sources		+= bseamplifier.genidl.hh
$(srcdir)/bseamplifier.cc: bseamplifier.genidl.hh

# == bsebalance ==
bseplugins_FPU_la_SOURCES	+= bsebalance.cc
plugins_built_sources		+= bsebalance.genidl.hh
$(srcdir)/bsebalance.cc: bsebalance.genidl.hh

# == bsenoise ==
bseplugins_FPU_la_SOURCES	+= bsenoise.cc
plugins_built_sources		+= bsenoise.genidl.hh
$(srcdir)/bsenoise.cc: bsenoise.genidl.hh

# == bsequantizer ==
bseplugins_FPU_la_SOURCES	+= bsequantizer.cc
plugins_built_sources		+= bsequantizer.genidl.hh
$(srcdir)/bsequantizer.cc: bsequantizer.genidl.hh

# == bsesummation ==
bseplugins_FPU_la_SOURCES	+= bsesummation.cc
plugins_built_sources		+= bsesummation.genidl.hh
$(srcdir)/bsesummation.cc: bsesummation.genidl.hh

# == standardsaturator ==
bseplugins_FPU_la_SOURCES	+= standardsaturator.cc
plugins_built_sources		+= standardsaturator.genidl.hh
$(srcdir)/standardsaturator.cc: standardsaturator.genidl.hh

# == standardguspatchenvelope ==
bseplugins_FPU_la_SOURCES	+= standardguspatchenvelope.cc
plugins_built_sources		+= standardguspatchenvelope.genidl.hh
$(srcdir)/standardguspatchenvelope.cc: standardguspatchenvelope.genidl.hh

# == bsecontribsampleandhold ==
bseplugins_FPU_la_SOURCES	+= bsecontribsampleandhold.cc
plugins_built_sources		+= bsecontribsampleandhold.genidl.hh
$(srcdir)/bsecontribsampleandhold.cc: bsecontribsampleandhold.genidl.hh

# == davbassfilter ==
bseplugins_FPU_la_SOURCES	+= davbassfilter.cc
plugins_built_sources		+= davbassfilter.genidl.hh
$(srcdir)/davbassfilter.cc: davbassfilter.genidl.hh

# == davchorus ==
bseplugins_FPU_la_SOURCES	+= davchorus.cc
plugins_built_sources		+= davchorus.genidl.hh
$(srcdir)/davchorus.cc: davchorus.genidl.hh

# == davorgan ==
bseplugins_FPU_la_SOURCES	+= davorgan.cc
plugins_built_sources		+= davorgan.genidl.hh
$(srcdir)/davorgan.cc: davorgan.genidl.hh
