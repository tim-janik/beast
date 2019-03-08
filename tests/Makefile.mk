# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/tests/*.d)
CLEANDIRS += $(wildcard $>/tests/)
tests/rpath..libbse ::= ../lib

# == tests/ files ==
tests/suite1.sources		::= $(strip	\
	tests/blocktests.cc			\
	tests/filterdesign.cc			\
	tests/filtertest.cc			\
	tests/firhandle.cc			\
	tests/loophandle.cc			\
	tests/misctests.cc			\
	tests/resamplehandle.cc			\
	tests/subnormals-aux.cc			\
	tests/subnormals.cc			\
	tests/suite1-main.cc			\
	tests/suite1-randomhash.cc		\
	tests/testfft.cc			\
	tests/testresampler.cc			\
	tests/testresamplerq.cc			\
	tests/testwavechunk.cc			\
)

# == suite1 defs ==
tests/suite1			::= $>/tests/suite1
tests/suite1.objects		::= $(sort $(tests/suite1.sources:%.cc=$>/%.o))

# == subdirs ==
include tests/audio/Makefile.mk

# == suite1 rules ==
$(tests/suite1.objects):	$(bse/libbse.deps) | $>/tests/
$(tests/suite1.objects):	EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_TEST, \
	$(tests/suite1), \
	$(tests/suite1.objects), \
	$(lib/libbse.so), \
	-lbse-$(VERSION_MAJOR) $(GLIB_LIBS), \
	$(tests/rpath..libbse))
