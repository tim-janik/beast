# BSE - Bedevilled Sound Engine
# Copyright (C) 2004 Tim Janik
#
## Makefile.am for BSE Tests

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(SFI_CFLAGS)
DEFS     += -DG_LOG_DOMAIN="\"BSE-TEST\"" # -DG_DISABLE_CONST_RETURNS
CFLAGS   += $(EXTRA_CFLAGS)
CPPFLAGS += $(EXTRA_CXXFLAGS)
# IDL Compiler
SFIDL     = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)
BSESH     = $(top_builddir)/shell/bsesh-$(BIN_VERSION)

# IDL generation rule for plugins
%.gen-idl.h: %.idl $(BSEPGEN) $(SFIDL)
	$(SFIDL) $(SFIDL_INC) --plugin --macro $(<F) $< > $@ || (rm $@ ; exit 1 )

#
# Plugin build rules
#
# bselatencytest:
bselatencytest_la_SOURCES = bselatencytest.cc
bselatencytest_la_LDFLAGS = -module -avoid-version
bselatencytest_la_LIBADD  = $(BSE_LIBS) -lm
EXTRA_HEADERS += bselatencytest.idl
$(srcdir)/bselatencytest.cc: bselatencytest.idl
$(srcdir)/bselatencytest.cc: bselatencytest.gen-idl.h
CLEANFILES += bselatencytest.gen-idl.h

# libraries to build
lib_LTLIBRARIES = bselatencytest.la
# prevent libraries from being installed
install-libLTLIBRARIES:

builddirplugins = ./.libs/*.so:$(top_builddir)/plugins/.libs/*.so

# 
# test rule
#
test: 	all
	cd . \
	&& rm -f ./tmp-midi-fifo					\
	&& mkfifo ./tmp-midi-fifo					\
	&& FIFO=tmp-midi-fifo $(BSESH)					\
	    -m oss=tmp-midi-fifo -p null				\
	   --bse-override-plugin-globs $(builddirplugins)		\
	   --bse-override-script-path $(top_builddir)/shell/		\
	   -s $(srcdir)/bseplay.scm $(srcdir)/midi-latency.bse
CLEANFILES += tmp-midi-fifo

EXTRA_DIST += $(strip 		\
	midi-latency.bse	\
	$(EXTRA_HEADERS)	\
)
