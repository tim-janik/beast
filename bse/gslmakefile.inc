# GSL targets
GSL_H_SOURCES =	gslcommon.h gsldatacache.h gsldatahandle.h gsldefs.h gslwavechunk.h \
		gslwavedsc.h gslmath.h gslengine.h gslfilter.h gsllooper.h
GSL_PRIVATE_H_SOURCES = gsloputil.h gslopmaster.h gslopnode.h gslopschedule.h
GSL_C_SOURCES = gsldatacache.c gsldatahandle.c gslwavechunk.c gslwavedsc.c \
		gslmath.c gslengine.c gsloputil.c gslopmaster.c \
		gslopschedule.c gslfilter.c gslcommon.c gsllooper.c
GSL_NOINST_PROGS = gslwchunk gslloader gsltests
gslwchunk_SOURCES = gslwchunk.c
gslwchunk_LDADD = $(progs_LDADD)
gslloader_SOURCES = gslloader.c
gslloader_LDADD = $(progs_LDADD)
gsltests_SOURCES = gsltests.c
gsltests_LDADD = $(progs_LDADD)
EXTRA_DIST += gsl.gnuplot gsl-mplan.txt gslarrows gslglibhash.cc gsl/gslglib.c gsl/gslglib.h
