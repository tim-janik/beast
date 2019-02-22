# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/plugins/*.d)
CLEANDIRS += $(wildcard $>/plugins/)

# == plugin files ==
plugins/cxxplugins.sources = $(strip		\
	plugins/bseadder.cc			\
	plugins/bseatandistort.cc		\
	plugins/bseiirfilter.cc			\
	plugins/bsemixer.cc			\
	plugins/bsemult.cc			\
	plugins/bsesequencer.cc			\
	plugins/bsesimpleadsr.cc		\
	plugins/davcanyondelay.cc		\
	plugins/davsyndrum.cc			\
	plugins/davxtalstrings.cc		\
)
plugins/bseplugins.sources = $(strip		\
	plugins/artscompressor.cc		\
	plugins/bseamplifier.cc			\
	plugins/bsebalance.cc			\
	plugins/bsenoise.cc			\
	plugins/bsequantizer.cc			\
	plugins/bsesummation.cc			\
	plugins/standardsaturator.cc		\
	plugins/standardguspatchenvelope.cc	\
	plugins/bsecontribsampleandhold.cc	\
	plugins/davbassfilter.cc		\
	plugins/davchorus.cc			\
	plugins/davorgan.cc			\
)
plugins/bseplugins.idlfiles = $(strip		\
	plugins/artscompressor.idl		\
	plugins/bseamplifier.idl		\
	plugins/bsebalance.idl			\
	plugins/bsecontribsampleandhold.idl	\
	plugins/bsenoise.idl			\
	plugins/bsequantizer.idl		\
	plugins/bsesummation.idl		\
	plugins/davbassfilter.idl		\
	plugins/davchorus.idl			\
	plugins/davorgan.idl			\
	plugins/standardguspatchenvelope.idl	\
	plugins/standardsaturator.idl		\
)

# == cxxplugins.so defs ==
cxxplugins.so			::= cxxplugins.so
cxxplugins.soname		::= $(cxxplugins.so)
plugins/cxxplugins.sofile	::= $>/plugins/$(cxxplugins.so)
ALL_TARGETS			 += $(plugins/cxxplugins.sofile)
plugins/cxxplugins.objects	::= $(sort $(plugins/cxxplugins.sources:%.cc=$>/%.o))
$(plugins/cxxplugins.sofile).LDFLAGS ::= -shared -Wl,-soname,$(cxxplugins.soname)

# == bseplugins.so defs ==
bseplugins.so			::= bseplugins.so
bseplugins.soname		::= $(bseplugins.so)
plugins/bseplugins.sofile	::= $>/plugins/$(bseplugins.so)
ALL_TARGETS			 += $(plugins/bseplugins.sofile)
plugins/bseplugins.objects	::= $(sort $(plugins/bseplugins.sources:%.cc=$>/%.o))
$(plugins/bseplugins.sofile).LDFLAGS ::= -shared -Wl,-soname,$(bseplugins.soname)

# == cxxplugins.so rules ==
$(plugins/cxxplugins.objects): $(bse/libbse.deps) | $>/plugins/
$(plugins/cxxplugins.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(eval $(call LINKER, $(plugins/cxxplugins.sofile), $(plugins/cxxplugins.objects), $(bse/libbse.sofiles), -lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS), ../bse))

# == bseplugins.so rules ==
$(plugins/bseplugins.objects): $(bse/libbse.deps) | $>/plugins/
$(plugins/bseplugins.objects): EXTRA_INCLUDES ::= -I$> -I$>/plugins/ $(GLIB_CFLAGS)
$(eval $(call LINKER, $(plugins/bseplugins.sofile), $(plugins/bseplugins.objects), $(bse/libbse.sofiles), -lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS), ../bse))

# == .genidl.hh ==
$>/plugins/%.genidl.hh: plugins/%.idl		$(sfi/sfidl) | $>/plugins/
	$(QGEN)
	$Q $(sfi/sfidl) $(sfi/sfidl.includes)	--plugin  --macro $(<F) -I. $<	> $@.tmp
	$Q mv $@.tmp $@
# IDL dependencies
$(plugins/bseplugins.idlfiles:%.idl=$>/%.o): $(plugins/bseplugins.idlfiles:%.idl=$>/%.genidl.hh)
# Example: $>/plugins/artscompressor.o $>/plugins/artscompressor.genidl.hh
