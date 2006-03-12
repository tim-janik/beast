# BSE - Bedevilled Sound Engine dynamic Plugins
# Copyright (C) 2003 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS =

INCLUDES += -I$(top_srcdir) -I$(top_builddir) $(SFI_CFLAGS)

DEFS += -DG_LOG_DOMAIN=\"BSE-TEST\" # -DG_DISABLE_CONST_RETURNS

TESTS = testfft testcxx testwavechunk
progs_ldadd = $(top_builddir)/bse/libbse.la $(top_builddir)/sfi/libsfi.la
testwavechunk_SOURCES = testwavechunk.c cxxdummy.cc
testwavechunk_LDADD = $(progs_ldadd)
testfft_SOURCES = testfft.c cxxdummy.cc
testfft_LDADD = $(progs_ldadd)
testcxx_SOURCES = testcxx.cc
testcxx_LDADD = $(progs_ldadd)

noinst_PROGRAMS = cutvorbis magictest mathtests $(TESTS) #testwavechunk
cutvorbis_SOURCES = cutvorbis.c cxxdummy.cc
cutvorbis_LDADD = $(progs_ldadd)
magictest_SOURCES = magictest.c cxxdummy.cc
magictest_LDADD = $(progs_ldadd)
mathtests_SOURCES = mathtests.c cxxdummy.cc
mathtests_LDADD = $(progs_ldadd)


EXTRA_DIST += $(strip \
)

MATHTEST=./mathtests

.PHONY: plottest

plottest:
	@$(MATHTEST) blp     7 0.3               0.1211
	@$(MATHTEST) bhp    12 1.8332            0.1033
	@$(MATHTEST) bbp    14 0.5 0.6           0.1033
	@$(MATHTEST) bbs    12 1.5 1.6           0.2
	@$(MATHTEST) t1l     7 0.3               0.0125
	@$(MATHTEST) t1h     8 1.8332            0.1033
	@$(MATHTEST) t1p     6 0.5 0.6           0.1033
	@$(MATHTEST) t1s     8 1.4 1.5           0.25
	@$(MATHTEST) t2l    13 0.1       1.1     0.176
	@$(MATHTEST) t2h    10 1.14      1.2     0.0763
	@$(MATHTEST) t2p    14 0.7 0.8   1.3     0.0763
	@$(MATHTEST) t2p    10 0.9 1.2   1.4     0.15
	@$(MATHTEST) t2s    10 0.9 1.2   1.1     0.15
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
