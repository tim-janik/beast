# BEAST - Bedevilled Audio System
# Copyright (C) 2003 Tim Janik
#
## Makefile.am for beast dialog resources


ZFILE_DEFS = $(strip \
	BST_GADGETS_STANDARD	$(srcdir)/gadgets-standard.xml	\
	BST_GADGETS_BEAST	$(srcdir)/gadgets-beast.xml	\
)
EXTRA_DIST  = $(filter-out BST_%, $(ZFILE_DEFS))

ZINTERN=$(top_builddir)/bse/zintern/zintern

beast-xml-zfiles.c:	$(ZINTERN) $(srcdir)/*.xml
	$(ZINTERN) -b -z $(ZFILE_DEFS) > xgen-bxc				\
	&& cp xgen-bxc $@							\
	&& rm -f xgen-bxc
CLEANFILES += xgen-bxc beast-xml-zfiles.c

noinst_DATA = beast-xml-zfiles.c
