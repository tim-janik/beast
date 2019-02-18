# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/ebeast/*.d)
CLEANDIRS         += $(wildcard $>/ebeast/)
ALL_TARGETS       += ebeast-all
CHECK_TARGETS     += ebeast-check
INSTALL_TARGETS   += ebeast-install
UNINSTALL_TARGETS += ebeast-uninstall
ebeast-all: $>/ebeast/app.rules

# Running ebeast:
# - Use 'make run' to start ebeast from the development tree, this ensures that libbse
#   is picked up from the development tree (instead of an installed version) and that
#   electron finds the ebeast app/ files.
# - DevTools can be activated with Shft+Ctrl+I when run from the devleopment tree.
#   To use DevTools on the installed ebeast bundle, install electron-devtools-installer
#   as npm package in $(pkginstalldir)/bundle/app/.
# - When DevTools are enabled, Shift+Ctrl+R can initiate an ebeast page reload.

# == sources ==
ebeast/js.inputs ::= $(strip 		\
	ebeast/main.js			\
	ebeast/menus.js			\
	ebeast/window.html		\
)
ebeast/vc/js.inputs      ::= $(wildcard ebeast/vc/*.js)
ebeast/vc/vue.inputs     ::= $(wildcard ebeast/vc/*.vue)
ebeast/vc/scss.inputs    ::= $(wildcard ebeast/vc/*.scss)
ebeast/app/files.js	 ::= $(addprefix $>/ebeast/app/,    $(notdir $(ebeast/js.inputs)))
ebeast/app/vc/files.js   ::= $(addprefix $>/ebeast/app/vc/, $(notdir $(ebeast/vc/js.inputs)))
ebeast/app/vc/files.vue  ::= $(addprefix $>/ebeast/app/vc/, $(notdir $(ebeast/vc/vue.inputs)))
ebeast/app/vc/files.scss ::= $(addprefix $>/ebeast/app/vc/, $(notdir $(ebeast/vc/scss.inputs)))
ebeast/app/tree ::= $(strip 			\
	$(ebeast/app/files.js)			\
	$(ebeast/app/vc/files.js)		\
	$(ebeast/app/vc/files.vue)		\
	$(ebeast/app/vc/files.scss)		\
	\
	$>/ebeast/app/app.scss			\
	$>/ebeast/app/assets/gradient-01.png	\
	$>/ebeast/app/assets/stylesheets.css	\
	$>/ebeast/app/assets/components.js	\
)

# == subdirs ==
include ebeast/v8bse/Makefile.mk

# == npm ==
$>/ebeast/npm.rules: ebeast/package.json.in	| $>/ebeast/app/
	$(QECHO) MAKE $@
	$Q sed	-e 's/@MAJOR@/$(VERSION_MAJOR)/g' \
		-e 's/@MINOR@/$(VERSION_MINOR)/g' \
		-e 's/@MICRO@/$(VERSION_MICRO)/g' \
		$< > $>/ebeast/package.json
	$Q cd $>/ebeast/ && npm install $(if $(PARALLEL_MAKE), --progress=false)
	$Q rm -f $>/ebeast/app/node_modules && ln -s ../node_modules $>/ebeast/app/
	$Q echo >$@

# == linting ==
ebeast/sed.uncommentjs ::= sed -nr 's,//.*$$,,g ; 1h ; 1!H ; $$ { g; s,/\*(\*[^/]|[^*])*\*/,,g ; p }' # beware, ignores quoted strings
ebeast/lint.appfiles   ::= $(ebeast/app/files.js) $(ebeast/app/vc/files.js) $(ebeast/app/vc/files.vue)
$>/ebeast/lint.rules: $(ebeast/lint.appfiles) $(ebeast/vc/vue.inputs) | $>/ebeast/npm.rules
	$(QECHO) MAKE $@
	$Q $>/ebeast/node_modules/.bin/eslint -c ebeast/.eslintrc.js -f unix $(ebeast/lint.appfiles)
	@: # check for component pitfalls
	$Q for f in $(ebeast/vc/vue.inputs) ; do \
	  $(ebeast/sed.uncommentjs) "$$f" | sed -e "s,^,$$f: ," \
	  | grep --color=auto '\b__dirname\b' \
	  && { echo 'Error: __dirname is invalid inside Vue component files' | grep --color=auto . ; exit 9 ; } ; \
	done ; :
	$Q echo >$@
ebeast-lint: .PHONY
	@rm -f $>/ebeast/lint.rules
	@$(MAKE) $>/ebeast/lint.rules

# == app ==
$>/ebeast/app.rules: $(ebeast/app/tree) $>/ebeast/lint.rules $>/ebeast/vue-docs.html $>/ebeast/v8bse/v8bse.node
	$(QECHO) MAKE $@
	$Q rm -rf $>/ebeast/bundlecache/				# avoid installing stale app/ files
	$Q cp -P $>/ebeast/package.json $>/ebeast/app/
	$Q cp -L $>/ebeast/v8bse/v8bse.node $>/ebeast/app/assets/
	$Q echo >$@

# == $>/ebeast/app/% ==
$>/ebeast/app/assets/stylesheets.css: $>/ebeast/app/app.scss $(ebeast/app/vc/files.scss)	| $>/ebeast/npm.rules
	$(QGEN) # NOTE: scss source and output file locations must be final, because .map is derived from it
	$Q cd $>/ebeast/app/ && ./node_modules/.bin/node-sass app.scss assets/stylesheets.css --source-map true
$>/ebeast/app/assets/gradient-01.png: $>/ebeast/app/assets/stylesheets.css ebeast/Makefile.mk
	$(QGEN) # generate non-banding gradient from stylesheets.css: gradient-01 { -im-convert: "..."; }
	$Q      # see: http://www.imagemagick.org/script/command-line-options.php#noise http://www.imagemagick.org/Usage/canvas/
	$Q tr '\n' ' ' < $>/ebeast/app/assets/stylesheets.css | \
	     sed -nr 's/.*\bgradient-01\s*\{[^}]*-im-convert:\s*"([^"]*)"\s*[;}].*/\1/; T; p' > $@.cli
	$Q test -s $@.cli # check that we actually found the -im-convert directive
	$Q convert $$(cat $@.cli) $@.tmp.png
	$Q rm $@.cli && mv $@.tmp.png $@
define ebeast/app/cp.EXT
$>/ebeast/app/%.$1:	  ebeast/%.$1	| $>/ebeast/app/vc/
	$$(QECHO) COPY $$@
	$Q cp -P $$< $$@
endef
$(eval $(call ebeast/app/cp.EXT ,scss))		# $>/ebeast/app/%.scss: ebeast/%.scss
$(eval $(call ebeast/app/cp.EXT ,html))		# $>/ebeast/app/%.html: ebeast/%.html
$(eval $(call ebeast/app/cp.EXT ,vue))		# $>/ebeast/app/%.vue: ebeast/%.vue
$(eval $(call ebeast/app/cp.EXT ,js))		# $>/ebeast/app/%.js: ebeast/%.js

# == assets/components.js ==
$>/ebeast/app/assets/components.js: $(ebeast/app/vc/files.js) $(ebeast/app/vc/files.vue)	| $>/ebeast/lint.rules
	$(QGEN)
	@: # all files required by vc/bundle.js are present, generate assets/components.js
	$Q cd $>/ebeast/app/ \
	  && node_modules/.bin/browserify --node --debug -t vueify -e vc/bundle.js -o assets/components.js
	@: # Note, since vc/*.js and vc/*.vue are bundled, they do not need to be installed

# == ebeast/vue-docs.html ==
$>/ebeast/vue-docs.html: $(ebeast/vc/vue.inputs) ebeast/Makefile.mk
	$(QGEN)
	$Q echo -e "# Vue Components \n\n"		> $@.tmp
	@: # extract <docs/> blocks from vue files and feed them into pandoc
	$Q for f in $(sort $(ebeast/vc/vue.inputs)) ; do \
	    echo ""								>>$@.tmp ; \
	    sed -n '/^<docs>\s*$$/{ :loop n; /^<\/docs>/q; p;  b loop }' <"$$f"	>>$@.tmp \
	    || exit $$? ; \
	done
	$Q sed 's/^  // ; s/^### /\n### /' -i $@.tmp # unindent
	$Q $(PANDOC) --columns=9999 -f markdown_github+pandoc_title_block-hard_line_breaks -t html -s -o $@ $@.tmp
	$Q rm $@.tmp

# == ebeast-run ==
# export ELECTRON_ENABLE_LOGGING=1
ebeast-run: $>/ebeast/app.rules
	test -f /usr/share/themes/Ambiance/gtk-2.0/gtkrc && export GTK2_RC_FILES='/usr/share/themes/Ambiance/gtk-2.0/gtkrc' ; \
	LD_PRELOAD="$>/bse/libbse-$(VERSION_MAJOR).so" \
	$>/ebeast/node_modules/electron/dist/electron $>/ebeast/app/

# NOTE1, prefer LD_PRELOAD over LD_LIBRARY_PATH, to pick up $(builddir)/libbse *before* /usr/lib/libbse
# NOTE2, add --js-flags="--expose-gc" to the command line to enable global.gc();
# If libdbusmenu-glib.so is missing, electron 1.4.15 displays a Gtk+2 menu bar, ignoring
# the BrowserWindow.darkTheme option. Here, we preselect a commonly installed dark Gtk+2
# theme if it's present.

# == ebeast-clean ==
ebeast-clean: .PHONY
	rm -f $>/ebeast/npm.rules $>/ebeast/* 2>/dev/null ; :
	rm -rf $>/ebeast/bundlecache/ $>/ebeast/app/

