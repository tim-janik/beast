# BEAST - Better Audio System
include $(top_srcdir)/Makefile.decl

SUBDIRS = imports images

QPRINT = @printf '  %-7s%s\n'

# Describe source tree version (release tag relative, e.g. yy.mm.z-nnn-gHEXHEX)
DSC_VERSION = $(shell git describe --always --match '[0-9]*' HEAD 2>/dev/null)
# Release version, shows last tag reachable
REL_VERSION = $(shell git describe --always --abbrev=0 HEAD 2>/dev/null)
# Project version as configured
PRJ_VERSION = $(shell sed -ne "/^[ \t]*VERSION[ \t]*=/ { s/^[^=]*=[ \t]*//; p; q }" < $(top_srcdir)/Makefile)
# Detailed documentation version
DOC_VERSION = $(or $(DSC_VERSION), $(PRJ_VERSION))

# == Extra Tarball Files ==
EXTRA_DIST += bse-categories.txt interpolation.txt ChangeLog.svn

# == Doxygen Install Rules ==
htmldocsbase  = ${docdir}
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
html/doc-version: @INGIT@ $(git_index_file)	# conditionally rebuild, depending on HEAD changes
	$(AM_V_GEN)
	$(Q) test -e $@ && test "`cat $@`" = "$(DOC_VERSION)" \
	|| $(MAKE) $(AM_MAKEFLAGS) --no-print-directory html-docs
	$(Q) rm -f $@ && echo "$(DOC_VERSION)" > $@
clean-html-docs:
	rm -rf html/
clean-local: @INGIT@ clean-html-docs doxygen-clean
EXTRA_DIST += html
html-docs: doxygen-check
	$(Q) rm -rf html/
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory build-docs DOXYGEN_GRAPHICS=NO DOC_TARGET=html
	$(Q) echo "$(DOC_VERSION)" > html/doc-version
.PHONY: html-docs

# == Doxygen Build Rules ==
HTMLMAN_FILES     	= imports/beast.1.html imports/bse.5.html imports/bsescm.1.html imports/bsewavetool.1.html imports/sfidl.1.html
DOXYGEN_DOC_FILES	= docs/main.dox
DOXYGEN_DOC_DIRS	= sfi/ bse/ plugins/ plugins/freeverb/ beast-gtk/ beast-gtk/gxk/ launchers/ shell/ tools/
DOXYGEN_EXCLUDES	= rope/cpy2rope.cc rope/cxx-client.[hc][hc] rcore/signalvariants.hh
DOXYGEN_CHANGELOG       = ChangeLog.tmp
DOXYGEN_NEWS     	= $(top_srcdir)/NEWS
DOXYGEN_PROJECT_LINK    = http://beast.testbit.eu/
DOXYGEN_TAGFILES        = imports/tagfile-rapicorn.xml=http://dev.testbit.eu/rapicorn/latest/ \
			  imports/tagfile-susv4.xml=http://pubs.opengroup.org/onlinepubs/9699919799/
build-docs: doxygen-check
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-clean
	$(QPRINT) "GEN" "Git ChangeLog"
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
	$(Q) ! test -e $(DOC_TARGET) || { echo "$@: target directory exists already: $(DOC_TARGET)"; exit 1; }
	$(Q) mv doxygen-html $(DOC_TARGET)
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-clean
.PHONY: build-docs
CLEANFILES += $(DOXYGEN_CHANGELOG)
GIT_CHANGELOG = ( git log --first-parent --date=short --pretty='%ad  %an 	\# %h%n%n%s%n%n%b' --abbrev=11 \
		  ce584d04999a7fb9393e1cfedde2048ba73e8878..HEAD \
		| sed 's/^/	/; s/^	//; /^[ 	]*<unknown>$$/d' )

# == Upload build rules ==
upload-docs: doxygen-check $(git_index_file)
	$(AM_V_GEN)
	$(Q) test -e $@ && test "`cat upload-docs/doc-version`" = "$(DOC_VERSION)" || (true \
	&& rm -rf upload-docs/ \
	&& $(MAKE) $(AM_MAKEFLAGS) --no-print-directory build-docs DOXYGEN_GRAPHICS=YES DOC_TARGET=upload-docs )
	$(Q) echo "$(DOC_VERSION)" > upload-docs/doc-version
clean-upload-docs:
	rm -rf upload-docs/
clean-local: clean-upload-docs
# Upload release versions as $DOC_VERSION, otherwise upload as latest/
upload: upload-docs
	$(AM_V_GEN)
	$(Q) ${CHECK_RSYNC} || { echo "$@: failed to detect recent version: rsync"; exit 1; }
	$(Q) test "${REL_VERSION}" != "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "upload-docs"   "testbit:/srv/dev/html/beast/${DOC_VERSION}" ; }
	$(Q) test "${REL_VERSION}"  = "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "upload-docs/"  "testbit:/srv/dev/html/beast/latest/" ; }
.PHONY: upload upload-docs
CHECK_RSYNC   = { command -v rsync >/dev/null && { echo "3.0.0" ; rsync --version 2>&1 | sed 's/[^0-9]*// ; 1q' ; } | sort -VC ; }
