# GSL targets
GSL_H_SRC =	gslcommon.h gsldatacache.h gsldatahandle.h gsldefs.h gslwavechunk.h \
		gslmath.h gslfilter.h gsldatautils.h gsldatahandle-vorbis.h \
		gslconvert.h gslfft.h gslieee754.h gslsignal.h gslmagic.h \
		gslengine.h gslwaveosc.h
GSL_PRIVATE_H_SRC = gsloputil.h gslopmaster.h gslopnode.h gslopschedule.h gslloader.h
GSL_C_SRC =	gsldatacache.c gsldatahandle.c gslwavechunk.c gsldatahandle-vorbis.c \
		gslmath.c gslfilter.c gslcommon.c gsldatautils.c gslmagic.c \
		gslloader-wav.c gslloader-gslwave.c gslloader-oggvorbis.c \
		gslconvert.c gslfft.c gslsignal.c gslloader.c gslwaveosc.c \
		gslengine.c gsloputil.c gslopmaster.c gslopschedule.c
EXTRA_DIST +=	gslwaveosc-aux.c
GSL_NOINST_PROGS = gslwchunk gsltests gslglibhashtest gslffttest
gslwchunk_SOURCES = gslwchunk.c $(GSL_cc_dummy)
gslwchunk_LDADD = $(GSL_progs_ldadd)
gsltests_SOURCES = gsltests.c $(GSL_cc_dummy)
gsltests_LDADD = $(GSL_progs_ldadd)
gslffttest_SOURCES = gslffttest.c $(GSL_cc_dummy)
gslffttest_LDADD = $(GSL_progs_ldadd)
gslglibhashtest_SOURCES = gslglibhashtest.cc # C++ already
gslglibhashtest_LDADD = $(GSL_progs_ldadd)
EXTRA_DIST += gsl.gnuplot gsl-mplan.txt gslarrows gslglibhash.cc
EXTRA_DIST += gslwave.header gsl/gslglib.c gsl/gslglib.h gsl-fftgen.pl

gslfft.c: $(srcdir)/gsl-fftgen.pl $(srcdir)/gsl-fftconf.sh
	$(srcdir)/gsl-fftconf.sh $(srcdir)/gsl-fftgen.pl \"gslfft.h\" >$@

.PHONY:	gsltestoutput

gsltestoutput:
	@./gsltests blp     7 0.3		0.1211
	@./gsltests bhp    12 1.8332		0.1033
	@./gsltests bbp    14 0.5 0.6		0.1033
	@./gsltests bbs    12 1.5 1.6		0.2
	@./gsltests t1l     7 0.3		0.0125
	@./gsltests t1h     8 1.8332		0.1033
	@./gsltests t1p     6 0.5 0.6		0.1033
	@./gsltests t1s     8 1.4 1.5		0.25
	@./gsltests t2l    13 0.1	1.1	0.176
	@./gsltests t2h    10 1.14	1.2	0.0763
	@./gsltests t2p    14 0.7 0.8	1.3	0.0763
	@./gsltests t2p    10 0.9 1.2	1.4	0.15
	@./gsltests t2s    10 0.9 1.2	1.1	0.15
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

