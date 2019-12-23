# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/doc/faketex/*.d)
CLEANDIRS += $(wildcard $>/doc/faketex/)

# == Resource Downloads ==
FAKETEX_TARGETS ::=
# pairs of sha256sum and download URL
docs/faketex/mathjax-download ::= \
  22fcffd20912b6d4602aec1d33484c62a4f46ffbb54d5487f38396a77e831d34 \
    https://github.com/tim-janik/assets/releases/download/stripped-mathjax-2.7.5.2/stripped-mathjax-2.7.5.2.tar.xz
docs/faketex/inconsolata-download ::= \
  7107d57304ec12d01f54fa8e064a9b4f89e77a2d03a3de64d2490d90f0e99cf2 \
    https://raw.githubusercontent.com/tim-janik/assets/be5c1df5618578a7499a92ea88871be955c6b464/Inconsolata-v16/QldKNThLqRwH-OJ1UHjlKGlZ5q4.woff
docs/faketex/charissil-downloads ::= \
  ac0bf31dfa6b49d979021df80493def2c3fc6a1ef4d36aa6b61489b0d3381c71 \
    https://raw.githubusercontent.com/tim-janik/assets/a8a12dd652e48532faa39657bc5cc268525a89ca/CharisSIL5/CharisSIL-BI.woff \
  46bf9ec1380f2265fefb442e7311420080b775fef1a3351fe9a4ff8b464fc8b4 \
    https://raw.githubusercontent.com/tim-janik/assets/a8a12dd652e48532faa39657bc5cc268525a89ca/CharisSIL5/CharisSIL-B.woff \
  7c59641eeb3dae22630b60e94f8c61cda4234aa0cf223a7afdf36f62508e8f2f \
    https://raw.githubusercontent.com/tim-janik/assets/a8a12dd652e48532faa39657bc5cc268525a89ca/CharisSIL5/CharisSIL-I.woff \
  bd9280db2beceaa516e4644d98afa1060f474bf18aaabc20ba653a8f350742bb \
    https://raw.githubusercontent.com/tim-janik/assets/a8a12dd652e48532faa39657bc5cc268525a89ca/CharisSIL5/CharisSIL-R.woff \
  cd0aca2f82c7fc7d54b5ee97ba797a048aadb9e754bcedc0f4f6d980533e8ab9 \
    https://raw.githubusercontent.com/tim-janik/assets/a8a12dd652e48532faa39657bc5cc268525a89ca/CharisSIL5/CharisSIL-webfont.css
docs/faketex/all-downloads ::=			\
	$(docs/faketex/mathjax-download)	\
	$(docs/faketex/inconsolata-download)	\
	$(docs/faketex/charissil-downloads)

# == Stripped MathJax rules ==
$>/doc/faketex/mathjax.rules: $>/doc/faketex/download.rules
	$(QGEN)
	$Q rm -fr $>/doc/faketex/stripped-mathjax
	$Q cd $>/doc/faketex/downloads/ \
	  && tar -C ../ -xf $(call foreachpair, docs/faketex/downloads_filename, $(docs/faketex/mathjax-download))
	$Q test -e $>/doc/faketex/stripped-mathjax/MathJax.js
	$Q touch $@
FAKETEX_TARGETS += $>/doc/faketex/mathjax.rules

# == Inconsolata webfont ==
$>/doc/faketex/fonts/Inconsolata-Regular.css: $>/doc/faketex/download.rules		| $>/doc/faketex/fonts/
	$(QGEN)
	$Q cd $>/doc/faketex/downloads/ \
	  && $(CP) $(call foreachpair, docs/faketex/downloads_filename, $(docs/faketex/inconsolata-download)) ../fonts/
	$Q echo -e	"@font-face {\n  " \
			"font-family: 'Inconsolata'; font-style: normal; font-weight: 400;\n  " \
			"src: local('Inconsolata Regular'), local('Inconsolata-Regular')," \
			"url(QldKNThLqRwH-OJ1UHjlKGlZ5q4.woff) format('woff');\n" \
			"}" > $>/doc/faketex/fonts/Inconsolata-Regular.css
FAKETEX_TARGETS += $>/doc/faketex/fonts/Inconsolata-Regular.css

# == CharisSIL5 webfont ==
$>/doc/faketex/fonts/CharisSIL-webfont.css: $>/doc/faketex/download.rules		| $>/doc/faketex/fonts/
	$(QGEN)
	$Q cd $>/doc/faketex/downloads/ \
	  && $(CP) $(call foreachpair, docs/faketex/downloads_filename, $(docs/faketex/charissil-downloads)) ../fonts/
	$Q test -e $@
FAKETEX_TARGETS += $>/doc/faketex/fonts/CharisSIL-webfont.css

# == faketex.css ==
doc/faketex/faketex.css ::= $>/doc/faketex/faketex.css
$(doc/faketex/faketex.css): $>/doc/faketex/fonts/Inconsolata-Regular.css
$(doc/faketex/faketex.css): $>/doc/faketex/fonts/CharisSIL-webfont.css
$(doc/faketex/faketex.css): docs/faketex/faketex.scss docs/faketex/features.scss $(NODE_MODULES.deps)	| $>/doc/faketex/fonts/
	$(QGEN)
	$Q $(NODE_MODULES.bin)/node-sass $< -t compact $@.tmp
	$Q mv $@.tmp $@
FAKETEX_TARGETS += $(doc/faketex/faketex.css)

# == Downloads ==
# $(call downloads_filenames, hash, url) - extract the url basename
docs/faketex/downloads_filename  = "$(notdir $2)"
$>/doc/faketex/download.rules: docs/faketex/Makefile.mk					| $>/doc/faketex/downloads/
	$(QECHO) FETCH	$>/doc/faketex/downloads/
	$Q cd $>/doc/faketex/downloads/ \
	     $(call foreachpair, AND_DOWNLOAD_SHAURL, $(docs/faketex/all-downloads))
	$Q touch $@

# == faketex ==
faketex: $(FAKETEX_TARGETS) FORCE
faketex-clean: FORCE
	rm -f -r $>/doc/faketex/

# == installation rules ==
doc/faketex/install.files ::= $>/doc/faketex/faketex.css $>/doc/faketex/fonts $>/doc/faketex/stripped-mathjax
$(call INSTALL_DIR_RULE, doc/faketex/install.files, $(DESTDIR)$(docs/doc.dir)/faketex, $(doc/faketex/install.files))
