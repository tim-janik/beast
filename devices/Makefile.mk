# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(call rwildcard, $>/devices/, *.d)
devices/cleandirs ::= $(wildcard $>/devices/)
CLEANDIRS          += $(devices/cleandirs)

# == devices/ definitions ==
# collect .cc sources under devices/
devices/libbse.ccfiles ::= $(call rwildcard, devices/, *.cc)
# derive object names
devices/libbse.objects ::= $(call BUILDDIR_O, $(devices/libbse.ccfiles))
# extract object directories
devices/libbse.objdirs ::= $(sort $(dir $(devices/libbse.objects)))

# == devices/ dependencies ==
# create object directories via explicit object dependency
$(devices/libbse.objects): | $(devices/libbse.objdirs)

# == devices/ rules ==
$(devices/libbse.objects): | $>/devices/check-stray-sources
$>/devices/check-stray-sources: $(devices/libbse.ccfiles)
	$(QECHO) CHECK devices/: check stray sources
	$(Q) test -z "$(DOTGIT)" || { \
	  git status -s -u -- $^ | { grep '^?? .*' && \
		{ echo 'devices/: error: untracked source files present'; exit 3 ; } || :; \
	  }; }
	$(Q) echo '$^' > $@
ifneq ($(devices/libbse.ccfiles), $(shell cat $>/devices/check-stray-sources 2>/dev/null))
.PHONY: $>/devices/check-stray-sources
endif
