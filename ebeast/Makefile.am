# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = v8bse .

# Considerations for building the ebeast bundle:
# - The target dir for all build rules has to be app/, because Electron expects to find the
#   file resources/app[.asar]/package.json on startup (unless extra args are given).
# - For .map files and a number of other references to work correctly, we have to build most
#   resources *inside* app/, using sources formerly copyied into app/.
# - Packaging the bundle with electron-packager takes quite some time, so we only build it
#   at installation time and cache the result in bundlecache/ (unless app/ changes).


# == all: ==
all: with_npm vue-components.html
	@echo "$$(basename $$(pwd))/: experimental code. Usage:"
	@echo "	make ebeast-app	# Build the app/ files folder"
	@echo "	make run	# Run the electron app/"
with_npm:
if WITH_NPM
with_npm:		ebeast-app
install-exec-local:	install-exec-ebeast-bundle
uninstall-local:	uninstall-ebeast-bundle
endif

# == .npmstamp ==
.npmstamp: package.json.in ../config.status
	$(AM_V_GEN)
	$(Q) rm -rf app/	# force asset rebuild, since we're updating the build tools
	$(Q) ../config.status --file package.json && rm -f config.log
	$(Q) npm install
	$(Q) touch $@
clean-local: clean-ebeast-npm
clean-ebeast-npm:
	rm -rf .npmstamp package.json node_modules/
MOSTLYCLEANFILES += config.log # created by config.status

# == ebeast-app ==
ebeast-app:
	$(AM_V_GEN)
	$(Q) rm -rf app/
	$(Q) $(MAKE) $(AM_MAKEFLAGS) app/package.json
clean-ebeast-app:
	rm -rf app/
mostlyclean-local: clean-ebeast-app

# == app/package.json ==
app/: .npmstamp
	$(QGEN)
	$(Q) rm -rf app/
	$(Q) mkdir app/ $@/vc $@/assets/
	$(Q) ln -s ../node_modules app/
APP_FILES = index.html app.js main.js menus.js
app/package.json: $(APP_FILES) $(CSS_ASSETS) $(VUE_ASSETS) v8bse/v8bse.node .npmstamp vue-components.html | app/
	$(AM_V_GEN)
	$(Q) cp -P $(APP_FILES) app/
	$(Q) cp -L v8bse/v8bse.node app/assets/
	$(Q) cp -P package.json app/

# == CSS_ASSETS ==
CSS_ASSETS = app/assets/stylesheets.css app/assets/gradient-01.png
app/assets/stylesheets.css: app.scss vc/*.scss .npmstamp | app/
	$(QGEN) # NOTE: scss source and output file names must be final, because .map is derived from it
	$(Q) cp -P app.scss app/
	$(Q) cp -P vc/*.scss app/vc/
	$(Q) cd app/ && ./node_modules/.bin/node-sass app.scss assets/stylesheets.css --source-map true
app/assets/gradient-01.png: app/assets/stylesheets.css Makefile
	$(AM_V_GEN) # generate non-banding gradient from stylesheets.css: gradient-01 { -im-convert: "..."; }
	$(Q)        # see: http://www.imagemagick.org/script/command-line-options.php#noise http://www.imagemagick.org/Usage/canvas/
	$(Q) tr '\n' ' ' < app/assets/stylesheets.css | \
	     sed -nr 's/.*\bgradient-01\s*\{[^}]*-im-convert:\s*"([^"]*)"\s*[;}].*/\1/; T; p' > $(@:%.png=%.cli)
	$(Q) test -s $(@:%.png=%.cli) # check that we actually found the -im-convert directive
	$(Q) convert $$(cat $(@:%.png=%.cli)) $(@:%.png=%.tmp.png)
	$(Q) rm -f $(@:%.png=%.cli) && mv $(@:%.png=%.tmp.png) $@

# == VUE_ASSETS ==
VUE_ASSETS = app/vc/includes.js app/assets/components.js
VC_MOSTJS := $(filter-out vc/includes.js, $(wildcard vc/*.js))
app/vc/includes.js: vc/includes.js $(VC_MOSTJS) | app/
	$(AM_V_GEN)
	$(Q) cp -P $(VC_MOSTJS) app/vc/
	$(Q) cp -P $< app/vc/
app/assets/components.js: vc/bundle.js vc/*.vue app/vc/includes.js .npmstamp | app/
	$(AM_V_GEN) # app/vc/bundle.js was copied with app/vc/includes.js
	$(Q) cd app/vc/ && \
	for f in $$(grep -o 'require *([^)]\+)' bundle.js | sed "s/require *( *['\"]//g ; s/['\"])//g") ; do \
	  cp -P $(abs_builddir)/vc/$$f . || { echo $(abs_builddir)/vc/: $$f: 'read error'; exit 1 ; } ; \
	done
	@: # all files required by vc/bundle.js are present, generate components.js
	$(Q) cd app/ && \
	node_modules/.bin/browserify --ignore-missing --no-builtins --no-commondir --no-browser-field \
	  --insert-global-vars="global" -d -t vueify -e vc/bundle.js -o assets/components.js

# == bundlecache/ ==
bundlecache/ebeast: app/package.json
	$(AM_V_GEN)
	$(Q) rm -rf bundlecache/ bundletemp/
	$(Q) time ./node_modules/.bin/electron-packager app ebeast --out=bundletemp
	$(Q) mv `for f in bundletemp/*/ ; do :; done && echo $$f` bundletemp/bundlecache/ # rename <appname>-<platform>-<arch>/
	@: # bundle polishing goes here...
	$(Q) mv bundletemp/bundlecache/ .
	$(Q) rmdir bundletemp
	$(Q) test -x bundlecache/ebeast
clean-ebeast-bundlecache:
	rm -rf bundlecache/
mostlyclean-local: clean-ebeast-bundlecache

# == install bundlecache/ ==
install-exec-ebeast-bundle: bundlecache/ebeast
	$(AM_V_GEN)
	$(MKDIR_P) "$(DESTDIR)$(beastdir)"
	rm -rf "$(DESTDIR)$(beastdir)/bundle" "$(DESTDIR)$(beastdir)/app"
	cp -RP bundlecache "$(DESTDIR)$(beastdir)/bundle"
	mv "$(DESTDIR)$(beastdir)/bundle/resources/app" "$(DESTDIR)$(beastdir)/"
	ln -s ../../app "$(DESTDIR)$(beastdir)/bundle/resources/app"
uninstall-ebeast-bundle:
	rm -rf "$(DESTDIR)$(beastdir)/bundle/" "$(DESTDIR)$(beastdir)/app/"

# == Docs for .vue files ==
VUE_COMPONENTS_INPUTS = $(sort $(wildcard vc/*.vue))
vue-components.html: $(VUE_COMPONENTS_INPUTS) Makefile
	$(AM_V_GEN)
	$(Q) rm -f $(@:.html=.tmp1)
	$(Q) echo -e "# Vue Components \n\n" > $(@:.html=.tmp1)
	@: # extract <docs/> blocks from *.vue and use these as input to pandoc
	$(Q) for i in $(VUE_COMPONENTS_INPUTS) ; do \
	  sed -n '1h;1!H;$${g; s|.*<docs>\(.*\)</docs>.*|\1|i; T; p }' < $$i >> $(@:.html=.tmp1) || exit $$? ; \
	  echo >> $(@:.html=.tmp1) ; \
	done
	$(Q) sed 's/^  // ; s/^### /\n### /' < $(@:.html=.tmp1) > $(@:.html=.tmp2)
	$(Q) $(PANDOC) --columns=9999 -f markdown_github+pandoc_title_block-hard_line_breaks -t html -s -o $@ $(@:.html=.tmp2)
	$(Q) rm -f $(@:.html=.tmp*)
MOSTLYCLEANFILES += vue-components.html

# == app ==
app: .appstamp
PRUNE_APPSTAMP := $(shell test -e v8bse/v8bse.node || rm -f .appstamp)
.appstamp: .npmstamp .lintstamp objects/app.css objects/gradient-01.png objects/vc-bundle.js objects/vue-components.html
	$(Q) $(MAKE) $(AM_MAKEFLAGS) -C v8bse v8bse.node
	$(Q) rm -f bundle/ebeast	# avoid electron picking up stale resources
	$(Q) touch $@
MOSTLYCLEANFILES += .appstamp

# == run ==
run: app
	export ELECTRON_ENABLE_LOGGING=1 && \
	$(GTK2_DARK) \
	LD_PRELOAD="$(abs_top_builddir)/bse/.libs/libbse-@MAJOR@.so" \
	./node_modules/electron/dist/electron .
# NOTE1, prefer LD_PRELOAD over LD_LIBRARY_PATH, to pick up $(builddir)/libbse *before* /usr/lib/libbse
# NOTE2, add --js-flags="--expose-gc" to the command line to enable global.gc();
# If libdbusmenu-glib.so is missing, electron 1.4.15 displays a Gtk+2 menu bar, ignoring
# the BrowserWindow.darkTheme option. Here, we preselect a commonly installed dark Gtk+2
# theme if it's present.
GTK2_DARK = $(shell test -f /usr/share/themes/Ambiance/gtk-2.0/gtkrc && echo 'GTK2_RC_FILES=/usr/share/themes/Ambiance/gtk-2.0/gtkrc')

# == Linting rules ==
HTML_LINT_FILES = index.html
JS_LINT_FILES = *.js vc/*.js vc/*.vue $(HTML_LINT_FILES)
.lintstamp: $(JS_LINT_FILES) .npmstamp
	$(AM_V_GEN)
	$(Q) rm -f $@
	$(Q) ./node_modules/.bin/eslint -f unix $(JS_LINT_FILES)
	$(Q) touch $@
MOSTLYCLEANFILES += .lintstamp

