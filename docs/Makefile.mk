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
docs/md_flags	  ::= -p -s -f markdown+autolink_bare_uris+emoji+lists_without_preceding_blankline
docs/html_flags   ::= --html-q-tags --section-divs --email-obfuscation=references --toc --toc-depth=6 # --css /pandoc-html.css

# == copy rules ==
$>/doc/COPYING: 	COPYING		| $>/doc/	; $(QECHO) COPY $@ ; cp -L $< $@
$>/doc/copyright:	docs/copyright	| $>/doc/	; $(QECHO) COPY $@ ; cp -L $< $@

# == markdown rules ==
$>/doc/%: %.md								| $>/doc/
	$(QECHO) MD2TXT $@
	$Q $(PANDOC) $(docs/md_flags) -t plain --columns=80 $< -o $@.tmp
	$Q mv $@.tmp $@
$>/doc/%.html: %.md							| $>/doc/
	$(QECHO) MD2HTML $@
	$Q $(PANDOC) $(docs/md_flags) $(docs/html_flags) -t html5 $< -o $@.tmp
	$Q mv $@.tmp $@

# == .revd.md (INTERMEDIATE) ==
$>/doc/%.revd.md: docs/%.md						| $>/doc/
	$Q V="$(VERSION_SHORT)" && D="$(VERSION_DATE)" \
	  && sed "s/[@]BUILDID[@]/$$V/g ; s/[@]FILE_REVISION[@]/$${D%% *}/g" < $< > $@

# == man build rules ==
%.1: %.1.revd.md							| $>/doc/
	$(QECHO) MD2MAN $@
	$Q $(PANDOC) $(docs/md_flags) -s -t man $< -o $@.tmp
	$Q mv $@.tmp $@
%.5: %.5.revd.md							| $>/doc/
	$(QECHO) MD2MAN $@
	$Q $(PANDOC) $(docs/md_flags) -s -t man $< -o $@.tmp
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
