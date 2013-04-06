# Build and upload development documentation
include $(top_srcdir)/Makefile.decl

all: build-stamp
build-stamp:
	$(MAKE) all-docs
	touch $@

top_srcdir  = ../..
git_stamp   = $(wildcard $(top_srcdir)/.git/$(shell git symbolic-ref -q HEAD) $(top_srcdir)/.git/packed-refs)
REL_VERSION   = $(shell git describe --always --abbrev=0 HEAD 2>/dev/null)
DSC_VERSION   = $(shell git describe --always --match '[0-9]*' HEAD 2>/dev/null)
PRJ_VERSION   = $(shell sed -ne "/^[ \t]*VERSION[ \t]*=/ { s/^[^=]*=[ \t]*//; p; q }" < $(top_srcdir)/Makefile)
DOC_VERSION = $(or $(DSC_VERSION), $(PRJ_VERSION))
SED_MKEVEN  = sed 's/1$$/0/;s/3$$/2/;s/5$$/4/;s/7$$/6/;s/9$$/8/'

DOC_DIRS   = birnet/ sfi/ bse/ plugins/ plugins/freeverb/ beast-gtk/ beast-gtk/gxk/ launchers/ shell/ tools/
DOCU_GLOBS  = *.dox *.py $(DOCU_HGLOBS)
DOCU_HGLOBS = *.idl *.h *.H *.hh *.proc
DOCU_CGLOBS = $(DOCU_GLOBS) *.c *.C *.cc
SRCTREE     = srctree
DOXYGENV    = doxygen-1.7.4
DOXYGEN     = $(if $(shell which $(DOXYGENV)), $(DOXYGENV), doxygen)
GPRINT	    = @printf '  %-7s%s\n'
Q           = $(if $(findstring 1, $(V)) ,, @)
QSTDOUT     = $(Q:@=1>/dev/null)
QSTDERR     = $(Q:@=2>/dev/null)

EXCLUDES = rope/cpy2rope.cc rope/cxx-client.[hc][hc] rcore/signalvariants.hh

CHANGELOG_PGT = "BEAST Development ChangeLog"
CHANGELOG_SED = -e 's/(@\w)/\\\1/g; s/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g; s,\*/,*\&nbps;/,g' \
		-e '/^[^ ].*\# [0-9a-f]/s/^(.*)\# ([0-9a-f]+)/@section commit_\2 \0/' \
		-e 's/^[ \t]+\*? ?([a-zA-Z0-9_.+\/-]+):[ \t]*(\w.*)$$/- @b \1: \2/' \
		-e 's/^[ \t]+\*? ?([a-zA-Z0-9_.+\/-]+):(.*)/- <em><b>\1:<\/b> \2<\/em>/' \
		-e 's/^/ * /'

.PHONY: $(SRCTREE)
$(SRCTREE):
	$(GPRINT) "GEN" $(SRCTREE)/
	$(Q) rm -rf $(SRCTREE)/ && mkdir -p $(SRCTREE)/
	$(GPRINT) "GEN" "ChangeLog"
	$(Q) test -f $(top_srcdir)/ChangeLog || { echo "Missing $(top_srcdir)/ChangeLog" >&2 ; false ; }
	$(Q) echo '/*! @page ChangeLog' $(CHANGELOG_PGT)	 > $(SRCTREE)/ChangeLog
	$(Q) sed -r $(CHANGELOG_SED) <$(top_srcdir)/ChangeLog	>> $(SRCTREE)/ChangeLog
	$(Q) echo ' */' 					>> $(SRCTREE)/ChangeLog
	$(GPRINT) "COPY" "Source tree..." \
	&& CWD=`pwd` \
	&& for dir in $(DOC_DIRS) ; do \
	  (mkdir -p $$CWD/$(SRCTREE)/$$dir/ \
	   && cd $(top_srcdir)/$$dir \
	   && find . -maxdepth 1 -type f \( $(patsubst %,-name '%' -o, $(DOCU_CGLOBS)) -false \) \
	      -exec cp {} $$CWD/$(SRCTREE)/$$dir/{} \; ) || exit $? ; \
	  done
	$(GPRINT) "ADD" "File docu commands..." \
	&& cd $(SRCTREE)/ \
	&& find . -type f \( $(patsubst %,-name '%' -o, $(DOCU_GLOBS)) -false \) \
	   -exec sed  '1,+0s,^,/** @file {} */,' {} --in-place \;

LINK_PROJECT_NAME = \
	-e 's|<div id="projectname">BEAST/BSE|<div id="projectname"><a href="http://beast.testbit.eu">BEAST/BSE</a>|'
R_ACCESSKEY_AS_F = -e 's/(<input [^>]*id="MSearchField"[^>]*accesskey=")S"/\1F"/'

download: download-local
download-local:
	$(Q) wget --progress=dot --output-document=- http://dev.testbit.eu/rapicorn/latest/tagfile.xml > tmp-tagfile-rapicorn.xml
	$(Q) mv tmp-tagfile-rapicorn.xml tagfile-rapicorn.xml

all-docs: $(git_stamp) doxygen.cfg
	$(GPRINT) "TIDY" "Temporaries..."
	$(Q) rm -rf html.tmp/ && mkdir -p html.tmp/
	$(Q) $(MAKE) $(SRCTREE)
	$(GPRINT) "GEN" "HTML Documentation..."
	$(Q) test -e tagfile-rapicorn.xml || touch tagfile-rapicorn.xml
	$(Q) (cat doxygen.cfg \
	&& echo "STRIP_FROM_PATH  = `pwd`/$(SRCTREE)" \
	&& echo "INPUT		  = $(SRCTREE)/" \
	&& echo "EXAMPLE_PATH	  = $(top_srcdir)/" \
	&& echo "OUTPUT_DIRECTORY = html.tmp/" \
	&& echo "TAGFILES         = tagfile-rapicorn.xml=http://dev.testbit.eu/rapicorn/latest/" \
	&& echo -n "PROJECT_NUMBER = $(PRJ_VERSION)" \
	) | nice $(DOXYGEN) - >error.log 2>&1
	$(Q) test ! -s error.log && rm -f error.log \
	|| { echo "WARNING: doxygen issues encountered ($$(wc -l <error.log)): $$(pwd)/error.log" ; cat error.log $(QSTDOUT) ; }
	$(GPRINT) "POLISH" "HTML Files..."
	$(Q) sed $(LINK_PROJECT_NAME) -i html.tmp/html/*.html
	$(Q) sed -r $(R_ACCESSKEY_AS_F) -i html.tmp/html/*.html
	$(Q) mv html.tmp/html/ html.tmp/$(DOC_VERSION)/
	$(Q) ln -s $(DOC_VERSION) html.tmp/latest
	$(GPRINT) "RSYNC" "HTML Documentation..."
	$(Q) rsync -aH --del html.tmp/ html/
	$(GPRINT) "CLEAN" "Temporaries..."
	$(Q) rm -rf $(SRCTREE)/ html.tmp/

# upload release versions directly, otherwise upload as latest/
upload: all
	$(Q) test "${REL_VERSION}" != "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html/${DOC_VERSION}"  "testbit:/srv/dev/html/beast/" ; }
	$(Q) test "${REL_VERSION}" = "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html/${DOC_VERSION}/"  "testbit:/srv/dev/html/beast/latest/" ; }

clean:
	rm -f build-stamp
	rm -rf $(SRCTREE)/ html.tmp/
	rm -rf html/
maintainer-clean: clean
