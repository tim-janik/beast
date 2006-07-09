# beast/tests
# Copyright (C) 2003 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = latency scripts bse

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(SFI_CFLAGS)
DEFS += -DG_LOG_DOMAIN="\"BEAST-TEST\"" # -DG_DISABLE_CONST_RETURNS
SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)



# check for .scm i18n strings
# 
# check for the presence of i18n strings extracted from .scm files.
# this test should be in po/Makefile.am if we had that.
check-local: check-scm-strings
.PHONY: check-scm-strings
check-scm-strings:
	@for lang in $(ALL_LINGUAS) ; do			\
	  file=$(top_srcdir)/po/$$lang.po ;			\
	  echo "CHECK: find scm strings in $$lang.po" ;		\
	  for pattern in $(PATTERNS) ; do			\
	    grep -Fq $$pattern $$file ||			\
	      { echo "$$file: misses strings from: $$pattern" ;	\
	        exit 1 ; }					\
	  done ;						\
	done
PATTERNS=$(strip					\
	library/scripts/mixer-splitup-by-track.scm	\
	library/scripts/modules2grid.scm		\
	library/scripts/part-harmonic-transposer.scm	\
	library/scripts/progressor-example.scm		\
	library/scripts/record-midi.scm			\
	library/scripts/song-parts-operations.scm	\
)
