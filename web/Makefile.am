# BEAST - Bedevilled Audio System
# Copyright (C) 2005-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

PHP_TARGETS = $(strip			\
	php/file-browser.php		\
	php/file-upload.php		\
	php/gallery.php			\
	php/screenshots.phpini		\
	php/logogallery.phpini		\
)
HTML_TARGETS = $(strip			\
	html/.htaccess			\
	html/robots.txt			\
	html/favicon.ico		\
	\
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
${HTML_TARGETS} ${RDOCU_TARGETS}: webmenu.doxi webframe.doxi ../docs/beastdefs.doxi

# tarball targets
beastweb-galleries.tar.bz2: ${TARGET_DIRS} html/screenshots/.md5sum html/logogallery/.md5sum
	tar jcf $@ html/screenshots/ html/logogallery/
beastweb-pages.tar.bz2: ${TARGET_DIRS} html/style/.md5sum html/web-images/.md5sum html/images/.md5sum ${PHP_TARGETS} ${HTML_TARGETS} ${RDOCU_TARGETS}
	tar jcf $@ php/ html/ --anchored --exclude='html/screenshots/*' --exclude='html/logogallery/*'
CLEANFILES += beastweb-galleries.tar.bz2 beastweb-pages.tar.bz2

${TARGET_DIRS}:	# rule to create directories
	mkdir -p $@

php/%.php: %.php
	cp $< $@
php/%.phpini: %/ scanimages.sh
	$(srcdir)/scanimages.sh $< >xgen-$(@F)
	cp xgen-$(@F) $@
	rm -f xgen-$(@F)

DOXER = $(top_srcdir)/doxer/doxer.py

WEBMENU_DOXI_FILES = faq.doxi
DOCFRAME_DEFS = -D DOXI_INCLUDE_FILE $(if $(findstring $(<F), ${WEBMENU_DOXI_FILES}), \"webmenu.doxi\", \"webframe.doxi\")
DOXIDEFS = -D BST_VERSION ${BST_VERSION} -I $(srcdir)

html/%.1.html: ../docs/%.1.doxi
	$(DOXER) doxi2htmlman $(strip $< -d html/  -D TOP_WEBDIR "." $(DOXIDEFS) $(call DOCFRAME_DEFS))
html/%.3.html: ../docs/%.3.doxi
	$(DOXER) doxi2htmlman $(strip $< -d html/  -D TOP_WEBDIR "." $(DOXIDEFS) $(call DOCFRAME_DEFS))
html/%.html: ../docs/%.doxi
	$(DOXER) doxi2html $(strip $< -d html/  -D TOP_WEBDIR "." $(DOXIDEFS) $(call DOCFRAME_DEFS))
html/%.phtml: %.doxi
	$(DOXER) doxi2html $(strip $< -d html/ -j .phtml  -D TOP_WEBDIR "." $(DOXIDEFS))
html/%.html: %.doxi
	$(DOXER) doxi2html $(strip $< -d html/  -D TOP_WEBDIR "." $(DOXIDEFS))
html/.htaccess: htaccess.in
	cp $< $@
html/robots.txt: robots.txt
	cp $< $@
html/favicon.ico: web-images/favicon.ico
	cp $< $@

# copy rules
html/style/.md5sum: $(wildcard style/*)
	rm -fr html/style/
	mkdir -p html/style/
	cp style/*.png style/*.css html/style/
	$(DOXER) writecss -d html/style/
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


# Generated .doxi files
../docs/gendoxi/bse-objects.doxi \
../docs/gendoxi/bse-interface.doxi \
../docs/gendoxi/engine-mplan.doxi \
../docs/gendoxi/news-file.doxi: PHONY-TARGET
	@$(MAKE) -C ../docs $(patsubst ../docs/%, %, $@)
html/%.html: ../docs/gendoxi/%.doxi
	$(DOXER) doxi2html $(strip $< -d html/  -D TOP_WEBDIR "." $(DOXIDEFS) $(call DOCFRAME_DEFS))


html/%.html: %.tmpdoxi
	$(DOXER) doxi2html $< -d html/  -D TOP_WEBDIR "." $(DOXIDEFS)

CHANGELOG2DOXI = $(top_srcdir)/doxer/changelog2doxi.py
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


.PHONY: PHONY-TARGET

# Reference Documentation
../docs/psource/bse-interface.psrc \
../docs/psource/bse-objects.psrc \
../docs/psource/sfi.psrc \
../docs/psource/bse.psrc \
../docs/psource/gxk.psrc: PHONY-TARGET
	@$(MAKE) -C ../docs $(patsubst ../docs/%, %, $@)
# SFI reference docu
tmpdoxi-sfi/index.doxi: ../docs/psource/sfi.psrc ../docs/docu-template.doxi
	rm -f tmpdoxi-sfi/*
	$(DOXER) pickle2doxi $(strip -i	$<	-D TOP_WEBDIR ".."		\
	         -d tmpdoxi-sfi/ $(DOXIDEFS) -T ../docs/docu-template.doxi	\
	         -D DOCU_TEMPLATE_TITLE "BEAST - SFI Interface Reference"  	\
	         -D DOCU_TEMPLATE_HEADING "SFI Interface Reference" )
html/rdocu-sfi/index.html: tmpdoxi-sfi/index.doxi $(wildcard tmpdoxi-sfi/*.doxi)
	rm -f html/rdocu-sfi/*
	$(DOXER) doxi2html $(strip			-D TOP_WEBDIR ".."	\
	         -d html/rdocu-sfi/ $(DOXIDEFS) $(call DOCFRAME_DEFS)		\
	         $(filter-out $<, $(shell echo tmpdoxi-sfi/*.doxi)) )
	$(DOXER) doxi2html $(strip			-D TOP_WEBDIR ".."	\
	         -d html/rdocu-sfi/ $(DOXIDEFS) $(call DOCFRAME_DEFS) $< )
# GXK reference docu
tmpdoxi-gxk/index.doxi: ../docs/psource/gxk.psrc ../docs/docu-template.doxi
	rm -f tmpdoxi-gxk/*
	$(DOXER) pickle2doxi $(strip -i	$<	-D TOP_WEBDIR ".."		\
	         -d tmpdoxi-gxk/ $(DOXIDEFS) -T ../docs/docu-template.doxi	\
	         -D DOCU_TEMPLATE_TITLE "BEAST - GXK Interface Reference"  	\
	         -D DOCU_TEMPLATE_HEADING "GXK Interface Reference" )
html/rdocu-gxk/index.html: tmpdoxi-gxk/index.doxi $(wildcard tmpdoxi-gxk/*.doxi)
	rm -f html/rdocu-gxk/*
	$(DOXER) doxi2html $(strip			-D TOP_WEBDIR ".."	\
	         -d html/rdocu-gxk/ $(DOXIDEFS) $(call DOCFRAME_DEFS)		\
	         $(filter-out $<, $(shell echo tmpdoxi-gxk/*.doxi)) )
	$(DOXER) doxi2html $(strip			-D TOP_WEBDIR ".."	\
	         -d html/rdocu-gxk/ $(DOXIDEFS) $(call DOCFRAME_DEFS) $< )
# BSE reference docu
tmpdoxi-bse/index.doxi: ../docs/psource/bse.psrc ../docs/docu-template.doxi
	rm -f tmpdoxi-bse/*
	$(DOXER) pickle2doxi $(strip -i	$<	-D TOP_WEBDIR ".."		\
	         -d tmpdoxi-bse/ $(DOXIDEFS) -T ../docs/docu-template.doxi	\
	         -D DOCU_TEMPLATE_TITLE "BEAST - BSE Interface Reference"  	\
	         -D DOCU_TEMPLATE_HEADING "BSE Interface Reference" )
html/rdocu-bse/index.html: tmpdoxi-bse/index.doxi $(wildcard tmpdoxi-bse/*.doxi)
	rm -f html/rdocu-bse/*
	$(DOXER) doxi2html $(strip			-D TOP_WEBDIR ".."	\
	         -d html/rdocu-bse/ $(DOXIDEFS) $(call DOCFRAME_DEFS)		\
	         $(filter-out $<, $(shell echo tmpdoxi-bse/*.doxi)) )
	$(DOXER) doxi2html $(strip			-D TOP_WEBDIR ".."	\
	         -d html/rdocu-bse/ $(DOXIDEFS) $(call DOCFRAME_DEFS) $< )

intermediate_files = $(strip	\
	log-beast.tmpdoxi	\
	log-bse.tmpdoxi		\
	log-plugins.tmpdoxi	\
	log-sfi.tmpdoxi		\
)
.INTERMEDIATE: $(intermediate_files)

clean:
	rm -fr html/style/
	rm -fr html/screenshots/ html/logogallery/ html/web-images/ html/images/
	rm -f html/rdocu-sfi/* html/rdocu-bse/* html/rdocu-gxk/*
	-rmdir -p html/rdocu-sfi/ html/rdocu-bse/ html/rdocu-gxk/
	rm -f tmpdoxi-sfi/* tmpdoxi-bse/* tmpdoxi-gxk/*
	-rmdir -p tmpdoxi-sfi/ tmpdoxi-bse/ tmpdoxi-gxk/
	rm -f html/* html/.htaccess html/robots.txt html/favicon.ico
	-rmdir -p html/
	rm -f php/*
	-rmdir -p php/
	rm -f screenshots/*.THUMB.jpg logogallery/*.THUMB.jpg
	rm -f ${intermediate_files} ${CLEANFILES}
.PHONY: clean

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
