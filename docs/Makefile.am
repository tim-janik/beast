# BEAST - Better Audio System
include $(top_srcdir)/Makefile.decl

SUBDIRS = images

# = Build & Dist Items =
# manual pages to be installed, downloaded from beast.testbit.eu
man_MANS      = ${MANUAL_PAGES} # automake-1.11 needs this expandable w/o patsubst
# html pages to be installed, some downloaded from beast.testbit.eu
htmldocsdir   = ${beastdocsdir}/html
htmldocs_DATA = $(HTML_ONLINE_DOCS) $(HTML_SIMPLE_DOCS) $(HTMLMAN_PAGES)
ONLINE_FILES  = # files stored online, to be checked for aging

# == Html File Builds ==
HTML_SIMPLE_DOCS = html/beast-index.html html/news-file.txt
EXTRA_DIST	+= $(HTML_SIMPLE_DOCS)
CLEANFILES      += $(HTML_SIMPLE_DOCS)
html/news-file.txt: $(top_srcdir)/NEWS
	cp $< $@
html/beast-index.html: beast-index.html
	cp $< $@
EXTRA_DIST += beast-index.html

# == Html File Download ==
HTML_ONLINE_DOCS      = html/Beast-Quickstart.html html/Beast_FAQ.html
MAINTAINERCLEANFILES += $(HTML_ONLINE_DOCS)
HTMLFILES_CACHED      = $(patsubst html/%, cache/%, $(HTML_ONLINE_DOCS))
HTMLFILES_NEW         = $(patsubst html/%, newhtml/%, $(HTML_ONLINE_DOCS))
EXTRA_DIST           += $(HTML_ONLINE_DOCS)
ONLINE_FILES         += $(HTML_ONLINE_DOCS)
download-html:
	test "$(srcdir)" = "$(builddir)" -a -d html/	# online docs are downloaded and stored in $(srcdir)/html/
	rm -rf cache/ newhtml/
	mkdir -p cache/ newhtml/
	$(MAKE) $(AM_MAKEFLAGS) $(HTMLFILES_CACHED)
	$(MAKE) $(AM_MAKEFLAGS) $(HTMLFILES_NEW)
	rm -rf cache/ $(srcdir)/html/
	mv -v newhtml/ $(srcdir)/html/
$(HTMLFILES_CACHED):
	curl -sfS http://beast.testbit.eu/$(basename $(@F))?action=render >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@

# === Html File Generation ===
newhtml/%.html: cache/%.html
	$(AM_V_GEN)
	$(Q) echo -e $(HTML1)		 > $(@D)/xgen-$(@F)
	$(Q) sed -n $(H1SED) $< 	>> $(@D)/xgen-$(@F)
	$(Q) echo -e $(HTML2)		>> $(@D)/xgen-$(@F)
	$(Q) cat $< 			>> $(@D)/xgen-$(@F)
	$(Q) echo -e $(HTML3)		>> $(@D)/xgen-$(@F)
	$(Q) mv $(@D)/xgen-$(@F) $@
HTML1 = '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">\n' \
	'<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">\n' \
	'<head>  <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />\n' \
	'<title>'
HTML2 = '</title></head>\n' \
	'<body>\n'
HTML3 = '\n</body></html>\n'
H1SED = '/<[hH]1\b/{ s/<[^>]\+>//g ; s/^\s\+//; p ; q }' # sed hack to extract first H1 title

# == Manual Page Download ==
MANUAL_PAGES          = mans/bse.5 mans/beast.1 mans/bsescm.1 mans/bsewavetool.1 mans/sfidl.1
MAINTAINERCLEANFILES += $(MANUAL_PAGES)
MANPAGES_CACHED       = $(patsubst mans/%, cache/%, $(MANUAL_PAGES))
MANPAGES_NEW          = $(patsubst mans/%, newman/%, $(MANUAL_PAGES))
EXTRA_DIST           += $(MANUAL_PAGES)
ONLINE_FILES         += $(MANUAL_PAGES)
download-man:
	test "$(srcdir)" = "$(builddir)" -a -d mans/	# manual pages are downloaded and stored in $(srcdir)/mans/
	rm -rf cache/ newman/
	mkdir -p cache/ newman/
	$(MAKE) $(AM_MAKEFLAGS) $(MANPAGES_CACHED)
	$(MAKE) $(AM_MAKEFLAGS) $(MANPAGES_NEW)
	rm -rf cache/ $(srcdir)/mans/
	mv -v newman/ $(srcdir)/mans/
$(MANPAGES_CACHED):
	curl -sfS http://beast.testbit.eu/$(@F)?action=render >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@
newman/%: cache/%	# used for $(MANPAGES_NEW)
	$(WIKIHTML2MAN) $< >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@

# == HTML Manual Pages ==
HTMLMAN_PAGES         = $(patsubst mans/%, html/%.html, $(MANUAL_PAGES))
MAINTAINERCLEANFILES += $(HTMLMAN_PAGES)
EXTRA_DIST           += $(HTMLMAN_PAGES)
html/%.html: mans/%
	test "$(srcdir)" = "$(builddir)" -a -d html/	# online docs are downloaded and stored in $(srcdir)/html/
	groff -mandoc -Thtml $< \
	| sed $(HREFS) >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@
HREFS = 's,&lt;\(\(http\|ftp\)s\?:[^<>]\+\)&gt;,\&lt;<a href="\1">\1</a>\&gt;,g'

# == Missing File Check ==
MISSING_MSG = "  Some source files hosted online appear missing; please issue: make download  "
check-missing-files:
	$(Q) ( for i in $(ONLINE_FILES) ; do 		\
	  test -e "$(srcdir)/$$i" || echo "$$i" ;	\
	done )				> xgen-missing.lst
	$(Q) test ! -s xgen-missing.lst || { 		\
	  echo $(MISSING_MSG) | sed 's/./=/g'		\
	  && echo $(MISSING_MSG)			\
	  && echo $(MISSING_MSG) | sed 's/./=/g'	\
	  && echo "Missing Files:"			\
	  && cat xgen-missing.lst ; false ; }
	$(Q) rm -f xgen-missing.lst

# == Aging File Check ==
OUTDATED_MSG = "  Some source files hosted online appear outdated; please issue: make download  "
CURRENTNESS  = 1440 # 1440 minutes require daily checks
check-aging-files: check-missing-files
	$(Q) (cd $(srcdir) && find $(ONLINE_FILES) -type f \! -mmin -$(strip $(CURRENTNESS)) ) > xgen-aging.lst
	$(Q) test ! -s xgen-aging.lst || { 		\
	  echo $(OUTDATED_MSG) | sed 's/./=/g'		\
	  && echo $(OUTDATED_MSG)			\
	  && echo $(OUTDATED_MSG) | sed 's/./=/g'	\
	  && echo "Outdated Files:"			\
	  && cat xgen-aging.lst ; false ; }
	$(Q) rm -f xgen-aging.lst
dist-hook: check-aging-files

# == Custom Rules ==
download: # use recursive make, because these rules replace entire subdirs containing files checked for dependencies
	$(MAKE) $(AM_MAKEFLAGS) download-html
	$(MAKE) $(AM_MAKEFLAGS) download-man
	$(MAKE) $(AM_MAKEFLAGS) $(HTMLMAN_PAGES)
clean-local:
	-rm -rf cache/
all: check-missing-files

# == Misc Distribution Files ==
EXTRA_DIST += $(strip		\
	bse-categories.txt	\
	interpolation.txt	\
	ChangeLog.svn		\
)
