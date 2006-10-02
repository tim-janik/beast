# beast/slowtests
# Copyright (C) 2003-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = . audio filecheck

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(SFI_CFLAGS)
DEFS += -DG_LOG_DOMAIN="\"BEAST-TEST\"" # -DG_DISABLE_CONST_RETURNS
SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)

# === rules to generate built sources ===
#
# bsecxxapi.hh
GENERATED_CLEANFILES += bsecxxapi.hh
$(srcdir)/perftest.cc: bsecxxapi.hh
bsecxxapi.hh: $(top_srcdir)/bse/bse.idl $(SFIDL)
	cd . \
	&& echo -e "#include <sfi/sficxx.hh>\n" > xgen-$(@F) \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --header $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bsecxxapi.cc
GENERATED_CLEANFILES += bsecxxapi.cc
$(srcdir)/perftest.cc: bsecxxapi.cc
bsecxxapi.cc: bsecxxapi.hh $(top_srcdir)/bse/bse.idl $(SFIDL)
	cd . \
	&& echo -e "#include \"bsecxxapi.hh\"\n" > xgen-$(@F) \
	&& $(SFIDL) --client-cxx $(SFIDL_INC) --source $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)


# === generation targets ===
# convenience targets for generated source files
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(GENERATED) stamp-*
generated: clean-generated $(GENERATED)


# === test programs ===
TESTS = perftest testwavechunk
noinst_PROGRAMS = $(TESTS)
progs_ldadd = $(top_builddir)/bse/libbse.la
# testwavechunk
testwavechunk_SOURCES = testwavechunk.c cxxdummy.cc
testwavechunk_LDADD = $(progs_ldadd)
# perftest
perftest_SOURCES = perftest.cc bsecxxapi.cc
perftest_LDADD = $(progs_ldadd)

# === Resampler tests ===
noinst_PROGRAMS += testresampler
testresampler_SOURCES = testresampler.cc
testresampler_LDADD = $(progs_ldadd)
check-before: resampler-quick-checks
slowcheck: resampler-checks
resampler-quick-checks:
	: ## some quick checks (resampler-checks contains actual tests - use make slowcheck)
	./testresampler filter-impl
	./testresampler accuracy --up --precision=8 --freq-scan=180,18000,180 --max-threshold=45
	./testresampler accuracy --down --precision=12 --freq-scan=90,9000,90 --max-threshold=72
	./testresampler accuracy --fpu --up --precision=16 --freq-scan=180,18000,180 --max-threshold=89.5
	./testresampler accuracy --fpu --oversample --precision=20 --freq-scan=180,18000,180 --max-threshold=113.5
	./testresampler accuracy --subsample --precision=24 --freq-scan=90,9000,90 --max-threshold=126

resampler-checks:
	: ## test SSE filter implementation
	./testresampler filter-impl
	: ## (possibly SSEified) upsampler tests
	./testresampler accuracy --up --precision=8 --freq-scan=50,18000,50  --max-threshold=45     # ideally: 48dB
	./testresampler accuracy --up --precision=12 --freq-scan=50,18000,50 --max-threshold=66.5   # ideally: 72dB
	./testresampler accuracy --up --precision=16 --freq-scan=50,18000,50 --max-threshold=89     # ideally: 96dB
	./testresampler accuracy --up --precision=20 --freq-scan=50,18000,50 --max-threshold=113.5  # ideally: 120dB
	./testresampler accuracy --up --precision=24 --freq-scan=50,18000,50 --max-threshold=126.5  # ideally: 144dB
	: ## (possibly SSEified) downsampler tests
	./testresampler accuracy --down --precision=8  --freq-scan=25,9000,25 --max-threshold=51    # ideally: 48dB
	./testresampler accuracy --down --precision=12 --freq-scan=25,9000,25 --max-threshold=72    # ideally: 72dB
	./testresampler accuracy --down --precision=16 --freq-scan=25,9000,25 --max-threshold=95    # ideally: 96dB
	./testresampler accuracy --down --precision=20 --freq-scan=25,9000,25 --max-threshold=119.5 # ideally: 120dB
	./testresampler accuracy --down --precision=24 --freq-scan=25,9000,25 --max-threshold=131   # ideally: 144dB
	: ## FPU upsampler tests
	./testresampler accuracy --fpu --up --precision=8  --freq-scan=50,18000,50 --max-threshold=45     # ideally: 48dB
	./testresampler accuracy --fpu --up --precision=12 --freq-scan=50,18000,50 --max-threshold=66.5   # ideally: 72dB
	./testresampler accuracy --fpu --up --precision=16 --freq-scan=50,18000,50 --max-threshold=89     # ideally: 96dB
	./testresampler accuracy --fpu --up --precision=20 --freq-scan=50,18000,50 --max-threshold=113.5  # ideally: 120dB
	./testresampler accuracy --fpu --up --precision=24 --freq-scan=50,18000,50 --max-threshold=126    # ideally: 144dB
	: ## FPU downsampler tests
	./testresampler accuracy --fpu --down --precision=8  --freq-scan=25,9000,25 --max-threshold=51    # ideally: 48dB
	./testresampler accuracy --fpu --down --precision=12 --freq-scan=25,9000,25 --max-threshold=72    # ideally: 72dB
	./testresampler accuracy --fpu --down --precision=16 --freq-scan=25,9000,25 --max-threshold=95    # ideally: 96dB
	./testresampler accuracy --fpu --down --precision=20 --freq-scan=25,9000,25 --max-threshold=119.5 # ideally: 120dB
	./testresampler accuracy --fpu --down --precision=24 --freq-scan=25,9000,25 --max-threshold=131   # ideally: 144dB
	: ## sparse testing of sub- and oversampling (we don't test every combination of
	: ## flags here, but this is also an uncommon usage scenario)
	./testresampler accuracy       --oversample --precision=8  --freq-scan=50,18000,50 --max-threshold=45   # ideally: 48dB
	./testresampler accuracy       --oversample --precision=16 --freq-scan=50,18000,50 --max-threshold=89   # ideally: 96dB
	./testresampler accuracy --fpu --oversample --precision=16 --freq-scan=50,18000,50 --max-threshold=89   # ideally: 96dB
	./testresampler accuracy       --subsample  --precision=16 --freq-scan=25,9000,25  --max-threshold=85.5 # ideally: 96dB
	./testresampler accuracy --fpu --subsample  --precision=16 --freq-scan=25,9000,25  --max-threshold=85.5 # ideally: 96dB
