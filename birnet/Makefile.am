# Birnet
# Copyright (C) 2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS		= . tests
INCLUDES       += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(BIRNET_CFLAGS)
DEFS            = -DBIRNET_LOG_DOMAIN=\"BIRNET\"
GLIB_MKENUMS    = glib-mkenums

birnet_headers = $(strip 		\
	birnetcdefs.h			\
	birnet.hh			\
	birnetcdefs.h			\
	birnetcpu.hh			\
	birnetmsg.hh			\
	birnetsignal.hh                 \
	birnettests.h			\
	birnetthread.hh			\
	birnetutf8.hh			\
	birnetutils.hh			\
)
birnet_sources = $(strip 		\
	birnetthreadimpl.cc		\
	birnetcpu.cc			\
	birnetmsg.cc			\
	birnetsignal.cc                 \
	birnetthread.cc			\
	birnetutf8.cc			\
	birnetutils.cc			\
)
birnet_private_headers = $(strip 	\
	birnetsignalslot.hh		\
	birnetsignaltemplate.hh		\
)
birnet_generated_headers = $(strip 	\
	birnetconfig.h			\
	birnetsignalvariants.hh         \
)
EXTRA_DIST += configure.inc mksignals.sh

# Birnet library
# we want a partial shared library here. libtool creates that if the library
# name ends in .lo or .o. however automake doesn't know .lo or .o libraries,
# so we simply build an ordinary non-installed .la library and then use our
# own rule to build the .o from the .la pieces.
noinst_LTLIBRARIES         = libbirnet.la
libbirnetincludedir        = $(includedir)/birnet
libbirnetinclude_HEADERS   = $(birnet_headers) $(birnet_generated_headers)
libbirnet_la_SOURCES       = $(birnet_sources)
libbirnet_la_DEPENDENCIES  =
libbirnet_la_LIBADD        = $(BIRNET_LIBS) -lm
libbirnet_la_LDFLAGS       = -no-undefined # -Wl,-Bsymbolic
# keep this .o rule in sync with the corresponding .la rule from Makefile.in
libbirnet.o: $(libbirnet_la_OBJECTS) $(libbirnet_la_DEPENDENCIES)
	$(CXXLINK)  $(libbirnet_la_LDFLAGS) $(libbirnet_la_OBJECTS) # $(libbirnet_la_LIBADD) $(LIBS)
all-am: libbirnet.o
CLEANFILES += libbirnet.lo libbirnet.o
EXTRA_DIST += $(birnet_private_headers)

# === birnetconfig.h ===
birnetconfig.h: $(top_builddir)/config.status # Makefile
	cd . \
	&& echo "/* Generated data from $< (by make $@) */" 					 > xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* select programming environment */"						>> xgen-$(@F) \
	&& echo "#ifndef _GNU_SOURCE"								>> xgen-$(@F) \
	&& echo "#define _GNU_SOURCE"								>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* standard C code wrappers */"						>> xgen-$(@F) \
	&& echo "#ifdef  __cplusplus"								>> xgen-$(@F) \
	&& echo '#define BIRNET_EXTERN_C_BEGIN()	extern "C" {'				>> xgen-$(@F) \
	&& echo "#define BIRNET_EXTERN_C_END()		}"					>> xgen-$(@F) \
	&& echo "#else"										>> xgen-$(@F) \
	&& echo "#define BIRNET_EXTERN_C_BEGIN()"						>> xgen-$(@F) \
	&& echo "#define BIRNET_EXTERN_C_END()"							>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* birnet version */"								>> xgen-$(@F) \
	&& echo "#define BIRNET_MAJOR_VERSION		(@BIRNET_MAJOR_VERSION@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_MINOR_VERSION		(@BIRNET_MINOR_VERSION@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_MICRO_VERSION		(@BIRNET_MICRO_VERSION@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_BINARY_AGE		(@BIRNET_BINARY_AGE@)"			>> xgen-$(@F) \
	&& echo "#define BIRNET_INTERFACE_AGE		(@BIRNET_INTERFACE_AGE@)"		>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* log domain */"								>> xgen-$(@F) \
	&& echo "#ifndef BIRNET_LOG_DOMAIN"							>> xgen-$(@F) \
	&& echo "#ifdef  G_LOG_DOMAIN"								>> xgen-$(@F) \
	&& echo "#define BIRNET_LOG_DOMAIN  G_LOG_DOMAIN"					>> xgen-$(@F) \
	&& echo "#else"										>> xgen-$(@F) \
	&& echo "#define BIRNET_LOG_DOMAIN  ((const char*) 0)"					>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* version checks */"								>> xgen-$(@F) \
	&& echo "#define BIRNET_CHECK_VERSION(major,minor,micro)	\\"			>> xgen-$(@F) \
	&& echo "  (BIRNET_MAJOR_VERSION > (major) || \\"					>> xgen-$(@F) \
	&& echo "   (BIRNET_MAJOR_VERSION == (major) && BIRNET_MINOR_VERSION > (minor)) || \\"	>> xgen-$(@F) \
	&& echo "   (BIRNET_MAJOR_VERSION == (major) && BIRNET_MINOR_VERSION == (minor) && \\"	>> xgen-$(@F) \
	&& echo "    BIRNET_MICRO_VERSION >= (micro)))"						>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* configure results */"							>> xgen-$(@F) \
	&& echo "#define BIRNET_OS_@BIRNET_OS@          (1)"					>> xgen-$(@F) \
	&& echo "#define BIRNET_SIZEOF_SYS_TYPESH_UINT	(@BIRNET_SIZEOF_SYS_TYPESH_UINT@)"	>> xgen-$(@F) \
	&& echo "#define BIRNET_SIZEOF_PTH_MUTEX_T	(@BIRNET_SIZEOF_PTH_MUTEX_T@)" 		>> xgen-$(@F) \
	&& echo "#define BIRNET_SIZEOF_PTH_COND_T	(@BIRNET_SIZEOF_PTH_COND_T@)" 		>> xgen-$(@F) \
	&& echo "#define BIRNET_HAVE_MUTEXATTR_SETTYPE	(@BIRNET_HAVE_MUTEXATTR_SETTYPE@ && \\" >> xgen-$(@F) \
	&& echo "                                        BIRNET_SIZEOF_PTH_MUTEX_T && \\" 	>> xgen-$(@F) \
	&& echo "                                        BIRNET_SIZEOF_PTH_COND_T)" 		>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* Generated data ends here */" 						>> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
CLEANFILES += birnetconfig.h
$(libbirnet_la_OBJECTS): birnetconfig.h

# === birnetsignalvariants.hh ===
$(srcdir)/*.cc: birnetsignalvariants.hh
birnetsignalvariants.hh: $(srcdir)/birnetsignalslot.hh $(srcdir)/birnetsignaltemplate.hh $(srcdir)/mksignals.sh
	cd . \
	&& echo "/* Signal Variants -- Generated from: $(^F) */"	>xgen-sigs \
	&& echo 						       >>xgen-sigs \
	&& $(srcdir)/mksignals.sh $(srcdir)/birnetsignalslot.hh     17 >>xgen-sigs \
	&& $(srcdir)/mksignals.sh $(srcdir)/birnetsignaltemplate.hh 16 >>xgen-sigs \
	&& cp xgen-sigs $@ \
	&& rm -f xgen-sigs
CLEANFILES += xgen-sigs birnetsignalvariants.hh xgen-signals.sed

noinst_PROGRAMS = $(ALLTESTS)
progs_ldadd     = libbirnet.o $(BIRNET_LIBS) -lm

noinst_PROGRAMS       += birnet-zintern
birnet_zintern_SOURCES = birnet-zintern.cc
birnet_zintern_LDADD   = $(progs_ldadd)
birnet_zintern_DEPS    = $(progs_deps)
