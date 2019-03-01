# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/doc/*.d)
CLEANDIRS += $(wildcard $>/doc/)

# == docs/ files ==
docs/doc.files ::= $(strip		\
	$>/doc/copyright		\
)
docs/doc.dir   ::= $(pkglibdir)/doc
ALL_TARGETS     += $(docs/doc.files)

# == doc build rules ==
$>/doc/copyright: docs/copyright		| $>/doc/
	$(QECHO) COPY $@
	$Q cp -L $< $@

# == installation rules ==
$(call INSTALL_DATA_RULE, docs/doc.files, $(DESTDIR)$(docs/doc.dir), $(docs/doc.files))
