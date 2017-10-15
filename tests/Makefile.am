# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = . latency scripts bse audio filecheck

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN="\"BEAST-TEST\"" # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(SFI_CPPFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE
AM_CFLAGS   += $(SFI_CPPFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE

SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)


# === generation targets ===
# convenience targets for generated source files
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(GENERATED) stamp-*
generated: clean-generated $(GENERATED)


# === test programs ===
noinst_PROGRAMS = $(ALLTESTS)
progs_ldadd     = ../bse/libbse-@MAJOR@.la $(LIBBSE_LIBS)
