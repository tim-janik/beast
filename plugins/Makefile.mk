# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/plugins/*.d $>/plugins/freeverb/*.d)
CLEANDIRS += $(wildcard $>/plugins/)
plugins/rpath..libbse ::= ../lib

# == cxxplugins.so defs ==
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
plugins/cxxplugins.so		::= $>/plugins/cxxplugins.so
plugins/cxxplugins.objects	::= $(sort $(plugins/cxxplugins.sources:%.cc=$>/%.o))

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

# == bseplugins.so defs ==
plugins/bseplugins.so		::= $>/plugins/bseplugins.so
plugins/bseplugins.objects	::= $(sort $(plugins/bseplugins.sources:%.cc=$>/%.o))
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

# == freeverb.so ==
plugins/freeverb.sources = $(strip		\
	plugins/freeverb/allpass.cc		\
	plugins/freeverb/comb.cc		\
	plugins/freeverb/revmodel.cc		\
	plugins/freeverb/bsefreeverbcpp.cc	\
	plugins/freeverb/bsefreeverb.cc		\
)
plugins/freeverb.so		::= $>/plugins/freeverb.so
plugins/freeverb.objects	::= $(sort $(plugins/freeverb.sources:%.cc=$>/%.o))
# rules
$(plugins/freeverb.objects): $(bse/libbse.deps) | $>/plugins/freeverb/
$(plugins/freeverb.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_SHARED_LIB, \
	$(plugins/freeverb.so), \
	$(plugins/freeverb.objects), \
	$(lib/libbse.so) | $>/plugins/freeverb/, \
	-L$>/lib -lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS), \
	$(plugins/rpath..libbse))
$(call INSTALL_BIN_RULE, plugins/freeverb, \
	$(DESTDIR)$(pkglibdir)/plugins, \
	$(plugins/freeverb.so))

# == install rules ==
$(call INSTALL_BIN_RULE, plugins/modules, \
	$(DESTDIR)$(pkglibdir)/plugins, \
	$(plugins/cxxplugins.so) $(plugins/bseplugins.so))

# == cxxplugins.so rules ==
$(plugins/cxxplugins.objects): $(bse/libbse.deps) | $>/plugins/
$(plugins/cxxplugins.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_SHARED_LIB, \
	$(plugins/cxxplugins.so), \
	$(plugins/cxxplugins.objects), \
	$(lib/libbse.so) | $>/plugins/, \
	-lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS), \
	$(plugins/rpath..libbse))

# == bseplugins.so rules ==
$(plugins/bseplugins.objects): $(bse/libbse.deps) | $>/plugins/
$(plugins/bseplugins.objects): EXTRA_INCLUDES ::= -I$> -I$>/plugins/ $(GLIB_CFLAGS)
$(call BUILD_SHARED_LIB, \
	$(plugins/bseplugins.so), \
	$(plugins/bseplugins.objects), \
	$(lib/libbse.so) | $>/plugins/, \
	-lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS), \
	$(plugins/rpath..libbse))

# == .genidl.hh ==
$>/plugins/%.genidl.hh: plugins/%.idl		$(sfi/sfidl) | $>/plugins/
	$(QGEN)
	$Q $(sfi/sfidl) $(sfi/sfidl.includes)	--plugin  --macro $(<F) -I. $<	> $@.tmp
	$Q mv $@.tmp $@
# IDL dependencies
$(plugins/bseplugins.idlfiles:%.idl=$>/%.o): $(plugins/bseplugins.idlfiles:%.idl=$>/%.genidl.hh)
# Example: $>/plugins/artscompressor.o $>/plugins/artscompressor.genidl.hh
