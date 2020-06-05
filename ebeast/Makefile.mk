# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/ebeast/*.d)
ebeast/cleandirs ::= $(wildcard $>/ebeast/ $>/electron/ $>/app/)
CLEANDIRS         += $(ebeast/cleandirs)
ALL_TARGETS       += ebeast/all
ebeast/all: $>/app/package.json

# This Makefile creates $>/electron/ebeast and builds the ebeast web UI in $>/app/.

# Running ebeast:
# - Use 'make run' to start electron/ebeast from the development tree,
#   this tries to fix the Gtk+ theme and prints console.log to stdout.
# - DevTools can be activated with Shft+Ctrl+I when run from the devleopment tree.
# - When DevTools are enabled, Shift+Ctrl+R can initiate a page reload.

# == sources ==
ebeast/b/vue.inputs	::= $(wildcard ebeast/b/*.vue)
ebeast/b/vue.stems	::= $(patsubst ebeast/b/%.vue, %, $(ebeast/b/vue.inputs))
ebeast/eslint.files	::= $(wildcard ebeast/*.html) $(wildcard ebeast/*.js ebeast/b/*.js)
ebeast/app.scss.d	::= $(wildcard ebeast/*.scss ebeast/b/*.scss)
app/assets/tri-pngs	::= $(strip	\
	$>/app/assets/tri-n.png		\
	$>/app/assets/tri-e.png		\
	$>/app/assets/tri-s.png		\
	$>/app/assets/tri-w.png		\
)
app/generated 		::= $(strip	\
	$(app/assets/tri-pngs)		\
	$>/app/assets/gradient-01.png	\
	$>/app/assets/forkawesome-webfont.css	\
	$>/app/assets/stylesheets.css	\
	$>/app/assets/material-icons.css \
)
app/assets.copies	::= $(strip	\
	$>/app/assets/spinners.svg	\
)
ebeast/copy.tool.targets ::= $(strip	\
	$>/ebeast/rollup.config.js	\
	$>/ebeast/babel.config.js	\
	$>/ebeast/.eslintrc.js		\
)
ebeast/copy.content.targets ::= $(strip	\
	$>/app/main.js			\
	$>/app/jsbse.js			\
	$>/app/menus.js			\
	$>/app/utilities.js		\
)
ebeast/copy.targets ::= $(ebeast/copy.tool.targets) $(ebeast/copy.content.targets)

# == ebeast/copy.targets ==
$(filter $>/ebeast/%, $(ebeast/copy.targets)):	$>/ebeast/%: ebeast/% | $>/ebeast/ ;	$(QGEN) && cp ebeast/$(@F) $@
$(filter $>/app/%, $(ebeast/copy.targets)):	$>/app/%:    ebeast/% | $>/app/    ;	$(QGEN) && cp ebeast/$(@F) $@

# == npm ==
NPM_INSTALL = npm --prefer-offline install $(if $(PARALLEL_MAKE), --progress=false)
$>/ebeast/node_modules/npm.done: ebeast/package.json.in	ebeast/fix-scss.diff | $>/ebeast/
	$(QGEN)
	$Q rm -f -r $>/ebeast/node_modules/
	$Q cp $< $>/ebeast/package.json # avoids dependency on other ebeast/copy.targets
	@: # Install all node_modules and anonymize build path
	$Q cd $>/ebeast/ \
	  && $(NPM_INSTALL) \
	  && find . -name package.json -print0 | xargs -0 sed -r "\|$$PWD|s|^(\s*(\"_where\":\s*)?)\"$$PWD|\1\"/...|" -i
	@: # Patch broken modules
	$Q (cd  $>/ebeast/ && patch -p0) < ebeast/fix-scss.diff
	$Q echo >$@
# provide node_modules/ for use in other makefiles
NODE_MODULES.bin  ::= $>/ebeast/node_modules/.bin/
NODE_MODULES.deps ::= $>/ebeast/node_modules/npm.done

# == linting ==
ebeast/sed.uncommentjs ::= sed -nr 's,//.*$$,,g ; 1h ; 1!H ; $$ { g; s,/\*(\*[^/]|[^*])*\*/,,g ; p }' # beware, ignores quoted strings
$>/ebeast/pitfalls.done: $(ebeast/eslint.files)
	$(QECHO) MAKE $@
	@: # check for component pitfalls
	$Q for f in $(ebeast/b/vue.inputs) ; do \
	  $(ebeast/sed.uncommentjs) "$$f" | sed -e "s,^,$$f: ," \
	  | grep --color=auto '\b__dirname\b' \
	  && { echo 'Error: __dirname is invalid inside Vue component files' | grep --color=auto . ; exit 9 ; } ; \
	done ; :
	$Q echo >$@
$>/ebeast/eslint.done: $(ebeast/eslint.files) ebeast/.eslintrc.js		| $>/ebeast/node_modules/npm.done
	$(QECHO) MAKE $@
	$Q $(NODE_MODULES.bin)/eslint -c ebeast/.eslintrc.js -f unix \
		--cache --cache-location $>/ebeast/eslint.cache $(ebeast/eslint.files)
	$Q echo >$@
$>/ebeast/lint.done: $>/ebeast/pitfalls.done $>/ebeast/eslint.done
	$Q echo >$@
ebeast-lint: FORCE
	@rm -f $>/ebeast/lint.done
	@$(MAKE) $>/ebeast/lint.done

# == electron/ebeast ==
$>/electron/ebeast:						| $>/
	$(QGEN)
	$Q rm -f -r $>/electron/ $>/electron.tmp/
	$Q mkdir $>/electron.tmp/ && cd $>/electron.tmp/	\
	  && echo '{"private":true}'	> package.json		\
	  && $(NPM_INSTALL) --no-save electron@7		\
	  && mv node_modules/electron/dist/ ../electron		\
	  && cd .. && rm -f -r electron.tmp/
	$Q rm -f -r $>/electron/resources/default_app.asar
	$Q ln -s ../../app $>/electron/resources/app
	$Q mv $>/electron/electron $>/electron/ebeast

# == app ==
$>/app/package.json: $>/electron/ebeast $>/ebeast/lint.done
$>/app/package.json: $(ebeast/copy.targets) $(app/generated) $(app/assets.copies)
$>/app/package.json: ebeast/index.html $>/app/bseapi_jsonipc.js $>/ebeast/node_modules/npm.done ebeast/eknob.svg
	$(QGEN)
	$Q echo -e '{ "name": "ebeast",'				> $@.tmp
	$Q echo -e '  "productName": "EBeast",'				>>$@.tmp
	$Q echo -e '  "version": "$(VERSION_M.M.M)",'			>>$@.tmp
	$Q echo -e '  "config": {'					>>$@.tmp
	$Q echo -e '    "version": "$(VERSION_M.M.M)",'			>>$@.tmp
	$Q echo -e '    "revdate": "$(shell ./version.sh -d)",'		>>$@.tmp
	$Q echo -e '    "revision": "$(shell ./version.sh -l)",'	>>$@.tmp
	$Q echo -e '    "debug": "$(if $(findstring $(MODE), debug quick),true,false)" },' >>$@.tmp
	$Q echo -e '  "main": "main.js" }'				>>$@.tmp
	@: # Embed package.json in index.html, using record-separator \036 in sed
	$Q PACKAGE_JSON=$$(tr '\n' ' ' < $@.tmp) && R=$$'\036' \
	  && sed -r \
		-e"s$$R<!--@-html-head-package_json@-->$$R$$PACKAGE_JSON$$R" \
		-e "/<!--'eknob.svg'-->/ r ebeast/eknob.svg" \
		< ebeast/index.html	>$>/app/index.html
	$Q ln -f -s ../doc $>/app/doc
	$Q $(CP) -a $>/ebeast/node_modules/vue/dist/vue.esm.browser.js $>/app/
	$Q $(CP) -a $>/ebeast/node_modules/vue-runtime-helpers/dist/index.mjs $>/app/vue-runtime-helpers.mjs \
	  && sed 's|^//# sourceMappingURL=index\.mjs\.map$$||' -i $>/app/vue-runtime-helpers.mjs
	$Q mv $@.tmp $@


# == $>/app/b/%.bundle.js ==
ebeast/targets.vuebundles.js  ::= $(patsubst %, $>/app/b/%.bundle.js,  $(ebeast/b/vue.stems))
ebeast/targets.vuebundles.css ::= $(patsubst %, $>/app/b/%.bundle.css, $(ebeast/b/vue.stems))
# $>/app/b/%.bundle.js: $(ebeast/app.scss.d)
$>/app/b/%.bundle.js: ebeast/b/%.vue $(ebeast/copy.tool.targets)	| $>/app/b/ $>/ebeast/b/ $>/ebeast/node_modules/npm.done
	$(QGEN)
	$Q cd $>/ebeast/ && $(abspath $(NODE_MODULES.bin)/rollup) -c ./rollup.config.js -i $(abspath $<) -o $(abspath $>/ebeast/b/$(@F))
	$Q mv $>/ebeast/b/$(@F) $>/ebeast/b/$(@F:.js=.css) $(@D)
# Note: rollups babel config currently expects $CWD/babel.config.js
$>/app/b/%.bundle.css: $>/app/b/%.bundle.js ;
$>/app/package.json: $(ebeast/targets.vuebundles.js) $(ebeast/targets.vuebundles.css)

# == $>/app/bundle.imports.js ==
$>/app/bundle.imports.js: $(wildcard ebeast/b/*.vue) | $>/app/
	$(QGEN)
	$Q cd ebeast && ( : \
	  && printf "export default [\n" \
	  && for f in b/*.vue ; do \
	       b="$${f%.vue}.bundle" \
	       && printf "  %-40s %s,\n" "'./$$b.js'," "'./$$b.css'" ; \
	     done \
	  && printf "];\n" ) > $(abspath $@)
$>/app/package.json: $>/app/bundle.imports.js

# == $>/ebeast/b.*.lint ==
ebeast/targets.lint ::= $(patsubst %, $>/ebeast/b/%.lint, $(ebeast/b/vue.stems))
$>/ebeast/b/%.lint: ebeast/b/%.vue	| $>/ebeast/b/ $>/ebeast/node_modules/npm.done
	$(QGEN)
	$Q $(NODE_MODULES.bin)/eslint -c ebeast/.eslintrc.js -f unix --cache --cache-location $(@:%=%.cache) $<
	$Q echo >$@
$>/app/package.json: $(ebeast/targets.lint)

# == $>/app/assets/ ==
$(app/assets.copies): $>/app/assets/%: ebeast/%		| $>/app/assets/
	$(QECHO) COPY $@
	$Q $(CP) -P $< $@

# == $>/app/bseapi_jsonipc.js ==
$>/app/bseapi_jsonipc.js: jsonipc/head.js $(lib/BeastSoundEngine) bse/bseapi.idl	| $>/app/
	$(QGEN)
	$Q cat jsonipc/head.js			> $@.tmp
	$Q $(lib/BeastSoundEngine) --js-bseapi	>>$@.tmp
	$Q rm -f $@ && chmod -w $@.tmp
	$Q mv $@.tmp $@

# == $>/app/assets/ ==
ebeast/inter-typeface-downloads ::= \
  9cd56084faa8cc5ee75bf6f3d01446892df88928731ee9321e544a682aec55ef \
    https://github.com/rsms/inter/raw/v3.10/docs/font-files/Inter-Medium.woff2
$>/app/assets/Inter-Medium.woff2:			| $>/app/assets/
	$(QGEN)
	$Q cd $(@D) \
		$(call foreachpair, AND_DOWNLOAD_SHAURL, $(ebeast/inter-typeface-downloads))
$>/app/assets/stylesheets.css: $(ebeast/app.scss.d) $>/app/assets/Inter-Medium.woff2	| $>/ebeast/node_modules/npm.done
	$(QGEN) # NOTE: scss source and output file locations must be final, because .map is derived from it
	$Q : # cd $>/app/ && ../ebeast/node_modules/.bin/node-sass app.scss assets/stylesheets.css --source-map true
	$Q $(NODE_MODULES.bin)/node-sass ebeast/app.scss $>/app/assets/stylesheets.css \
		--include-path ebeast/ --include-path $>/ebeast/ --source-map-embed --source-map-contents --source-comments
$>/app/assets/material-icons.css:			| $>/app/assets/
	$(QECHO) FETCH material-icons-190326.1.tar.xz
	$Q cd $>/app/assets/ \
	     $(call AND_DOWNLOAD_SHAURL, \
		53eba258da6170f5aa3925579f1552ef7d7a06d5b762260efac5e26d5f95e721, \
		  https://github.com/tim-janik/assets/releases/download/material-icons-190326.1/material-icons-190326.1.tar.xz)
	$(QGEN)
	$Q tar -C $>/app/assets/ -xf $>/app/assets/material-icons-190326.1.tar.xz
	$Q mv $>/app/assets/material-icons/material-icons.woff2 $>/app/assets/material-icons/material-icons.css $>/app/assets/
	$Q rm $>/app/assets/material-icons-190326.1.tar.xz && rm -r $>/app/assets/material-icons/
ebeast/fork-awesome-downloads ::= \
  844517a2bc5430242cb857e56b6dccf002f469c4c1b295ed8d0b7211fb452f50 \
    https://raw.githubusercontent.com/ForkAwesome/Fork-Awesome/b0605a81632452818bf19c8fa97469da1206b52b/fonts/forkawesome-webfont.woff2 \
  630b0e84fa43579f7e97a26fd47d4b70cb5516ca7e6e73393597d12ca249a8ee \
    https://raw.githubusercontent.com/ForkAwesome/Fork-Awesome/b0605a81632452818bf19c8fa97469da1206b52b/css/fork-awesome.css
$>/app/assets/forkawesome-webfont.css:				| $>/app/assets/
	$(QGEN)
	$Q cd $(@D) $(call foreachpair, AND_DOWNLOAD_SHAURL, $(ebeast/fork-awesome-downloads))
	$Q sed "/^ *src: *url/s,src: *url(.*);,src: url('forkawesome-webfont.woff2');," -i $>/app/assets/fork-awesome.css
	$Q mv $>/app/assets/fork-awesome.css $@
$>/app/assets/gradient-01.png: $>/app/assets/stylesheets.css ebeast/Makefile.mk
	$(QGEN) # generate non-banding gradient from stylesheets.css: gradient-01 { -im-convert: "..."; }
	$Q      # see: http://www.imagemagick.org/script/command-line-options.php#noise http://www.imagemagick.org/Usage/canvas/
	$Q tr '\n' ' ' < $>/app/assets/stylesheets.css | \
	     sed -nr 's/.*@supports\s*\(--makefile:\s*rule\)\s*\{[^{]*\bgradient-01\s*\{\s*im-convert:\s*"([^"]*)"\s*[;}].*/\1/; T; p' > $@.cli
	$Q test -s $@.cli # check that we actually found the -im-convert directive
	$Q $(IMAGEMAGICK_CONVERT) $$(cat $@.cli) $@.tmp.png
	$Q rm $@.cli && mv $@.tmp.png $@
$>/app/assets/tri-n.png: ebeast/triangle32.png $>/app/assets/stylesheets.css	| $>/app/assets/
	$(QGEN)
	$Q tr '\n' ' ' < $>/app/assets/stylesheets.css | \
	     sed -nr 's/.*@supports\s*\(--makefile:\s*rule\)\s*\{[^{]*\bscrollbar-arrow\s*\{\s*im-convert:\s*"([^"]*)"\s*[;}].*/\1/; T; p' > $@.cli
	$Q test -s $@.cli # check that we actually found the -im-convert directive
	$Q $(IMAGEMAGICK_CONVERT) $< $$(cat $@.cli) $@.tmp.png
	$Q rm $@.cli && mv $@.tmp.png $@
$>/app/assets/tri-e.png: $>/app/assets/tri-n.png
	$(QGEN)
	$Q $(IMAGEMAGICK_CONVERT) $< -rotate 90 $@.tmp.png
	$Q mv $@.tmp.png $@
$>/app/assets/tri-s.png: $>/app/assets/tri-n.png
	$(QGEN)
	$Q $(IMAGEMAGICK_CONVERT) $< -rotate 180 $@.tmp.png
	$Q mv $@.tmp.png $@
$>/app/assets/tri-w.png: $>/app/assets/tri-n.png
	$(QGEN)
	$Q $(IMAGEMAGICK_CONVERT) $< -rotate 270 $@.tmp.png
	$Q mv $@.tmp.png $@

# == check-ebeast ==
check-ebeast: FORCE
	$(QGEN)
	@: # basic check for electron/ebeast startup
	$Q $>/electron/ebeast --version | fgrep '$(VERSION_M.M.M)'
check-x11: check-ebeast

# == installation ==
ebeast/install: $>/app/package.json FORCE
	@$(QECHO) INSTALL '$(DESTDIR)$(pkglibdir)/{app|electron}'
	$Q rm -f -r $(DESTDIR)$(pkglibdir)/electron $(DESTDIR)$(pkglibdir)/app
	$Q $(CP) -a $>/electron $>/app $(DESTDIR)$(pkglibdir)/
	$Q $(call INSTALL_SYMLINK, '../electron/ebeast', '$(DESTDIR)$(pkglibdir)/bin/beast')
install: ebeast/install
ebeast/uninstall: FORCE
	@$(QECHO) REMOVE '$(DESTDIR)$(pkglibdir)/{app|electron}'
	$Q rm -f -r $(DESTDIR)$(pkglibdir)/electron $(DESTDIR)$(pkglibdir)/app
	$Q rm -f '$(DESTDIR)$(pkglibdir)/bin/beast'
uninstall: ebeast/uninstall

# == ebeast/run ==
ebeast/run: $>/app/package.json $>/doc/beast-manual.html
	test -f /usr/share/themes/Ambiance/gtk-2.0/gtkrc && export GTK2_RC_FILES='/usr/share/themes/Ambiance/gtk-2.0/gtkrc' ; \
	export ELECTRON_ENABLE_LOGGING=1 ; \
	$>/electron/ebeast

# == ebeast/vue-docs ==
$>/ebeast/vue-docs.md: $(ebeast/b/vue.inputs) ebeast/Makefile.mk docs/filt-docs2.py	| $>/ebeast/
	$(QGEN)
	$Q echo -e "<!-- Vue Components -->\n\n"				> $@.tmp
	@: # extract <docs/> blocks from vue files and feed them into pandoc
	$Q for f in $(sort $(ebeast/b/vue.inputs)) ; do \
	    echo ""								>>$@.tmp ; \
	    sed -n '/^<docs>\s*$$/{ :loop n; /^<\/docs>/q; p;  b loop }' <"$$f"	>>$@.tmp \
	    || exit $$? ; \
	done
	$Q sed -r 's/^  // ; s/^#/\n#/; ' -i $@.tmp # unindent, spread heading_without_preceding_blankline
	$Q $(PANDOC) -t markdown -F docs/filt-docs2.py -f markdown+compact_definition_lists $@.tmp -o $@.tmp2
	$Q mv $@.tmp2 $@ && rm -f $@.tmp

# NOTE1, prefer LD_PRELOAD over LD_LIBRARY_PATH, to pick up $(builddir)/libbse *before* /usr/lib/libbse
# NOTE2, add --js-flags="--expose-gc" to the command line to enable global.gc();
# If libdbusmenu-glib.so is missing, electron 1.4.15 displays a Gtk+2 menu bar, ignoring
# the BrowserWindow.darkTheme option. Here, we preselect a commonly installed dark Gtk+2
# theme if it's present.

# == ebeast/clean ==
ebeast/clean: FORCE
	rm -f -r $(ebeast/cleandirs)
