# BEAST - Bedevilled Audio System
# Copyright (C) 1998-2002 Tim Janik
#
## Makefile.am for generated documentation

SUBDIRS = # none

docsdir = ${beastdocsdir}
docs_DATA = ${markup_targets}

# install man pages
man_MANS = sfi.3 bse.3 beast-gxk.3 bse-procs.3 bse-structs.3
ps_targets = $(patsubst %.3, %.ps, $(man_MANS))
# installed docs
markup_targets = $(strip	\
	faq.markup		\
	release-notes.markup	\
	news.markup		\
	gsl-mplan.markup	\
	quickstart.markup	\
	beast-index.markup	\
	sfi.3.markup		\
	bse.3.markup		\
	beast-gxk.3.markup	\
	bse-procs.3.markup	\
	bse-structs.3.markup	\
)
# website html generation
html_targets = $(patsubst %.markup, %.html, $(markup_targets)) # $(patsubst %.3, %.html, $(man_MANS))
# extra dist
build_tools = $(strip		\
	man.xsl			\
	html.xsl		\
	markup.xsl		\
	markup.dtd		\
	texinfo.dtd		\
	scandocs.pl		\
)
EXTRA_DIST += ${build_tools}

#
# reference documentation
#
sfi_refsrc_globs=$(top_srcdir)/sfi/sfi*.[hc]
bse_refsrc_globs=$(top_srcdir)/bse/bse*.[hc] $(top_srcdir)/bse/gsl*.[hc]
gxk_refsrc_globs=$(top_srcdir)/beast-gtk/gxk/gxk*.[hc]
AUTODOC=$(top_builddir)/bse/bseautodoc
SCANDOCS=$(srcdir)/scandocs.pl

sfi.3.texi: $(sfi_refsrc_globs) $(SCANDOCS) $(top_srcdir)/configure
	$(SCANDOCS) --name "SFI-Functions" \
		    --blurb "SFI Function Reference" \
		    --package "BEAST-$(VERSION)" \
		      $(sfi_refsrc_globs) >$@
bse.3.texi: $(bse_refsrc_globs) $(SCANDOCS) $(top_srcdir)/configure
	$(SCANDOCS) --name "BSE-Functions" \
		    --blurb "BSE Function Reference" \
		    --package "BEAST-$(VERSION)" \
		    --seealso "beast-man://3/bse-procs,BSE Procedures" \
		      $(bse_refsrc_globs) >$@
beast-gxk.3.texi: $(gxk_refsrc_globs) $(SCANDOCS) $(top_srcdir)/configure
	$(SCANDOCS) --name "GXK-Functions" \
		    --blurb "Gtk+ Extension Kit Function Reference" \
		    --package "BEAST-$(VERSION)" \
		      $(gxk_refsrc_globs) >$@
bse-procs.3.texi: $(AUTODOC) $(top_srcdir)/configure
	$(AUTODOC)  --seealso "beast-man://3/bse-structs,BSE Structures" \
		    procs >$@
bse-structs.3.texi: $(AUTODOC) $(top_srcdir)/configure
	$(AUTODOC)  --seealso "beast-man://3/bse-procs,BSE Procedures" \
		    structs >$@
temp_texi = sfi.3.texi bse.3.texi beast-gxk.3.texi bse-procs.3.texi bse-structs.3.texi

#
# texi/xml/target rules
#
XMLANTISPACE=$(top_builddir)/docs/utils/xmlantispace
if HAVE_XMLMAKEINFO_AND_XSLTPROC
%.texi: $(srcdir)/../%.texi
	cp -a $< $@
%.xml: %.texi			$(XMLANTISPACE)
	$(XMLMAKEINFO) -I $(srcdir)/.. --xml -D "TexiStamp `date -r $<`" \
		$< -o - | $(XMLANTISPACE) >$@
%.markup: %.xml			$(srcdir)/markup.xsl $(srcdir)/texinfo.dtd
	$(XSLTPROC) $(srcdir)/markup.xsl $< >$@
%.html: %.xml			$(srcdir)/html.xsl $(srcdir)/texinfo.dtd
	$(XSLTPROC) $(srcdir)/html.xsl $< >$@
%.3: %.3.xml			$(srcdir)/man.xsl $(srcdir)/texinfo.dtd
	$(XSLTPROC) --param man_section 3 $(srcdir)/man.xsl $< >$@
%.nroff: %.xml man.xsl
	$(XSLTPROC) $(srcdir)/man.xsl $< >$@
%.ps: %.nroff
	groff -man -t $< >$@
%.ps: %.3
	groff -man -t $< >$@
gsl-mplan.texi: $(top_srcdir)/bse/gsl-mplan.txt
	cd . \
	&& echo "\input texinfo"		 >$@ \
	&& echo "@settitle GSL Flow Engine"	>>$@ \
	&& echo "@example"			>>$@ \
	&& echo "@verbatiminclude $<"		>>$@ \
	&& echo "@end example"			>>$@ \
	&& echo "@bye"				>>$@
news.texi: $(top_srcdir)/NEWS
	cd . \
	&& echo "\input texinfo"		>$@ \
	&& echo "@settitle BEAST/BSE NEWS"	>>$@ \
	&& echo "@example"			>>$@ \
	&& echo "@verbatiminclude $<"		>>$@ \
	&& echo "@end example"			>>$@ \
	&& echo "@bye"				>>$@
conditional_clean = $(markup_targets) gsl-mplan.texi news.texi faq.xml $(man_MANS) $(temp_texi)
endif

CLEANFILES += $(conditional_clean) $(html_targets) $(ps_targets)
MAINTAINERCLEANFILES +=

#
# extra rules
#
html-targets:	$(html_targets)
ps-targets: $(ps_targets)
lint:
	for i in $(markup_targets) ; do \
		xmllint --noout --loaddtd --valid $$i ; \
	done
.PHONY: html-targets lint
