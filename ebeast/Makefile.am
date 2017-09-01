# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = v8bse .

export ELECTRON_ENABLE_LOGGING=1

# == all: ==
all: with_npm
	@echo "$$(basename $$(pwd))/: experimental code. Usage:"
	@echo "	make app	# build the v8 Bse module and app files"
	@echo "	make run	# run the electron app in the current directory"
	@echo "	make clean	# remove build files"
if WITH_NPM
with_npm:		.bundlestamp
install-exec-local:	install-exec-bundle
uninstall-local:	uninstall-bundle
else
with_npm:
endif

# == package.json ==
package.json: package.json.in ../config.status
	$(AM_V_GEN)
	$(Q) ../config.status --file package.json
	$(Q) npm install
clean-local: clean-node_modules
clean-node_modules: ; rm -rf node_modules/ package.json
MOSTLYCLEANFILES += config.log # created by config.status

# == ebeast bundle ==
INSTALL_VERSION = @MAJOR@.@MINOR@.@MICRO@
BUNDLEDIR       = ebeast
.bundlestamp: .appstamp
	$(AM_V_GEN)
	$(Q) rm -rf .bundlestamp "./$(BUNDLEDIR)" ebeast-bundle/
	$(Q) mkdir ebeast-bundle/
	$(Q) ./node_modules/.bin/electron-packager . ebeast --out=ebeast-bundle/
	$(Q) mv "`echo ebeast-bundle/*`" $(BUNDLEDIR) # rename <appname>-<platform>-<arch>/
	$(Q) rmdir ebeast-bundle/
	$(Q) test -x $(BUNDLEDIR)/ebeast
	$(Q) touch $@
clean-local: clean-ebeast-bundle
clean-ebeast-bundle:
	rm -rf .bundlestamp "./$(BUNDLEDIR)" ebeast-bundle/

# == bundle.js ==
objects/vc-bundle.js: vc/bundle.js vc/*.vue
	$(AM_V_GEN)
	@: # check all require('...') file references
	$(Q) cd $(<D) && \
	for f in $$(grep -o 'require *([^)]\+)' $(<F) | \
		    sed "s/require *( *['\"]//g ; s/['\"])//g") ; do \
	  test -r $$f || { echo $$(pwd -P)/: $$f: 'read error'; exit 1 ; } ; \
	done
	@: # all files present, generate bundle
	$(Q) node_modules/.bin/browserify --ignore-missing --no-builtins --no-commondir \
	  --insert-global-vars="global" --no-browser-field -d \
	  -t vueify -e $< -o $@
MOSTLYCLEANFILES += objects/vc-bundle.js

# == Docs for .vue files ==
VUE_COMPONENTS_INPUTS = $(sort $(wildcard vc/*.vue))
objects/vue-components.html: $(VUE_COMPONENTS_INPUTS) Makefile
	$(AM_V_GEN)
	$(Q) rm -f $(@:.html=.tmp1)
	$(Q) echo -e "# Vue Components \n\n" > $(@:.html=.tmp1)
	$(Q) for i in $(VUE_COMPONENTS_INPUTS) ; do \
	  sed -n '1h;1!H;$${g; s|.*<docs>\(.*\)</docs>.*|\1|i; T; p }' < $$i >> $(@:.html=.tmp1) || exit $$? ; \
	  echo >> $(@:.html=.tmp1) ; \
	done
	$(Q) sed 's/^  // ; s/^### /\n### /' < $(@:.html=.tmp1) > $(@:.html=.tmp2)
	$(Q) pandoc --columns=9999 -f markdown_github+pandoc_title_block-hard_line_breaks -t html -s -o $@ $(@:.html=.tmp2)
	$(Q) rm -f $(@:.html=.tmp*)

# == app ==
app: .appstamp
PRUNE_APPSTAMP := $(shell test -e v8bse/v8bse.node || rm -f .appstamp)
.appstamp: package.json xlinting objects/app.css objects/gradient-01.png objects/vc-bundle.js objects/vue-components.html
	$(Q) $(MAKE) $(AM_MAKEFLAGS) -C v8bse v8bse.node
	$(Q) rm -rf .bundlestamp "./$(BUNDLEDIR)" # avoid electron picking up stale resources
	$(Q) touch $@
MOSTLYCLEANFILES += .appstamp

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
objects/app.css: app.scss package.json | *.scss */*.scss
	$(QGEN) # NOTE: scss output file names must not be temporary, since .map is derived from it
	$(Q) mkdir -p objects/.deps/
	$(Q) INPUT=$$(readlink -f $<) && cd $(@D) && ../node_modules/.bin/node-sass $$INPUT $(@F) --source-map true
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

# == Linting rules ==
HTML_LINT_FILES = index.html
JS_LINT_FILES = *.js vc/*.js vc/*.vue $(HTML_LINT_FILES)
xlinting: $(JS_LINT_FILES) package.json
	$(AM_V_GEN)
	$(Q) rm -f $@
	$(Q) ./node_modules/.bin/eslint -f unix $(JS_LINT_FILES)
	$(Q) touch $@
MOSTLYCLEANFILES += xlinting

# == install ==
install-exec-bundle: .bundlestamp
	$(MKDIR_P) "$(DESTDIR)$(beastdir)"
	rm -rf "$(DESTDIR)$(beastdir)/$(BUNDLEDIR)"
	cp -RP $(BUNDLEDIR) "$(DESTDIR)$(beastdir)/"
uninstall-bundle:
	rm -rf "$(DESTDIR)$(beastdir)/$(BUNDLEDIR)"
