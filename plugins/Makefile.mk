# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/plugins/*.d)
CLEANDIRS += $(wildcard $>/plugins/)

# == files ==
plugins/cxxplugins.sources = $(strip	\
	plugins/bseadder.cc		\
	plugins/bseatandistort.cc	\
	plugins/bseiirfilter.cc		\
	plugins/bsemixer.cc		\
	plugins/bsemult.cc		\
	plugins/bsesequencer.cc		\
	plugins/bsesimpleadsr.cc	\
	plugins/davcanyondelay.cc	\
	plugins/davsyndrum.cc		\
	plugins/davxtalstrings.cc	\
)
plugins/cxxplugins.objects ::= $(sort $(plugins/cxxplugins.sources:%.cc=$>/%.o))

# == cxxplugins ==
cxxplugins.so			::= cxxplugins.so
cxxplugins.soname		::= $(cxxplugins.so)
plugins/cxxplugins.sofile	::= $>/plugins/$(cxxplugins.so)
ALL_TARGETS			 += $(plugins/cxxplugins.sofile)
$(plugins/cxxplugins.objects): $(bse/libbse.deps) | $>/plugins/
$(plugins/cxxplugins.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(plugins/cxxplugins.sofile).LDFLAGS ::= -shared -Wl,-soname,$(cxxplugins.soname)
$(eval $(call LINKER, $(plugins/cxxplugins.sofile), $(plugins/cxxplugins.objects), $(bse/libbse.solinks) | $>/plugins/, -lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS), ../bse))


