# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CXXFLAGS  += $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE -DG_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS
AIDACC_VDEBUG = $(AIDACC) $(if $(findstring 1, $(V)), --aida-debug)
CLEANDIRS     =

# == v8bse ==
AIDASTDINC = $(shell ${AIDACC} -x V8Stub.py -G --print-include-path /dev/null)
# v8 Bse bindings
v8bse.cc: ../../bse/bseapi.idl ./V8Stub.py
	$(AM_V_GEN)
	$(Q) ${AIDACC} -x V8Stub.py $< -o $@ -G strip-path=$(abs_top_srcdir)/
	@touch v8bse.cc # FIXME
CLEANFILES += v8bse.cc *.pyc *.pyo
# v8 'Bse' node module
NODE_GYP = $(strip \
	HOME=./node_modules/.electron-gyp \
	CXX="$(CXX)" CXXFLAGS="$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS)" \
	./node_modules/.bin/node-gyp \
	$(if $(findstring 1, $(V)) , --verbose) \
	--target=$(shell ./node_modules/electron/dist/electron --version) \
)
v8bse.node: v8bse.cc ./node_modules/electron/dist/electron
	$(AM_V_GEN)
	$(Q) rm -f v8bse.node build/Release/v8bse.node
	$(Q) $(NODE_GYP) rebuild --dist-url=https://atom.io/download/electron
	$(Q) ln -sv build/Release/v8bse.node
all: v8bse.node
CLEANFILES += v8bse.node
CLEANDIRS += build/

# == electron ==
npm:
	npm install --save electron		# ./node_modules/electron/dist/electron
	npm install --save-dev node-gyp		# builds build/Release/v8bse.node
node_modules/: npm
clean-node_modules:
	rm -rf node_modules/

# == clean ==
clean-local: cleandirs # clean-node_modules
cleandirs:
	rm -rf $(CLEANDIRS)

EXTRA_DIST += $(strip \
)
