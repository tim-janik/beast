## Process this file with automake to produce Makefile.in

DOC_SOURCE_GLOB = $(top_srcdir)/bse/bse*.[hcp]*

MAN_PAGES_3 = bse-procs.3 bse.3 gsl.3
BSE_MAN_SRC_GLOBS=$(top_srcdir)/bse/bse*.[hc]
GSL_MAN_SRC_GLOBS=$(top_srcdir)/bse/gsl*.[hc]

EXTRA_DIST += mkdocs.pl $(MAN_PAGES_3)

all-local: $(MAN_PAGES_3)

TEMP_FILES = temp-bse.doc
bse-procs.3: $(top_builddir)/bse/bsequery $(srcdir)/mkdocs.pl
	$(top_builddir)/bse/bsequery -p procdoc >temp-bse.doc \
	&& $(srcdir)/mkdocs.pl --name "BSE-Procedures" \
	                       --blurb "BSE Procedure Reference" \
			       --package "BEAST/BSE-$(VERSION)" \
	                       temp-bse.doc >$@ \
	&& rm -f temp-bse.doc

bse.3: $(BSE_MAN_SRC_GLOBS) $(srcdir)/mkdocs.pl
	$(srcdir)/mkdocs.pl --name "BSE-Functions" \
	                    --blurb "BSE Function Reference" \
			    --package "BEAST/BSE-$(VERSION)" \
	                    $(BSE_MAN_SRC_GLOBS) >$@

gsl.3: $(GSL_MAN_SRC_GLOBS) $(srcdir)/mkdocs.pl
	$(srcdir)/mkdocs.pl --name "GSL-Functions" \
	                    --blurb "GSL Function Reference" \
			    --package "GSL (BSE-$(VERSION))" \
	                    $(GSL_MAN_SRC_GLOBS) >$@

clean-local:
	rm -f $(TEMP_FILES) $(MAN_PAGES_3)
