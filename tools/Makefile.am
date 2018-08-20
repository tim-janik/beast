# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = . scripts

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir)
DEFS        += -DG_LOG_DOMAIN=\"BSE-TOOLS\" -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE

noinst_PROGRAMS = $(ALLTESTS)
progs_ldadd     = ../bse/libbse-@MAJOR@.la $(BSE_LIBS)

doc_sources = $(beast_headers) $(bst_cc_sources) $(beast_extra_files)

# == doc_sources.lst ==
doc_sources.lst: Makefile
	$(AM_V_GEN)
	$(Q) echo $(patsubst %, tools/%, $(HEADERS) $(SOURCES)) > $@
CLEANFILES += doc_sources.lst

#
# tools to build
#

beastbin_PROGRAMS   = bsewavetool
bsewavetool_SOURCES = bsewavetool.cc bwtwave.cc bseloopfuncs.cc
nodist_bsewavetool_SOURCES = ../topbuildid.cc
bsewavetool_LDADD   = $(progs_ldadd)
../.deps/topbuildid.Po: ; mkdir -p $(@D) && touch $@ # bad hack to fix maintainer-clean

noinst_PROGRAMS  += cutvorbis
cutvorbis_SOURCES = cutvorbis.cc
cutvorbis_LDADD   = $(progs_ldadd)

noinst_PROGRAMS    += bsefextract
bsefextract_SOURCES = bsefextract.cc
bsefextract_LDADD   = $(progs_ldadd)

noinst_PROGRAMS    += bsefcompare
bsefcompare_SOURCES = bsefcompare.cc
bsefcompare_LDADD   = $(progs_ldadd)

noinst_PROGRAMS  += magictest
magictest_SOURCES = magictest.cc
magictest_LDADD   = $(progs_ldadd)

noinst_PROGRAMS += mathtool
mathtool_SOURCES = mathtool.cc
mathtool_LDADD   = $(progs_ldadd)

# === Bsewavetool tests ===
check-local: bsewavetool-tests
bsewavetool-tests:
	./bsewavetool --unit-test

#
# bonbon ;)
# plot various filter functions designed by mathtool
MATHTOOL=./mathtool

.PHONY: plottest

plottest:
	@$(MATHTOOL) blp     7 0.3               0.1211
	@$(MATHTOOL) bhp    12 1.8332            0.1033
	@$(MATHTOOL) bbp    14 0.5 0.6           0.1033
	@$(MATHTOOL) bbs    12 1.5 1.6           0.2
	@$(MATHTOOL) t1l     7 0.3               0.0125
	@$(MATHTOOL) t1h     8 1.8332            0.1033
	@$(MATHTOOL) t1p     6 0.5 0.6           0.1033
	@$(MATHTOOL) t1s     8 1.4 1.5           0.25
	@$(MATHTOOL) t2l    13 0.1       1.1     0.176
	@$(MATHTOOL) t2h    10 1.14      1.2     0.0763
	@$(MATHTOOL) t2p    14 0.7 0.8   1.3     0.0763
	@$(MATHTOOL) t2p    10 0.9 1.2   1.4     0.15
	@$(MATHTOOL) t2s    10 0.9 1.2   1.1     0.15
	@echo -n "plot [0:pi] [-96:1]"
	@echo -n " dB(BL7(Z(x))),"
	@echo -n " dB(BH12(Z(x))),"
	@echo -n " dB(BP14(Z(x))),"
	@echo -n " dB(BS12(Z(x))),"
	@echo -n " dB(T1L7(Z(x))),"
	@echo -n " dB(T1H8(Z(x))),"
	@echo -n " dB(T1P6(Z(x))),"
	@echo -n " dB(T1S8(Z(x))),"
	@echo -n " dB(T2L13(Z(x))),"
	@echo -n " dB(T2H10(Z(x))),"
	@echo -n " dB(T2P14(Z(x))),"
	@echo -n " dB(T2P10(Z(x))),"
	@echo -n " dB(T2S10(Z(x))),"
	@echo " -3"
