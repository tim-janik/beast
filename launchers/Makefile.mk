# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/launchers/*.d)
CLEANDIRS += $(wildcard $>/launchers/)
launchers/rpath..libbse ::= ../lib

# == beast defs ==
launchers/beast.sources ::= $(strip	\
	launchers/suidmain.c		\
	launchers/beaststart.c		\
)
launchers/beast         ::= $>/launchers/beast
launchers/beast.objects ::= $(call BUILDDIR_O, $(launchers/beast.sources))
launchers/beast.deps    ::= $(bse/libbse.deps)

# == beast rules ==
$(launchers/beast.objects): $(launchers/beast.deps)				| $>/launchers/
$(launchers/beast.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_PROGRAM, \
	$(launchers/beast), \
	$(launchers/beast.objects), \
	$(lib/libbse.so) | $>/launchers/, \
	-lbse-$(VERSION_MAJOR) $(GLIB_LIBS), ../lib)
$(call INSTALL_BIN_RULE, bin/beast, $(DESTDIR)$(pkglibdir)/bin, $(launchers/beast))
install--bin/beast: INSTALL_RULE.post-hook ::= \
	$Q echo '  MKSUID  ' .../bin/beast \
	  && cd $(DESTDIR)$(pkglibdir)/bin/ \
	  && ( chown root beast && chmod 4755 beast ) \
	  || ( echo -e "***\n*** WARNING: $(DESTDIR)$(pkglibdir)/bin/beast needs to be installed as root to allow renicing \\_(o.o)_/\n***" && sleep 1 )
