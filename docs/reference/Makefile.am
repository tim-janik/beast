## Process this file with automake to produce Makefile.in

DOC_SOURCE_GLOB = $(top_srcdir)/bse/bse*.[hcp]*

HTML_PAGES = bse-procs.html bse.html gsl.html bst.html
MAN_PAGES_3 = bse-procs.3 bse.3 gsl.3 bst.3
BSE_MAN_SRC_GLOBS=$(top_srcdir)/bse/bse*.[hc]
GSL_MAN_SRC_GLOBS=$(top_srcdir)/bse/gsl*.[hc]
BST_MAN_SRC_GLOBS=$(top_srcdir)/beast-gtk/bst*.[hc]

EXTRA_DIST += mkdocs.pl $(MAN_PAGES_3) $(HTML_PAGES)

all-local: $(MAN_PAGES_3) $(HTML_PAGES)

TEMP_FILES = temp-bsw-gen.doc

gsl.3: $(GSL_MAN_SRC_GLOBS) $(srcdir)/mkdocs.pl
	$(srcdir)/mkdocs.pl --name "GSL-Functions" \
	                    --blurb "GSL Function Reference" \
			    --package "GSL (BSE-$(VERSION))" \
	                    $(GSL_MAN_SRC_GLOBS) >$@

bse.3: $(BSE_MAN_SRC_GLOBS) $(srcdir)/mkdocs.pl
	$(srcdir)/mkdocs.pl --name "BSE-Functions" \
	                    --blurb "BSE Function Reference" \
			    --package "BEAST/BSE-$(VERSION)" \
	                    $(BSE_MAN_SRC_GLOBS) >$@

bse-procs.3: $(top_builddir)/bse/bsequery $(srcdir)/mkdocs.pl
	$(top_builddir)/bse/bsequery -p procdoc >temp-bsw-gen.doc \
	&& $(srcdir)/mkdocs.pl --name "BSE-Procedures" \
	                       --blurb "BSE Procedure Reference" \
			       --package "BEAST/BSE-$(VERSION)" \
	                       temp-bsw-gen.doc >$@ \
	&& rm -f temp-bsw-gen.doc

bst.3: $(BST_MAN_SRC_GLOBS) $(srcdir)/mkdocs.pl
	$(srcdir)/mkdocs.pl --name "BST-Functions" \
	                    --blurb "BST Function Reference" \
			    --package "BEAST/BSE-$(VERSION)" \
	                    $(BST_MAN_SRC_GLOBS) >$@

gsl.html: $(GSL_MAN_SRC_GLOBS) $(srcdir)/mkdocs.pl
	$(srcdir)/mkdocs.pl --name "GSL-Functions" \
	                    --blurb "GSL Function Reference" \
			    --package "GSL (BSE-$(VERSION))" \
			    --html \
	                    $(GSL_MAN_SRC_GLOBS) >$@

bse.html: $(BSE_MAN_SRC_GLOBS) $(srcdir)/mkdocs.pl
	$(srcdir)/mkdocs.pl --name "BSE-Functions" \
	                    --blurb "BSE Function Reference" \
			    --package "BEAST/BSE-$(VERSION)" \
			    --html \
	                    $(BSE_MAN_SRC_GLOBS) >$@

bse-procs.html: $(top_builddir)/bse/bsequery $(srcdir)/mkdocs.pl
	$(top_builddir)/bse/bsequery -p procdoc >temp-bsw-gen.doc \
	&& $(srcdir)/mkdocs.pl --name "BSE-Procedures" \
	                       --blurb "BSE Procedure Reference" \
			       --package "BEAST/BSE-$(VERSION)" \
			       --html \
	                       temp-bsw-gen.doc >$@ \
	&& rm -f temp-bsw-gen.doc

bst.html: $(BST_MAN_SRC_GLOBS) $(srcdir)/mkdocs.pl
	$(srcdir)/mkdocs.pl --name "BST-Functions" \
	                    --blurb "BST Function Reference" \
			    --package "BEAST/BSE-$(VERSION)" \
			    --html \
	                    $(BST_MAN_SRC_GLOBS) >$@


clean-local:
	rm -f $(TEMP_FILES) $(MAN_PAGES_3)
