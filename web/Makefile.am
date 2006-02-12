# BEAST - Bedevilled Audio System
# Copyright (C) 2005-2006 Tim Janik
#
# GNU General Public License version 2 or (at your option) any later version.

# FIXME: remove:
top_srcdir = ..
top_builddir = ..
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
	\
	html/bse-interface.html		\
)
RDOCU_TARGETS = $(strip			\
	html/rdocu-gxk/index.html	\
	html/rdocu-bse/index.html	\
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
	mkdir -p tmpdoxi-gxk/ tmpdoxi-bse/
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
	rm -f html/style/*
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

TEST_INSTALL_DIR=~/public_html/beast/
TEST_INSTALL_DIR=/ptmp/beast-preview/
test-install: ${TEST_INSTALL_DIR}
	rm -rf ${TEST_INSTALL_DIR}/*
	cp -av ${TEST_INSTALL_DIR}/.htaccess-html html/.htaccess
	cp -av php/ html/ ${TEST_INSTALL_DIR}
	ln -s ../../doxer-pics/bse-files ${TEST_INSTALL_DIR}/html/test-files
	ln -s news.html ${TEST_INSTALL_DIR}/html/index.html

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


docu_filter = $(shell egrep '^/\*\*' -l $(wildcard $(1)))

gxk_rdocu_globs = $(top_srcdir)/beast-gtk/gxk/gxk*.[hc] $(top_srcdir)/beast-gtk/bstutils.[hc]
gxk_rdocu_exclude = gxkgentypes.h gxkmarshal.[hc]
gxk_rdocu_files = $(filter-out $(foreach f, $(gxk_rdocu_exclude), $(wildcard $(top_srcdir)/beast-gtk/gxk/$(f))), $(wildcard $(gxk_rdocu_globs)))
tmpdoxi-gxk/index.tmpdoxi: $(gxk_rdocu_files) rdocu-template.doxi
	rm -f tmpdoxi-gxk/*
	doxer src2doxi -d .tmpdoxi -o tmpdoxi-gxk/ -T rdocu-template.doxi	\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE GXK			\
	         $(filter %.h %.hh, $(gxk_rdocu_files))				\
	      -Y $(filter %.c %.cc, $(gxk_rdocu_files))
html/rdocu-gxk/index.html: tmpdoxi-gxk/index.tmpdoxi $(wildcard tmpdoxi-gxk/*.tmpdoxi)
html/rdocu-gxk/%.html: tmpdoxi-gxk/%.tmpdoxi
	rm -f html/rdocu-gxk/*
	doxer doxi2html -o html/rdocu-gxk/				\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE GXK		\
	      $(filter-out $<, $(shell echo tmpdoxi-gxk/*.tmpdoxi))
	doxer doxi2html -o html/rdocu-gxk/				\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE GXK		\
	      $<

bse_rdocu_globs = $(top_srcdir)/bse/bse*.[hc] $(top_srcdir)/bse/bse*.[hc][hc]
bse_rdocu_exclude = *.gen-proc.c *.genidl.hh bsegentypes.h bseautodoc.c
bse_rdocu_files = $(filter-out $(foreach f, $(bse_rdocu_exclude), $(wildcard $(top_srcdir)/bse/$(f))), $(wildcard $(bse_rdocu_globs)))
bse_rdocu_dscrs = $(call docu_filter, $(bse_rdocu_files))
tmpdoxi-bse/index.tmpdoxi: $(bse_rdocu_files) rdocu-template.doxi
	rm -f tmpdoxi-bse/*
	doxer src2doxi -d .tmpdoxi -o tmpdoxi-bse/ -T rdocu-template.doxi	\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE BSE			\
	         $(filter %.h %.hh, $(bse_rdocu_files))				\
	      -Y $(filter %.c %.cc, $(bse_rdocu_dscrs))
html/rdocu-bse/index.html: tmpdoxi-bse/index.tmpdoxi $(wildcard tmpdoxi-bse/*.tmpdoxi)
	rm -f html/rdocu-bse/*
	doxer doxi2html -o html/rdocu-bse/				\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE BSE		\
	      $(filter-out $<, $(shell echo tmpdoxi-bse/*.tmpdoxi))
	doxer doxi2html -o html/rdocu-bse/				\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE BSE		\
	      $<

AUTODOC = $(top_builddir)/bse/bseautodoc
.PHONY: $(AUTODOC) bse-interface.scad bse-interface.tmpdoxi  # FIXME

bse-interface.scad: $(AUTODOC) $(top_builddir)/config.status
	cd . \
	&& $(AUTODOC) -p -s procs	 >xgen-atd	\
	&& $(AUTODOC) -p -s structs	>>xgen-atd	\
	&& cp xgen-atd $@				\
	&& rm -f xgen-atd
CLEANFILES += xgen-atd
bse-interface.tmpdoxi: bse-interface.scad $(top_builddir)/config.status
	doxer scad2doxi -d .tmpdoxi $< -T rdocu-template.doxi		\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE "BSE API"

intermediate_files = $(strip	\
	news-file.tmpdoxi	\
	engine-mplan.tmpdoxi	\
	log-beast.tmpdoxi	\
	log-bse.tmpdoxi		\
	log-plugins.tmpdoxi	\
	log-sfi.tmpdoxi		\
	bse-interface.scad	\
	bse-interface.tmpdoxi	\
)
.INTERMEDIATE: $(intermediate_files)

.PHONY: all-targets target-dirs screenshot-files logogallery-files web-image-files docs-image-files clean style-files test-install

clean:
	rm -f html/style/*
	-rmdir -p html/style/
	rm -f html/screenshots/* html/logogallery/* html/web-images/* html/images/*
	-rmdir -p html/screenshots/ html/logogallery/ html/web-images/ html/images/
	rm -f html/rdocu-gxk/* html/rdocu-bse/*
	-rmdir -p html/rdocu-gxk/ html/rdocu-bse/
	rm -f tmpdoxi-gxk/* tmpdoxi-bse/*
	-rmdir -p tmpdoxi-gxk/ tmpdoxi-bse/
	rm -f html/* html/.htaccess
	-rmdir -p html/
	rm -f php/*
	-rmdir -p php/
	rm -f ${GENERATED} ${intermediate_files}
