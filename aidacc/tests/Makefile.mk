# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/aidacc/tests/*.d)
CLEANDIRS += $(wildcard $>/aidacc/tests/)
aidacc/tests/rpath..libbse ::= ../../lib


# == check--t301-pass.idl ==
aidacc/tests/check--t301-pass.idl: aidacc/tests/t301-pass.idl $(aidacc/aidacc)
	$(QECHO) CHECK $@
	$Q $(aidacc/aidacc) $< # --aida-debug
CHECK_TARGETS += aidacc/tests/check--t301-pass.idl

# == check--t302-fail.idl ==
aidacc/tests/check--t302-fail.idl: aidacc/tests/t302-fail.idl aidacc/tests/t302-fail.ref $(aidacc/aidacc)
	$(QECHO) CHECK $@
	$Q $(aidacc/aidacc) --aida-fail-file-test $<	> $@.out
	$Q sed 's/\(:[0-9]\+:[0-9]\+: Trying to find one of \).*/\1.../' < $@.out > $@.notokens && mv $@.notokens $@.out
	$Q diff -up aidacc/tests/t302-fail.ref $@.out && rm -f $@.out
CHECK_TARGETS += aidacc/tests/check--t302-fail.idl

# == check-output--t304-cxxserver ==
# test CxxStub Generation for Client & Server
aidacc/tests/check-output--t304-cxxserver: aidacc/tests/t301-pass.idl aidacc/tests/t304-cxxserver.ref $(aidacc/aidacc)	| $>/aidacc/tests/
	$(QECHO) CHECK $@
	$Q cp $< $>/aidacc/tests/t304-testpass.idl
	$Q $(aidacc/aidacc) -x CxxStub -I aidacc/tests -G iface-prefix=I_ -G aidaids -G strip-path=$(abspath $>)/ $>/aidacc/tests/t304-testpass.idl
	$Q cat $>/aidacc/tests/t304-testpass_interfaces.cc		>> $>/aidacc/tests/t304-testpass_interfaces.hh
	$Q cat $>/aidacc/tests/t304-testpass_handles.cc			>> $>/aidacc/tests/t304-testpass_handles.hh
	$Q diff -up aidacc/tests/t304-cxxserver.ref $>/aidacc/tests/t304-testpass_interfaces.hh
	$Q diff -up aidacc/tests/t304-cxxclient.ref $>/aidacc/tests/t304-testpass_handles.hh
CHECK_TARGETS += aidacc/tests/check-output--t304-cxxserver

