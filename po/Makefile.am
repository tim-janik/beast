# BEAST & BSE
include $(top_srcdir)/Makefile.decl
AUTOMAKE_OPTIONS = -Wno-portability

POSRC_TYPES =
INGIT_CLEANFILES =
CLEANFILES += $(INGIT_CLEANFILES)

# Verbosity macros
Qignore = $(if $(findstring 1, $(V)) ,, : )
Vignore = $(if $(findstring 1, $(V)) , : ,)

# Variables:
# ALL_LINGUAS : setup by configure.ac from po/LINGUAS
# PO_LINGUAS  : command line override for ALL_LINGUAS (subset)
# GMOFILES    : .gmo files for all TRANSLATIONS (GNU .mo)
# POFILES     : .po files for all TRANSLATIONS
#
# Files:
# LINGUAS     : list of supported languages
# poscan.list : list of translatable files tracked in git

TRANSLATIONS = $(if ${PO_LINGUAS}, ${PO_LINGUAS}, ${ALL_LINGUAS})
GMOFILES     = $(patsubst %, %.gmo, $(TRANSLATIONS))
POFILES      = $(patsubst %, %.po,  $(TRANSLATIONS))
#EXTRA_DIST += LINGUAS $(POFILES) messages.po
CLEANFILES  += $(GMOFILES)
KEYWORDS     = _ N_ U_ Q_ _:1,2,3t
TEXTFLAGS    = _:1:pass-c-format
TMPDIR       = tmp-cpots
LOCALEDIR    = $(pkginstalldir)/locale

all: $(GMOFILES)

# compile translation catalogs
%.gmo: %.po
	@echo $(ECHO_N) "$(@F): " \
	&& $(GMSGFMT) --statistics --directory=$(srcdir) -o xgen-$(@F) $(<F) \
	&& mv xgen-$(@F) $@

# install translation catalogs
install: install-data
install-data: $(GMOFILES)
	@echo "Installing localised message files..."
	@for lang in $(TRANSLATIONS); do \
	  $(mkinstalldirs) "$(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES/" || exit ; \
	  test ! -r "$$lang.gmo" || { \
	    echo "  INSTALL  " "$$lang.gmo" ; \
	    ( $(Qignore) set -x ; \
	      $(INSTALL_DATA) "$$lang.gmo" \
	        "$(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES/$(BST_GETTEXT_DOMAIN).mo" ) || exit 1 ; \
	  } ; \
	done
uninstall:
	@echo $(ECHO_N) "Uninstalling message files..."
	@for lang in $(TRANSLATIONS); do \
	  test -n "$$c" && test "$$c" -le 6 || { c=0; printf "\n  "; }; c=$$(($$c + 1)) ; \
	  printf "%-10s" "$$lang" ; \
	  rm -f "$(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES/$(BST_GETTEXT_DOMAIN).mo" ; \
	done ; test "$$c" = 0 || echo

shdie = { echo "ERROR:" $(1) >&2 ; exit $(or $(2), 127) ; }

merge-po: $(srcdir)/messages.po # FILE=...
	@test -r "$(FILE)" || $(call shdie, "$@: source argument required: make $@ FILE=...")
	@echo "Merging external po file: $(FILE)..."
	@r=`realpath "$(FILE)"` ; for f in $(POFILES) ; do \
	  test "`realpath $$f`" != "$$r" || \
	  $(call shdie, "$@: FILE already exists in POFILES: $$r"); \
	done
	@c= ; : "# find POFILE candidate for merging FILE";\
	p=`basename "$(FILE)" .po`; test -n "$$c" || \
	for f in $(POFILES) ; do \
	  test "`basename $$f .po`" == "$$p" && \
	  { c="$$f" ; r="(basenames match)"; break ; } ; done ; \
	p=`sed -n -e '/^"Language-Team:/p; 45q' < "$(FILE)"`; test -n "$$c" || \
	for f in $(POFILES) ; do \
	  fp=`sed -n -e '/^"Language-Team:/p; 45q' < "$$f"`; \
	  test -n "$$p" -a "$$p" == "$$fp" && \
	  { c="$$f" ; r="(Language-Team fields match)"; break ; } ; done ; \
	p=`sed -n -e '/^"Language:/p; 45q' < "$(FILE)"`; test -n "$$c" || \
	for f in $(POFILES) ; do \
	  fp=`sed -n -e '/^"Language:/p; 45q' < "$$f"`; \
	  test -n "$$p" -a "$$p" == "$$fp" && \
	  { c="$$f" ; r="(Language fields match)"; break ; } ; done ; \
	if test -z "$$c" ; then \
	  $(call shdie, "$@: failed to find po merging candidate; try renaming FILE") ; \
	else \
	  echo "Target for merging: $$c $$r" ; \
	  $(MSGMERGE) -F -C "$$c" -o "$$c" "$(FILE)" $(srcdir)/messages.po || exit $? ; \
	  $(GMSGFMT) -o /dev/null --verbose "$$c" ; \
	fi

SED_RM_CREATIONDATE = sed -E '1,30s/^"POT-Creation-Date: 2[0-9]{3}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9:+-]+\\n"/""/'

# force regeneration of all po files
update-po: Makefile
	@rm -f $(srcdir)/messages.po # force rebuild
	@$(MAKE) --no-print-directory $(srcdir)/messages.po
	@echo "  MERGE   " "messages.po into po files..."
	@for i in $(POFILES) ; do \
	  printf "    %12s: " "$$i" ; \
	  $(MSGMERGE) -qsU --backup=none $(srcdir)/$$i $(srcdir)/messages.po && \
	  $(SED_RM_CREATIONDATE) -i $(srcdir)/$$i || exit ; \
	  $(GMSGFMT) -o /dev/null --verbose $(srcdir)/$$i ; \
	done
.PHONY: update-po
check-update-po: Makefile $(srcdir)/messages.po
	$(Q) rm -f messages.po # force rebuild
	$(Q) $(MAKE) --no-print-directory messages.po && mv messages.po check-messages.po
	@echo -n "Checking for updated po files: "
	$(Q) for i in $(POFILES) ; do \
	  echo -n '.' ; \
	  cp $(srcdir)/$$i xtmp-$$i && \
	  $(MSGMERGE) -qsU --backup=none xtmp-$$i check-messages.po || exit ; \
	  $(SED_RM_CREATIONDATE) -i xtmp-$$i || exit ; \
	  cmp -s $(srcdir)/$$i xtmp-$$i || \
	  { echo ; echo "$$i: error: message updates missing, run 'update-po' first" >&2 ; exit 1 ; } ; \
	  rm -f xtmp-$$i ; \
	done ; echo
	$(Q) rm -f check-messages.po
.PHONY: check-update-po
CLEANFILES += check-messages.po

POTSCAN_MATCH = '(\.cc|\.hh|\.idl|\.xml|\.vue|\.js|\.html|\.(desktop|xml|keys)\.in)$$'
POTSCAN_SKIP  = '^tests/|/tests/|^aidacc/|^plugins/evaluator/|^yapps2_deb/|^drivers/bse-portaudio/'

noinst_DATA += poscan.list
EXTRA_DIST  += poscan.list	# needed for tarball builds
poscan.list: $(GITSTAMP)
	$(AM_V_GEN)
	$(Q) git ls-tree --name-only --full-tree -r HEAD > $@.tmp1
	$(Q) grep -E $(POTSCAN_MATCH) $@.tmp1 > $@.tmp2
	$(Q) grep -vE $(POTSCAN_SKIP) $@.tmp2 > $@.tmp3
	$(Q) mv $@.tmp3 $@ && rm -f $@.tmp1 $@.tmp2
INGIT_CLEANFILES += @INGIT@ poscan.list

# (split_lines, INPUTFILE, OUTPUTFILE, pattern)
split_lines = test -e $(1) -a -w . && { \
	grep  -Ee $(3) < $(1) > $(2) ; \
	grep -vEe $(3) < $(1) > $(1)-$$$$ ; \
	mv $(1)-$$$$ $(1) ; \
}

XGETTEXT_TMPDIR = $(XGETTEXT) "--package-name=$(PACKAGE)" "--package-version=$(VERSION)" -F -c -n \
		  --foreign-user --from-code=UTF-8 -D $(TMPDIR)/ -D $(abs_top_srcdir) \
		  $(patsubst %, --keyword=%, $(KEYWORDS)) $(patsubst %, --flag=%,    $(TEXTFLAGS))

$(srcdir)/messages.po: poscan.list
	$(AM_V_GEN)
	$(Q) rm -f potscan.all potscan.tmp && rm -rf $(TMPDIR)/
	$(Q) cp poscan.list potscan.all && mkdir -p $(TMPDIR)/
	$(Q) # process .idl files as C++
	$(Q) $(call split_lines, potscan.all, potscan.tmp, '\.idl$$')
	$(Q) $(XGETTEXT_TMPDIR) -f potscan.tmp -o $@ --force-po -L C++
	$(Q) sed -r '1,20 s/^("Content-Type:.*)CHARSET/\1UTF-8/' -i $@
	$(Q) # extract .xml and .html files and parse as JS
	$(Q) $(call split_lines, potscan.all, potscan.tmp, '\.(xml|html)(.in)?$$')
	$(Q)  while read file ; do \
	    d=`dirname "$$file"` && mkdir -p "$(TMPDIR)/$$d" || exit ; \
	    ./xt9n-xml.py $(top_srcdir)/"$$file" > "$(TMPDIR)/$$file" || exit ; \
	  done < potscan.tmp
	$(Q) $(XGETTEXT_TMPDIR) -f potscan.tmp -o $@ -j -L Javascript
	$(Q) # extract .keys and .desktop files and parse as C
	$(Q) $(call split_lines, potscan.all, potscan.tmp, '\.(keys|desktop)(\.in)?$$')
	$(Q)  while read file ; do \
	    d=`dirname "$$file"` && mkdir -p "$(TMPDIR)/$$d" || exit ; \
	    gawk -f ./xt9n-las.awk $(top_srcdir)/"$$file" > "$(TMPDIR)/$$file" || exit ; \
	  done < potscan.tmp
	$(Q) $(XGETTEXT_TMPDIR) -f potscan.tmp -o $@ -j -L C
	$(Q) # extract .vue files and parse as JS
	$(Q) $(call split_lines, potscan.all, potscan.tmp, '\.vue$$')
	$(Q)  while read file ; do \
	    d=`dirname "$$file"` && mkdir -p "$(TMPDIR)/$$d" || exit ; \
	    sed '1,/^<script\b/ s/.*// ; /^<\/script>/ s/.*//' $(top_srcdir)/"$$file" > "$(TMPDIR)/$$file" || exit ; \
	  done < potscan.tmp
	$(Q) $(XGETTEXT_TMPDIR) -f potscan.tmp -o $@ -j -L Javascript
	$(Q) # process remaining files with language detection
	$(Q) $(XGETTEXT_TMPDIR) -f potscan.all -o $@ -j
	$(Q) # cleanup
	$(Q) rm -rf $(TMPDIR)/ && rm -f potscan.all
CLEANFILES += $(TMPDIR)/

# clean INTLMERGE leftovers
CLEANFILES += intlmerge.tmpcache
