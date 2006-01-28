# FIXME: remove:
srcdir = .
BST_VERSION = 0.6.6

all:	all-targets


GENERATED = $(strip			\
	screenshots.phpini		\
	logogallery.phpini		\
)
PHP_TARGETS = $(strip			\
	php/file-browser.php		\
	php/file-upload.php		\
	php/gallery.php			\
	php/screenshots.phpini		\
	php/logogallery.phpini		\
)
HTML_TARGETS = $(strip			\
	html/about.html			\
	html/contact.html		\
	html/download.html		\
	html/documentation.html		\
	html/development.html		\
	html/logogallery.phtml		\
	html/news.html 			\
	html/historic.html		\
	html/oldnews.html 		\
	html/overview2003de.html	\
	html/resources.html		\
	html/related-links.html		\
	html/synthesis-links.html	\
	html/search.phtml		\
	html/screenshots.phtml		\
	html/sound-browser.phtml	\
	html/sound-upload.phtml		\
	html/wiki.phtml			\
	\
	html/faq.html			\
	html/quickstart.html		\
	\
	html/beast.1.html		\
	html/bsesh.1.html		\
	html/sfidl.1.html		\
)
unportet_targets = $(strip	\
	log-beast.html		\
	log-bse.html		\
	log-plugins.html	\
	log-sfi.html		\
	objdoc.html		\
	release-news.html	\
	architecture.html	\
	plugin-devel.html	\
	sfidl-manual.html	\
	plugin-devel.html	\
	engine-mplan.html	\
)
${HTML_TARGETS}: webmenu.doxi webframe.doxi beastdefs.doxi

all-targets: target-dirs ${PHP_TARGETS} ${HTML_TARGETS} screenshot-files logogallery-files web-image-files style-files test-install

.DELETE_ON_ERROR: ${PHP_TARGETS} ${HTML_TARGETS} ${GENERATED}

target-dirs:	# rule to create directories
	mkdir -p php/ html/ html/style/ html/screenshots/ html/logogallery/ html/web-images/

php/%.php: %.php
	cp -v $< $@

php/%.phpini: %.phpini
	cp -v $< $@
%.phpini: %/ scanimages.sh
	$(srcdir)/scanimages.sh $<

DOXIDEFS = -D BST_VERSION ${BST_VERSION} -I $(srcdir)

html/%.html: %.doxi
	doxer doxi2html $< -o html/ $(DOXIDEFS)
html/%.phtml: %.doxi
	doxer doxi2html $< -o html/ -e .phtml $(DOXIDEFS)
html/%.1.html: ../docs/%.1.doxi
	doxer doxi2htmlman $< -o html/ $(DOXIDEFS)
html/%.3.html: ../docs/%.3.doxi
	doxer doxi2htmlman $< -o html/ $(DOXIDEFS)
html/%.html: ../docs/%.doxi
	doxer doxi2html $< -o html/ $(DOXIDEFS)

style-files: target-dirs
	cp -a style/* html/style/

screenshot-files: target-dirs
	cp -a screenshots/* html/screenshots/

logogallery-files: target-dirs
	cp -a logogallery/* html/logogallery/

web-image-files:
	cp -a web-images/* html/web-images/

TEST_INSTALL_DIR=~/public_html/beast/
test-install: ${TEST_INSTALL_DIR}
	rm -rf ${TEST_INSTALL_DIR}/*
	cp -av ${TEST_INSTALL_DIR}/.htaccess-html html/.htaccess
	cp -av php/ html/ ${TEST_INSTALL_DIR}
	ln -s ../../doxer-pics/bse-files ${TEST_INSTALL_DIR}/html/test-files

.PHONY: all-targets target-dirs screenshot-files logogallery-files web-image-files clean style-files test-install

clean:
	rm -f html/style/*
	-rmdir -p html/style/
	rm -f html/screenshots/* html/logogallery/* html/web-images/*
	-rmdir -p html/screenshots/ html/logogallery/ html/web-images/
	rm -f html/*
	-rmdir -p html/
	rm -f php/*
	-rmdir -p php/
	rm -f ${GENERATED}
