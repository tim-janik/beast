# BEAST & BSE
include $(top_srcdir)/Makefile.decl
AUTOMAKE_OPTIONS = -Wno-portability

POSRC_TYPES =

# Verbosity macros
Qignore = $(if $(findstring 1, $(V)) ,, : )
Vignore = $(if $(findstring 1, $(V)) , : ,)

# Variables:
# ALL_LINGUAS : setup by configure.ac from po/LINGUAS
# PO_LINGUAS  : command line override for ALL_LINGUAS (subset)
# GMOFILES    : .gmo files for all TRANSLATIONS (GNU .mo)
# POFILES     : .po files for all TRANSLATIONS
# Files:
# POTSCAN     : list of project sources with translations
# POTSKIP     : list of translatable project sources to skip
# POTIGNORE   : ERE patterns of project sources to ignore
# LINGUAS     : list of supported languages
# unlisted    : (Makefile target) files missing from POTSCAN

# Translatable file types
#
POSRC_TYPES          += XML
POSRC_XML_PATTERN     = '\.xml\(.in\)\?$$'
POSRC_XML_FILTER      = gawk -f $(INTLFIXDIR)/intlfix-xml.awk
#
POSRC_TYPES          += SCM
POSRC_SCM_PATTERN     = '\.scm\(.in\)\?$$'
POSRC_SCM_FILTER      = sed -f $(INTLFIXDIR)/intlfix-scm.sed
#
POSRC_TYPES          += KEYS
POSRC_KEYS_PATTERN    = '\.keys\(.in\)\?$$'
POSRC_KEYS_FILTER     = gawk -f $(INTLFIXDIR)/intlfix-las.awk
#
POSRC_TYPES          += DESKTOP
POSRC_DESKTOP_PATTERN = '\.desktop\(.in\)\?$$'
POSRC_DESKTOP_FILTER  = gawk -f $(INTLFIXDIR)/intlfix-las.awk
#
POSRC_TYPES          += IDL
POSRC_IDL_PATTERN     = '\.idl\(.in\)\?$$'
POSRC_IDL_FILTER      =
#
POSRC_TYPES          += PROC
POSRC_PROC_NAME	      = Procedure Files
POSRC_PROC_PATTERN    = '\.proc\(.in\)\?$$'
POSRC_PROC_FILTER     =
#
POSRC_TYPES          += CCH
POSRC_CCH_NAME	      = C/C++ Files
POSRC_CCH_PATTERN     = '\.[hcHC][hcHC]\?\(.in\)\?$$'
POSRC_CCH_FILTER      =

TRANSLATIONS = $(if ${PO_LINGUAS}, ${PO_LINGUAS}, ${ALL_LINGUAS})
GMOFILES     = $(patsubst %, %.gmo, $(TRANSLATIONS))
POFILES      = $(patsubst %, %.po,  $(TRANSLATIONS))
INTLFIXDIR   = $(top_builddir)/$(ac_aux_dir)
#EXTRA_DIST += LINGUAS POTIGNORE potscan.list POTSKIP $(POFILES) messages.pot
#EXTRA_DIST += $(INTLFIXDIR)/intltool-merge  $(INTLFIXDIR)/intlfix-scm.sed
#EXTRA_DIST += $(INTLFIXDIR)/intlfix-xml.awk $(INTLFIXDIR)/intlfix-las.awk
CLEANFILES  += $(GMOFILES)
KEYWORDS     = _ N_ U_ Q_ _:1,2,3t
TEXTFLAGS    = _:1:pass-c-format
POTLIST      = $(abs_builddir)/POTLIST
TMPLIST      = $(abs_builddir)/POTLIST.tmp
TMPDIR       = tmp-cpots
I18NPATTERN  = '\b[NUQ]\?_[[:space:]]*([[:space:]]*"'
INLTFIXPATTERN = '\b__INTLFIX__\b.*'$(I18NPATTERN)
LOCALEDIR    = $(beastlocaledir)
NEWLINE	    := $(shell echo)

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

# check for translatable files missing from potscan.list
check-local: check-unlisted
check-unlisted:
	@echo "Check top source directory for missing translations..."
	@$(MAKE) unlisted
	@! test -e unlisted
.PHONY: check-unlisted

# check SCM i18n string extraction
check-local: check-scm-strings
.PHONY: check-scm-strings
check-scm-strings:
	@echo "Check po files missing translation strings..."
	@for lang in $(TRANSLATIONS); do	\
	  file=$(srcdir)/$$lang.po ;		\
	  echo $(ECHO_N) "  $$lang.po: " ;	\
	  for pattern in $(CHECK_SCM_PATS) ; do	\
	    grep -Fq $$pattern $$file || {	\
	      echo "$$file: misses strings from: $$pattern" ; exit 1 ; } \
	  done ;				\
	  echo "OK" ;				\
	done
CHECK_SCM_PATS=$(strip					\
)

shdie = { echo "ERROR:" $(1) >&2 ; exit $(or $(2), 127) ; }

merge-po: $(srcdir)/messages.pot # FILE=...
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
	  $(MSGMERGE) -s -C "$$c" -o "$$c" "$(FILE)" $(srcdir)/messages.pot || exit $? ; \
	  $(GMSGFMT) -o /dev/null --verbose "$$c" ; \
	fi

# force regeneration of all po files
update-po: Makefile
	@rm -f $(srcdir)/messages.pot # force rebuild
	@$(MAKE) --no-print-directory $(srcdir)/messages.pot
	@echo "Merging messages.pot into po files..."
	@for i in $(POFILES) ; do \
	  printf "%12s: " "$$i" ; \
	  $(MSGMERGE) -qsU --backup=none $(srcdir)/$$i $(srcdir)/messages.pot || exit ; \
	  $(GMSGFMT) -o /dev/null --verbose $(srcdir)/$$i ; \
	done
.PHONY: update-po
check-update-po: Makefile $(srcdir)/messages.pot
	$(Q) rm -f check-messages.pot # force rebuild
	$(Q) $(MAKE) --no-print-directory check-messages.pot
	@echo -n "Checking for updated po files: "
	$(Q) for i in $(POFILES) ; do \
	  echo -n '.' ; \
	  cp $(srcdir)/$$i xtmp-$$i && $(MSGMERGE) -qsU --backup=none xtmp-$$i check-messages.pot || exit ; \
	  cmp -s $(srcdir)/$$i xtmp-$$i || \
	  { echo ; echo "$$i: error: message updates missing, run 'update-po' first" >&2 ; exit 1 ; } ; \
	  rm -f xtmp-$$i ; \
	done ; echo
	$(Q) rm -f check-messages.pot
.PHONY: check-update-po
CLEANFILES += check-messages.pot

CLEANFILES += $(POTLIST) $(TMPLIST) $(TMPDIR)/

POTSCAN_MATCH = '(\.cc|\.hh|\.idl|\.xml|\.vue|\.js|\.html|\.(desktop|xml|keys)\.in)$$'
POTSCAN_SKIP  = '(^|/)tests/|^aidacc/'

potscan.list: $(GITSTAMP)
	$(AM_V_GEN)
	$(Q) git ls-tree --name-only --full-tree -r HEAD > $@.tmp1
	$(Q) grep -E $(POTSCAN_MATCH) $@.tmp1 > $@.tmp2
	$(Q) grep -vE $(POTSCAN_SKIP) $@.tmp2 > $@.tmp3
	$(Q) mv $@.tmp3 $@ && rm -f $@.tmp1 $@.tmp2

$(srcdir)/messages.pot check-messages.pot: potscan.list
	@echo "Building $(@F)..."
	@rm -f $(POTLIST) $(TMPLIST) ; rm -rf $(TMPDIR)/
	@sed -e '/^#/d' -e 's/^\[[^:]*:[^]]*][[:space:]]*//' -e '/^[[:space:]]*$$/d' < potscan.list > $(POTLIST) \
	&& cp $(POTLIST) $(TMPLIST) && mkdir -p $(TMPDIR)/
	@$(foreach TYPE, $(POSRC_TYPES), \
	  $(call prepare_pot_source, $(TYPE), $(TMPLIST)))
	@test `wc -l < $(TMPLIST)` -eq 0 || { \
	  echo "WARNING: unknown potscan.list entries:" ; \
	  sed 's/^/  /' $(TMPLIST) ; \
	} >&2
	@echo "  Extracting translatable strings with xgettext..."
	@$(XGETTEXT) -F -c -n -o $(abs_srcdir)/$(@F) --foreign-user \
	  --language=C++ --from-code=UTF-8 \
	  --files-from=$(POTLIST) \
	  -D $(TMPDIR)/ -D $(abs_top_srcdir) \
	  $(patsubst %, --keyword=%, $(KEYWORDS)) \
	  $(patsubst %, --flag=%,    $(TEXTFLAGS)) \
	  "--package-name=$(PACKAGE)" "--package-version=$(VERSION)"
	@rm -f $(POTLIST) $(TMPLIST) ; rm -rf $(TMPDIR)/
# (split_lines, INPUT, OUTPUT, pattern)
split_lines = test -e $(1) -a -w . && { \
	grep  -e $(3) < $(1) > $(2) ; \
	grep -ve $(3) < $(1) > $(1)-$$$$ ; \
	mv $(1)-$$$$ $(1) ; \
}
# (prepare_pot_source, POSRC_TYPE, INFILE)
define prepare_pot_source
	DOING=$(if $(POSRC_$(strip $(1))_FILTER),"Preparing","Listing") ; \
	printf "  %-32s" "$$DOING $(strip $(or $(POSRC_$(strip $(1))_NAME), $(1) Files)): " ; \
	$(call split_lines, $(2), "potlist$$$$.tmp", $(POSRC_$(strip $(1))_PATTERN)) && \
	wc -l < "potlist$$$$.tmp" || exit ; \
	$(if $(POSRC_$(strip $(1))_FILTER), \
	  while read file ; do \
	    d=`dirname "$$file"` && mkdir -p "$(TMPDIR)/$$d" || exit ; \
	    $(POSRC_$(strip $(1))_FILTER) $(top_srcdir)/"$$file" > "$(TMPDIR)/$$file" || exit ; \
	  done < "potlist$$$$.tmp" \
	, \
	  : \
	) ; rm -f "potlist$$$$.tmp"
	$(NEWLINE)
endef # GNU Make needs an explicit last newline

unlisted:
	@echo "Creating source file list..."
	@rm -f unlisted
	@ # list possible POT sources...
	$(Q) (cd $(top_srcdir) && set -o pipefail \
	&& find . $(FIND_NOHIDDEN) -type f $(patsubst %, ! -path \*/%, $(IGNORE_UNLISTED)) -print) \
	| sed 's,^\./,,' \
	| grep -vxF -f $(srcdir)/potscan.list -f $(srcdir)/POTSKIP \
	| grep -vEf $(srcdir)/POTIGNORE -e '^_(inst|build)/' -e $(DISTDIRPAT) > "xgen-$(@F)"
	@ # scanning file types...
	$(foreach TYPE, $(POSRC_TYPES), \
	  $(Q) $(call test_unlisted, $(TYPE), "xgen-$(@F)", "unlisted"))
	@rm -f "xgen-$(@F)"
	@ # report...
	@test `wc -l <unlisted` -gt 0 || rm -f unlisted
	@if test -e unlisted ; then \
	  echo "Missing `wc -l <unlisted` untranslated file(s) from potscan.list, see:"; head -n7 -v "`pwd`/unlisted" ; \
	else echo "Done, all files listed." ; fi
DISTDIRPAT = '^$(PACKAGE)-$(VERSION)/' # ignore $(distdir)/*
FIND_NOHIDDEN = \( -name '.?*' -prune -false -o ! -name '.?*' \) # skip .foobar dirs & files
IGNORE_UNLISTED = $(strip		\
	docs/doxy/*			\
	tmp*				\
)
# (test_unlisted, POSRC_TYPE, INFILE, OUTFILE)
# use 'readlink -ve' to catch missing files
define test_unlisted
	printf "  %-50s" "Scanning for unlisted $(strip $(or $(POSRC_$(strip $(1))_NAME), $(1) Files)): " && \
	set -o pipefail && \
	readlink -ve $(2) > /dev/null && \
	{ grep $(POSRC_$(strip $(1))_PATTERN) $(2) || :; } | \
	while read file ; do \
	  readlink -ve "$(top_srcdir)/$$file" > /dev/null || exit $? ; \
	  $(if $(POSRC_$(strip $(1))_FILTER), \
	    $(POSRC_$(strip $(1))_FILTER) "$(top_srcdir)/$$file" | \
	    { grep -q $(INLTFIXPATTERN) && echo "$$file" || :; } \
	    , \
	    { grep -q $(I18NPATTERN) "$(top_srcdir)/$$file" && echo "$$file" || :; } \
	  ); \
	done \
	| tee -a $(3) | wc -l
	$(NEWLINE)
endef # GNU Make needs an explicit last newline
.PHONY: unlisted
CLEANFILES += unlisted

# clean INTLMERGE leftovers
CLEANFILES += intlmerge.tmpcache
