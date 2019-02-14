# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/bse/tests/*.d)
CLEANDIRS += $(wildcard $>/bse/tests/)


# == integrity ==
bse/tests/integrity		::= $>/bse/tests/integrity
ALL_TARGETS			 += $(bse/tests/integrity)
bse/tests/integrity.sources	::= bse/tests/integrity.cc
bse/tests/integrity.objects	::= $(sort $(bse/tests/integrity.sources:%.cc=$>/%.o))
$(bse/tests/integrity.objects):	$(bse/libbse.deps) | $>/bse/tests/
$(bse/tests/integrity.objects):	EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
bse/tests/integrity.objects.FLAGS = -O0
$(eval $(call LINKER, $(bse/tests/integrity), $(bse/tests/integrity.objects), $(bse/libbse.solinks), -lbse-$(VERSION_MAJOR), ../../bse) )

# == check ==
bse-tests-check: .PHONY
check: bse-tests-check

# == bse-tests-check-integrity ==
bse-tests-check-integrity: .PHONY	| $(bse/tests/integrity)
	$(QECHO) RUN… $@
	$Q $(bse/tests/integrity)
bse-tests-check: bse-tests-check-integrity

# == bse-tests-check-assertions ==
$>/bse/tests/t279-assertions-test: .PHONY	| $(bse/tests/integrity)
	$(QECHO) RUN $@
	$Q $(bse/tests/integrity) --return_unless1 || $(QDIE) --return_unless1 failed
	$Q $(bse/tests/integrity) --assert_return1 || $(QDIE) --assert_return1 failed
	$Q (trap ':' SIGTRAP && $(bse/tests/integrity) --return_unless0) $(QSTDERR) ; test "$$?" -eq 7 || $(QDIE) --return_unless0 failed
	$Q (trap ':' SIGTRAP && $(bse/tests/integrity) --assert_return0) $(QSTDERR) ; test "$$?"  != 0 || $(QDIE) --assert_return0 failed
	$Q (trap ':' SIGTRAP && $(bse/tests/integrity) --assert_return_unreached) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --assert_return_unreached failed
	$Q (trap ':' SIGTRAP && $(bse/tests/integrity) --fatal_error) $(QSTDERR) ; test "$$?" != 0 || $(QDIE) --fatal_error failed
	$Q $(bse/tests/integrity) --backtrace	2> $@.tmp && \
		grep -qi 'Backtrace'		   $@.tmp && \
		grep -qi 'in.*my_compare_func'	   $@.tmp || $(QDIE) --backtrace failed
	$Q rm $@.tmp
	@echo "  PASS    " $@
bse-tests-check: $>/bse/tests/t279-assertions-test
