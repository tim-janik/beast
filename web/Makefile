# BEAST - Bedevilled Audio System
# Copyright (C) 2005-2006 Tim Janik
#
# GNU General Public License version 2 or (at your option) any later version.

# FIXME: remove:
top_srcdir = ..
top_builddir = ..
srcdir = .
BST_VERSION = 0.6.6

all:	site


PHP_TARGETS = $(strip			\
	php/file-browser.php		\
	php/file-upload.php		\
	php/gallery.php			\
	php/screenshots.phpini		\
	php/logogallery.phpini		\
)
HTML_TARGETS = $(strip			\
	html/.htaccess			\
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
	html/bse-objects.html		\
	html/bse-interface.html		\
)
RDOCU_TARGETS = $(strip			\
	html/rdocu-sfi/index.html	\
	html/rdocu-bse/index.html	\
	html/rdocu-gxk/index.html	\
)
TARGET_DIRS = $(strip					\
	php/ html/ html/style/				\
	html/web-images/ html/images/			\
	html/screenshots/ html/logogallery/		\
	tmpdoxi-sfi/ tmpdoxi-bse/ tmpdoxi-gxk/		\
	html/rdocu-sfi/ html/rdocu-bse/ html/rdocu-gxk/	\
)
${HTML_TARGETS} ${RDOCU_TARGETS}: webmenu.doxi webframe.doxi beastdefs.doxi

# tarball targets
beastweb-galleries.tar.bz2: ${TARGET_DIRS} html/screenshots/.md5sum html/logogallery/.md5sum
	tar jcf $@ html/screenshots/ html/logogallery/
beastweb-pages.tar.bz2: ${TARGET_DIRS} html/style/.md5sum html/web-images/.md5sum html/images/.md5sum ${PHP_TARGETS} ${HTML_TARGETS} ${RDOCU_TARGETS}
	tar jcf $@ php/ html/ --anchored --exclude='html/screenshots/*' --exclude='html/logogallery/*'
CLEANFILES += beastweb-galleries.tar.bz2 beastweb-pages.tar.bz2

# website targets
site: beastweb-pages.tar.bz2 beastweb-galleries.tar.bz2
.PHONY: site site-preview site-update site-update-all site-update-galleries site-update-pages
# remote installation
ACCOUNT=${USER}@beast.gtk.org
ACDEST=/web/beast/
ACTMP=xgen-tmp
site-update-pages: beastweb-pages.tar.bz2
	scp -p $< ${ACCOUNT}:${ACTMP}-$(<F)
	ssh ${ACCOUNT} 					\
	 "set -ex "					\
	 "&& tar jxmpf ${ACTMP}-$(<F) -C ${ACDEST} "	\
	 "&& rm ${ACTMP}-$(<F)"
site-update-galleries: beastweb-galleries.tar.bz2
	scp -p $< ${ACCOUNT}:${ACTMP}-$(<F)
	ssh ${ACCOUNT} 					\
	 "set -ex "					\
	 "&& tar jxmpf ${ACTMP}-$(<F) -C ${ACDEST} "	\
	 "&& rm ${ACTMP}-$(<F)"
site-update-all: beastweb-pages.tar.bz2 beastweb-galleries.tar.bz2 site-update-pages site-update-galleries
site-update: site-update-pages
# preview installation
BEAST_PREVIEW=~/public_html/beast-preview/
site-preview: ${BEAST_PREVIEW}/. beastweb-pages.tar.bz2 beastweb-galleries.tar.bz2
	rm -rf ${BEAST_PREVIEW}/html/ ${BEAST_PREVIEW}/php/
	tar jxmpf beastweb-pages.tar.bz2 -C ${BEAST_PREVIEW}
	tar jxmpf beastweb-galleries.tar.bz2 -C ${BEAST_PREVIEW}
	ln -s ../beast-ftp ${BEAST_PREVIEW}/html/beast-ftp
	ln -s news.html ${BEAST_PREVIEW}/html/index.html

${TARGET_DIRS}:	# rule to create directories
	mkdir -p $@

php/%.php: %.php
	cp $< $@
php/%.phpini: %/ scanimages.sh
	$(srcdir)/scanimages.sh $< >xgen-$(@F)
	cp xgen-$(@F) $@
	rm -f xgen-$(@F)

WEBMENU_DOXI_FILES = faq.doxi
WEBMENU_DEFS  = $(patsubst %, -D DOXI_INCLUDE_FILE \"webframe.doxi\" , $(findstring $(<F), ${WEBMENU_DOXI_FILES}))
WEBFRAME_DOXI_FILES = quickstart.doxi
WEBFRAME_DEFS = $(patsubst %, -D DOXI_INCLUDE_FILE \"webframe.doxi\" , $(findstring $(<F), ${WEBFRAME_DOXI_FILES}))
DOXIDEFS = -D TOP_WEBDIR "." -D BST_VERSION ${BST_VERSION} -I $(srcdir) $(call WEBFRAME_DEFS) $(call WEBMENU_DEFS)

html/%.1.html: ../docs/%.1.doxi
	doxer doxi2htmlman $< -o html/ $(DOXIDEFS)
html/%.3.html: ../docs/%.3.doxi
	doxer doxi2htmlman $< -o html/ $(DOXIDEFS)
html/%.phtml: %.doxi
	doxer doxi2html $< -o html/ -e .phtml $(DOXIDEFS)
html/%.html: %.doxi
	doxer doxi2html $< -o html/ $(DOXIDEFS)
html/%.html: ../docs/%.doxi
	doxer doxi2html $< -o html/ $(DOXIDEFS)
html/.htaccess: htaccess.in
	cp $< $@

# copy rules
html/style/.md5sum: $(wildcard style/*)
	rm -fr html/style/
	mkdir -p html/style/
	cp style/*.png style/*.css html/style/
	doxer writecss -o html/style/
	md5sum html/style/* >xgen-html-style-md5sum
	mv xgen-html-style-md5sum $@		# atomic
html/web-images/.md5sum: $(wildcard web-images/*)
	rm -fr html/web-images/
	mkdir -p html/web-images/
	cp web-images/*.png html/web-images/
	md5sum html/web-images/* >xgen-html-web-images-md5sum
	mv xgen-html-web-images-md5sum $@	# atomic
html/images/.md5sum: $(wildcard ../docs/images/*)
	rm -fr html/images/
	mkdir -p html/images/
	cp ../docs/images/*.png html/images/
	md5sum html/images/* >xgen-html-images-md5sum
	mv xgen-html-images-md5sum $@		# atomic
html/screenshots/.md5sum: $(wildcard screenshots/*)
	rm -fr html/screenshots/
	mkdir -p html/screenshots/
	cp screenshots/*.png screenshots/*.jpg html/screenshots/
	md5sum html/screenshots/* >xgen-html-screenshots-md5sum
	mv xgen-html-screenshots-md5sum $@	# atomic
html/logogallery/.md5sum: $(wildcard logogallery/*)
	rm -fr html/logogallery/
	mkdir -p html/logogallery/
	cp logogallery/*.png logogallery/*.jpg html/logogallery/
	md5sum html/logogallery/* >xgen-html-logogallery-md5sum
	mv xgen-html-logogallery-md5sum $@	# atomic

html/%.html: %.tmpdoxi
	doxer doxi2html $< -o html/ $(DOXIDEFS)
news-file.tmpdoxi: $(top_srcdir)/NEWS # Makefile
	cd . \
	&& echo '@doxer_dnl'					>$@  \
	&& echo '@doxer_set{webmenu} development'		>>$@ \
	&& echo '@include "webframe.doxi"'			>>$@ \
	&& echo '@doxer_set{title} BEAST/BSE - NEWS'		>>$@ \
	&& echo '@subheading'					>>$@ \
	&& echo '@example'					>>$@ \
	&& sed  's/@/@@/g' 		<$<			>>$@ \
	&& echo '@done'						>>$@
engine-mplan.tmpdoxi: $(top_srcdir)/bse/engine-mplan.txt # Makefile
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


AUTODOC = $(top_builddir)/bse/bseautodoc
docu_filter = $(shell egrep '^/\*\*' -l $(wildcard $(1)))

# SFI reference docu
sfi_rdocu_globs = $(top_srcdir)/sfi/sfi*.[hc]
sfi_rdocu_exclude = sfidl*.*
sfi_rdocu_files = $(filter-out $(foreach f, $(sfi_rdocu_exclude), $(wildcard $(top_srcdir)/sfi/$(f))), $(wildcard $(sfi_rdocu_globs)))
tmpdoxi-sfi/index.tmpdoxi: $(sfi_rdocu_files) rdocu-template.doxi
	rm -f tmpdoxi-sfi/*
	doxer src2doxi $(strip 						\
	      -d .tmpdoxi -o tmpdoxi-sfi/ -T rdocu-template.doxi -i	\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE SFI		\
	         $(filter     %.h %.hh, $(sfi_rdocu_files))		\
	      -Y $(filter-out %.h %.hh, $(sfi_rdocu_files)) )
html/rdocu-sfi/index.html: tmpdoxi-sfi/index.tmpdoxi $(wildcard tmpdoxi-sfi/*.tmpdoxi)
	rm -f html/rdocu-sfi/*
	doxer doxi2html $(strip						\
	      -o html/rdocu-sfi/					\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE SFI		\
	      $(filter-out $<, $(shell echo tmpdoxi-sfi/*.tmpdoxi)) )
	doxer doxi2html $(strip						\
	      -o html/rdocu-sfi/					\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE SFI		\
	      $< )

# GXK reference docu
gxk_rdocu_globs = $(top_srcdir)/beast-gtk/gxk/gxk*.[hc] $(top_srcdir)/beast-gtk/bstutils.[hc]
gxk_rdocu_exclude = gxkgentypes.h gxkmarshal.[hc]
gxk_rdocu_files = $(filter-out $(foreach f, $(gxk_rdocu_exclude), $(wildcard $(top_srcdir)/beast-gtk/gxk/$(f))), $(wildcard $(gxk_rdocu_globs)))
tmpdoxi-gxk/index.tmpdoxi: $(gxk_rdocu_files) rdocu-template.doxi
	rm -f tmpdoxi-gxk/*
	doxer src2doxi $(strip						\
	      -d .tmpdoxi -o tmpdoxi-gxk/ -T rdocu-template.doxi -i	\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE GXK		\
	         $(filter %.h %.hh, $(gxk_rdocu_files))			\
	      -Y $(filter %.c %.cc, $(gxk_rdocu_files)) )
html/rdocu-gxk/index.html: tmpdoxi-gxk/index.tmpdoxi $(wildcard tmpdoxi-gxk/*.tmpdoxi)
	rm -f html/rdocu-gxk/*
	doxer doxi2html $(strip						\
	      -o html/rdocu-gxk/					\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE GXK		\
	      $(filter-out $<, $(shell echo tmpdoxi-gxk/*.tmpdoxi)) )
	doxer doxi2html $(strip						\
	      -o html/rdocu-gxk/					\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE GXK		\
	      $< )

# BSE reference docu
bse_rdocu_globs = $(top_srcdir)/bse/bse*.[hc] $(top_srcdir)/bse/bse*.[hc][hc]
bse_rdocu_exclude = *.gen-proc.c *.genidl.hh bsegentypes.h bseautodoc.c
bse_rdocu_files = $(filter-out $(foreach f, $(bse_rdocu_exclude), $(wildcard $(top_srcdir)/bse/$(f))), $(wildcard $(bse_rdocu_globs)))
bse_rdocu_dscrs = $(call docu_filter, $(bse_rdocu_files))
tmpdoxi-bse/index.tmpdoxi: $(bse_rdocu_files) rdocu-template.doxi
	rm -f tmpdoxi-bse/*
	doxer src2doxi $(strip						\
	      -d .tmpdoxi -o tmpdoxi-bse/ -T rdocu-template.doxi -i	\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE BSE		\
	         $(filter %.h %.hh, $(bse_rdocu_files))			\
	      -Y $(filter %.c %.cc, $(bse_rdocu_dscrs)) )
html/rdocu-bse/index.html: tmpdoxi-bse/index.tmpdoxi $(wildcard tmpdoxi-bse/*.tmpdoxi)
	rm -f html/rdocu-bse/*
	doxer doxi2html $(strip						\
	      -o html/rdocu-bse/					\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE BSE		\
	      $(filter-out $<, $(shell echo tmpdoxi-bse/*.tmpdoxi)) )
	doxer doxi2html $(strip						\
	      -o html/rdocu-bse/					\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE BSE		\
	      $< )

# BSE procedure and object docu
bse-objects.scad: $(AUTODOC) $(top_builddir)/config.status
	cd . \
	&& $(AUTODOC) -p -s objects	 >xgen-bojs	\
	&& cp xgen-bojs $@				\
	&& rm -f xgen-bojs
CLEANFILES += xgen-bojs
bse-objects.tmpdoxi: bse-objects.scad $(top_builddir)/config.status
	doxer scad2doxi -d .tmpdoxi $< -T rdocu-template.doxi		\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE "BSE OBJECTS"
bse-interface.scad: $(AUTODOC) $(top_builddir)/config.status
	cd . \
	&& $(AUTODOC) -p -s procs	 >xgen-bifs	\
	&& $(AUTODOC) -p -s structs	>>xgen-bifs	\
	&& cp xgen-bifs $@				\
	&& rm -f xgen-bifs
CLEANFILES += xgen-bifs
bse-interface.tmpdoxi: bse-interface.scad $(top_builddir)/config.status
	doxer scad2doxi -d .tmpdoxi $< -T rdocu-template.doxi		\
	      $(DOXIDEFS) -D TOP_WEBDIR ".." -D MODULE "BSE INTERFACE"

.PHONY: # $(AUTODOC) bse-objects.scad bse-objects.tmpdoxi bse-interface.scad bse-interface.tmpdoxi # FIXME

intermediate_files = $(strip	\
	news-file.tmpdoxi	\
	engine-mplan.tmpdoxi	\
	log-beast.tmpdoxi	\
	log-bse.tmpdoxi		\
	log-plugins.tmpdoxi	\
	log-sfi.tmpdoxi		\
	bse-objects.scad	\
	bse-objects.tmpdoxi	\
	bse-interface.scad	\
	bse-interface.tmpdoxi	\
)
.INTERMEDIATE: $(intermediate_files)

clean:
	rm -fr html/style/
	rm -fr html/screenshots/ html/logogallery/ html/web-images/ html/images/
	rm -f html/rdocu-sfi/* html/rdocu-bse/* html/rdocu-gxk/*
	-rmdir -p html/rdocu-sfi/ html/rdocu-bse/ html/rdocu-gxk/
	rm -f tmpdoxi-sfi/* tmpdoxi-bse/* tmpdoxi-gxk/*
	-rmdir -p tmpdoxi-sfi/ tmpdoxi-bse/ tmpdoxi-gxk/
	rm -f html/* html/.htaccess
	-rmdir -p html/
	rm -f php/*
	-rmdir -p php/
	rm -f ${intermediate_files} ${CLEANFILES}
.PHONY: clean
