# BEAST - Better Audio System
# Copyright (C) 2003 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

ZFILE_DEFS = $(strip \
	bst-radgets-standard	$(srcdir)/radgets-standard.xml	\
	bst-radgets-beast	$(srcdir)/radgets-beast.xml	\
)
XML_FILES  = $(filter-out bst-%, $(ZFILE_DEFS))
EXTRA_DIST += $(XML_FILES)

ZINTERN=$(top_builddir)/birnet/birnet-zintern

XMLLINTCALL=$(if $(XMLLINT),$(XMLLINT),true) # handle undefined XMLLINT

beast-xml-zfiles.cc: $(XML_FILES) $(ZINTERN)
	cd .						\
	&& $(XMLLINTCALL) --noout $(XML_FILES)		\
	&& $(ZINTERN) -b -z $(ZFILE_DEFS) > xgen-bxc	\
	&& cp xgen-bxc $@				\
	&& rm -f xgen-bxc
CLEANFILES += xgen-bxc beast-xml-zfiles.cc

noinst_DATA = beast-xml-zfiles.cc
