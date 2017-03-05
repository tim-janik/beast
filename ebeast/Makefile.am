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
	$(GTK2_DARK) \
	LD_PRELOAD="$(abs_top_builddir)/bse/.libs/libbse-@MAJOR@.so" \
	./node_modules/electron/dist/electron .
# NOTE1, prefer LD_PRELOAD over LD_LIBRARY_PATH, to pick up $(builddir)/libbse *before* /usr/lib/libbse
# NOTE2, add --js-flags="--expose-gc" to the command line to enable global.gc();
# If libdbusmenu-glib.so is missing, electron 1.4.15 displays a Gtk+2 menu bar, ignoring
# the BrowserWindow.darkTheme option. Here, we preselect a commonly installed dark Gtk+2
# theme if it's present.
GTK2_DARK = $(shell test -f /usr/share/themes/Ambiance/gtk-2.0/gtkrc && echo 'GTK2_RC_FILES=/usr/share/themes/Ambiance/gtk-2.0/gtkrc')
