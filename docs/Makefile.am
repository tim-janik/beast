# BEAST - Better Audio System
include $(top_srcdir)/Makefile.decl

SUBDIRS = images

# = Build & Dist Items =
# manual pages to be installed, downloaded from beast.testbit.eu
man_MANS      = ${MANUAL_PAGES} # automake-1.11 needs this expandable w/o patsubst
# html pages to be installed, some downloaded from beast.testbit.eu
htmldocsdir   = ${beastdocsdir}/html
htmldocs_DATA = $(HTML_SIMPLE_DOCS) $(HTML_ONLINE_DOCS) $(HTMLMAN_PAGES)

# == Html File Builds ==
HTML_SIMPLE_DOCS = html/beast-index.html html/news-file.txt
EXTRA_DIST	+= $(HTML_SIMPLE_DOCS)
CLEANFILES      += $(HTML_SIMPLE_DOCS)
html/news-file.txt: $(top_srcdir)/NEWS
	$(AM_V_GEN)
	$(Q) cp $< $@
html/beast-index.html: beast-index.html
	$(AM_V_GEN)
	$(Q) cp $< $@
EXTRA_DIST += beast-index.html

# == Download Imports ==
IMPORT_FILES = # files to be imported/updated
import:	# online docs are downloaded and stored in $(srcdir)/import/
	$(Q) test "$(srcdir)" = "$(builddir)" -a -d html/
	$(Q) rm -rf newimport/ && mkdir newimport/
	$(Q) for FN in $(IMPORT_FILES) ; do \
	  echo "  IMPORT" $$FN; \
	  curl -sfS http://beast.testbit.eu/$$FN?action=render > newimport/$$FN || exit $? ; \
	done
	$(Q) date +%s > newimport/stamp
	$(Q) rm -rf import/ && mv newimport/ import/
	$(MAKE) $(AM_MAKEFLAGS) $(MANUAL_PAGES)
.PHONY: import

# == Html Online Docs ==
HTML_ONLINE_DOCS      = html/Beast-Quickstart.html html/Beast_FAQ.html
IMPORT_FILES         += $(HTML_ONLINE_DOCS:html/%.html=%)
CLEANFILES 	     += $(HTML_ONLINE_DOCS)
html/:
	mkdir -p $@
$(HTML_ONLINE_DOCS): html/%.html: import/% | html/
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

# == Online Manual Pages ==
MANUAL_PAGES          = mans/bse.5 mans/beast.1 mans/bsescm.1 mans/bsewavetool.1 mans/sfidl.1
IMPORT_FILES         += $(MANUAL_PAGES:mans/%=%)
EXTRA_DIST           += $(MANUAL_PAGES)
MAINTAINERCLEANFILES += $(MANUAL_PAGES)
mans/:	# manual pages are downloaded and stored in $(srcdir)/mans/
	mkdir -p $@
$(MANUAL_PAGES): mans/%: import/% | mans/
	$(AM_V_GEN)
	$(Q) $(WIKIHTML2MAN) $< >$(@D)/xgen-$(@F)
	$(Q) mv $(@D)/xgen-$(@F) $@

# == HTML Manual Pages ==
HTMLMAN_PAGES = $(patsubst mans/%, html/%.html, $(MANUAL_PAGES))
CLEANFILES   += $(HTMLMAN_PAGES)
html/%.html: mans/%
	$(AM_V_GEN)
	$(Q) groff -mandoc -Thtml $< \
	| sed $(HREFS) >$(@D)/xgen-$(@F)
	$(Q) mv $(@D)/xgen-$(@F) $@
HREFS = 's,&lt;\(\(http\|ftp\)s\?:[^<>]\+\)&gt;,\&lt;<a href="\1">\1</a>\&gt;,g'

# == File Checks ==
MISSING_MSG = "  Some files hosted online appear missing or outdated; please issue: make import  "
CURRENTNESS = 21600 # 21600 seconds = 6 hours
check-aging-files: check-missing-files
	$(Q) LAST=`test -e import/stamp && cat import/stamp || echo 0` ; NOW=`date +%s` \
	; test $$(($$NOW - $$LAST)) -lt $(CURRENTNESS) || { \
	  echo $(MISSING_MSG) | sed 's/./=/g' && echo $(MISSING_MSG) && echo $(MISSING_MSG) | sed 's/./=/g' \
	  && false ; }
dist-hook: check-aging-files
check-missing-files:
	$(Q) test 1 $(patsubst %, -a -e $(srcdir)/import/%, $(IMPORT_FILES)) || { \
	  echo $(MISSING_MSG) | sed 's/./=/g' && echo $(MISSING_MSG) && echo $(MISSING_MSG) | sed 's/./=/g' \
	  && false ; }
import/%:
	$(MAKE) $(AM_MAKEFLAGS) check-missing-files

# == Custom Rules ==
all: check-missing-files

# == Misc Distribution Files ==
EXTRA_DIST += $(strip		\
	bse-categories.txt	\
	interpolation.txt	\
	ChangeLog.svn		\
)
