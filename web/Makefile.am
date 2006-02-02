# BEAST - Bedevilled Audio System
# Copyright (C) 2005-2006 Tim Janik
#
# GNU General Public License version 2 or (at your option) any later version.

# FIXME: remove:
top_srcdir = ..
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
	html/doxer-docu.html		\
	html/engine-mplan.html		\
	html/logogallery.phtml		\
	html/news.html 			\
	html/news-file.html		\
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
	html/beast.1.html		\
	html/bsesh.1.html		\
	html/sfidl.1.html		\
	\
	html/architecture.html		\
	html/faq.html			\
	html/plugin-devel.html		\
	html/quickstart.html		\
	html/sfidl-manual.html		\
	\
	html/log-beast.html		\
	html/log-bse.html		\
	html/log-plugins.html		\
	html/log-sfi.html		\
)
RDOCU_TARGETS = $(strip			\
	html/rdocu-bse/index.html	\
	html/rdocu-gxk/index.html	\
)
unportet_targets = $(strip	\
	objdoc.html		\
	plugin-devel.html	\
)
${HTML_TARGETS} ${RDOCU_TARGETS}: webmenu.doxi webframe.doxi beastdefs.doxi

gallery-targets: screenshot-files logogallery-files
image-targets:	 web-image-files docs-image-files
all-targets: target-dirs ${PHP_TARGETS} ${HTML_TARGETS} style-files gallery-targets image-targets ${RDOCU_TARGETS} test-install

.DELETE_ON_ERROR: ${PHP_TARGETS} ${HTML_TARGETS} ${GENERATED}

target-dirs:	# rule to create directories
	mkdir -p php/ html/ html/style/ html/screenshots/ html/logogallery/ html/web-images/ html/images/
	mkdir -p html/rdocu-gxk/ html/rdocu-bse/

php/%.php: %.php
	cp -v $< $@

php/%.phpini: %.phpini
	cp -v $< $@
%.phpini: %/ scanimages.sh
	$(srcdir)/scanimages.sh $<

DOXIDEFS = -D TOP_WEBDIR "." -D BST_VERSION ${BST_VERSION} -I $(srcdir)

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
	doxer writecss -o html/style/

screenshot-files: target-dirs
	cp -a screenshots/* html/screenshots/

logogallery-files: target-dirs
	cp -a logogallery/* html/logogallery/

web-image-files:
	cp -a web-images/* html/web-images/

docs-image-files:
	cp -a $(srcdir)/../docs/images/*.png html/images/

TEST_INSTALL_DIR=/ptmp/beast-preview/
TEST_INSTALL_DIR=~/public_html/beast/
test-install: ${TEST_INSTALL_DIR}
	rm -rf ${TEST_INSTALL_DIR}/*
	cp -av ${TEST_INSTALL_DIR}/.htaccess-html html/.htaccess
	cp -av php/ html/ ${TEST_INSTALL_DIR}
	ln -s ../../doxer-pics/bse-files ${TEST_INSTALL_DIR}/html/test-files
	ln -s news.html ${TEST_INSTALL_DIR}/html/index.html

.PHONY: all-targets target-dirs screenshot-files logogallery-files web-image-files docs-image-files clean style-files test-install

.INTERMEDIATE: $(intermediate_files)

clean:
	rm -f html/style/*
	-rmdir -p html/style/
	rm -f html/screenshots/* html/logogallery/* html/web-images/* html/images/*
	-rmdir -p html/screenshots/ html/logogallery/ html/web-images/ html/images/
	rm -f html/rdocu-gxk/* html/rdocu-bse/*
	-rmdir -p html/rdocu-gxk/ html/rdocu-bse/
	rm -f html/* html/.htaccess
	-rmdir -p html/
	rm -f php/*
	-rmdir -p php/
	rm -f ${GENERATED} ${intermediate_files}

html/%.html: %.tmpdoxi
	doxer doxi2html $< -o html/ $(DOXIDEFS)
news-file.tmpdoxi: $(top_srcdir)/NEWS Makefile
	cd . \
	&& echo '@doxer_dnl'					>$@  \
	&& echo '@doxer_set{webmenu} development'		>>$@ \
	&& echo '@include "webframe.doxi"'			>>$@ \
	&& echo '@doxer_set{title} BEAST/BSE - NEWS'		>>$@ \
	&& echo '@subheading'					>>$@ \
	&& echo '@example'					>>$@ \
	&& sed  's/@/@@/g' 		<$<			>>$@ \
	&& echo '@done'						>>$@
engine-mplan.tmpdoxi: $(top_srcdir)/bse/engine-mplan.txt Makefile
	cd . \
	&& echo '@doxer_dnl'					>$@  \
	&& echo '@doxer_set{webmenu} development'		>>$@ \
	&& echo '@include "webframe.doxi"'			>>$@ \
	&& echo '@doxer_set{title} BEAST - BSE Engine'		>>$@ \
	&& echo '@subheading'					>>$@ \
	&& echo '@example'					>>$@ \
	&& sed  's/@/@@/g' 		<$<			>>$@ \
	&& echo '@done'						>>$@
CHANGELOG2DOXI = ~/sources/doxer/changelog2doxi.py # FIXME
log-beast.tmpdoxi: $(top_srcdir)/ChangeLog $(CHANGELOG2DOXI)
	cd . \
	&& echo '@doxer_dnl'					>$@  \
	&& echo '@doxer_set{webmenu} development'		>>$@ \
	&& echo '@include "webframe.doxi"'			>>$@ \
	&& echo '@doxer_set{title} BEAST - ChangeLog'		>>$@ \
	&& echo '@subheading'					>>$@ \
	&& $(CHANGELOG2DOXI) 		<$<			>>$@
log-bse.tmpdoxi: $(top_srcdir)/bse/ChangeLog $(CHANGELOG2DOXI)
	cd . \
	&& echo '@doxer_dnl'					>$@  \
	&& echo '@doxer_set{webmenu} development'		>>$@ \
	&& echo '@include "webframe.doxi"'			>>$@ \
	&& echo '@doxer_set{title} BEAST - BSE ChangeLog'	>>$@ \
	&& echo '@subheading'					>>$@ \
	&& $(CHANGELOG2DOXI) 		<$<			>>$@
log-plugins.tmpdoxi: $(top_srcdir)/plugins/ChangeLog $(CHANGELOG2DOXI)
	cd . \
	&& echo '@doxer_dnl'					>$@  \
	&& echo '@doxer_set{webmenu} development'		>>$@ \
	&& echo '@include "webframe.doxi"'			>>$@ \
	&& echo '@doxer_set{title} BEAST - Plugin ChangeLog'	>>$@ \
	&& echo '@subheading'					>>$@ \
	&& $(CHANGELOG2DOXI) 		<$<			>>$@
log-sfi.tmpdoxi: $(top_srcdir)/sfi/ChangeLog $(CHANGELOG2DOXI)
	cd . \
	&& echo '@doxer_dnl'					>$@  \
	&& echo '@doxer_set{webmenu} development'		>>$@ \
	&& echo '@include "webframe.doxi"'			>>$@ \
	&& echo '@doxer_set{title} BEAST - SFI ChangeLog'	>>$@ \
	&& echo '@subheading'					>>$@ \
	&& $(CHANGELOG2DOXI) 		<$<			>>$@
intermediate_files = $(strip	\
	news-file.tmpdoxi	\
	engine-mplan.tmpdoxi	\
	log-beast.tmpdoxi	\
	log-bse.tmpdoxi		\
	log-plugins.tmpdoxi	\
	log-sfi.tmpdoxi		\
)

gxk_rdocu_globs = $(top_srcdir)/beast-gtk/gxk/gxk*.[hc] $(top_srcdir)/beast-gtk/bstutils.[hc]
gxk_rdocu_exclude = gxkgentypes.h gxkmarshal.[hc]
bse_rdocu_globs = $(top_srcdir)/bse/bse*.[hc] $(top_srcdir)/bse/bse*.[hc][hc]
bse_rdocu_exclude = *.gen-proc.c *.genidl.hh bsegentypes.h

docu_filter = $(shell egrep '^/\*\*' -l $(wildcard $(1)))

gxk_rdocu_files = $(filter-out $(foreach f, $(gxk_rdocu_exclude), $(wildcard $(top_srcdir)/beast-gtk/gxk/$(f))), $(wildcard $(gxk_rdocu_globs)))
html/rdocu-gxk/index.html: $(gxk_rdocu_files) rdocu-template.doxi
	doxer src2html -o html/rdocu-gxk/ $(DOXIDEFS)	\
	      -T rdocu-template.doxi -D MODULE GXK	\
	      -D TOP_WEBDIR ".."			\
	         $(filter %.h %.hh, $(gxk_rdocu_files)) \
	      -A $(filter %.c %.cc, $(gxk_rdocu_files))
bse_rdocu_files = $(filter-out $(foreach f, $(bse_rdocu_exclude), $(wildcard $(top_srcdir)/bse/$(f))), $(wildcard $(bse_rdocu_globs)))
bse_rdocu_dscrs = $(call docu_filter, $(bse_rdocu_files))
html/rdocu-bse/index.html: $(bse_rdocu_files) rdocu-template.doxi
	doxer src2html -o html/rdocu-bse/ $(DOXIDEFS)	--debug \
	      -T rdocu-template.doxi -D MODULE BSE	\
	      -D TOP_WEBDIR ".."			\
	         $(filter %.h %.hh, $(bse_rdocu_files)) \
	      -A $(filter %.c %.cc, $(bse_rdocu_dscrs))
