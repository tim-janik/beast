## Process this file with automake to generate Makefile.in
# Copyright (C) 2010 Tim Janik
#
# This work is provided "as is"; see: http://beast.gtk.org/mirror/LICENSE-AS-IS
include $(top_srcdir)/Makefile.decl
AUTOMAKE_OPTIONS = -Wno-portability

POSRC_TYPES =

# Variables:
# ALL_LINGUAS : setup by configure.ac from po/LINGUAS
# PO_LINGUAS  : command line override for ALL_LINGUAS (subset)
# GMOFILES    : .gmo files for all TRANSLATIONS (GNU .mo)
# POFILES     : .po files for all TRANSLATIONS
# DATADIRNAME : setup by GLIB_GNU_GETTEXT for installation
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
EXTRA_DIST  += POTIGNORE POTSCAN POTSKIP $(POFILES) messages.pot
EXTRA_DIST  += $(INTLFIXDIR)/intltool-merge  $(INTLFIXDIR)/intlfix-scm.sed
EXTRA_DIST  += $(INTLFIXDIR)/intlfix-xml.awk $(INTLFIXDIR)/intlfix-las.awk
CLEANFILES  += $(GMOFILES)
KEYWORDS     = _ N_ U_ Q_
TEXTFLAGS    = _:1:pass-c-format
POTLIST      = $(abs_builddir)/POTLIST
TMPLIST      = $(abs_builddir)/POTLIST.tmp
TMPDIR       = tmp-cpots
I18NPATTERN  = '\b[NUQ]\?_[[:space:]]*([[:space:]]*"'
INLTFIXPATTERN = '\b__INTLFIX__\b.*'$(I18NPATTERN)
LOCALEDIR    = $(prefix)/$(DATADIRNAME)/locale
NEWLINE	    := $(shell echo)

all: $(GMOFILES)

# compile translation catalogs
%.gmo: %.po
	@echo $(ECHO_N) "$(@F): " \
	&& $(GMSGFMT) --statistics --directory=$(srcdir) -o xgen-$(@F) $< \
	&& mv xgen-$(@F) $@

# install translation catalogs
install: install-data
install-data: $(GMOFILES)
	@echo "Installing localised message files..."
	@for lang in $(TRANSLATIONS); do \
	  $(mkinstalldirs) "$(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES/"; \
	  test ! -r "$$lang.gmo" || { \
	    printf "  %-12s -> %s\n" "$$lang.gmo" ".../$$lang/$(BST_GETTEXT_DOMAIN).mo" ; \
            $(INSTALL_DATA) "$$lang.gmo" \
	      "$(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES/$(BST_GETTEXT_DOMAIN).mo" || exit 1 ; \
	  } ; \
	done
uninstall:
	@echo $(ECHO_N) "Uninstalling message files..."
	@for lang in $(TRANSLATIONS); do \
	  test -n "$$c" && test "$$c" -le 6 || { c=0; printf "\n  "; }; c=$$(($$c + 1)) ; \
	  printf "%-10s" "$$lang" ; \
	  rm -f "$(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES/$(BST_GETTEXT_DOMAIN).mo" ; \
	done ; test "$$c" = 0 || echo

# check for translatable files missing from POTSCAN
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
	  file=$$lang.po ;			\
	  echo $(ECHO_N) "  $$lang.po: " ;	\
	  for pattern in $(CHECK_SCM_PATS) ; do	\
	    grep -Fq $$pattern $$file || {	\
	      echo "$$file: misses strings from: $$pattern" ; exit 1 ; } \
	  done ;				\
	  echo "OK" ;				\
	done
CHECK_SCM_PATS=$(strip					\
	library/scripts/mixer-splitup-by-track.scm	\
	library/scripts/modules2grid.scm		\
	library/scripts/part-harmonic-transposer.scm	\
	library/scripts/progressor-example.scm		\
	library/scripts/record-midi.scm			\
	library/scripts/song-parts-operations.scm	\
)


# force regeneration of all po files
update-po: Makefile
	@rm -f $(srcdir)/messages.pot # force rebuild
	@$(MAKE) --no-print-directory $(srcdir)/messages.pot
	@echo "Merging messages.pot into po files..."
	@for i in $(POFILES) ; do \
	  printf "%12s: " "$$i" ; \
	  $(MSGMERGE) -qsU --backup=none $(srcdir)/$$i $(srcdir)/messages.pot || exit 1 ; \
	  $(GMSGFMT) -o /dev/null --verbose $(srcdir)/$$i ; \
	done
.PHONY: update-po

$(srcdir)/messages.pot:
	@echo "Rebuilding $(@F)..."
	@rm -f $(POTLIST) $(TMPLIST) ; rm -rf $(TMPDIR)/
	@sed -e '/^#/d' -e 's/^\[[^:]*:[^]]*][[:space:]]*//' -e '/^[[:space:]]*$$/d' < POTSCAN > $(POTLIST) \
	&& cp $(POTLIST) $(TMPLIST) && mkdir -p $(TMPDIR)/
	@$(foreach TYPE, $(POSRC_TYPES), \
	  $(call prepare_pot_source, $(TYPE), $(TMPLIST)))
	@test `wc -l < $(TMPLIST)` -eq 0 || { \
	  echo "WARNING: unknown POTSCAN entries:" ; \
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
	DOING=$(if $(POSRC_$(strip $(1))_FILTER),"Preparing","Listing"); \
	printf "  %-32s" "$$DOING $(strip $(or $(POSRC_$(strip $(1))_NAME), $(1) Files)): " ; \
	$(call split_lines, $(2), "potlist$$$$.tmp", $(POSRC_$(strip $(1))_PATTERN)) ; \
	wc -l < "potlist$$$$.tmp" ; \
	$(if $(POSRC_$(strip $(1))_FILTER), \
	  cat "potlist$$$$.tmp" | \
	  while read file ; do \
	    d=`dirname "$$file"` ; mkdir -p "$(TMPDIR)/$$d" || exit $$? ; \
	    $(POSRC_$(strip $(1))_FILTER) $(top_srcdir)/"$$file" > "$(TMPDIR)/$$file" \
	    || exit $$? ; \
	  done \
	, \
	  : \
	) ; rm -f "potlist$$$$.tmp"
	$(NEWLINE)
endef # GNU Make needs an explicit last newline

unlisted:
	@echo "Creating source file list..."
	@rm -f unlisted
	@ # list possible POT sources...
	@(cd $(top_srcdir) && find . $(FIND_NOHIDDEN) -type f -print) | sed 's,^\./,,' \
	| grep -vxF -f $(srcdir)/POTSCAN -f $(srcdir)/POTSKIP \
	| grep -vEf $(srcdir)/POTIGNORE -e '^_(inst|build)/' -e $(DISTDIRPAT) > "xgen-$(@F)" ; :
	@ # scanning file types...
	@$(foreach TYPE, $(POSRC_TYPES), \
	  $(call test_unlisted, $(TYPE), "xgen-$(@F)", "unlisted"))
	@rm -f "xgen-$(@F)"
	@ # report...
	@test `wc -l <unlisted` -gt 0 || rm -f unlisted
	@if test -e unlisted ; then \
	  echo "Found `wc -l <unlisted` untranslated files, see:"; ls -l "`pwd`/unlisted" ; \
	else echo "Done, all files listed." ; fi
DISTDIRPAT = '^$(PACKAGE)-$(VERSION)/' # ignore $(distdir)/*
FIND_NOHIDDEN = \( -name '.?*' -prune -false -o ! -name '.?*' \) # skip .foobar dirs & files
# (test_unlisted, POSRC_TYPE, INFILE, OUTFILE)
define test_unlisted
	printf "  %-32s" "Scanning $(strip $(or $(POSRC_$(strip $(1))_NAME), $(1) Files)): " ; \
	grep $(POSRC_$(strip $(1))_PATTERN) $(2) | \
	  $(if $(POSRC_$(strip $(1))_FILTER), \
	    while read file ; do \
	       $(POSRC_$(strip $(1))_FILTER) $(top_srcdir)/"$$file" | \
	       grep -q $(INLTFIXPATTERN) && echo "$$file" ; \
	    done \
	  , \
	    (cd $(top_srcdir) && xargs grep -l $(I18NPATTERN) ) \
	  ) | tee -a $(3) | wc -l
	$(NEWLINE)
endef # GNU Make needs an explicit last newline
.PHONY: unlisted
CLEANFILES += unlisted

# clean INTLMERGE leftovers
CLEANFILES += intlmerge.tmpcache
