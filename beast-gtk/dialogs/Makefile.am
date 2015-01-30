# BEAST - Better Audio System
include $(top_srcdir)/Makefile.decl

RAPIDRES     = rapidres

RESOURCES = $(strip \
	$(srcdir)/radgets-standard.xml	\
	$(srcdir)/radgets-beast.xml	\
)
XML_FILES  = $(filter-out bst-%, $(RESOURCES))
EXTRA_DIST += $(XML_FILES)


XMLLINTCALL=$(if $(XMLLINT),$(XMLLINT),true) # handle undefined XMLLINT

beast-resources.cc: $(XML_FILES)
	cd .						\
	&& $(XMLLINTCALL) --noout $(XML_FILES)		\
	&& $(RAPIDRES) $(RESOURCES) > xgen-bxc		\
	&& cp xgen-bxc $@				\
	&& rm -f xgen-bxc
CLEANFILES += xgen-bxc beast-resources.cc

noinst_DATA = beast-resources.cc
