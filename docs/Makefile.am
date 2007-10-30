# BEAST - Bedevilled Audio System
# Copyright (C) 1998-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = images imports

MAN_TARGETS = $(strip				\
	mans/beast.1				\
	mans/bsescm.1				\
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
	html/sfidl-manual.html			\
	\
	html/beast.1.html			\
	html/bsescm.1.html			\
	html/sfidl.1.html			\
	html/bse.5.html				\
	html/coding-style.html			\
	\
	${HTML_IMPORTS}				\
)
HTML_IMPORTS = $(strip				\
	html/beastdocs.css			\
	html/Beast-Quickstart			\
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
# noinst_DATA = ${PSOURCE_TARGETS}

DOXER = $(top_srcdir)/doxer/doxer.py
DOCFRAME_DEFS = -D DOXI_INCLUDE_FILE \"docframe.doxi\"
DOXIDEFS = -D TOP_WEBDIR "." -D BST_VERSION ${BST_VERSION} -I $(srcdir) $(call DOCFRAME_DEFS)

# ship readily built documentation
EXTRA_DIST += $(MAN_TARGETS) $(HTML_TARGETS) $(STYLE_TARGETS)

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
html/style/doxer-style.css: @DOXRULE@
	$(DOXER) writecss -d html/style/

# MAN generation rules
mans/%.1: @DOXRULE@ %.1.doxi
	@mkdir -p mans/
	$(DOXER) doxi2man $< -d mans/ $(DOXIDEFS)
mans/%.3: @DOXRULE@ %.3.doxi
	@mkdir -p mans/
	$(DOXER) doxi2man $< -d mans/ $(DOXIDEFS)
mans/%.5: @DOXRULE@ %.5.doxi
	@mkdir -p mans/
	$(DOXER) doxi2man $< -d mans/ $(DOXIDEFS)

# HTML generation rules
html/%.1.html: @DOXRULE@ %.1.doxi
	$(DOXER) doxi2htmlman $< -d html/ $(DOXIDEFS)
html/%.3.html: @DOXRULE@ %.3.doxi
	$(DOXER) doxi2htmlman $< -d html/ $(DOXIDEFS)
html/%.html: @DOXRULE@ %.doxi
	@mkdir -p html/
	$(DOXER) doxi2html $< -d html/ $(DOXIDEFS)
html/%.html: @DOXRULE@ gendoxi/%.doxi
	$(DOXER) doxi2html $< -d html/ $(DOXIDEFS)

# HTML import rules
${HTML_IMPORTS}:
	cp $(subst html/, ${srcdir}/imports/, ${HTML_IMPORTS}) html/

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
gendoxi/bse-objects.doxi: @DOXRULE@ psource/bse-objects.psrc docu-template.doxi
	@mkdir -p gendoxi/
	$(DOXER) pickle2doxi $(strip	$<	-D TOP_WEBDIR ".."		\
	         -d gendoxi/ $(DOXIDEFS)	-T docu-template.doxi		\
	         -D DOCU_TEMPLATE_TITLE "BEAST - BSE Object Reference"		\
	         -D DOCU_TEMPLATE_HEADING "BSE Object Reference" )
# BSE interface docu
gendoxi/bse-interface.doxi: @DOXRULE@ psource/bse-interface.psrc docu-template.doxi
	@mkdir -p gendoxi/
	$(DOXER) pickle2doxi $(strip	$<	-D TOP_WEBDIR ".."		\
	         -d gendoxi/ $(DOXIDEFS)	-T docu-template.doxi		\
	         -D DOCU_TEMPLATE_TITLE "BEAST - BSE Interface Reference"	\
	         -D DOCU_TEMPLATE_HEADING "BSE Interface Reference" )

# Reference Documentation
AUTODOC = $(top_builddir)/bse/bseautodoc
AUTODOC_NORC = $(AUTODOC) --bse-rcfile "/dev/null"
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
psource/bse-objects.psrc: $(wildcard ../bse/*.[hc]) $(wildcard ../bse/*.[hc][hc])
psource/bse-objects.psrc: $(wildcard ../plugins/*.[hc]) $(wildcard ../plugins/*.[hc][hc])
psource/bse-objects.psrc: $(wildcard ../drivers/*/*.[hc]) $(wildcard ../drivers/*/*.[hc][hc])
psource/bse-objects.psrc: @DOXRULE@ $(AUTODOC) $(top_builddir)/config.status $(bse_objects_deps)
	@mkdir -p psource/
	cd . \
	&& $(AUTODOC_NORC) -p -s objects >psource/bse-objects.scad	\
	&& $(DOXER) scad2pickle -o $@ psource/bse-objects.scad		\
	&& rm -f psource/bse-objects.scad
# BSE interface docu
psource/bse-interface.psrc: @DOXRULE@ $(AUTODOC) $(top_builddir)/config.status
	@mkdir -p psource/
	cd . \
	&& $(AUTODOC_NORC) -p -s procs	  >psource/bse-interface.scad	\
	&& $(AUTODOC_NORC) -p -s structs >>psource/bse-interface.scad	\
	&& $(DOXER) scad2pickle -o $@ psource/bse-interface.scad	\
	&& rm -f psource/bse-interface.scad
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
	sfidl-manual.doxi	\
	beast-index.doxi	\
	bsescm.1.doxi		\
	plugin-devel.doxi	\
	sfidl.1.doxi		\
	coding-style.doxi	\
)

EXTRA_DIST += $(strip		\
	$(doxi_files)		\
	bse-categories.txt	\
	interpolation.txt	\
)
