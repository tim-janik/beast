# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/images/*.d)
CLEANDIRS += $(wildcard $>/images/)

# == images/ files ==
images/img.files ::= $(strip		\
	$>/images/beast-logo.png	\
	$>/images/beast-mime.png	\
	$>/images/beast-splash.png	\
	$>/images/beast.png		\
	$>/images/bse-mime.png		\
)
images/img.dir     ::= $(pkglibdir)/images
ALL_TARGETS	    += $(images/img.files)

# == image build rule ==
$>/images/%.png: images/%.png			| $>/images/
	$(QGEN)
	$Q cp -L $< $@

# == installation rules ==
$(call INSTALL_DATA_RULE, images/img.files, $(DESTDIR)$(images/img.dir), $(images/img.files))
