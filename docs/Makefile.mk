# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/doc/*.d)
CLEANDIRS += $(wildcard $>/doc/)

# == plain text files ==
docs/doc.files ::= $(strip		\
	$>/doc/COPYING			\
	$>/doc/HACKING			\
	$>/doc/NEWS			\
	$>/doc/README			\
	$>/doc/copyright		\
)
docs/doc.dir	  ::= $(pkglibdir)/doc
ALL_TARGETS	   += $(docs/doc.files)

# == man1 files ==
docs/man1.files	::= $>/doc/beast.1 $>/doc/bsewavetool.1
ALL_TARGETS	 += $(docs/man1.files)

# == man5 files ==
docs/man5.files	::= $>/doc/bse.5
ALL_TARGETS	 += $(docs/man5.files)

# == pandoc flags ==
docs/markdown-flavour ::= -f markdown+autolink_bare_uris+emoji+lists_without_preceding_blankline
docs/html_flags   ::= --html-q-tags --section-divs --email-obfuscation=references --toc --toc-depth=6 # --css /pandoc-html.css

# == beast-manual.pdf defs ==
docs/manual-chapters ::= $(strip	\
	docs/ch-intro.md		\
	docs/ch-tutorials.md		\
	docs/ch-howotos.md		\
	$>/doc/ch-man-pages.md		\
	docs/ch-formats.md		\
	docs/ch-background.md		\
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
# shell command to produce 'Month 20YY' from VERSION_DATE
docs/version_month ::= echo '$(VERSION_DATE)' | sed -r -e 's/^([2-9][0-9][0-9][0-9])-([0-9][0-9])-.*/m\2 \1/' \
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
	$Q $(PANDOC) $(docs/markdown-flavour) -s - p $(docs/html_flags) -t html5 $< -o $@.tmp
	$Q mv $@.tmp $@

# == .revd.md (INTERMEDIATE) ==
$>/doc/%.revd.md: docs/%.md						| $>/doc/
	$Q V="$(VERSION_SHORT)" && D="$(VERSION_DATE)" \
	  && sed "s/[@]BUILDID[@]/$$V/g ; s/[@]FILE_REVISION[@]/$${D%% *}/g" < $< > $@

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
