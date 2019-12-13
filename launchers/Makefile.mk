# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/launchers/*.d)
CLEANDIRS += $(wildcard $>/launchers/)

# == bselauncher defs ==
launchers/bselauncher.sources ::= $(strip	\
	launchers/suidmain.c			\
	launchers/beaststart.c			\
)
lib/BeastSoundEngine-rt       ::= lib/BeastSoundEngine-$(VERSION_M.M.M)-rt
launchers/bselauncher.objects ::= $(call BUILDDIR_O, $(launchers/bselauncher.sources))
ALL_TARGETS                    += $>/$(lib/BeastSoundEngine-rt)

# == beast rules ==
$(launchers/bselauncher.objects): 		| $>/launchers/
$(call BUILD_PROGRAM, \
	$>/$(lib/BeastSoundEngine-rt), \
	$(launchers/bselauncher.objects), \
	| $>/launchers/, \
	, ../lib)
$(call INSTALL_BIN_RULE, BeastSoundEngine-rt, $(DESTDIR)$(pkglibdir)/lib, $>/$(lib/BeastSoundEngine-rt))
install--BeastSoundEngine-rt: INSTALL_RULE.post-hook ::= \
	$Q echo '  MKSUID  ' .../$(lib/BeastSoundEngine-rt) \
	  && ( chown root $(DESTDIR)$(pkglibdir)/$(lib/BeastSoundEngine-rt) && chmod 4755 $(DESTDIR)$(pkglibdir)/$(lib/BeastSoundEngine-rt) ) \
	  || ( echo -e "***\n*** WARNING: $(DESTDIR)$(pkglibdir)/$(lib/BeastSoundEngine-rt) needs to be installed as root to allow renicing \\_(o.o)_/\n***" && sleep 1 )
