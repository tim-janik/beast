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
launchers/beast.objects ::= $(sort $(launchers/beast.sources:%.c=$>/%.o))
launchers/beast.deps    ::= $(bse/libbse.deps)

# == beast rules ==
$(launchers/beast.objects): $(launchers/beast.deps)				| $>/launchers/
$(launchers/beast.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_PROGRAM, \
	$(launchers/beast), \
	$(launchers/beast.objects), \
	$(lib/libbse.so) | $>/launchers/, \
	-lbse-$(VERSION_MAJOR) $(GLIB_LIBS), ../lib)
