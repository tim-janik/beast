# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = v8bse .

EXTRA_DIST += $(strip \
)

# == all: ==
all:
	@echo "$$(basename $$(pwd))/: experimental code, not built by default. Usage:"
	@echo "	make app	# build the v8 Bse module and app files"
	@echo "	make run	# run the electron app in the current directory"
	@echo "	make clean	# remove build files"

# == package.json ==
package.json: package.json.in ../config.status
	$(AM_V_GEN)
	$(Q) ../config.status --file package.json
	$(Q) npm install
clean-local: clean-node_modules
clean-node_modules: ; rm -rf node_modules/ package.json

# == app ==
app:	package.json
	$(Q) $(MAKE) $(AM_MAKEFLAGS) -C v8bse v8bse.node

# == run ==
run: app
	LD_PRELOAD="$(abs_top_builddir)/bse/.libs/libbse-@MAJOR@.so" \
	./node_modules/electron/dist/electron .
# NOTE, prefer LD_PRELOAD over LD_LIBRARY_PATH, to pick up $(builddir)/libbse *before* /usr/lib/libbse
