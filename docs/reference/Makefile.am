## Process this file with automake to produce Makefile.in

DOC_SOURCE_GLOB = $(top_srcdir)/bse/bse*.[hcp]*

MAN_PAGES_3 = gsl.3 bse.3 bse-procs.3 bst.3
DATA_FILES = gsl.dat bse.dat bse-procs.dat bst.dat

BSE_MAN_SRC_GLOBS=$(top_srcdir)/bse/bse*.[hc]
GSL_MAN_SRC_GLOBS=$(top_srcdir)/bse/gsl*.[hc]
BST_MAN_SRC_GLOBS=$(top_srcdir)/beast-gtk/bst*.[hc]

TAGS2DOC = $(srcdir)/../tags2doc.pl

EXTRA_DIST += scandocs.pl $(MAN_PAGES_3) $(DATA_FILES)

all-local: $(DATA_FILES) $(MAN_PAGES_3)

TEMP_FILES = temp-bsw-gen1.doc temp-bsw-gen2.doc

#
# documentation data generation
#
gsl.dat: $(GSL_MAN_SRC_GLOBS) $(srcdir)/scandocs.pl
	$(srcdir)/scandocs.pl --name "GSL-Functions" \
                            --blurb "GSL Function Reference" \
                            --package "BEAST-$(VERSION)" \
                            $(GSL_MAN_SRC_GLOBS) >$@
bse.dat: $(BSE_MAN_SRC_GLOBS) $(srcdir)/scandocs.pl
	$(srcdir)/scandocs.pl --name "BSE-Functions" \
                            --blurb "BSE Function Reference" \
                            --package "BEAST-$(VERSION)" \
                            $(BSE_MAN_SRC_GLOBS) >$@
bse-procs.dat: $(top_builddir)/bse/bsequery $(srcdir)/scandocs.pl
	$(top_builddir)/bse/bsequery -p procdoc >temp-bsw-gen1.doc \
        && $(srcdir)/scandocs.pl --name "BSE-Procedures" \
                               --blurb "BSE Procedure Reference" \
                               --package "BEAST-$(VERSION)" \
                               temp-bsw-gen1.doc >$@ \
        && rm -f temp-bsw-gen1.doc
bst.dat: $(BST_MAN_SRC_GLOBS) $(srcdir)/scandocs.pl
	$(srcdir)/scandocs.pl --name "BST-Functions" \
                            --blurb "BST Function Reference" \
                            --package "BEAST-$(VERSION)" \
                            $(BST_MAN_SRC_GLOBS) >$@


#
# man page generation
#
MAN_DEPS = $(TAGS2DOC) $(srcdir)/man-conf.pl
DOC2MAN = $(TAGS2DOC) --config $(srcdir)/man-conf.pl
gsl.3:		gsl.dat $(MAN_DEPS)
	$(DOC2MAN) < gsl.dat > $@
bse.3:		bse.dat $(MAN_DEPS)
	$(DOC2MAN) < bse.dat > $@
bse-procs.3:	bse-procs.dat $(MAN_DEPS)
	$(DOC2MAN) < bse-procs.dat > $@
bst.3:		bst.dat $(MAN_DEPS)
	$(DOC2MAN) < bst.dat > $@


#
# clean up
#
clean-local:
	rm -f $(TEMP_FILES) $(MAN_PAGES_3) $(DATA_FILES)
