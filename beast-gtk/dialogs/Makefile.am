# BEAST - Bedevilled Audio System
# Copyright (C) 2003 Tim Janik
#
## Makefile.am for beast dialog resources


ZFILE_DEFS = $(strip \
	BST_RADGETS_STANDARD	$(srcdir)/radgets-standard.xml	\
	BST_RADGETS_BEAST	$(srcdir)/radgets-beast.xml	\
)
XML_FILES  = $(filter-out BST_%, $(ZFILE_DEFS))
EXTRA_DIST = $(XML_FILES)

ZINTERN=$(top_builddir)/bse/zintern/zintern

xmllint: $(XML_FILES)
	cd .					\
	&& test -z "$(XMLLINT)"			\
	|| $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint

beast-xml-zfiles.c: $(XML_FILES) xmllint $(ZINTERN)
	$(ZINTERN) -b -z $(ZFILE_DEFS) > xgen-bxc	\
	&& cp xgen-bxc $@				\
	&& rm -f xgen-bxc
CLEANFILES += xgen-bxc beast-xml-zfiles.c

noinst_DATA = beast-xml-zfiles.c
