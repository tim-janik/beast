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
app:	package.json xlint-html xlint-js objects/app.css objects/gradient-01.png
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

# == CSS generation ==
objects/app.css: app.less
	$(AM_V_GEN)
	$(Q) mkdir -p objects/.deps/
	$(Q) ./node_modules/.bin/lessc $< $@ --source-map=$@.map
	$(Q) ./node_modules/.bin/lessc $< $@ -M >objects/.deps/app.P
-include objects/.deps/app.P
clean-local: clean-objects
clean-objects: ; rm -rf objects/

# == objects/gradient-01.png ==
# generate non-banding gradient from gradient-01 { -im-convert: "..."; }
# http://www.imagemagick.org/script/command-line-options.php#noise
# http://www.imagemagick.org/Usage/canvas/
objects/gradient-01.png: objects/app.css Makefile
	$(AM_V_GEN)
	$(Q) tr '\n' ' ' <objects/app.css \
	| sed -nr 's/.*\bgradient-01\s*\{[^}]*-im-convert:\s*"([^"]*)"\s*[;}].*/\1/; T; p' >$@.tmp && test -s $@.tmp
	$(Q) convert $$(cat $@.tmp) $@
	$(Q) rm -f $@.tmp
MOSTLYCLEANFILES += objects/gradient-01.png objects/gradient-01.*tmp

# == HTML linting ==
HTML_LINT_FILES = index.html
xlint-html: $(HTML_LINT_FILES)
	$(AM_V_GEN)
	$(Q) rm -f $@.log $@
	$(Q) for file in $(HTML_LINT_FILES) ; do \
	  xmllint --html --noout --valid $$file >>$@.log 2>&1 ; \
	done
	$(Q) cat $@.log ; test ! -s $@.log
	$(Q) mv $@.log $@
MOSTLYCLEANFILES += xlint-html xlint-html.log

# == JS linting ==
JS_LINT_FILES = *.js $(HTML_LINT_FILES)
xlint-js: $(JS_LINT_FILES) package.json
	$(AM_V_GEN)
	$(Q) rm -f $@
	$(Q) for file in $(JS_LINT_FILES) ; do \
	  ./node_modules/.bin/eslint -f unix $$file || exit $$? ; \
	done
	$(Q) touch $@
MOSTLYCLEANFILES += xlint-js
