SUBDIRS = # none

docsdir = ${beastdocsdir}
docs_DATA = ${public_docs}

public_docs = $(strip \
	faq.markup \
	release-notes.markup \
	gsl-mplan.markup \
)
built_sources = $(strip \
	faq.markup \
	release-notes.markup \
	gsl-mplan.markup \
)
doc_sources = $(strip \
	faq.dat \
	release-notes.dat \
)
build_tools = $(strip \
	faq-conf.pl \
	faq-header.markup \
)


TAGS2DOC = $(srcdir)/../tags2doc.pl

faq.markup: $(TAGS2DOC) $(srcdir)/faq-header.markup $(srcdir)/faq.dat
	$(TAGS2DOC) --config $(srcdir)/faq-conf.pl < $(srcdir)/faq.dat > $@

release-notes.markup: $(srcdir)/release-notes.dat $(srcdir)/../../NEWS
	cat $(srcdir)/release-notes.dat $(srcdir)/../../NEWS > $@

# ok, this is a bit odd, gsl-mplan.txt sits in bse/ for a couple reasons
# so we need to pull it in here
GSL_MPLAN = $(srcdir)/../../bse/gsl-mplan.txt
gsl-mplan.markup: $(GSL_MPLAN)
	cat $(GSL_MPLAN) > $@

CLEANFILES += $(built_sources)
# MAINTAINERCLEANFILES += 

EXTRA_DIST = ${public_docs} ${doc_sources} $(build_tools)
