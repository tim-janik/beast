# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/ebeast/*.d)
CLEANDIRS         += $(wildcard $>/ebeast/)
ALL_TARGETS       += ebeast-all
CHECK_TARGETS     += ebeast-check
INSTALL_TARGETS   += ebeast-install
UNINSTALL_TARGETS += ebeast-uninstall
ebeast-v8bse-all: $>/ebeast/v8bse/v8bse.node

# == variables ==
ebeast/v8bse/cc.sources  ::= $(strip	\
	$>/ebeast/v8bse/nodemodule.cc	\
)
ebeast/v8bse/gyp.inputs   ::= $(notdir $(ebeast/v8bse/cc.sources))
ebeast/v8bse/gyp.notflags ::= -fno-exceptions -fno-rtti -std=gnu++0x
ebeast/v8bse/gyp.ccflags  ::= $(strip	\
	-Wno-type-limits -Wno-unknown-pragmas -Wno-implicit-fallthrough \
	-Wno-unused-but-set-variable -Wno-unused-variable \
)
ebeast/v8bse/gyp.cxxflags   = $(CXXSTD) -fPIC $(DEFS) $(EXTRA_DEFS) $($@.DEFS) $(pkgcxxflags) $(EXTRA_FLAGS) $($@.FLAGS) $(ebeast/v8bse/gyp.ccflags)
ebeast_v8bse/gyp.incdirs    = $(abspath $(patsubst -%, , $(patsubst -I%, %, $(INCLUDES) $(EXTRA_INCLUDES) $($@.INCLUDES))))
ebeast/v8bse/gyp.libs       = -L$(abspath $>/bse/) -lbse-$(VERSION_MAJOR)

# == v8bse.cc ==
# generate v8 Bse bindings
$>/ebeast/v8bse/v8bse.cc: bse/bseapi.idl ebeast/v8bse/V8Stub.py $(aidacc/aidacc)	| $>/ebeast/v8bse/
	$(QECHO) AIDACC $@
	$Q $(aidacc/aidacc) -x ebeast/v8bse/V8Stub.py $< -o $@.tmp -G strip-path=$(abspath $>)/
	$Q mv $@.tmp $@
$>/ebeast/v8bse/%.cc: ebeast/v8bse/%.cc		| $>/ebeast/v8bse/
	$(QECHO) COPY $@
	$Q cp -P $< $@

# == v8bse.node ==
# Note, node-gyp is the standard way to build nodejs modules and is configured via binding.gyp,
# so we go through some lengths here to make that work.
# I.e. we need to pass CXXFLAGS and more on to binding.gyp, setup $HOME, because node-gyp caches downloads
# in $HOME, it requires the electron target version to pick V8 headers and it calls $(MAKE) internally,
# so we need to unset MAKEFLAGS, to unconfuse the node-gyp invocation of MAKE wrg jobserver setups.
# See also: /usr/include/nodejs/common.gypi https://electronjs.org/docs/tutorial/using-native-node-modules
$>/ebeast/v8bse/v8bse.node: EXTRA_INCLUDES ::= -I$> -Iexternal/v8pp/ $(GLIB_CFLAGS)
$>/ebeast/v8bse/v8bse.node: $>/ebeast/v8bse/v8bse.cc $(ebeast/v8bse/cc.sources) $(bse/libbse.solinks) $>/ebeast/npm.rules
	$(QECHO) 'SETUP' $>/ebeast/v8bse/binding.gyp
	@: # binding.gyp must be generated on the fly, because it captures Makefile values like CXXFLAGS
	$Q echo "{ 'targets': [ {                           # -*- mode: javascript -*-"		 >$@.tmp
	$Q echo "  'target_name': 'v8bse',"							>>$@.tmp
	$Q echo "  'sources':      [ $(patsubst %, '%'$(,), $(ebeast/v8bse/gyp.inputs)) ],"	>>$@.tmp
	$Q echo "  'cflags!':      [ $(patsubst %, '%'$(,), $(ebeast/v8bse/gyp.notflags)) ],"	>>$@.tmp
	$Q echo "  'cflags_cc!':   [ $(patsubst %, '%'$(,), $(ebeast/v8bse/gyp.notflags)) ],"	>>$@.tmp
	$Q echo "  'cflags_cc':    [ $(patsubst %, '%'$(,), $(ebeast/v8bse/gyp.cxxflags)) ],"	>>$@.tmp
	$Q echo "  'include_dirs': [ $(patsubst %, '%'$(,), $(ebeast_v8bse/gyp.incdirs)) ],"	>>$@.tmp
	$Q echo "  'libraries':    [ $(patsubst %, '%'$(,), $(ebeast/v8bse/gyp.libs))"		>>$@.tmp
	$Q echo "                    \"'-Wl,-rpath,"'$$$$'"ORIGIN/../../../lib/'\" ],"		>>$@.tmp
	@: # Adding -rpath,'$ORIGIN' requires single-quotes for this Makefile's subshell, escaping '$' in the current
	@: # Makefile, escaping '$' in the generated *.target.mk file and signle-quotes for the *.target.mk subshell.
	$Q echo "  } ] }"									>>$@.tmp
	$Q mv $@.tmp $>/ebeast/v8bse/binding.gyp
	$(QECHO) 'COMPILE' $@
	$Q rm -fr $@ $>/ebeast/v8bse/build/
	@: # Due to 'cd $(@D)', paths are relative to $>/ebeast/v8bse/
	$Q cd $(@D) \
	  && sed -n '/^ \ "version":/s/.*"\([0-9.]\+\)".*/\1/p'				\
		    ../node_modules/electron/package.json > $(@F).tmpev			\
	  && ELECTRON_VERSION=`grep '^[0-9.]\+$$' $(@F).tmpev` && rm $(@F).tmpev	\
	  && CXX="$(CCACHE) $(CXX)"							\
		HOME='../node_modules/node-gyp/cache/' MAKEFLAGS=''			\
		../node_modules/.bin/node-gyp --target="$$ELECTRON_VERSION"		\
		  rebuild --dist-url=https://atom.io/download/electron			\
		  $(if $(findstring 1, $(V)) , --verbose, --silent)
	$Q ln -sv build/Release/v8bse.node $>/ebeast/v8bse/
	@: # Note, we leave cleaning of ebeast/node_modules/node-gyp/cache/ to ../node_modules/ cleanups
$>/ebeast/v8bse/v8bse.node: $(config-stamps) ebeast/v8bse/Makefile.mk	# Rebuild binding.gyp once makefiles change

