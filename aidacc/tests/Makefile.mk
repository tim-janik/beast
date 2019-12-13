# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/aidacc/tests/*.d)
CLEANDIRS += $(wildcard $>/aidacc/tests/)
aidacc/tests/rpath..libbse ::= ../../lib

# == check--t301-pass.idl ==
aidacc/tests/check--t301-pass.idl: aidacc/tests/t301-pass.idl $(aidacc/aidacc)
	$(QECHO) CHECK $@
	$Q $(aidacc/aidacc) $< # --aida-debug
aidacc/check: aidacc/tests/check--t301-pass.idl

# == check--t302-fail.idl ==
aidacc/tests/check--t302-fail.idl: aidacc/tests/t302-fail.idl aidacc/tests/t302-fail.ref $(aidacc/aidacc)
	$(QECHO) CHECK $@
	$Q $(aidacc/aidacc) --aida-fail-file-test $<	> $@.out
	$Q sed 's/\(:[0-9]\+:[0-9]\+: Trying to find one of \).*/\1.../' < $@.out > $@.notokens && mv $@.notokens $@.out
	$Q diff -up aidacc/tests/t302-fail.ref $@.out && rm -f $@.out
aidacc/check: aidacc/tests/check--t302-fail.idl

# == check--t304-cxxserver-output ==
# test CxxStub Generation for Interfaces
aidacc/tests/check--t304-cxxserver-output: aidacc/tests/t301-pass.idl aidacc/tests/t304-cxxserver.ref $(aidacc/aidacc)	| $>/aidacc/tests/
	$(QECHO) CHECK $@
	$Q cp $< $>/aidacc/tests/t304-testpass.idl
	$Q $(aidacc/aidacc) -x CxxStub -I aidacc/tests -G aidaids -G strip-path=$(abspath $>)/ $>/aidacc/tests/t304-testpass.idl
	$Q cat $>/aidacc/tests/t304-testpass_interfaces.cc		>> $>/aidacc/tests/t304-testpass_interfaces.hh
	$Q diff -up aidacc/tests/t304-cxxserver.ref $>/aidacc/tests/t304-testpass_interfaces.hh
aidacc/check: aidacc/tests/check--t304-cxxserver-output

# == check--t305-idlcode-compile ==
aidacc/tests/check--t305-idlcode-compile: aidacc/tests/t301-pass.idl aidacc/tests/t301-inc1.idl aidacc/tests/t301-inc2.idl $(aidacc/aidacc)	| $>/aidacc/tests/
	$(QECHO) CHECK $@
	$Q cp aidacc/tests/t301-inc1.idl $>/aidacc/tests/t305-inc1.idl
	$Q cp aidacc/tests/t301-inc2.idl $>/aidacc/tests/t305-inc2.idl
	$Q cp aidacc/tests/t301-pass.idl $>/aidacc/tests/t305-pass.idl
	$Q $(aidacc/aidacc) -x CxxStub -I aidacc/tests -G strip-path=$(abspath $>)/ $>/aidacc/tests/t305-inc1.idl
	$Q $(aidacc/aidacc) -x CxxStub -I aidacc/tests -G strip-path=$(abspath $>)/ $>/aidacc/tests/t305-inc2.idl
	$Q $(aidacc/aidacc) -x CxxStub -I aidacc/tests -G strip-path=$(abspath $>)/ $>/aidacc/tests/t305-pass.idl
	$Q sed -e '1i#undef _' -e '1i#define _(x) x' \
	       -e '1i#include "t305-inc2_interfaces.cc"' \
	       -e '1i#include "t305-inc1_interfaces.cc"' \
	       -i $>/aidacc/tests/t305-pass_interfaces.cc
	$Q $(CCACHE) $(CXX) $(CXXSTD) -I $(abspath .) -c $>/aidacc/tests/t305-pass_interfaces.cc -o $>/aidacc/tests/t305-pass_interfaces.o
aidacc/check: aidacc/tests/check--t305-idlcode-compile
