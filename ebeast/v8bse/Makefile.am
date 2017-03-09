# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CXXFLAGS  += $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE -DG_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS
AIDACC_VDEBUG = $(AIDACC) $(if $(findstring 1, $(V)), --aida-debug)
CLEANDIRS     =

# == all: ==
all:
	@echo "$$(basename $$(pwd))/: build via ../ebeast/ rules"

# == v8bse.cc ==
AIDASTDINC = $(shell ${AIDACC} -x V8Stub.py -G --print-include-path /dev/null)
# v8 Bse bindings
v8bse.cc: ../../bse/bseapi.idl ./V8Stub.py
	$(AM_V_GEN)
	$(Q) ${AIDACC} -x V8Stub.py $< -o $@ -G strip-path=$(abs_top_srcdir)/
	@touch v8bse.cc # FIXME
CLEANFILES += v8bse.cc *.pyc *.pyo

# == v8bse.node ==
v8bse_sources = nodemodule.cc
v8bse_deps    = v8bse.cc binding.gyp
v8bse_rmflags = -fno-exceptions -fno-rtti
v8bse_incdirs = ../../ ../../external/v8pp/ $(patsubst -%, , $(patsubst -I%, %, $(LIBBSE_CFLAGS)))
v8bse_libs    = $(LIBBSE_LIBS) -L$(abs_top_builddir)/bse/.libs -lbse-@MAJOR@
v8bse.node: $(v8bse_sources) $(v8bse_deps) ../node_modules/electron/dist/electron
	$(AM_V_GEN)
	$(Q) rm -f v8bse.node build/Release/v8bse.node
	$(Q) $(NODE_GYP) rebuild --dist-url=https://atom.io/download/electron
	$(Q) ln -sv build/Release/v8bse.node
CLEANFILES += v8bse.node
clean-local: clean-build
clean-build: ; rm -rf build/
NODE_GYP = $(strip \
	HOME=../node_modules/.electron-gyp \
	CXX="$(CXX)" CXXFLAGS="$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS)" \
	../node_modules/.bin/node-gyp \
	--target=$(shell ../node_modules/electron/dist/electron --version) \
	$(if $(findstring 1, $(V)) , --verbose) \
)

# == binding.gyp ==
cma = ,
binding.gyp: Makefile # binding.gyp needs Makefile variables
	$(AM_V_GEN)
	@echo "{                                             # -*- mode: javascript -*-" >$@
	@echo "  'targets': ["								>>$@
	@echo "    {"									>>$@
	@echo "      'target_name': 'v8bse',"						>>$@
	@echo "      'sources':      [ $(patsubst %, '%'$(cma), $(v8bse_sources)) ],"	>>$@
	@echo "      'cflags!':      [ $(patsubst %, '%'$(cma), $(v8bse_rmflags)) ],"	>>$@
	@echo "      'cflags_cc!':   [ $(patsubst %, '%'$(cma), $(v8bse_rmflags)) ],"	>>$@
	@echo "      'include_dirs': [ $(patsubst %, '%'$(cma), $(v8bse_incdirs)) ],"	>>$@
	@echo "      'libraries':    [ $(patsubst %, '%'$(cma), $(v8bse_libs))"		>>$@
	@echo "                        '-Wl,-rpath,$(libdir)' ],"			>>$@
	@echo "    }"									>>$@
	@echo "  ]"									>>$@
	@echo "}"									>>$@
