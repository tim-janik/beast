# Birnet
# Copyright (C) 2006 Tim Janik
#
# GNU General Public License version 2 or (at your option) any later version.

SUBDIRS		= . tests
INCLUDES  	= -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(BIRNET_CFLAGS)
GLIB_MKENUMS    = glib-mkenums
DEFS		= -DBIRNET_PRIVATE -DPARANOID

birnet_c_headers = $(strip 		\
	birnet.h			\
	birnetmsg.h			\
	birnetring.h			\
	birnetthread.h			\
)
birnet_c_sources = $(strip 		\
	birnetmsg.c			\
	birnetring.c			\
	birnetthread.c			\
)
birnet_cc_headers = $(strip 		\
	birnetcore.hh			\
)
birnet_cc_sources = $(strip 		\
	birnetcore.cc			\
)
birnet_private_headers = $(strip 	\
)
birnet_generated_headers = $(strip 	\
	birnetconfig.h			\
)

# Birnet library
# we want a partial shared library here. libtool creates that if the library
# name ends in .lo or .o. however automake doesn't know .lo or .o libraries,
# so we simply build an ordinary non-installed .la library and then use our
# own rule to build the .o from the .la pieces.
noinst_LTLIBRARIES         = libbirnet.la
libbirnetincludedir        = $(includedir)/birnet
libbirnetinclude_HEADERS   = $(birnet_c_headers) $(birnet_cc_headers) $(birnet_generated_headers)
libbirnet_la_SOURCES       = $(birnet_c_sources) $(birnet_cc_sources)
libbirnet_la_DEPENDENCIES +=
libbirnet_la_LIBADD        = $(BIRNET_LIBS) -lm
libbirnet_la_LDFLAGS       = -no-undefined # -Wl,-Bsymbolic
# keep this .o rule in sync with the corresponding .la rule from Makefile.in
libbirnet.o: $(libbirnet_la_OBJECTS) $(libbirnet_la_DEPENDENCIES)
	$(CXXLINK)  $(libbirnet_la_LDFLAGS) $(libbirnet_la_OBJECTS) # $(libbirnet_la_LIBADD) $(LIBS)
all-am: libbirnet.o
CLEANFILES += libbirnet.lo libbirnet.o
EXTRA_DIST += $(birnet_private_headers)

birnetconfig.h: $(top_builddir)/config.status # Makefile
	cd . \
	&& echo "/* Generated data from $< (by make $@) */" 					 > xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "#ifdef  __cplusplus"								>> xgen-$(@F) \
	&& echo "#define BIRNET_BEGIN_EXTERN_C()	extern \"C\" {"				>> xgen-$(@F) \
	&& echo "#define BIRNET_END_EXTERN_C()		}"					>> xgen-$(@F) \
	&& echo "#else"										>> xgen-$(@F) \
	&& echo "#define BIRNET_BEGIN_EXTERN_C()"						>> xgen-$(@F) \
	&& echo "#define BIRNET_END_EXTERN_C()"							>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
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
