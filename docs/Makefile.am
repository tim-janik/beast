# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = images

MOSTLYCLEANFILES =
beastmandir = ${beastsharedir}/man
man1dir     = $(beastmandir)/man1
man1_MANS   =
man1_HTMLS  =
man5dir     = $(beastmandir)/man5
man5_MANS   =
man5_HTMLS  =

topdir = $(abs_top_srcdir)
V_gen = @printf '  %-9s%s\n'
topbuildid ::= $(shell $(topdir)/misc/mkbuildid.sh -p)

# == Collect Doxygen Sources ==
# The following variable definitions create files as a side effect, which should be
# avoided e.g. upon 'make clean'. So we confine the assignments to a rule execution.
collect_docs_doxy_sources:
	$(eval sfi_doc_sources	 ::= $(shell make doc_sources.lst -C ../sfi >&2 && cat ../sfi/doc_sources.lst) )
	$(eval bse_doc_sources	 ::= $(shell make doc_sources.lst -C ../bse >&2 && cat ../bse/doc_sources.lst) )
	$(eval plugs_doc_sources ::= $(shell make doc_sources.lst -C ../plugins >&2 && cat ../plugins/doc_sources.lst) )
	$(eval gxk_doc_sources	 ::= $(shell make doc_sources.lst -C ../beast-gtk/gxk >&2 && cat ../beast-gtk/gxk/doc_sources.lst) )
	$(eval bst_doc_sources	 ::= $(shell make doc_sources.lst -C ../beast-gtk >&2 && cat ../beast-gtk/doc_sources.lst) )
	$(eval tools_doc_sources ::= $(shell make doc_sources.lst -C ../tools >&2 && cat ../tools/doc_sources.lst) )

# == Configure docs ==
docs_project_link   = http://beast.testbit.eu
docs_doxy_extra     = docs/main.dox
docs_doxy_sources   = $(sfi_doc_sources) $(bse_doc_sources) $(plugs_doc_sources) $(gxk_doc_sources) $(bst_doc_sources) $(tools_doc_sources)
docs_doxy_src_dirs  = sfi bse plugins beast-gtk beast-gtk/gxk tools
docs_doxy_headers   = $(filter     %.h %.H %.hh, $(docs_doxy_sources))
docs_doxy_impls     = $(filter-out %.h %.H %.hh, $(docs_doxy_sources))
docs_doxy_tagsrcs   = $(strip \
	tagfile-susv4.xml=http://pubs.opengroup.org/onlinepubs/9699919799/	\
	tagfile-cppreference-15.xml=http://en.cppreference.com/w/		\
	tagfile-rapicorn.xml=http://dev.testbit.eu/rapicorn/latest/		\
)
docs_doxy_tagstems  = $(filter-out =JUNK%, $(subst =, =JUNK, $(docs_doxy_tagsrcs))) # uses 'subst' for word splitting
docs_doxygen_tagbzs = $(docs_doxy_tagstems:%=docs/%.bz2)
docs_doxygen_werror = false

# Build with dot files by default
DOXYGEN_GRAPHICS  ?= NO # YES

# == docs/srctree ==
doxy/src: $(patsubst %, ../%, $(docs_doxy_sources) $(docs_doxy_extra) $(docs_doxygen_tagbzs)) | collect_docs_doxy_sources
	$(AM_V_GEN)
	$(Q) rm -Rf doxy/src doxy/src.tmp
	$(Q) mkdir -p $(docs_doxy_src_dirs:%=doxy/src.tmp/%)
	@: # copy source files *and* prefix each with an '@file' command
	$(Q) cd $(topdir) && for file in $(docs_doxy_headers) $(docs_doxy_impls) ; do \
	  sed "1,+0s,^,/** @file $$file */ ," $$file > docs/doxy/src.tmp/$$file \
	  || exit $? ; done
	@: # copy extra files verbatim
	$(Q) cd $(topdir) && cp $(docs_doxy_extra) docs/doxy/src.tmp/
	@: # avoid docu generation from enums residing in *[cC] files
	$(Q) find doxy/src.tmp -type f -name '*[cC]' -exec sed 's/^\([^"]*\)\benum\b/\1Enum/' -i {} \;
	@: # extract doxygen tag files
	$(Q) for file in $(docs_doxy_tagstems) ; do \
	  bunzip2 < ../docs/$$file.bz2 > doxy/$$file \
	  || exit $? ; done
	$(Q) mv doxy/src.tmp doxy/src

# == docs/doxy/html ==
docs_doxy_auxfiles = ../docs/extrastyles.css ../docs/footer.html ../docs/references.bib
# rebuild documentation after commits, unless NODOC is set to be non-empty
doxy/html: $(if $(NODOC), , ../topbuildid.cc)
# use order-only prerequisite on docs/doxy/src to spare doxygen runs for source edits
doxy/html: ../docs/doxygen.cfg ../docs/error.ign $(docs_doxy_auxfiles) | doxy/src
	$(V_gen) "PREPARE" $@
	$(Q) rm -Rf doxy/html doxy/output
	$(Q) mkdir -p doxy/output
	$(Q) cp $(docs_doxy_auxfiles) doxy/
	$(Q) (cat ../docs/doxygen.cfg \
	&& echo "PROJECT_NUMBER		= $(topbuildid)" \
	&& echo "HAVE_DOT		= $(DOXYGEN_GRAPHICS)" \
	&& echo "CLASS_DIAGRAMS		= $(DOXYGEN_GRAPHICS)" \
	&& echo "DOT_MULTI_TARGETS	= YES" \
	&& echo "INPUT			= src/" \
	&& echo "STRIP_FROM_PATH	= $$(pwd)/doxy/src" \
	&& echo "OUTPUT_DIRECTORY	= output/" \
	&& echo "GENERATE_TAGFILE	= output/html/tagfile.xml" \
	&& echo "HTML_EXTRA_STYLESHEET	= extrastyles.css" \
	&& echo "EXAMPLE_PATH		= $(topdir)/" \
	&& echo "TAGFILES              += $(docs_doxy_tagsrcs)" \
	) > doxy/Doxyfile
	$(V_gen) "DOXYGEN" $@
	$(Q) cd ../docs/doxy && $(DOXYGEN) Doxyfile > error.log 2>&1
	$(Q) test ! -s doxy/error.log && rm -f doxy/error.log \
	|| { echo "WARNING: doxygen issues encountered ($$(wc -l <doxy/error.log)): $$(pwd)/doxy/error.log" ; \
	     ! grep -E -v -f $(srcdir)/../docs/error.ign doxy/error.log || ! $(docs_doxygen_werror) ; }
	$(V_gen) "POLISH" $@
	$(Q) sed -r $(docs_sed_accesskey_f) $(docs_sed_project_link) -i doxy/output/html/*.html
	$(Q) mv doxy/output/html doxy/html
docs_sed_accesskey_f  = -e 's;(<input [^>]*id="MSearchField"[^>]*accesskey=")S";\1F";'
docs_sed_project_link = -e 's;<div id="projectname">((\w|[+_/~*%^-])+);<div id="projectname"><a href="$(docs_project_link)">\1</a>;'
CLEANFILES += ../docs/error.log
doxy/html/doc-version: doxy/html $(man1_HTMLS) $(man5_HTMLS)
	$(AM_V_GEN)
	$(Q) test -z "$(man1_HTMLS)$(man5_HTMLS)" || { $(MKDIR_P) doxy/html/man/ && cp $(man1_HTMLS) $(man5_HTMLS) doxy/html/man/ ; }
	$(Q) echo "$(PACKAGE)-$(topbuildid)" > $@

# == install docs/doxy/html ==
docs_html_installbase	 = $(beastdocdir)
noinst_DATA		+= doxy/html/doc-version	# AM target to trigger docs/doxy/html builds
docs-html-install: $(docs_html_install_DATA)
	rm -Rf "$(DESTDIR)$(docs_html_installbase)"/html/
	umask 022 && $(MKDIR_P) "$(DESTDIR)$(docs_html_installbase)"
	umask 022 && cp -RP doxy/html/ "$(DESTDIR)$(docs_html_installbase)"
.PHONY: docs-html-install
install-data-local: docs-html-install

# == uninstall docs/doxy/html ==
docs-html-uninstall:
	rm -Rf "$(DESTDIR)$(docs_html_installbase)"/html/
.PHONY: docs-html-uninstall
uninstall-local: docs-html-uninstall

# == clean docs/doxy/ ==
clean-docs:
	rm -Rf doxy/
.PHONY: clean-docs
clean-local: clean-docs


# == revisioned_files ==
revisioned_files         = # files that need @FILE_REVISION@ substitution, see revisions.lst

# == revisions.lst ==
noinst_DATA += revisions.lst
revisions.lst: @INGIT@ $(revisioned_files) Makefile
	$(AM_V_GEN)
	$(Q) rm -f $@.tmplst
	$(Q) for file in $(revisioned_files) ; do \
	  git log --format="%ai $$file" -n 99 -- $$file > $@.tmp1 && \
	  sort -rn $@.tmp1 > $@.tmp2 && \
	  sed -n '1{ p; q}' $@.tmp2 >> $@.tmplst || exit $$? ; \
	done
	$(Q) mv $@.tmplst $@
	$(Q) rm -f $@.tmp1 $@.tmp2 $@.tmplst
clean-revisions.lst: # git is required to rebuild revisions.lst, so conditionalize cleaning
	! git rev-parse --git-dir 2>/dev/null 1>&2 || rm -f revisions.lst
.PHONY: clean-revisions.lst
clean-local: clean-revisions.lst


# == .revd.md (INTERMEDIATE) ==
%.revd.md: %.md revisions.lst
	$(AM_V_GEN)
	$(Q) V=`sed -n '\| $<$$|{ s/ .*//; p; q; }' revisions.lst` \
	&& sed "s/[@]FILE_REVISION[@]/$$V/g; s/[@]BUILDID[@]/$(topbuildid)/g" < $< > $@

# == man.1 (markdown) ==
%.1: %.1.revd.md
	$(AM_V_GEN)
	$(Q) $(PANDOC) -s -t man $< -o $@
# == man.5 (markdown) ==
%.5: %.5.revd.md
	$(AM_V_GEN)
	$(Q) $(PANDOC) -s -t man $< -o $@

# == man.*.html (markdown) ==
manpage_header.html:
	$(Q) echo $$'<a href="../index.html">Return to Index</a>\n<hr>\n' > $@
.INTERMEDIATE: manpage_header.html
MOSTLYCLEANFILES += manpage_header.html
manpage_footer.html:
	$(Q) echo $$'<hr>\n<a href="../index.html">Return to Index</a>\n' > $@
.INTERMEDIATE: manpage_footer.html
MOSTLYCLEANFILES += manpage_footer.html

# == man.1.html (markdown) ==
%.1.html: %.1.revd.md manpage_header.html manpage_footer.html ../docs/manpage.css
	$(AM_V_GEN)
	$(Q) $(PANDOC) -s -t html -S --toc -H $(topdir)/docs/manpage.css -B manpage_header.html -A manpage_footer.html $< -o $@
# == man.5.html (markdown) ==
%.5.html: %.5.revd.md manpage_header.html manpage_footer.html ../docs/manpage.css
	$(AM_V_GEN)
	$(Q) $(PANDOC) -s -t html -S --toc -H $(topdir)/docs/manpage.css -B manpage_header.html -A manpage_footer.html $< -o $@

# == beast.1 ==
revisioned_files	+= beast.1.md
man1_MANS		+= beast.1
man1_HTMLS		+= beast.1.html
MOSTLYCLEANFILES	+= beast.1 beast.1.html

# == bsewavetool.1 ==
revisioned_files	+= bsewavetool.1.md
man1_MANS		+= bsewavetool.1
man1_HTMLS		+= bsewavetool.1.html
MOSTLYCLEANFILES	+= bsewavetool.1 bsewavetool.1.html

# == bse.5 ==
revisioned_files	+= bse.5.md
man5_MANS		+= bse.5
man5_HTMLS		+= bse.5.html
MOSTLYCLEANFILES	+= bse.5 bse.5.html

# == bsescm.1 ==
revisioned_files	+= bsescm.1.md
man1_MANS		+= bsescm.1
man1_HTMLS		+= bsescm.1.html
MOSTLYCLEANFILES	+= bsescm.1 bsescm.1.html

# == sfidl.1 ==
revisioned_files	+= sfidl.1.md
man1_MANS		+= sfidl.1
man1_HTMLS		+= sfidl.1.html
MOSTLYCLEANFILES	+= sfidl.1 sfidl.1.html
