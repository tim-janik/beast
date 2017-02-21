# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = v8bse .

EXTRA_DIST += $(strip \
)

# == all: ==
all:
	@echo "$$(basename $$(pwd))/: experimental code, not built by default. Usage:"
	@echo "	make npm	# install electron and build tools locally"
	@echo "	make app	# build the v8 Bse module and app files"
	@echo "	make run	# run the electron app in the current directory"
	@echo "	make clean	# remove build files"
	@echo "	make npmclean	# remove npm installations and make clean"

# == electron ==
npm:
	npm install --save electron		# ./node_modules/electron/dist/electron
	npm install --save-dev node-gyp		# builds build/Release/v8bse.node
node_modules/: npm
clean-node_modules:
	rm -rf node_modules/
npmclean: clean-node_modules clean

# == app ==
app:
	$(Q) $(MAKE) $(AM_MAKEFLAGS) -C v8bse v8bse.node

# == run ==
run: app	# requires: npm
	LD_PRELOAD="$(abs_top_builddir)/bse/.libs/libbse-@MAJOR@.so" \
	./node_modules/electron/dist/electron .
# NOTE, prefer LD_PRELOAD over LD_LIBRARY_PATH, to pick up $(builddir)/libbse *before* /usr/lib/libbse
