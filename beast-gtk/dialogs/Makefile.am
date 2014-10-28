# BEAST - Better Audio System
include $(top_srcdir)/Makefile.decl

ZINTERN      = rapicorn-zintern

ZFILE_DEFS = $(strip \
	bst-radgets-standard	$(srcdir)/radgets-standard.xml	\
	bst-radgets-beast	$(srcdir)/radgets-beast.xml	\
)
XML_FILES  = $(filter-out bst-%, $(ZFILE_DEFS))
EXTRA_DIST += $(XML_FILES)


XMLLINTCALL=$(if $(XMLLINT),$(XMLLINT),true) # handle undefined XMLLINT

beast-xml-zfiles.cc: $(XML_FILES)
	cd .						\
	&& $(XMLLINTCALL) --noout $(XML_FILES)		\
	&& $(ZINTERN) -b -z $(ZFILE_DEFS) > xgen-bxc	\
	&& cp xgen-bxc $@				\
	&& rm -f xgen-bxc
CLEANFILES += xgen-bxc beast-xml-zfiles.cc

noinst_DATA = beast-xml-zfiles.cc
