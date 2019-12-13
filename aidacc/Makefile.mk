# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/aidacc/*.d)
aidacc/cleandirs ::= $(wildcard $>/aidacc/)
CLEANDIRS         += $(aidacc/cleandirs)
aidacc/check: FORCE
CHECK_TARGETS += aidacc/check
aidacc/all:
ALL_TARGETS += aidacc/all

# == aidacc/ files ==
aidacc/include.headers ::= $(strip	\
	aidacc/aida.hh			\
)
aidacc/aidacc.imports ::= $(strip	\
	aidacc/AuxData.py		\
	aidacc/CxxStub.py		\
	aidacc/Decls.py			\
	aidacc/ExtractDocs.py		\
	aidacc/GenUtils.py		\
	aidacc/PrettyDump.py		\
	aidacc/yapps2runtime.py		\
)
aidacc/aidacc.generated ::= $(strip	\
	$>/aidacc/IdStrings.py		\
	$>/aidacc/Parser.py		\
	$>/aidacc/TmplFiles.py		\
)
aidacc/aidacc.templates ::= $(strip	\
	aidacc/CxxStub-client.cc	\
	aidacc/CxxStub-server.cc	\
)

# == aidacc defs ==
aidacc/aidacc		::= $>/aidacc/aidacc
aidacc/aidacc.config	::= "aidaccpydir" : "../aidacc", "AIDA_VERSION" : "${VERSION_SHORT}"

# == subdirs ==
include aidacc/tests/Makefile.mk

# == aidacc rules ==
$(aidacc/aidacc): aidacc/main.py	$(aidacc/aidacc.imports) $(aidacc/aidacc.generated)
	$(QGEN)
	$Q cp $(aidacc/aidacc.imports) $>/aidacc/
	$Q sed < $<		> $@.tmp \
	  -e '1,24s|^ *#@PKGINSTALL_CONFIGVARS_IN24LINES@|  ${aidacc/aidacc.config}|'
	$Q chmod +x $@.tmp
	$Q mv $@.tmp $@
aidacc/all: $(aidacc/aidacc)

# == IdStrings.py ==
$>/aidacc/IdStrings.py: aidacc/aida.hh	aidacc/ExtractIdStrings.py	| $>/aidacc/
	$(QGEN)
	$Q $(PYTHON2) aidacc/ExtractIdStrings.py $< > $@.tmp
	$Q mv $@.tmp $@

# == Parser.py ==
$>/aidacc/Parser.py: aidacc/Parser.g	| $>/aidacc/
	$(QGEN)
	$Q $(YAPPS) $< $@.tmp 2>&1 | tee $@.errors			# yapps exit code is bogus
	$Q ! grep -q '.' $@.errors					# catch all yapps errors & warnings
	$Q sed < $@.tmp > $@.tmp2 -e 's/^from yapps import runtime$$//'	# Parser.g has all imports
	$Q mv $@.tmp2 $@ && rm $@.tmp $@.errors

# == TmplFiles.py ==
$>/aidacc/TmplFiles.py: $(aidacc/aidacc.templates)	| $>/aidacc/
	$(QGEN)
	$Q ( echo "# $@: generated from: $^" \
	  && for file in $^ ; \
	    do echo -n "$$(basename $$file)" | sed 's/[^a-zA-Z0-9_]/_/g' && echo ' = """' \
	    && sed 's/\\/\\\\/g; s/"""/\\"""/g' "$$file" && echo '"""' || exit $$? ; \
	  done ) > $@.tmp
	$Q mv $@.tmp $@

# == installation ==
$(call INSTALL_DATA_RULE,			\
	aidacc/headers,				\
	$(DESTDIR)$(pkglibdir)/include/aidacc,	\
	$(aidacc/include.headers))

# == build-test ==
aidacc/build-test: FORCE		| $(aidacc/aidacc)
	$(QECHO) RUN $@
	$Q $(aidacc/aidacc) -x CxxStub --list-formats | grep -q CxxStub
aidacc/check: aidacc/build-test

# == aidacc/clean ==
aidacc/clean: FORCE
	rm -f -r $(aidacc/cleandirs)
