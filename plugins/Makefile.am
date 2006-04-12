# BSE-Plugins - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 1998-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = icons . freeverb # evaluator

INCLUDES += -I$(top_srcdir) -I. $(BSE_CFLAGS) -DG_LOG_DOMAIN=BSE_PLUGIN_NAME
INCLUDES += -DBSE_PLUGIN_FALLBACK="\"$(strip $(basename $(<F)))\""

#
# C++ Plugins
#
plugin_idl_files = $(strip		\
	davchorus.idl			\
	bsebalance.idl			\
	artscompressor.idl		\
	davbassfilter.idl		\
	bsecontribsampleandhold.idl	\
	bsenoise.idl			\
	standardsaturator.idl		\
	standardguspatchenvelope.idl	\
)
EXTRA_HEADERS += $(plugin_idl_files)
# setup generation of C++ plugins from idl files
SFIDL = ../sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)
%.genidl.hh: %.idl $(SFIDL)
	$(SFIDL) --plugin  --macro $(<F) $(SFIDL_INC) -I$(top_builddir) $< > $@ || (rm $@ ; exit 1 )

# old/unported plugin list
UNPORTED = $(strip		\
	bsefirfilter.[hc]	\
	davguitar.[hc]		\
)
PLUGIN_CFILES = $(strip		\
	bseadder.[hc]		\
	bseatandistort.[hc]	\
	bseiirfilter.[hc]	\
	bsemixer.[hc]		\
	bsemult.[hc]		\
	bsesimpleadsr.[hc]	\
	bsesequencer.[hc]	\
	davorgan.[hc]		\
	davxtalstrings.[hc]	\
	davsyndrum.[hc]		\
	davcanyondelay.[hc]	\
)
EXTRA_DIST += $(strip \
	bsesimpleadsr-aux.c	\
        $(EXTRA_HEADERS) \
)

# === plugin build rule setup ===
plugins_ldflags 	  = -avoid-version
plugins_libs    	  = $(top_builddir)/bse/libbse.la $(BSE_LIBS) -lm
plugins_built_sources 	  = 	# listed in Makefile.plugins
CLEANFILES 		 += $(plugins_built_sources)
# +PLAIN plugins
plugin_PLAIN_ltlibs	  =	# listed in Makefile.plugins
plugin_PLAIN_LTLIBRARIES  = $(plugin_PLAIN_ltlibs)
plugin_PLAINdir 	  = $(bseplugindir)+PLAIN
# +SSE plugins
plugin_SSE_ltlibs	  =	# listed in Makefile.plugins
plugin_SSE_LTLIBRARIES	  = $(plugin_SSE_cond_libs)
plugin_SSEdir		  = $(bseplugindir)+SSE
if WITH_SSE_FLAGS
plugin_SSE_cond_libs	  = $(plugin_SSE_ltlibs)
else
plugin_SSE_cond_libs	  =
endif


# === Makefile.plugins adds plugin rules ===
include Makefile.plugins
# === Makefile.plugins generation rules ===
#### C++ Plugin bsefoo
##$(srcdir)/bsefoo.cc: bsefoo.genidl.hh
##plugins_built_sources   += bsefoo.genidl.hh
##plugin_PLAIN_ltlibs     += bsefoo+PLAIN.la
##bsefoo_PLAIN_la_SOURCES  = bsefoo.cc
##bsefoo_PLAIN_la_LDFLAGS  = -module $(plugins_ldflags)
##bsefoo_PLAIN_la_LIBADD   = $(plugins_libs)
##bsefoo_PLAIN_la_CXXFLAGS = $(AM_CXXFLAGS)
.PHONY: refresh-Makefile.plugins
refresh-Makefile.plugins: Makefile.am Makefile.in Makefile
	echo									 >xgen-$(@F)
	: # generate C++ plugin rules
	for name in $(plugin_idl_files:.idl=) ; do						 \
	  _name=`echo $$name | sed 's/[^A-Za-z0-9]/_/g' `					;\
	  echo -e "\n## C++ Plugin $$name"					>>xgen-$(@F) 	;\
	  echo '$$(srcdir)/'"$$name.cc: $$name.genidl.hh"			>>xgen-$(@F)	;\
	  echo "plugins_built_sources   += $$name.genidl.hh"			>>xgen-$(@F)	;\
	  echo "plugin_PLAIN_ltlibs    += $$name+PLAIN.la"			>>xgen-$(@F)	;\
	  echo "$$_name"'_PLAIN_la_SOURCES = '"$$name.cc"			>>xgen-$(@F)	;\
	  echo "$$_name"'_PLAIN_la_LDFLAGS = -module $$(plugins_ldflags)'	>>xgen-$(@F)	;\
	  echo "$$_name"'_PLAIN_la_LIBADD  = $$(plugins_libs)'			>>xgen-$(@F)	;\
	  echo "$$_name"'_PLAIN_la_CXXFLAGS  = $$(AM_CXXFLAGS)'			>>xgen-$(@F)	;\
	  echo "plugin_SSE_ltlibs      += $$name+SSE.la"			>>xgen-$(@F)	;\
	  echo "$$_name"'_SSE_la_SOURCES   = $$('"$$_name"'_PLAIN_la_SOURCES)'	>>xgen-$(@F)	;\
	  echo "$$_name"'_SSE_la_LDFLAGS   = $$('"$$_name"'_PLAIN_la_LDFLAGS)'	>>xgen-$(@F)	;\
	  echo "$$_name"'_SSE_la_LIBADD    = $$('"$$_name"'_PLAIN_la_LIBADD)'	>>xgen-$(@F)	;\
	  echo "$$_name"'_SSE_la_CXXFLAGS  = $$(AM_CXXFLAGS) $$(SSE_FLAGS)'	>>xgen-$(@F)	;\
	done
	: # generate C plugin rules
	for wcfile in $(filter %.c, $(wildcard $(PLUGIN_CFILES))) ; do				 \
	  name=`echo $$wcfile | sed 's/.c$$//' `						;\
	  _name=`echo $$name | sed 's/[^A-Za-z0-9]/_/g' `					;\
	  cfile="$$name.c"									;\
	  hfile="$$name.h"									;\
	  test -e "$$cfile" || {								 \
	    echo "$$cfile: No such file" >&2							;\
	    exit 1										;\
	  }											;\
	  grep -qE "BSE_EXPORTS_BEGIN|BSE_DEFINE_EXPORTS" $$cfile || {				 \
	    echo "$$cfile: missing BSE_DEFINE_EXPORTS() directive" >&2				;\
	    exit 1										;\
	  }											;\
	  echo									>>xgen-$(@F)	;\
	  echo -e "\n## C Plugin $$name"					>>xgen-$(@F) 	;\
	  test -e "$$hfile" && {								 \
	    echo "EXTRA_HEADERS      += $$hfile"				>>xgen-$(@F)	;\
	    echo '$$(srcdir)/'"$$cfile: $$hfile"				>>xgen-$(@F)	;\
	  }											;\
	  echo "plugin_PLAIN_ltlibs += $$name+PLAIN.la"				>>xgen-$(@F)	;\
	  echo "$$_name"'_PLAIN_la_SOURCES = '"$$cfile"				>>xgen-$(@F)	;\
	  echo "$$_name"'_PLAIN_la_LDFLAGS = -module $$(plugins_ldflags)'	>>xgen-$(@F)	;\
	  echo "$$_name"'_PLAIN_la_LIBADD  = $$(plugins_libs)'			>>xgen-$(@F)	;\
	  echo "$$_name"'_PLAIN_la_CFLAGS    = $$(AM_CFLAGS)'			>>xgen-$(@F)	;\
	  echo "plugin_SSE_ltlibs   += $$name+SSE.la"				>>xgen-$(@F)	;\
	  echo "$$_name"'_SSE_la_SOURCES   = $$('"$$_name"'_PLAIN_la_SOURCES)'	>>xgen-$(@F)	;\
	  echo "$$_name"'_SSE_la_LDFLAGS   = $$('"$$_name"'_PLAIN_la_LDFLAGS)'	>>xgen-$(@F)	;\
	  echo "$$_name"'_SSE_la_LIBADD    = $$('"$$_name"'_PLAIN_la_LIBADD)'	>>xgen-$(@F)	;\
	  echo "$$_name"'_SSE_la_CFLAGS    = $$(AM_CFLAGS) $$(SSE_FLAGS)'	>>xgen-$(@F)	;\
	done
	: # move Makefile.plugins in place
	cp xgen-$(@F) Makefile.plugins
	rm -f xgen-$(@F)
# $ echo >Makefile.plugins && make refresh-Makefile.plugins
