SUBDIRS = # none

docsdir = ${beastdocsdir}
docs_DATA = ${public_docs}

public_docs = $(strip		\
	faq.markup		\
	release-notes.markup	\
	news.markup		\
	gsl-mplan.markup	\
	quickstart.markup	\
)
build_targets = $(strip		\
	faq.markup		\
	release-notes.markup	\
	news.markup		\
	gsl-mplan.markup	\
	quickstart.markup	\
)
html_targets = $(patsubst %.markup, %.html, $(build_targets))
texi_docs = $(strip			\
	$(srcdir)/../faq.texi		\
	$(srcdir)/../quickstart.texi	\
	$(srcdir)/../release-notes.texi	\
)
build_tools = $(strip		\
	html.xsl		\
	markup.xsl		\
	texinfo.dtd		\
)

if HAVE_XMLMAKEINFO_AND_XSLTPROC
%.xml: $(srcdir)/../%.texi
	$(XMLMAKEINFO) -I $(srcdir)/.. --xml $< -o $@
%.markup: %.xml $(srcdir)/markup.xsl $(srcdir)/texinfo.dtd
	$(XSLTPROC) --stringparam revision "`date -r $<`" $(srcdir)/markup.xsl $< >$@
%.html: %.xml $(srcdir)/html.xsl $(srcdir)/texinfo.dtd
	$(XSLTPROC) --stringparam revision "`date -r $<`" $(srcdir)/html.xsl $< >$@
gsl-mplan.xml: $(top_srcdir)/bse/gsl-mplan.txt
	cd . \
	&& echo '<texinfo>' > $@ \
	&& echo '<settitle>GSL Flow Engine</settitle><preformat><para>' >> $@ \
	&& sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' < $< >> $@ \
	&& echo '</para></preformat></texinfo>' >> $@
news.xml: $(top_srcdir)/NEWS
	cd . \
	&& echo '<texinfo>' > $@ \
	&& echo '<settitle>BEAST/BSE NEWS</settitle><preformat><para>' >> $@ \
	&& sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' < $< >> $@ \
	&& echo '</para></preformat></texinfo>' >> $@
conditional_clean = $(build_targets) gsl-mplan.xml news.xml
endif

html-targets:	$(html_targets)

.PHONY: html-tagets

CLEANFILES += $(conditional_clean) $(html_targets)

MAINTAINERCLEANFILES += # $(build_tools)

EXTRA_DIST = ${public_docs} ${build_tools}
