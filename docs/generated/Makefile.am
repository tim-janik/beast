# BEAST - Bedevilled Audio System
# Copyright (C) 1998-2002 Tim Janik
#
## Makefile.am for generated documentation

SUBDIRS = # none

docsdir = ${beastdocsdir}
docs_DATA = ${markup_targets}

# install man pages
man_MANS = sfi.3 bse.3 beast-gxk.3 bse-procs.3 bse-structs.3

# installed docs
markup_targets = $(strip	\
	faq.markup		\
	release-notes.markup	\
	release-news.markup	\
	gsl-mplan.markup	\
	quickstart.markup	\
	beast-index.markup	\
	$(patsubst %.3, %.3.markup, $(man_MANS)) \
)

# website html targets
html_targets = $(strip		\
	news.html		\
	oldnews.html		\
	about.html		\
	download.html		\
	contact.html		\
	resources.html		\
	links-related.html	\
	links-synthesis.html	\
	documentation.html	\
	development.html	\
	historic.html		\
	log-beast.html		\
	log-bse.html		\
	log-plugins.html	\
	log-sfi.html		\
	release-news.html	\
	$(patsubst %.3, %.3.html, $(man_MANS)) \
	faq.html		\
	quickstart.html		\
	gsl-mplan.html		\
)

# extra dist
build_tools = $(strip		\
	navigation.xsl          \
	reference-index.xsl     \
	man.xsl			\
	html.xsl		\
	markup.xsl		\
	markup.dtd		\
	texinfo.dtd		\
	scandocs.pl		\
	log2texi.sh		\
)
EXTRA_DIST += ${build_tools}

#
# reference docu generation
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
temp_ref_docu = sfi.3.texi bse.3.texi beast-gxk.3.texi bse-procs.3.texi bse-structs.3.texi


#
# ChangeLog generation
#
LOG2TEXI=$(srcdir)/log2texi.sh
log-beast.texi: $(top_srcdir)/ChangeLog			$(LOG2TEXI)
	$(LOG2TEXI) $< >$@ "BEAST"
log-bse.texi: $(top_srcdir)/bse/ChangeLog		$(LOG2TEXI)
	$(LOG2TEXI) $< >$@ "BSE"
log-plugins.texi: $(top_srcdir)/plugins/ChangeLog	$(LOG2TEXI)
	$(LOG2TEXI) $< >$@ "Plugins"
log-sfi.texi: $(top_srcdir)/sfi/ChangeLog		$(LOG2TEXI)
	$(LOG2TEXI) $< >$@ "SFI"
temp_log2texi = log-beast.texi log-bse.texi log-plugins.texi log-sfi.texi


#
# texi/xml/target rules
#
XMLANTISPACE=$(top_builddir)/docs/utils/xmlantispace
REFERENCE_XMLS = $(patsubst %.3, %.3.crossfix.xml, $(man_MANS))
if HAVE_DOC_TOOLS
gnome-index.lst.gz: # default-generate if not present to avoid build breakage
	echo | gzip -9 >$@
gnome-index.lst: $(srcdir)/gnome-index.lst.gz	# uncompress rule
	cat $< | gzip -d >$@
# generate *.xml from *.texi with makeinfo gnome-index
%.xml: %.crossfix.xml $(srcdir)/reference-filter.pl index.lst gnome-index.lst
	$(srcdir)/reference-filter.pl 		\
		--index index.lst		\
		--index gnome-index.lst		\
		$< >$@
%.crossfix.xml: %.texi		$(srcdir)/../teximacros.texi $(XMLANTISPACE)
	$(XMLMAKEINFO)  -D "TexiStamp `date -r $<`"	\
			-I $(srcdir)/.. --xml $< -o -	\
	| $(XMLANTISPACE) >$@
index.lst: $(REFERENCE_XMLS) $(srcdir)/reference-index.xsl
	rm -f $@ && touch $@
	for i in $(REFERENCE_XMLS) ; do					\
	  $(XSLTPROC)							\
	    --stringparam protocol "`echo beast-man://3/$$i		\
                                     | sed -e 's/.3.crossfix.xml//'`"	\
	    $(srcdir)/reference-index.xsl $$i >>$@ ;			\
	done
# *.markup from xml (program docs)
%.markup: %.xml			$(srcdir)/markup.xsl $(srcdir)/texinfo.dtd
	$(XSLTPROC) $(srcdir)/markup.xsl $< >$@
# man from xml (installed)
%.3: %.3.xml			$(srcdir)/man.xsl $(srcdir)/texinfo.dtd
#       Below code's purpose is to extract valuable info from
#       man files and then to combine them into one file.
#
#       TODO After we source files, especially TOC, page numbers
#       change, so there should be another step to calculate the
#       final numbers.
#
#       @XR@, @IF@, and @TF@ stands for the places where certion
#       info is included into the main document, ie. @XR@ means
#       'put xref info here'.
#
#       First, we create a nroff file from the xml source, and
#       remove @XR@, and such lines as we don't have anything to do
#       with them yet.
	$(XSLTPROC) --param man_section 3 $(srcdir)/man.xsl $< \
	| sed -e '/@XR@/ d' \
	      -e '/@IF@/ d' \
	      -e '/@TF@/ d' >$@.tmp
#       We pass this file thru groff and catch its stderr which
#       contains the info we are interested into.
	$(GROFF) -man -t $@.tmp >/dev/null 2>$@.err
#       Each type of info line has its specific prefix.  We create
#       separate files according to those prefixes and remove them.
	-grep '^XR: ' $@.err | cut -c5- >$@.xrf
	-grep '^IX: ' $@.err | cut -c5- >$@.ind
	-grep '^TC: ' $@.err | cut -c5- >$@.toc
#       We create another nroff file from the xml source, but this
#       time we substitute @XR@ and such lines with filenames to be
#       sourced at those points.
	$(XSLTPROC) --param man_section 3 $(srcdir)/man.xsl $< \
	| sed -e 's/@XR@/.so $@.xrf/g' \
	      -e 's/@IF@/.so $@.ind/g' \
	      -e 's/@TF@/.so $@.toc/g' >$@.tmp
#       Soelim loses the first line of the nroff file, which contains
#       preprocessor info for `man'.
	echo '.\" t' >$@
#       Soelim processes .so lines and sources files into a big one
#       which contains all of them.
	$(SOELIM) <$@.tmp >>$@
#       Remove scratch files.
	rm -f $@.err $@.xrf $@.ind $@.toc $@.tmp
# html from xml (website)
%.html: %.xml	texinfo.dtd html.xsl navigation.xsl navigation.xml
	$(XSLTPROC) --stringparam this_file "$@" \
	            --stringparam images_prefix "cvs-images/" \
	            $(srcdir)/html.xsl $< >$@
# postscript rules
%.nroff: %.xml man.xsl
#       See the %.3 rule above for an explanation of these steps
	$(XSLTPROC) $(srcdir)/man.xsl $< \
	| sed -e '/@XR@/ d' \
	      -e '/@IF@/ d' \
	      -e '/@TF@/ d' >$@.tmp
	$(GROFF) -man -t $@.tmp >/dev/null 2>$@.err
	-grep '^XR: ' $@.err | cut -c5- >$@.xrf
	-grep '^IX: ' $@.err | cut -c5- >$@.ind
	-grep '^TC: ' $@.err | cut -c5- >$@.toc
	$(XSLTPROC) $(srcdir)/man.xsl $< \
	| sed -e 's/@XR@/.so $@.xrf/g' \
	      -e 's/@IF@/.so $@.ind/g' \
	      -e 's/@TF@/.so $@.toc/g' >$@.tmp
	echo '.\" t' >$@
	$(SOELIM) <$@.tmp >>$@
	rm -f $@.err $@.xrf $@.ind $@.toc $@.tmp
%.ps: %.nroff
	$(GROFF) -man -t $< >$@
%.ps: %.3
	$(GROFF) -man -t $< >$@
%.pdf: %.ps
	ps2pdf13 $< $@
# where to get *.texi docs from
%.texi: $(srcdir)/../%.texi
	sed -e "s#@sedfix{top_srcdir}#${top_srcdir}#" <$< >$@
	touch $@ -r $<
# where to get website *.texi from
%.texi: $(srcdir)/../site-%.texi
	sed -e "s#@sedfix{top_srcdir}#${top_srcdir}#" <$< >$@
	touch $@ -r $<
# where to get website *.xml from
%.xml: $(srcdir)/../site-%.xml
	cp -p $< $@
# plain text generation rules
gsl-mplan.texi: $(top_srcdir)/bse/gsl-mplan.txt Makefile
	cd . \
	&& echo "\input texinfo"		 >$@ \
	&& echo "@settitle GSL Flow Engine"	>>$@ \
	&& echo "@include teximacros.texi" 	>>$@ \
	&& echo "@example"			>>$@ \
	&& echo "@verbatiminclude $<"		>>$@ \
	&& echo "@end example"			>>$@ \
	&& echo "@bye"				>>$@
release-news.texi: $(top_srcdir)/NEWS Makefile
	cd . \
	&& echo "\input texinfo"		>$@ \
	&& echo "@settitle BEAST/BSE NEWS"	>>$@ \
	&& echo "@include teximacros.texi" 	>>$@ \
	&& echo "@hasbanner{large}"             >>$@ \
	&& echo "@navigation{navigation.xml}"   >>$@ \
	&& echo "@example"			>>$@ \
	&& echo "@verbatiminclude $<"		>>$@ \
	&& echo "@end example"			>>$@ \
	&& echo "@bye"				>>$@
# cleanup
conditional_clean = $(strip \
	navigation.xml		gnome-index.lst		\
	gsl-mplan.texi		release-news.texi	\
	$(markup_targets)	$(man_MANS)		\
	$(temp_ref_docu)	$(temp_log2texi)	\
)
endif
CLEANFILES += $(conditional_clean)


#
# website generation
#
ACCOUNT=${USER}@beast.gtk.org
XPREFIX=_tmpFile~
index.html: news.html
	ln -s $< $@
site:	$(html_targets) navigation.xsl navigation.xml index.html
	tar jcf beast-site-con.tar.bz2 --mode=0664 $(html_targets) index.html css/*.css
	tar zcf beast-site-img.tar.gz  --mode=0664 css/images/*.png
	tar zcf beast-site-nav.tar.gz  --mode=0664 navigation.xml navigation.xsl
site-update-con: site
	echo Connecting to ${ACCOUNT}... \
	&& uuencode ${XPREFIX}-beast-site-con.tar.bz2 < beast-site-con.tar.bz2 \
	| ssh ${ACCOUNT} \
	  "echo Receiving... \
	   && uudecode \
	   && echo Extracting... \
	   && tar jxmpf ${XPREFIX}-beast-site-con.tar.bz2 -C /web/beast/html \
	   && echo Cleaning up. \
	   && rm ${XPREFIX}-beast-site-con.tar.bz2"
site-update-img: site
	echo Connecting to ${ACCOUNT}... \
	&& uuencode ${XPREFIX}-beast-site-img.tar.gz < beast-site-img.tar.gz \
	| ssh ${ACCOUNT} \
	  "echo Receiving... \
	   && uudecode \
	   && echo Extracting... \
	   && tar zxmpf ${XPREFIX}-beast-site-img.tar.gz -C /web/beast/html \
	   && chmod g+w /web/beast/html/css /web/beast/html/css/images \
	   && echo Cleaning up. \
	   && rm ${XPREFIX}-beast-site-img.tar.gz"
site-update-nav: site
	echo Connecting to ${ACCOUNT}... \
	&& uuencode ${XPREFIX}-beast-site-nav.tar.gz < beast-site-nav.tar.gz \
	| ssh ${ACCOUNT} \
	  "echo Receiving... \
	   && uudecode \
	   && echo Extracting... \
	   && tar zxpf ${XPREFIX}-beast-site-nav.tar.gz -C /web/beast/dyn \
	   && echo Cleaning up... \
	   && rm ${XPREFIX}-beast-site-nav.tar.gz \
	   && echo Rebuilding... \
	   && (cd /web/beast/dyn && make)"
site-update: site-update-con # site-update-img site-update-nav
site-upgrade: site-update-con site-update-img site-update-nav
CLEANFILES += beast-site-*.tar.gz beast-site-*.tar.bz2 index.html


#
# extra rules
#
clean-all-targets:	# clean targets that aren't shipped with tarballs
	rm -f *.xml *.ps *.pdf *.html index.lst
# postscript and pdf variants
ps-targets:	$(patsubst %.3, %.ps, $(man_MANS))
pdf-targets:	$(patsubst %.3, %.pdf, $(man_MANS))
# xml linting
lint: $(markup_targets)
	@for i in $^ ; do \
	  echo "xmllint --noout --loaddtd --valid $$i" ; \
	  xmllint --noout --loaddtd --valid $$i ; \
	done

MAINTAINERCLEANFILES +=

# intermediate files we don't need to keep
.INTERMEDIATE: $(strip \
	release-news.texi gsl-mplan.texi navigation.xml index.html	\
	log-beast.texi log-bse.texi log-plugins.texi log-sfi.texi	\
	$(REFERENCE_XMLS) \
)

# non-file rules
.PHONY: site ps-targets pdf-targets lint clean-all-targets
clean: clean-all-targets
