# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/doc/*.d)
docs/cleandirs ::= $(wildcard $>/doc/)
CLEANDIRS       += $(docs/cleandirs)
ALL_TARGETS    += docs/all

# == plain text files ==
docs/doc.files ::= $(strip		\
	$>/doc/COPYING			\
	$>/doc/HACKING			\
	$>/doc/NEWS			\
	$>/doc/README			\
	$>/doc/beast-manual.html	\
	$>/doc/copyright		\
)
docs/doc.dir	  ::= $(pkglibdir)/doc
docs/all: $(docs/doc.files)

# == man1 files ==
docs/man1.files	::= $>/doc/beast.1 $>/doc/bsewavetool.1
docs/all: $(docs/man1.files)

# == man5 files ==
docs/man5.files	::= $>/doc/bse.5
docs/all: $(docs/man5.files)

# == pandoc flags ==
docs/html_flags       ::= --html-q-tags --section-divs --email-obfuscation=references --toc --toc-depth=6
docs/pandoc-nosmart     = $(if $(HAVE_PANDOC1),,-smart)
docs/markdown-flavour   = -f markdown+autolink_bare_uris+emoji+lists_without_preceding_blankline$(docs/pandoc-nosmart)

# == beast-manual defs ==
docs/manual-chapters ::= $(strip	\
	docs/ch-intro.md		\
	docs/ch-tutorials.md		\
	docs/ch-howotos.md		\
	$>/doc/ch-man-pages.md		\
	docs/ch-formats.md		\
	docs/ch-background.md		\
	ebeast/b/ch-vue.md		\
	$>/ebeast/vue-docs.md		\
	docs/ch-reference.md		\
	docs/ch-historic.md		\
	docs/ch-appendix.md		\
)
docs/manual-man-pages ::= $(strip	\
	docs/beast.1.md			\
	docs/bsewavetool.1.md		\
	docs/bse.5.md			\
	docs/sfidl.1.md			\
)

# == Documentation Date ==
# shell command to produce 'Month 20YY' from version date
docs/version_month ::= ./version.sh -d | sed -r -e 's/^([2-9][0-9][0-9][0-9])-([0-9][0-9])-.*/m\2 \1/' \
			-e 's/m01/January/ ; s/m02/February/ ; s/m03/March/ ; s/m04/April/ ; s/m05/May/ ; s/m06/June/' \
			-e 's/m07/July/ ; s/m08/August/ ; s/m09/September/ ; s/m10/October/ ; s/m11/November/ ; s/m12/December/'


# == subdirs ==
include docs/faketex/Makefile.mk


# == copy rules ==
$>/doc/COPYING: 	COPYING		| $>/doc/	; $(QECHO) COPY $@ ; cp -L $< $@
$>/doc/copyright:	docs/copyright	| $>/doc/	; $(QECHO) COPY $@ ; cp -L $< $@

# == markdown rules ==
$>/doc/%: %.md								| $>/doc/
	$(QECHO) MD2TXT $@
	$Q $(PANDOC) $(docs/markdown-flavour) -s -p -t plain --columns=80 $< -o $@.tmp
	$Q mv $@.tmp $@
$>/doc/%.html: %.md							| $>/doc/
	$(QECHO) MD2HTML $@
	$Q $(PANDOC) $(docs/markdown-flavour) -s -p $(docs/html_flags) -t html5 $< -o $@.tmp
	$Q mv $@.tmp $@

# == .revd.md (INTERMEDIATE) ==
$>/doc/%.revd.md: docs/%.md						| $>/doc/
	$Q V="$(VERSION_SHORT)" && D="$$(./version.sh -d)" \
	  && sed "s/[@]BUILDVERSION[@]/$$V/g ; s/[@]FILE_REVISION[@]/$${D%% *}/g" < $< > $@

# == man build rules ==
%.1: %.1.revd.md							| $>/doc/
	$(QECHO) MD2MAN $@
	$Q $(PANDOC) $(docs/markdown-flavour) -s -p -t man $< -o $@.tmp
	$Q mv $@.tmp $@
%.5: %.5.revd.md							| $>/doc/
	$(QECHO) MD2MAN $@
	$Q $(PANDOC) $(docs/markdown-flavour) -s -p -t man $< -o $@.tmp
	$Q mv $@.tmp $@

# == beast-manual.html rules ==
$>/doc/beast-manual.html: $(docs/manual-chapters) $(FAKETEX_TARGETS)	| $>/doc/
	$(QGEN)
	$Q DATE=$$( $(docs/version_month) ) \
	  && $(PANDOC) $(docs/markdown-flavour) \
		--toc --number-sections \
		--variable=subparagraph \
		-s -c faketex/$(notdir $(doc/faketex/faketex.css)) \
		--mathjax='faketex/stripped-mathjax/MathJax.js?config=TeX-AMS_SVG-full' \
		$(docs/manual-chapters) -o $@

# == beast-manual.pdf rules ==
# needs: python3-pandocfilters texlive-xetex pandoc2
$>/doc/beast-manual.pdf: $(docs/manual-chapters) docs/pandoc-pdf.tex docs/Makefile.mk	| $>/doc/
	$(QGEN)
	$Q xelatex --version 2>&1 | grep -q '^XeTeX 3.14159265' \
	  || { echo '$@: missing xelatex, required version: XeTeX >= 3.14159265' >&2 ; false ; }
	$Q DATE=$$( $(docs/version_month) ) \
	  && $(PANDOC) $(docs/markdown-flavour) \
		--toc --number-sections \
		--variable=subparagraph \
		--variable=lot \
		-V date="$$DATE" \
		-H docs/pandoc-pdf.tex \
		--pdf-engine=xelatex -V mainfont='Charis SIL' -V mathfont=Asana-Math -V monofont=inconsolata \
		-V fontsize=12pt -V papersize:a4 -V geometry:margin=2cm \
		$(docs/manual-chapters) -o $@
# --pdf-engine=pdflatex ; $Q pdflatex --version 2>&1 | grep -q '^pdfTeX 3.14159265' \
# || { echo '$@: missing pdflatex, required version: pdfTeX >= 3.14159265' >&2 ; false ; }
$>/doc/ch-man-pages.md: $(docs/manual-man-pages) docs/filt-man.py docs/Makefile.mk	| $>/doc/
	$(QGEN)
	$Q echo '# Manual Pages'			 > $@.tmp
	$Q echo ''					>> $@.tmp
	$Q for m in $(docs/manual-man-pages) ; do		\
	  n="$${m%%.md}" ; u="$${n^^}" ; n="$${u##*/}" ;	\
	  echo "## $${n/\./(})" ; echo ;			\
	  $(PANDOC) -t markdown -F docs/filt-man.py "$$m"	\
	  || exit $$? ; echo ; done			>> $@.tmp
	$Q mv $@.tmp $@

# == installation rules ==
$(call INSTALL_DATA_RULE, docs/doc.files, $(DESTDIR)$(docs/doc.dir), $(docs/doc.files) $(docs/man1.files) $(docs/man5.files))

# == install symlinks ==
# Create links from $(prefix) paths according to the FHS, into $(pkglibdir).
doc/install-prefix-symlinks: $(docs/man1.files) $(docs/man5.files) FORCE
	$(QECHO) INSTALL $@
	$Q $(call INSTALL_SYMLINK, '$(pkglibdir)/doc/beast.1',        '$(DESTDIR)$(mandir)/man1/beast.1')
	$Q $(call INSTALL_SYMLINK, '$(pkglibdir)/doc/bsewavetool.1',  '$(DESTDIR)$(mandir)/man1/bsewavetool.1')
	$Q $(call INSTALL_SYMLINK, '$(pkglibdir)/doc/bse.5',          '$(DESTDIR)$(mandir)/man5/bse.5')
install: doc/install-prefix-symlinks
doc/uninstall-prefix-symlinks: FORCE
	$(QECHO) REMOVE $@
	$Q rm -f '$(DESTDIR)$(mandir)/man1/beast.1'
	$Q rm -f '$(DESTDIR)$(mandir)/man1/bsewavetool.1'
	$Q rm -f '$(DESTDIR)$(mandir)/man5/bse.5'
uninstall: doc/uninstall-prefix-symlinks

# == docs/clean ==
docs/clean: FORCE
	rm -f -r $(docs/cleandirs)

# == docs/doxy/ ==
$>/doc/bse-api.pdf: docs/refman.patch $>/bse/bseapi_interfaces.hh docs/Makefile.mk $(wildcard bse/*.[hc][hc])	| $>/doc/
	$(QGEN)
	@$(eval docs/month_date != $(docs/version_month))
	@: # clean builddir
	rm -Rf $>/doc/doxy/ && mkdir -p $>/doc/doxy/src/
	@: # configure Doxyfile
	$(Q) (cat docs/doxygen.cfg \
	&& echo 'PROJECT_NAME     = "Bse API Reference"' \
	&& echo "PROJECT_NUMBER   = $(VERSION_M.M.M)" \
	&& echo "INPUT            = src/" \
	&& echo "STRIP_FROM_PATH  = $(abspath $>/doc/doxy/src)" \
	&& echo "STRIP_FROM_INC_PATH = $(abspath $>/doc/doxy/src)" \
	&& echo "OUTPUT_DIRECTORY = ." \
	&& echo "EXAMPLE_PATH     = src/" \
	&& echo "TAGFILES        += tagfile-susv4.xml=http://pubs.opengroup.org/onlinepubs/9699919799/" \
	&& echo "TAGFILES        += tagfile-cppreference-15.xml=http://en.cppreference.com/w/" \
	) > $>/doc/doxy/Doxyfile
	@: # provide aux and tag files
	cp docs/footer.html docs/extrastyles.css docs/references.bib $>/doc/doxy/
	bunzip2 < docs/tagfile-susv4.xml.bz2           > $>/doc/doxy/tagfile-susv4.xml
	bunzip2 < docs/tagfile-cppreference-15.xml.bz2 > $>/doc/doxy/tagfile-cppreference-15.xml
	@: # provide sources
	git ls-tree -r --name-only -z HEAD bse/ | xargs -0 cp -R --parents -t $>/doc/doxy/src/
		cp $>/bse/bseapi_interfaces.hh $>/doc/doxy/src/bse/
	@: # run doxygen
	cd $>/doc/doxy/ && doxygen Doxyfile # > error.log 2>&1
	@: # polish output
	(cd $>/doc/doxy/latex/ && echo >namespacestd.tex && sed $(docs/sed-refman) -i refman.tex && patch -b) < docs/refman.patch
	@: # run Tex for PDF
	(cd $>/doc/doxy/latex/ && echo >namespacestd.tex && \
		latexmk -lualatex -bibtex refman.tex )
	@: # final document
	mv -v $>/doc/doxy/latex/refman.pdf $@
docs/sed-foot1       = \\fancyfoot[LE,LO]{\\fancyplain{}{\\bfseries\\scriptsize Bse $(VERSION_M.M.M)}}
docs/sed-foot2       = \\fancyfoot[CE,CO]{\\fancyplain{}{\\bfseries\\scriptsize $(docs/month_date)}}
docs/sed-foot3       = \\fancyfoot[RE,RO]{\\fancyplain{}{\\bfseries\\scriptsize Beast Sound Engine}}
docs/sed-refman      =	-e '/^\\fancyfoot.*{.*Generated by Doxygen.*}.*/d' \
			-e 's| *Generated by Doxygen [0-9.]\+|\\bfseries The Beast Project <beast.testbit.org> \\\\[1ex] $(docs/month_date)|' \
			-e '/^{\\Large Bse A..PI Reference/{ s/\\Large/\\huge\\bfseries/; s/\\large/\\Large/; }' \
			-e 's|^\(\\fancyfoot\[RO]{\\fancyplain{}{}}\)|\1\n$(docs/sed-foot1)\n$(docs/sed-foot2)\n$(docs/sed-foot3)|'
