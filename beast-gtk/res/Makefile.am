# BEAST & BSE
include $(top_srcdir)/Makefile.decl

RAPIDRES     = rapidres
XMLLINTCALL  = $(if $(XMLLINT),$(XMLLINT),true) # handle undefined XMLLINT

RESOURCES = $(strip 		\
	radgets-standard.xml	\
	radgets-beast.xml	\
)

noinst_DATA = beast-resources.cc
beast-resources.cc: $(RESOURCES)
	cd .						\
	&& $(XMLLINTCALL) --noout $(RESOURCES)		\
	&& (cd $(srcdir) && $(RAPIDRES) $(RESOURCES) ) > xgen-bxc		\
	&& cp xgen-bxc $@				\
	&& rm -f xgen-bxc
CLEANFILES += xgen-bxc beast-resources.cc
