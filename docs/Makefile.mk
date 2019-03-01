# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/doc/*.d)
CLEANDIRS += $(wildcard $>/doc/)

# == docs/ files ==
docs/doc.files ::= $(strip		\
	$>/doc/COPYING			\
	$>/doc/HACKING			\
	$>/doc/README			\
	$>/doc/copyright		\
)
docs/doc.dir	  ::= $(pkglibdir)/doc
ALL_TARGETS	   += $(docs/doc.files)
docs/md_flags	  ::= $(strip	--html-q-tags -p -s --section-divs --email-obfuscation=references \
				-f markdown+autolink_bare_uris+emoji+lists_without_preceding_blankline \
				--toc --toc-depth=6)
# --css /pandoc-html.css

# == doc build rules ==
$>/doc/%: %								| $>/doc/
	$(QECHO) COPY $@
	$Q cp -L $< $@
$>/doc/%: docs/%							| $>/doc/
	$(QECHO) COPY $@
	$Q cp -L $< $@
$>/doc/%: %.md								| $>/doc/
	$(QECHO) MD2TXT $@
	$Q $(PANDOC) $(docs/md_flags) -t plain $< -o $@.tmp
	$Q mv $@.tmp $@

# == installation rules ==
$(call INSTALL_DATA_RULE, docs/doc.files, $(DESTDIR)$(docs/doc.dir), $(docs/doc.files))
