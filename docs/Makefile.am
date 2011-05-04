# BEAST - Better Audio System
# Copyright (C) 1998-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = images

# === Manual Pages to be fetched, built and installed
man_MANS              = ${MANUAL_PAGES} # automake-1.11 needs this expandable w/o patsubst
MANUAL_PAGES	      = mans/bse.5 mans/beast.1 mans/bsescm.1 mans/bsewavetool.1 mans/sfidl.1
MANUAL_CACHE_TARGETS  = $(patsubst mans/%,cache/%,$(MANUAL_PAGES))
HTMLMAN_TARGETS       = $(patsubst mans/%,html/%.html,$(MANUAL_PAGES))
EXTRA_DIST           += $(MANUAL_PAGES) $(HTMLMAN_TARGETS)
MAINTAINERCLEANFILES += $(MANUAL_CACHE_TARGETS) $(MAN_TARGETS)
CLEANFILES           += $(HTMLMAN_TARGETS)
# rules
mans/:
	mkdir -p $@
$(MANUAL_CACHE_TARGETS): cache/stamp-refresh
	curl -sfS http://beast.testbit.eu/$(@F)?action=render >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@ ; rm -f $(@D)/xgen-$(@F)
WIKIHTML2MAN = wikihtml2man.py
mans/%: cache/% | mans/
	$(WIKIHTML2MAN) $< >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@ ; rm -f $(@D)/xgen-$(@F)
html/%.html: mans/% | html/
	groff -mandoc -Thtml $< >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@ ; rm -f $(@D)/xgen-$(@F)

# === Html Docs to be fetched, built and installed
htmldocsdir           = ${beastdocsdir}/html
htmldocs_DATA         = $(HTML_DOCS) $(HTML_TARGETS) $(HTMLMAN_TARGETS)
HTML_DOCS             = html/Beast-Quickstart.html
HTML_CACHE_TARGETS    = $(patsubst html/%,cache/%,$(HTML_DOCS))
EXTRA_DIST	     += $(HTML_DOCS) $(HTML_TARGETS)
MAINTAINERCLEANFILES += $(HTML_CACHE_TARGETS)
# rules
html/:
	mkdir -p $@
$(HTML_CACHE_TARGETS): cache/stamp-refresh
	curl -sfS http://beast.testbit.eu/$(basename $(@F))?action=render >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@ ; rm -f $(@D)/xgen-$(@F)
html/%.html: cache/%.html | html/
	@echo 'htmlgen... $(@F)...'
	@echo -e $(HTML1) > $(@D)/xgen-$(@F) && sed -n $(H1SED) $< >> $(@D)/xgen-$(@F)
	@echo -e $(HTML2) >> $(@D)/xgen-$(@F) && cat $< >> $(@D)/xgen-$(@F) && echo -e $(HTML3) >> $(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@ ; rm -f $(@D)/xgen-$(@F)
HTML1 = '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">\n' \
	'<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">\n' \
	'<head>  <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />\n' \
	'<title>'
HTML2 = '</title>\n' \
	'</head> <body>\n'
HTML3 = '\n</body></html>\n'
H1SED = '/<[hH]1\b/{ s/<[^>]\+>//g ; s/^\s\+//; p ; q }' # sed hack to extract first H1 title

# === cache/ handling for file downloads
cache/:
	mkdir -p $@
.PHONY: stamp-refresh-message
stamp-refresh-message:	# conditional phony dependency
	@TXT="  Some source files hosted online appear outdated; forcing refresh...  " \
	&& echo "$$TXT" | sed 's/./=/g' && echo "$$TXT" && echo "$$TXT" | sed 's/./=/g'
cache/stamp-refresh: $(call AGINGRULE, cache/stamp-refresh, 1440, stamp-refresh-message) | cache/ # refresh daily
	touch $@
# $(call AGINGRULE, stamp-file-for-aging, AgingInMinutes, rule-name-for-remaking)
AGINGRULE = $(shell test -e $(1) && find $(1) -type f -mmin -"$(strip $(2))" | grep -q '.' || echo " $(3)")


# === Html built targets, old-doxer
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
	html/coding-style.html			\
)
CLEANFILES += ${HTML_TARGETS}
STYLE_FILES = $(strip				\
	style/beast-style.css			\
	style/plain.css				\
	style/doxer-style.css			\
	style/beast-dot.png			\
	style/beast-small.png			\
	style/title-arrow-24x64.png		\
	style/home-arrow-24x64.png		\
	style/triangle-down-blue.png		\
	style/triangle-right-blue.png		\
)
EXTRA_DIST   += $(patsubst %, %, $(STYLE_FILES))
STYLE_TARGETS = $(patsubst %, html/%, $(STYLE_FILES))
CLEANFILES   += ${STYLE_TARGETS}

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
html/style/%: $(srcdir)/style/%
	@mkdir -p html/style/
	cp $< $@
style/doxer-style.css: @DOXRULE@
	$(DOXER) writecss -d style/

# HTML generation rules
html/%.html: @DOXRULE@ %.doxi
	@mkdir -p html/
	$(DOXER) doxi2html $< -d html/ $(DOXIDEFS)
html/%.html: @DOXRULE@ gendoxi/%.doxi
	@mkdir -p html/
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
	rm -rf gendoxi/ html/ psource/

intermediate_files = $(strip    \
        news-file.tmpdoxi       \
)
.INTERMEDIATE: $(intermediate_files)

doxi_files = $(strip		\
	architecture.doxi	\
	faq.doxi		\
	sfidl-manual.doxi	\
	beast-index.doxi	\
	plugin-devel.doxi	\
	coding-style.doxi	\
)

EXTRA_DIST += $(strip		\
	$(doxi_files)		\
	bse-categories.txt	\
	interpolation.txt	\
	ChangeLog.svn		\
)
