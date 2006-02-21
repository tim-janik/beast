# BEAST - Bedevilled Audio System
# Copyright (C) 1998-2006 Tim Janik
#
# GNU General Public License version 2 or (at your option) any later version.
#
## Makefile.am for beast/docs/

SUBDIRS = images

STYLE_TARGETS = $(strip				\
	html/style/beast-style.css		\
	html/style/plain.css			\
	html/style/beast-dot.png		\
	html/style/beast-small.png		\
	html/style/title-arrow-24x64.png	\
	html/style/home-arrow-24x64.png		\
	\
	html/beast.1.html			\
	html/bsesh.1.html			\
	html/sfidl.1.html			\
)
CLEANFILES += ${STYLE_TARGETS}
HTML_TARGETS = $(strip				\
	html/architecture.html			\
	html/beast-index.html			\
	html/engine-mplan.html			\
	html/faq.html				\
	html/plugin-devel.html			\
	html/quickstart.html			\
	html/release-notes.html			\
	html/sfidl-manual.html			\
)
CLEANFILES += ${HTML_TARGETS}

htmldocsdir = ${beastdocsdir}
htmldocs_DATA = ${HTML_TARGETS} ${STYLE_TARGETS}

DOCFRAME_DEFS = -D DOXI_INCLUDE_FILE \"docframe.doxi\"
DOXIDEFS = -D TOP_WEBDIR "." -D BST_VERSION ${BST_VERSION} -I $(srcdir) $(call DOCFRAME_DEFS)

make-install-dirs:
	$(mkinstalldirs) $(DESTDIR)$(htmldocsdir)/html/style
	rm -f $(DESTDIR)$(htmldocsdir)/html/images
	ln -s ../../images $(DESTDIR)$(htmldocsdir)/html/images
uninstall-hook:
	rm -f $(DESTDIR)$(htmldocsdir)/html/images
uninstall: uninstall-hook

# some dependencies have to be written indirectly, to prevent automake from
# changing it's rule generation (it sometimes automagically adapts its generation).
install_htmldocsDATA_VARIABLE = install-htmldocsDATA
${install_htmldocsDATA_VARIABLE}: make-install-dirs

html/style/%: $(top_srcdir)/web/style/%
	@mkdir -p html/style/
	cp $< $@
html/%.1.html: %.1.doxi
	doxer doxi2htmlman $< -o html/ $(DOXIDEFS)
html/%.3.html: %.3.doxi
	doxer doxi2htmlman $< -o html/ $(DOXIDEFS)
html/%.html: %.doxi
	@mkdir -p html/
	doxer doxi2html $< -o html/ $(DOXIDEFS)


html/%.html: %.tmpdoxi
	doxer doxi2html $< -o html/ $(DOXIDEFS)
engine-mplan.tmpdoxi: $(top_srcdir)/bse/engine-mplan.txt # Makefile
	cd . \
	&& echo '@doxer_dnl'                                    >$@  \
	&& echo '@doxer_set{webmenu} development'               >>$@ \
	&& echo '@include @doxer_get{DOXI_INCLUDE_FILE}'        >>$@ \
	&& echo '@doxer_set{title} BEAST - BSE Engine'          >>$@ \
	&& echo '@subheading'                                   >>$@ \
	&& echo '@example'                                      >>$@ \
	&& sed  's/@/@@/g'              <$<                     >>$@ \
	&& echo '@done'                                         >>$@

intermediate_files = $(strip    \
        news-file.tmpdoxi       \
        engine-mplan.tmpdoxi    \
        bse-objects.scad        \
        bse-objects.tmpdoxi     \
        bse-interface.scad      \
        bse-interface.tmpdoxi   \
)
.INTERMEDIATE: $(intermediate_files)


doxi_files = $(strip		\
	architecture.doxi	\
	beast.1.doxi		\
	faq.doxi		\
	quickstart.doxi		\
	sfidl-manual.doxi	\
	beast-index.doxi	\
	bsesh.1.doxi		\
	plugin-devel.doxi	\
	release-notes.doxi	\
	sfidl.1.doxi		\
)

EXTRA_DIST += $(strip		\
	$(doxi_files)		\
	bse-categories.txt	\
	interpolation.txt	\
)
