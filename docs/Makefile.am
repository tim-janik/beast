# BEAST - Better Audio System
include $(top_srcdir)/Makefile.decl

SUBDIRS = imports images

QPRINT = @printf '  %-7s%s\n'

# Describe source tree version (release tag relative, e.g. yy.mm.z-nnn-gHEXHEX)
DSC_VERSION = $(shell git describe --always --match '[0-9]*' HEAD 2>/dev/null)
# Release version, shows last tag reachable
REL_VERSION = $(shell git describe --always --abbrev=0 HEAD 2>/dev/null)
# Detailed documentation version, falls back to package version for tarballs
DOC_VERSION = $(or $(DSC_VERSION), $(VERSION))

# == Extra Tarball Files ==
EXTRA_DIST += bse-categories.txt interpolation.txt ChangeLog.svn

# == Doxygen Install Rules ==
htmldocsbase  = $(pkgdocdir)
htmldocsdir   = ${htmldocsbase}/html
htmldocs_DATA = html/doc-version		# build dependency for doxygen docs
htmlparent    = $(if $(findstring \#, ..\@INGIT@..), $(srcdir), $(builddir))
install-data-hook: @INGIT@ $(htmldocs_DATA)
	umask 022 && cp -RP $(htmlparent)/html/ "$(DESTDIR)$(htmldocsbase)/"
	chmod -R u+w "$(DESTDIR)$(htmldocsbase)/"
# u+w is needed for uninstall (distcheck enforces read-only srcdir contents)
uninstall-local:
	rm -Rf "$(DESTDIR)$(htmldocsbase)/html/"

# == html/ build rules ==
include Makefile.doxygen
git_index_file = $(shell test ! -x $(top_srcdir)/.git/ || echo $(top_srcdir)/.git/index)
html/doc-version: @INGIT@ $(if $(NODOC),, $(git_index_file)) # conditionally rebuild, depending on HEAD changes
	$(AM_V_GEN)
	$(Q) test -e $@ && test "`cat $@`" = "$(DOC_VERSION)" \
	|| $(MAKE) $(AM_MAKEFLAGS) --no-print-directory html-docs
	$(Q) test -e $@ # created by 'html-docs'

# == Doxygen Build Rules ==
HTMLMAN_FILES     	= imports/beast.1.html imports/bse.5.html imports/bsescm.1.html imports/bsewavetool.1.html imports/sfidl.1.html
DOXYGEN_DOC_FILES	= docs/main.dox
DOXYGEN_DOC_DIRS	= sfi/ bse/ plugins/ plugins/freeverb/ beast-gtk/ beast-gtk/gxk/ launchers/ shell/ tools/
DOXYGEN_EXCLUDES	= rope/cpy2rope.cc rope/cxx-client.[hc][hc] rcore/signalvariants.hh
DOXYGEN_CHANGELOG       = ChangeLog.tmp
DOXYGEN_CHANGELOG_TITLE = "Development Changes"
DOXYGEN_NEWS     	= $(top_srcdir)/NEWS
DOXYGEN_PROJECT_LINK    = http://beast.testbit.eu/
DOXYGEN_TAGFILES        = $(strip \
	imports/tagfile-susv4.xml=http://pubs.opengroup.org/onlinepubs/9699919799/	\
	imports/cppreference-doxygen-web.tag.xml=http://en.cppreference.com/w/		\
)
# imports/tagfile-rapicorn.xml=http://dev.testbit.eu/rapicorn/latest/
html-docs: doxygen-check
	$(AM_V_GEN)
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-clean
	$(QPRINT) "GEN" "Documentation Sources"
	$(Q) $(GIT_CHANGELOG) > $(DOXYGEN_CHANGELOG)
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-srctree
	$(Q) rm -f $(DOXYGEN_CHANGELOG)
	$(QPRINT) "GEN" "docs/docextract.dox"
	$(Q) cd doxygen-srctree && \
	  find . -type f -name \*[hcHC] -print | \
	  $(PYTHON) $(abs_srcdir)/docextract.py > docs/docextract.dox
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-html
	$(QPRINT) "GEN" "HTML manual pages..."
	$(Q) cp $(HTMLMAN_FILES) doxygen-html/
	$(QPRINT) "MOVE" "HTML Documentation..."
	$(Q) rm -rf html/
	$(Q) mv doxygen-html html/
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-clean
	$(Q) rm -f html/doc-version && echo "$(DOC_VERSION)" > html/doc-version
EXTRA_DIST += html
.PHONY: html-docs
CLEANFILES += $(DOXYGEN_CHANGELOG)
GIT_CHANGELOG = ( git log --first-parent --date=short --pretty='%ad  %an 	\# %h%n%n%s%n%n%b' --abbrev=11 \
		  ce584d04999a7fb9393e1cfedde2048ba73e8878..HEAD \
		| sed 's/^/	/; s/^	//; /^[ 	]*<unknown>$$/d' )
clean-html:
	rm -rf html/
clean-local: doxygen-clean @INGIT@ clean-html

# == Upload Rule ==
CHECK_RSYNC = { command -v rsync >/dev/null && { echo "3.0.0" ; rsync --version 2>&1 | sed 's/[^0-9]*// ; 1q' ; } | sort -VC ; }
# Upload release versions as $DOC_VERSION, otherwise upload as latest/
upload-docs: html/doc-version
	$(AM_V_GEN)
	$(Q) ${CHECK_RSYNC} || { echo "$@: failed to detect recent version: rsync"; exit 1; }
	$(Q) test "${REL_VERSION}" != "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html/"   "testbit:pub/docs/beast/${DOC_VERSION}" ; }
	$(Q) test "${REL_VERSION}"  = "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html/"   "testbit:pub/docs/beast/latest/" ; }
upload: upload-docs
.PHONY: upload upload-docs
