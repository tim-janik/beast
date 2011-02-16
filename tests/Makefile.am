# beast/tests
# Copyright (C) 2003-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = . latency scripts bse audio filecheck

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
noinst_PROGRAMS = $(ALLTESTS)
progs_ldadd     = $(top_builddir)/bse/libbse.la $(BSE_LIBS)

# testwavechunk
TESTS                += testwavechunk
SLOWTESTS            += testwavechunk
testwavechunk_SOURCES = testwavechunk.c cxxdummy.cc
testwavechunk_LDADD = $(progs_ldadd)
# perftest
TESTS           += perftest
perftest_SOURCES = perftest.cc bsecxxapi.cc
perftest_LDADD   = $(progs_ldadd)
# testresampler
noinst_PROGRAMS      += testresampler
testresampler_SOURCES = testresampler.cc
testresampler_LDADD   = $(progs_ldadd)
# testresamplerq
TESTS                 += testresamplerq
testresamplerq_SOURCES = testresamplerq.cc
testresamplerq_LDADD   = $(progs_ldadd)


# === Resampler tests ===
resampler-quick-checks:
	: ## some quick checks (resampler-checks contains actual tests - use make slowcheck)
	./testresampler filter-impl
	./testresampler accuracy --up --precision=8 --freq-scan=180,18000,1979 --max-threshold=45
	./testresampler accuracy --down --precision=12 --freq-scan=90,9000,997 --max-threshold=72
	./testresampler accuracy --fpu --up --precision=16 --freq-scan=180,18000,1453 --max-threshold=89.5
	./testresampler accuracy --fpu --oversample --precision=20 --freq-scan=180,18000,1671 --max-threshold=113.5
	./testresampler accuracy --subsample --precision=24 --freq-scan=90,9000,983 --max-threshold=126

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

resampler-perf:
	./testresampler perf --fpu --precision=8  --up
	./testresampler perf --fpu --precision=8  --down
	./testresampler perf --fpu --precision=8  --subsample
	./testresampler perf --fpu --precision=8  --oversample
	./testresampler perf       --precision=8  --up
	./testresampler perf       --precision=8  --down
	./testresampler perf       --precision=8  --subsample
	./testresampler perf       --precision=8  --oversample
	./testresampler perf --fpu --precision=16 --up
	./testresampler perf --fpu --precision=16 --down
	./testresampler perf --fpu --precision=16 --subsample
	./testresampler perf --fpu --precision=16 --oversample
	./testresampler perf       --precision=16 --up
	./testresampler perf       --precision=16 --down
	./testresampler perf       --precision=16 --subsample
	./testresampler perf       --precision=16 --oversample
	./testresampler perf --fpu --precision=20 --up
	./testresampler perf --fpu --precision=20 --down
	./testresampler perf --fpu --precision=20 --subsample
	./testresampler perf --fpu --precision=20 --oversample
	./testresampler perf       --precision=20 --up
	./testresampler perf       --precision=20 --down
	./testresampler perf       --precision=20 --subsample
	./testresampler perf       --precision=20 --oversample
	./testresampler perf --fpu --precision=24 --up
	./testresampler perf --fpu --precision=24 --down
	./testresampler perf --fpu --precision=24 --subsample
	./testresampler perf --fpu --precision=24 --oversample
	./testresampler perf       --precision=24 --up
	./testresampler perf       --precision=24 --down
	./testresampler perf       --precision=24 --subsample
	./testresampler perf       --precision=24 --oversample

# perf tests
perf:
	./perftest
