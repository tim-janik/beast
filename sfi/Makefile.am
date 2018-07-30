# BEAST & BSE
include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)

SUBDIRS      = . tests
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN=\"SFI\" -DG_DISABLE_CONST_RETURNS
AM_CPPFLAGS += -I$(top_srcdir)

sfi_public_headers = $(strip	\
	backtrace.hh		\
	bcore.hh		\
	blob.hh			\
	cxxaux.hh		\
	datalist.hh		\
	entropy.hh		\
	formatter.hh		\
	gbsearcharray.hh	\
	glib-extra.hh		\
	path.hh			\
	platform.hh		\
	randomhash.hh		\
	sfi.hh			\
	sficomport.hh		\
	sficomwire.hh		\
	sficxx.hh		\
	sfifilecrawler.hh	\
	sfiglue.hh		\
	sfigluecodec.hh		\
	sfiglueproxy.hh		\
	sfimemory.hh		\
	sfinote.hh		\
	sfiparams.hh		\
	sfiprimitives.hh	\
	sfiring.hh		\
	sfiserial.hh		\
	sfistore.hh		\
	sfitime.hh		\
	sfitypes.hh		\
	sfiustore.hh		\
	sfivalues.hh		\
	sfivisitors.hh		\
	sfivmarshal.hh		\
	sfiwrapper.hh		\
	strings.hh		\
	testing.hh		\
	unicode.hh		\
)
sfi_all_sources = $(strip	\
	backtrace.cc		\
	bcore.cc		\
	blob.cc			\
	datalist.cc		\
	entropy.cc		\
	formatter.cc		\
	glib-extra.cc		\
	path.cc			\
	platform.cc		\
	randomhash.cc		\
	sficomport.cc		\
	sficomwire.cc		\
	sfifilecrawler.cc	\
	sfiglue.cc		\
	sfigluecodec.cc		\
	sfiglueproxy.cc		\
	sfimemory.cc		\
	sfinote.cc		\
	sfiparams.cc		\
	sfiprimitives.cc	\
	sfiring.cc		\
	sfiserial.cc		\
	sfistore.cc		\
	sfitime.cc		\
	sfitypes.cc		\
	sfiustore.cc		\
	sfivalues.cc		\
	sfivisitors.cc		\
	sfivmarshal.cc		\
	sfiwrapper.cc		\
	strings.cc		\
	testing.cc		\
	unicode.cc		\
	weaksym.cc		\
)
sfi_extra_sources = $(strip \
)

# == doc_sources.lst ==
doc_sources.lst: Makefile
	$(AM_V_GEN)
	$(Q) echo $(patsubst %, sfi/%, $(sfi_public_headers) $(sfi_all_sources)) > $@
CLEANFILES += doc_sources.lst

# == libsfi-@MAJOR@.so ==
noinst_LTLIBRARIES            = libsfi-@MAJOR@.la
libsfi_@MAJOR@includedir      = $(bseincludedir)/sfi
libsfi_@MAJOR@include_HEADERS = $(sfi_public_headers) sysconfig.h
libsfi_@MAJOR@_la_SOURCES     = $(sfi_all_sources)
libsfi_@MAJOR@_la_CXXFLAGS    = $(AM_CXXFLAGS) $(SFI_CFLAGS) -DBSE_CONVENIENCE
libsfi_@MAJOR@_la_LIBADD      = $(SFI_LIBS)
$(sfi_public_headers):		sysconfig.h

#
# programs to build
#
# source files

beastbin_PROGRAMS = sfidl
sfidl_SOURCES  = sfidl.cc
sfidl_CXXFLAGS = $(AM_CXXFLAGS) $(SFI_CFLAGS) -DBSE_CONVENIENCE -DSFIDL_INTERNALS
sfidl_LDADD    = $(SFI_LIBS)
# included files
sfidl_includes  = sfidl-generator.cc sfidl-namespace.cc sfidl-options.cc sfidl-parser.cc sfidl-factory.cc
sfidl_includes += sfidl-typelist.cc sfidl-cbase.cc sfidl-clientc.cc sfidl-clientcxx.cc sfidl-corec.cc
sfidl_includes += sfidl-corecxx.cc sfidl-cxxbase.cc sfidl-hostc.cc sfidl-utils.cc
# headers
sfidl_includes += sfidl-generator.hh sfidl-namespace.hh sfidl-options.hh sfidl-parser.hh sfidl-factory.hh
sfidl_includes += sfidl-cbase.hh sfidl-clientc.hh sfidl-clientcxx.hh sfidl-cxxbase.hh sfidl-hostc.hh sfidl-utils.hh

# == zres.cc ==
include ../res/Makefile.sub
# ^^^^^ FIXME: the above should only be included by the toplevel Makefile.am
MISC_PACKRES = $(top_srcdir)/misc/packres.py
zres.cc: $(top_srcdir)/res/resfiles.list $(MISC_PACKRES) # res_resfiles_list contains res/resfiles.list
	$(AM_V_GEN)
	$(Q) $(MISC_PACKRES) -s '.*/res/' $(res_resfiles_list:%=$(top_srcdir)/res/%) > $@.tmp
	$(Q) mv $@.tmp $@
CLEANFILES += zres.cc
bcore.cc: zres.cc
