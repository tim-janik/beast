# BEAST - Better Audio System
# Copyright (C) 1998-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = images

# === Manual Pages to be fetched, built and installed
man_MANS              = ${MANUAL_PAGES} # automake-1.11 needs this expandable w/o patsubst
MANUAL_PAGES	      = mans/bse.5 mans/beast.1 mans/bsescm.1 mans/bsewavetool.1 mans/sfidl.1
MANUAL_CACHE_TARGETS  = $(patsubst mans/%,cache/%,$(MANUAL_PAGES))
HTMLMAN_TARGETS       = $(patsubst mans/%,html/%.html,$(MANUAL_PAGES))
EXTRA_DIST           += $(MANUAL_PAGES) $(HTMLMAN_TARGETS)
MAINTAINERCLEANFILES += $(MANUAL_CACHE_TARGETS) $(MAN_TARGETS)
CLEANFILES           += $(HTMLMAN_TARGETS)
# rules
mans/:
	mkdir -p $@
$(MANUAL_CACHE_TARGETS): cache/stamp-refresh
	curl -sfS http://beast.testbit.eu/$(@F)?action=render >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@ ; rm -f $(@D)/xgen-$(@F)
WIKIHTML2MAN = wikihtml2man.py
mans/%: cache/% | mans/
	$(WIKIHTML2MAN) $< >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@ ; rm -f $(@D)/xgen-$(@F)
html/%.html: mans/% | html/
	groff -mandoc -Thtml $< >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@ ; rm -f $(@D)/xgen-$(@F)

# === Html Docs to be fetched, built and installed
htmldocsdir           = ${beastdocsdir}/html
htmldocs_DATA         = $(HTML_ONLINE_DOCS) $(HTML_SIMPLE_DOCS) $(HTMLMAN_TARGETS)
HTML_SIMPLE_DOCS      = html/beast-index.html html/news-file.txt
HTML_ONLINE_DOCS      = html/Beast-Quickstart.html html/Beast_FAQ.html
HTML_CACHE_TARGETS    = $(patsubst html/%,cache/%,$(HTML_ONLINE_DOCS))
EXTRA_DIST	     += $(HTML_ONLINE_DOCS) $(HTML_SIMPLE_DOCS)
MAINTAINERCLEANFILES += $(HTML_CACHE_TARGETS)
CLEANFILES           += html/ $(HTML_ONLINE_DOCS) $(HTML_SIMPLE_DOCS)
# rules
html/:
	mkdir -p $@
html/news-file.txt: $(top_srcdir)/NEWS | html/
	cp $< $@
html/beast-index.html: beast-index.html | html/
	cp $< $@
EXTRA_DIST += beast-index.html
$(HTML_CACHE_TARGETS): cache/stamp-refresh
	curl -sfS http://beast.testbit.eu/$(basename $(@F))?action=render >$(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@ ; rm -f $(@D)/xgen-$(@F)
html/%.html: cache/%.html | html/
	@echo 'htmlgen... $(@F)...'
	@echo -e $(HTML1) > $(@D)/xgen-$(@F) && sed -n $(H1SED) $< >> $(@D)/xgen-$(@F)
	@echo -e $(HTML2) >> $(@D)/xgen-$(@F) && cat $< >> $(@D)/xgen-$(@F) && echo -e $(HTML3) >> $(@D)/xgen-$(@F)
	mv $(@D)/xgen-$(@F) $@ ; rm -f $(@D)/xgen-$(@F)
HTML1 = '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">\n' \
	'<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">\n' \
	'<head>  <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />\n' \
	'<title>'
HTML2 = '</title>\n' \
	'</head> <body>\n'
HTML3 = '\n</body></html>\n'
H1SED = '/<[hH]1\b/{ s/<[^>]\+>//g ; s/^\s\+//; p ; q }' # sed hack to extract first H1 title

# === cache/ handling for file downloads
cache/:
	mkdir -p $@
.PHONY: stamp-refresh-message
stamp-refresh-message:	# conditional phony dependency
	@TXT="  Some source files hosted online appear outdated; forcing refresh...  " \
	&& echo "$$TXT" | sed 's/./=/g' && echo "$$TXT" && echo "$$TXT" | sed 's/./=/g'
cache/stamp-refresh: $(call AGINGRULE, cache/stamp-refresh, 1440, stamp-refresh-message) | cache/ # refresh daily
	touch $@
MAINTAINERCLEANFILES += cache/
# $(call AGINGRULE, stamp-file-for-aging, AgingInMinutes, rule-name-for-remaking)
AGINGRULE = $(shell test -e $(1) && find $(1) -type f -mmin -"$(strip $(2))" | grep -q '.' || echo " $(3)")


EXTRA_DIST += $(strip		\
	coding-style.doxi	\
	bse-categories.txt	\
	interpolation.txt	\
	ChangeLog.svn		\
)
