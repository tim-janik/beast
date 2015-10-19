# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = . data sfi bse plugins drivers shell beast-gtk launchers library tools po tests docs

EXTRA_DIST += HACKING.md
noinst_DATA = HACKING.html
CLEANFILES += HACKING.html

MAINTAINERCLEANFILES += configure
ACLOCAL_AMFLAGS  = -I autotools

%.html: %.md
	pandoc -s --toc -N -S -f markdown_github+pandoc_title_block -t html --email-obfuscation=javascript \
	  $< -o xgen-$(@F)
	mv xgen-$(@F) $@

site site-preview site-update site-update-all site-update-pages site-update-galleries:
	@$(MAKE) -C web/ $(AM_MAKEFLAGS) $@

# === doc/ files ===
maindocsdir   = $(pkgdocdir)/
maindocs_DATA = README.md NEWS
EXTRA_DIST   += $(maindocs_DATA)

# check: message
ALL_CHECKED_MSG = "All package tests passed"
check: check-recursive
	@echo $(ALL_CHECKED_MSG) | sed 's/./=/g'
	@echo $(ALL_CHECKED_MSG)
	@echo $(ALL_CHECKED_MSG) | sed 's/./=/g'

# == Dist Fixes ==
# Some files remain after distcheck, that we cannot clean up. So we role our own listfiles filter.
distuninstallcheck_listfiles  = find . -type f $(patsubst ./%, ! -path \*/%, $(uninstall_filter_files)) -print
#distuninstallcheck_listfiles = find . -type f -print	# original automake-1.14.1 setting
uninstall_filter_files = $(strip	\
	./share/mime/subclasses		./share/mime/XMLnamespaces	./share/mime/globs2	\
	./share/mime/version		./share/mime/icons		./share/mime/types	\
	./share/mime/treemagic		./share/mime/aliases		./share/mime/magic	\
	./share/mime/mime.cache		./share/mime/generic-icons	./share/mime/globs	\
)

# (test_option, INPUTSTRING, YESOPT, [NOOPT]) :- true if YESOPT is present
test_option = test `echo ":$(strip $(1)):" | sed 's/.*:$(strip $(2))://g' | wc -c` -lt \
      `echo ":$(strip $(1)):" | sed 's/.*:$(or $(strip $(3)), no-$(strip $(2)))://g' | wc -c`

# == ChangeLog & Release Rules ==
CHANGELOG_RANGE = $(shell git cat-file -e ce584d04999a7fb9393e1cfedde2048ba73e8878 && \
		    echo ce584d04999a7fb9393e1cfedde2048ba73e8878..HEAD || echo HEAD)
ChangeLog: $(GITSTAMPS)
	$(AM_V_GEN)
	$(Q) git log --pretty='^^%ad  %an 	# %h%n%n%B%n' --abbrev=11 \
		--date=short --first-parent $(CHANGELOG_RANGE)   > xgen-$(@F) # Generate ChangeLog with ^^-prefixed records
	$(Q) sed 's/^/	/; s/^	^^// ; s/[[:space:]]\+$$// '    -i xgen-$(@F) # Tab-indent commit bodies, kill trailing whitespaces
	$(Q) sed '/^\s*$$/{ N; /^\s*\n\s*$$/D }'                -i xgen-$(@F) # Compress multiple newlines
	$(Q) mv xgen-$(@F) $@
	$(Q) test -s $@ || { mv $@ $@.empty ; ls -al --full-time $@.empty ; exit 1 ; }
noinst_DATA          += ChangeLog
MAINTAINERCLEANFILES += ChangeLog
EXTRA_DIST           += ChangeLog
