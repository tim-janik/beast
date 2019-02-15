# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/tests/*.d)
CLEANDIRS += $(wildcard $>/tests/)
tests-rpath.bse ::= ../bse

# == check ==
tests-check: .PHONY
check: tests-check

# == suite1 ==
tests/suite1.sources		::= $(strip	\
	tests/filterdesign.cc			\
	tests/filtertest.cc			\
	tests/misctests.cc			\
	tests/suite1-main.cc			\
	tests/suite1-randomhash.cc		\
	tests/testfft.cc			\
	tests/testresampler.cc			\
	tests/testresamplerq.cc			\
	tests/testwavechunk.cc			\
)
tests/suite1			::= $>/tests/suite1
ALL_TARGETS			 += $(tests/suite1)
tests/suite1.objects		::= $(sort $(tests/suite1.sources:%.cc=$>/%.o))
$(tests/suite1.objects):	$(bse/libbse.deps) | $>/tests/
$(tests/suite1.objects):	EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(eval $(call LINKER, $(tests/suite1), $(tests/suite1.objects), $(bse/libbse.solinks), -lbse-$(VERSION_MAJOR) $(GLIB_LIBS), $(tests-rpath.bse)) )

# == check suite1 ==
tests-check-suite1: .PHONY	| $(tests/suite1)
	$(QECHO) RUN… $@
	$Q $(tests/suite1)
tests-check: tests-check-suite1
