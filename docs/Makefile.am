# BEAST - Bedevilled Audio System
# Copyright (C) 1998-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = images

MAN_TARGETS = $(strip				\
	mans/beast.1				\
	mans/bsesh.1				\
	mans/sfidl.1				\
	mans/bse.5				\
)
CLEANFILES += ${MAN_TARGETS}
HTML_TARGETS = $(strip				\
	html/architecture.html			\
	html/beast-index.html			\
	html/bse-interface.html			\
	html/bse-objects.html			\
	html/news-file.html			\
	html/engine-mplan.html			\
	html/faq.html				\
	html/plugin-devel.html			\
	html/quickstart.html			\
	html/release-notes.html			\
	html/sfidl-manual.html			\
	\
	html/beast.1.html			\
	html/bsesh.1.html			\
	html/sfidl.1.html			\
	html/bse.5.html				\
)
CLEANFILES += ${HTML_TARGETS}
STYLE_TARGETS = $(strip				\
	html/style/beast-style.css		\
	html/style/plain.css			\
	html/style/doxer-style.css		\
	html/style/beast-dot.png		\
	html/style/beast-small.png		\
	html/style/title-arrow-24x64.png	\
	html/style/home-arrow-24x64.png		\
)
CLEANFILES += ${STYLE_TARGETS}
PSOURCE_TARGETS = $(strip			\
	psource/birnet.psrc			\
	psource/sfi.psrc			\
	psource/bse.psrc			\
	psource/gxk.psrc			\
	psource/bse-objects.psrc		\
	psource/bse-interface.psrc		\
)

DOXER = $(top_srcdir)/doxer/doxer.py
DOCFRAME_DEFS = -D DOXI_INCLUDE_FILE \"docframe.doxi\"
DOXIDEFS = -D TOP_WEBDIR "." -D BST_VERSION ${BST_VERSION} -I $(srcdir) $(call DOCFRAME_DEFS)

# noinst_DATA = ${PSOURCE_TARGETS}

# build and install MAN pages
dist_man_MANS = $(MAN_TARGETS)	# sfi.3 bse.3 beast-gxk.3 bse-procs.3 bse-structs.3

# build and install HTML docs
htmldocsdir = ${beastdocsdir}/html
htmldocs_DATA = ${HTML_TARGETS}
# build and install HTML style/ targets
styledocsdir = ${htmldocsdir}/style
styledocs_DATA = ${STYLE_TARGETS}
# link html/images/ to ordinary docs/images/
install-data-local:
	$(mkinstalldirs)   $(DESTDIR)$(htmldocsdir)
	rm -f              $(DESTDIR)$(htmldocsdir)/images
	ln -s ../../images $(DESTDIR)$(htmldocsdir)/images
uninstall-local:
	rm -f		   $(DESTDIR)$(htmldocsdir)/images

# some dependencies have to be written indirectly, to prevent automake from
# changing it's rule generation (which it sometimes automagically adapts).
##install_htmldocsDATA_VARIABLE = install-htmldocsDATA
##${install_htmldocsDATA_VARIABLE}: make-install-dirs

# CSS style sheet rules
html/style/%: $(top_srcdir)/web/style/%
	@mkdir -p html/style/
	cp $< $@
html/style/doxer-style.css:
	$(DOXER) writecss -d html/style/

# MAN generation rules
mans/%.1: %.1.doxi
	@mkdir -p mans/
	$(DOXER) doxi2man $< -d mans/ $(DOXIDEFS)
mans/%.3: %.3.doxi
	@mkdir -p mans/
	$(DOXER) doxi2man $< -d mans/ $(DOXIDEFS)
mans/%.5: %.5.doxi
	@mkdir -p mans/
	$(DOXER) doxi2man $< -d mans/ $(DOXIDEFS)

# HTML generation rules
html/%.1.html: %.1.doxi
	$(DOXER) doxi2htmlman $< -d html/ $(DOXIDEFS)
html/%.3.html: %.3.doxi
	$(DOXER) doxi2htmlman $< -d html/ $(DOXIDEFS)
html/%.html: %.doxi
	@mkdir -p html/
	$(DOXER) doxi2html $< -d html/ $(DOXIDEFS)
html/%.html: gendoxi/%.doxi
	$(DOXER) doxi2html $< -d html/ $(DOXIDEFS)

# Generated .doxi files
gendoxi/:
	mkdir -p $@
# NEWS file
gendoxi/news-file.doxi: $(top_srcdir)/NEWS # Makefile
	@mkdir -p gendoxi/
	cd . \
	&& echo '@doxer_dnl'                                    > $@ \
	&& echo '@doxer_set{webmenu} development'               >>$@ \
	&& echo '@include @doxer_get{DOXI_INCLUDE_FILE}'        >>$@ \
	&& echo '@doxer_set{title} BEAST/BSE - NEWS'            >>$@ \
	&& echo '@subheading'                                   >>$@ \
	&& echo '@example'                                      >>$@ \
	&& sed  's/@/@@/g'              <$<                     >>$@ \
	&& echo '@done'                                         >>$@
# engine-mplan
gendoxi/engine-mplan.doxi: $(top_srcdir)/bse/engine-mplan.txt # Makefile
	@mkdir -p gendoxi/
	cd . \
	&& echo '@doxer_dnl'                                    > $@ \
	&& echo '@doxer_set{webmenu} development'               >>$@ \
	&& echo '@include @doxer_get{DOXI_INCLUDE_FILE}'        >>$@ \
	&& echo '@doxer_set{title} BEAST - BSE Engine'          >>$@ \
	&& echo '@subheading'                                   >>$@ \
	&& echo '@example'                                      >>$@ \
	&& sed  's/@/@@/g'              <$<                     >>$@ \
	&& echo '@done'                                         >>$@
# BSE object docu
gendoxi/bse-objects.doxi: psource/bse-objects.psrc docu-template.doxi
	@mkdir -p gendoxi/
	$(DOXER) pickle2doxi $(strip	$<	-D TOP_WEBDIR ".."		\
	         -d gendoxi/ $(DOXIDEFS)	-T docu-template.doxi		\
	         -D DOCU_TEMPLATE_TITLE "BEAST - BSE Object Reference"		\
	         -D DOCU_TEMPLATE_HEADING "BSE Object Reference" )
# BSE interface docu
gendoxi/bse-interface.doxi: psource/bse-interface.psrc docu-template.doxi
	@mkdir -p gendoxi/
	$(DOXER) pickle2doxi $(strip	$<	-D TOP_WEBDIR ".."		\
	         -d gendoxi/ $(DOXIDEFS)	-T docu-template.doxi		\
	         -D DOCU_TEMPLATE_TITLE "BEAST - BSE Interface Reference"	\
	         -D DOCU_TEMPLATE_HEADING "BSE Interface Reference" )

# Reference Documentation
AUTODOC = $(top_builddir)/bse/bseautodoc
has_docu_filter = $(shell egrep '^/\*\*' -l $(wildcard $(1)))
make_psource = $(strip $(DOXER) src2pickle -o $(1) $(DOXIDEFS) $(2))
# BIRNET reference docu
birnet_rdocu_globs = $(top_srcdir)/birnet/birnet*.[hc] $(top_srcdir)/birnet/birnet*.[hc][hc]
birnet_rdocu_exclude = birnetconfig.h
birnet_rdocu_files = $(filter-out $(foreach f, $(birnet_rdocu_exclude), $(wildcard $(top_srcdir)/birnet/$(f))), $(wildcard $(birnet_rdocu_globs)))
psource/birnet.psrc: $(birnet_rdocu_files)
	@mkdir -p psource/
	$(call make_psource, $@, $(filter %.h %.hh, $(birnet_rdocu_files)) -Y $(filter-out %.h %.hh, $(birnet_rdocu_files)) )
# SFI reference docu
sfi_rdocu_globs = $(top_srcdir)/sfi/sfi*.[hc]
sfi_rdocu_exclude = sfidl*.*
sfi_rdocu_files = $(filter-out $(foreach f, $(sfi_rdocu_exclude), $(wildcard $(top_srcdir)/sfi/$(f))), $(wildcard $(sfi_rdocu_globs)))
psource/sfi.psrc: $(sfi_rdocu_files)
	@mkdir -p psource/
	$(call make_psource, $@, $(filter %.h %.hh, $(sfi_rdocu_files)) -Y $(filter-out %.h %.hh, $(sfi_rdocu_files)) )
# BSE reference docu
bse_rdocu_globs = $(top_srcdir)/bse/bse*.[hc] $(top_srcdir)/bse/bse*.[hc][hc]
bse_rdocu_exclude = *.gen-proc.c *.genidl.hh bsegentypes.h bseautodoc.c
bse_rdocu_files = $(filter-out $(foreach f, $(bse_rdocu_exclude), $(wildcard $(top_srcdir)/bse/$(f))), $(wildcard $(bse_rdocu_globs)))
bse_rdocu_dscrs = $(call has_docu_filter, $(bse_rdocu_files))
psource/bse.psrc: $(bse_rdocu_files)
	@mkdir -p psource/
	$(call make_psource, $@, $(filter %.h %.hh, $(bse_rdocu_files)) -Y $(filter-out %.h %.hh, $(bse_rdocu_dscrs)) )
# GXK reference docu
gxk_rdocu_globs = $(top_srcdir)/beast-gtk/gxk/gxk*.[hc] $(top_srcdir)/beast-gtk/bstutils.[hc]
gxk_rdocu_exclude = gxkgentypes.h gxkmarshal.[hc]
gxk_rdocu_files = $(filter-out $(foreach f, $(gxk_rdocu_exclude), $(wildcard $(top_srcdir)/beast-gtk/gxk/$(f))), $(wildcard $(gxk_rdocu_globs)))
psource/gxk.psrc: $(gxk_rdocu_files)
	@mkdir -p psource/
	$(call make_psource, $@, $(filter %.h %.hh, $(gxk_rdocu_files)) -Y $(filter-out %.h %.hh, $(gxk_rdocu_files)) )
# BSE object docu
psource/bse-objects.psrc: $(AUTODOC) $(top_builddir)/config.status
	@mkdir -p psource/
	cd . \
	&& $(AUTODOC) -p -s objects	>psource/bse-objects.scad	\
	&& $(DOXER) scad2pickle -o $@ psource/bse-objects.scad		\
	&& rm -f psource/bse-objects.scad
# BSE interface docu
psource/bse-interface.psrc: $(AUTODOC) $(top_builddir)/config.status
	@mkdir -p psource/
	cd . \
	&& $(AUTODOC) -p -s procs	 >psource/bse-interface.scad	\
	&& $(AUTODOC) -p -s structs     >>psource/bse-interface.scad	\
	&& $(DOXER) scad2pickle -o $@ psource/bse-interface.scad	\
	&& rm -f psource/bse-interface.scad
# Force AUTODOC builds
.PHONY: # $(AUTODOC)
CLEANFILES += psource/bse-objects.scad psource/bse-interface.scad
clean-local:
	rm -rf gendoxi/ html/ psource/ mans/

intermediate_files = $(strip    \
        news-file.tmpdoxi       \
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
