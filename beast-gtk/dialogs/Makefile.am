# BEAST - Bedevilled Audio System
# Copyright (C) 2003 Tim Janik
#
## Makefile.am for beast dialog resources


ZFILE_DEFS = $(strip \
	BST_GADGETS_STANDARD	$(srcdir)/gadgets-standard.xml	\
	BST_GADGETS_BEAST	$(srcdir)/gadgets-beast.xml	\
)
XML_FILES  = $(filter-out BST_%, $(ZFILE_DEFS))
EXTRA_DIST = $(XML_FILES)

ZINTERN=$(top_builddir)/bse/zintern/zintern

xmllint:
	$(WANT_XMLLINT)
if MAINTAINER_MODE
WANT_XMLLINT=xmllint --noout $(XML_FILES)
endif

beast-xml-zfiles.c:	$(ZINTERN) $(srcdir)/*.xml
	$(WANT_XMLLINT) 
	$(ZINTERN) -b -z $(ZFILE_DEFS) > xgen-bxc	\
	&& cp xgen-bxc $@				\
	&& rm -f xgen-bxc
CLEANFILES += xgen-bxc beast-xml-zfiles.c

noinst_DATA = beast-xml-zfiles.c
