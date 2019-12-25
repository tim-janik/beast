# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/tools/*.d)
CLEANDIRS += $(wildcard $>/tools/)
tools/rpath..libbse ::= ../lib

# == bsetool defs ==
tools/bsetool.sources ::= $(strip	\
	tools/bsetool.cc		\
	tools/magictest.cc		\
	tools/bsefcompare.cc		\
	tools/bsefextract.cc		\
)
tools/bsetool         ::= $>/tools/bsetool
tools/bsetool.objects ::= $(call BUILDDIR_O, $(tools/bsetool.sources))
tools/bsetool.deps    ::= $(bse/libbse.deps)
# CUSTOMIZATIONS: tools/bsetool.cc.FLAGS = -O2   ||   $>/tools/bsetool.o.FLAGS = -O3    ||    $>/tools/bsetool.o: EXTRA_FLAGS = -O1

# == bsetool rules ==
$(tools/bsetool.objects): $(tools/bsetool.deps)				| $>/tools/
$(tools/bsetool.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_PROGRAM, \
	$(tools/bsetool), \
	$(tools/bsetool.objects), \
	$(lib/libbse.so) | $>/tools/, \
	-lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS), ../lib)
