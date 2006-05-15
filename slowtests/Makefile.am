# beast/slowtests
# Copyright (C) 2003-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

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



# === BSE file check ===
# this test checks that all .bse files contained in the beast tarball
# will load without any warnings or errors being issued. this is done
# by first constructing the tarball file list via the distfile-list:-rule
# which is specified in Makefile.decl and included in each subdir, then
# filtering out bse files and finally trying to load these without warnings.
DISTFILE_LIST_NAME=$(shell pwd)/distfiles.list
distfiles.list: Makefile
	rm -f distfiles.list
	test ! -e distfiles.list
	test ! -e $(DISTFILE_LIST_NAME)
	echo > $(DISTFILE_LIST_NAME)
	test -e distfiles.list
	test -e $(DISTFILE_LIST_NAME)
	cd $(top_srcdir) && make distfile-list DISTFILE_LIST=$(DISTFILE_LIST_NAME)
.PHONY: bse-check
bse-check: distfiles.list
	cd $(top_srcdir) &&				\
	for file in `grep '\.bse$$' $(subdir)/distfiles.list` ; do	\
	  echo "Test load project: $$file" ;				\
	  $(TOPDIR_BSESHELL) "$$file" 2>&1 | tee warnings.tmp ;		\
	  test ! -s "warnings.tmp" || exit 1 ;				\
	done
	@echo "======================================="
	@echo "All stock BSE files passed loading test"
	@echo "======================================="
check-am: bse-check # run bse-check *before* ordinary tests
# check-local: bse-check # run bse-check *after* ordinary tests
TOPDIR_BSESHELL = $(strip						\
	./shell/bsesh-$(BIN_VERSION)					\
	  -p null -m null						\
	  --bse-rcfile "/dev/null" --bse-no-load			\
	  --bse-override-plugin-globs './plugins/.libs/*.so'		\
	  -s ./slowtests/checkproject.scm				\
)
