# BEAST - Bedevilled Audio System
# Copyright (C) 2003 Tim Janik
#
## Makefile.am for beast dialog resources


ZFILE_DEFS = $(strip \
	bst-radgets-standard	$(srcdir)/radgets-standard.xml	\
	bst-radgets-beast	$(srcdir)/radgets-beast.xml	\
)
XML_FILES  = $(filter-out bst-%, $(ZFILE_DEFS))
EXTRA_DIST += $(XML_FILES)

ZINTERN=$(top_builddir)/bse/zintern/zintern

XMLLINTCALL=$(if $(XMLLINT),$(XMLLINT),true) # handle undefined XMLLINT

beast-xml-zfiles.c: $(XML_FILES) $(ZINTERN)
	cd .						\
	&& $(XMLLINTCALL) --noout $(XML_FILES)		\
	&& $(ZINTERN) -b -z $(ZFILE_DEFS) > xgen-bxc	\
	&& cp xgen-bxc $@				\
	&& rm -f xgen-bxc
CLEANFILES += xgen-bxc beast-xml-zfiles.c

noinst_DATA = beast-xml-zfiles.c
