srcdir = .

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
	html/resources.html		\
	html/related-links.html		\
	html/synthesis-links.html	\
	html/search.phtml		\
	html/screenshots.phtml		\
	html/sound-browser.phtml	\
	html/sound-upload.phtml		\
	\
	html/faq.html			\
	html/quickstart.html		\
)
${HTML_TARGETS}: webmenu.doxi beastdefs.doxi

all-targets: target-dirs ${PHP_TARGETS} ${HTML_TARGETS} screenshot-files logogallery-files style-files test-install

.DELETE_ON_ERROR: ${PHP_TARGETS} ${HTML_TARGETS} ${GENERATED}

target-dirs:	# rule to create directories
	mkdir -p php/ html/ html/style/ html/screenshots/ html/logogallery/

php/%.php: %.php
	cp -v $< $@

php/%.phpini: %.phpini
	cp -v $< $@
%.phpini: %/ scanimages.sh
	$(srcdir)/scanimages.sh $<

html/%.html: %.doxi
	doxer doxi2html $< -o html/
html/%.phtml: %.doxi
	doxer doxi2html $< -o html/ -e .phtml
html/%.html: ../docs/%.doxi
	doxer doxi2html $< -o html/

style-files: target-dirs
	cp -a style/* html/style/

screenshot-files: target-dirs
	cp -a screenshots/* html/screenshots/

logogallery-files: target-dirs
	cp -a logogallery/* html/logogallery/

TEST_INSTALL_DIR=~/public_html/beast/
test-install: ${TEST_INSTALL_DIR}
	rm -rf ${TEST_INSTALL_DIR}/*
	cp -av ${TEST_INSTALL_DIR}/.htaccess-html html/.htaccess
	cp -av php/ html/ ${TEST_INSTALL_DIR}
	ln -s ../../doxer-pics/bse-files ${TEST_INSTALL_DIR}/html/test-files

.PHONY: all-targets target-dirs screenshot-files logogallery-files clean style-files test-install

clean:
	rm -f html/style/*
	-rmdir -p html/style/
	rm -f html/screenshots/* html/logogallery/*
	-rmdir -p html/screenshots/ html/logogallery/
	rm -f html/*
	-rmdir -p html/
	rm -f php/*
	-rmdir -p php/
	rm -f ${GENERATED}
