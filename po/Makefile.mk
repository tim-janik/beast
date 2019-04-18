# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/po/*.d)
CLEANDIRS += $(wildcard $>/po/ $>/locale/)

# == po/ files ==
# Explanations:
# po/LINGUAS	: list of supported languages
# po/MOPATHS	: locale/... relative message catalogs
# po/MOTARGETS	: builddir relative message catalogs
# POFILES	: .po files for all LINGUAS
po/LINGUAS	!= grep -v '^\#' po/LINGUAS | xargs echo -n
po/MOPATHS	 = $(patsubst %, locale/%/LC_MESSAGES/$(BSE_GETTEXT_DOMAIN).mo, $(po/LINGUAS))
po/MOTARGETS	 = $(addprefix $>/, $(po/MOPATHS))
po/POFILES	 = $(patsubst %, po/%.po, $(po/LINGUAS))
ALL_TARGETS	+= $(po/MOTARGETS)
# xgettext helpers
po/KEYWORDS	 = _ N_ U_ Q_ _:1,2,3t
po/TEXTFLAGS	 = _:1:pass-c-format
po/SED_FIXHEAD   = sed -E '1,30s/^"POT-Creation-Date: 2[0-9]{3}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9:+-]+\\n"/""/'
# intltool-merge halper with cache generation
INTLTOOL_MERGE       ::= po/intltool-merge
INTLTOOL_MERGE_CACHE ::= $>/po/intltool-merge.cache

# == intltool-merge.cache ==
$(INTLTOOL_MERGE_CACHE): $(wildcard po/*.po)				| $>/po/
	$(QGEN)
	$Q LC_ALL=C $(INTLTOOL_MERGE) -u -d -q -c $@ po/ /dev/null /dev/null

# == message catalogs ==
# Compile message catalog to binary format, using installation path.
$>/locale/%/LC_MESSAGES/$(BSE_GETTEXT_DOMAIN).mo: po/%.po		| $>/locale/%/LC_MESSAGES/
	$Q STATS=$$($(MSGFMT) --statistics -o $@.tmp $< 2>&1) \
	  && printf '  %-9s %s\n' '$*:' "$$STATS" || { echo "$$STATS" ; false ; }
	$Q mv $@.tmp $@

# == installation ==
po/install: $(po/MOTARGETS) FORCE
	$(QECHO) INSTALL '$(DESTDIR)$(pkglibdir)/locale/...'
	$Q cd . \
	  $(foreach P, $(po/MOPATHS), \
	    && $(INSTALL_DATA) '$>/$P' -D '$(DESTDIR)$(pkglibdir)/$P')
install: po/install
po/uninstall: FORCE
	$(QECHO) REMOVE '$(DESTDIR)$(pkglibdir)/locale/...'
	$Q rm -f \
	  $(foreach P, $(po/MOPATHS), \
	    '$(DESTDIR)$(pkglibdir)/$P')
uninstall: po/uninstall

# == update-po ==
# Update the list of translatable strings in all srcdir po files.
update-po: $>/po/messages.po FORCE
	$(QGEN)
	$(QECHO) 'MERGE' "messages.po into po files in srcdir..."
	$Q for i in $(po/POFILES) ; do \
	  printf "    %12s: " "$$i" ; \
	  $(MSGMERGE) -s -qU --backup=none $$i $>/po/messages.po && \
	  $(po/SED_FIXHEAD) -i $$i || exit ; \
	  $(MSGFMT) -o /dev/null --verbose $$i ; \
	done

# == merge-po ==
# Merge the translations from $(PO) into the corresponding srcdir po file.
merge-po: po/merge.po = po/$(notdir $(PO))
merge-po: $>/po/messages.po FORCE	# PO=...
	$(QECHO) MERGE $(PO) into $(po/merge.po)
	$Q test -n '$(PO)'	|| { echo "$@: usage: make $@ PO=lang.po" >&2 ; false ; }
	$Q for f in '$(PO)' '$(po/merge.po)' ; do \
	    test -e "$$f"	|| { echo "$@: no such file: $$f" >&2 ; exit 1 ; } ; done
	$Q $(MSGMERGE) -s -q -C '$(po/merge.po)' -o '$(po/merge.po)' '$(PO)' $>/po/messages.po \
	  && $(po/SED_FIXHEAD) -i '$(po/merge.po)' \
	  && printf "    %12s: " '$(po/merge.po)' \
	  && $(MSGFMT) -o /dev/null --verbose '$(po/merge.po)'

# == poscan.list ==
# List files with translatable strings, tracked in Git.
po/scan_match = '(\.cc|\.hh|\.idl|\.xml|\.vue|\.js|\.html|\.(desktop|xml|keys)\.in)$$'
po/scan_skip  = '^external/|^tests/|/tests/|^aidacc/|^plugins/evaluator/|^yapps2_deb/|^drivers/bse-portaudio/'
$>/po/poscan.list: $(GITCOMMITDEPS)	| $>/po/
	$(QGEN)
	$Q git ls-tree --name-only --full-tree -r HEAD > $@.tmp1
	$Q grep -E $(po/scan_match) $@.tmp1 > $@.tmp2
	$Q grep -vE $(po/scan_skip) $@.tmp2 > $@.tmp3
	$Q rm -f $@.tmp1 $@.tmp2
	$Q mv $@.tmp3 $@

# == move_lines ==
# $(call po/move_lines, INPUTFILE, OUTPUTFILE, PATTERN)
# Move lines matching PATTERN from INPUTFILE to OUTPUTFILE.
po/move_lines = test -e $1 -a -w $(dir $2) && { \
	grep  -Ee $3 < $1 > $2 ; \
	grep -vEe $3 < $1 > $1-$$$$ ; \
	mv $1-$$$$ $1 ; \
}

# == xgettext_tmpdir ==
# Use xgettext for translatable string extraction, include pre-transformed
# files from $(po/tmpdir) in source file search.
po/tmpdir          = $>/po/alt-pofiles
po/xgettext_tmpdir = $(XGETTEXT) --package-name="beast" --package-version="$(VERSION_M.M.M)" \
			-F -c -n --foreign-user --from-code=UTF-8 -D $(po/tmpdir)/ -D . \
			$(patsubst %, --keyword=%, $(po/KEYWORDS)) $(patsubst %, --flag=%, $(po/TEXTFLAGS))

# == messages.po ==
# Scrape translatable strings from listed source files, some files need to be
# pre-transformed to be gettext digestible.
$>/po/messages.po: $>/po/poscan.list po/xt9n-xml.py po/xt9n-las.awk
	$(QGEN)
	$Q cp $>/po/poscan.list $>/po/poscan.all && rm -f -r $(po/tmpdir)/ && mkdir -p $(po/tmpdir)/
	$Q # process .idl files as C++
	$Q $(call po/move_lines, $>/po/poscan.all, $>/po/poscan.tmp, '\.idl$$')
	$Q $(po/xgettext_tmpdir) -f $>/po/poscan.tmp -o $@ --force-po -L C++
	$Q sed -r '1,20 s/^("Content-Type:.*)CHARSET/\1UTF-8/' -i $@
	$Q # extract .xml and .html files and parse as JS
	$Q $(call po/move_lines, $>/po/poscan.all, $>/po/poscan.tmp, '\.(xml|html)(.in)?$$')
	$Q  while read file ; do \
	    d=`dirname "$$file"` && mkdir -p "$(po/tmpdir)/$$d" || exit ; \
	    po/xt9n-xml.py "$$file" > "$(po/tmpdir)/$$file" || exit ; \
	  done < $>/po/poscan.tmp
	$Q $(po/xgettext_tmpdir) -f $>/po/poscan.tmp -o $@ -j -L Javascript
	$Q # extract .keys and .desktop files and parse as C
	$Q $(call po/move_lines, $>/po/poscan.all, $>/po/poscan.tmp, '\.(keys|desktop)(\.in)?$$')
	$Q  while read file ; do \
	    d=`dirname "$$file"` && mkdir -p "$(po/tmpdir)/$$d" || exit ; \
	    gawk -f po/xt9n-las.awk "$$file" > "$(po/tmpdir)/$$file" || exit ; \
	  done < $>/po/poscan.tmp
	$Q $(po/xgettext_tmpdir) -f $>/po/poscan.tmp -o $@ -j -L C
	$Q # extract .vue files and parse as JS
	$Q $(call po/move_lines, $>/po/poscan.all, $>/po/poscan.tmp, '\.vue$$')
	$Q  while read file ; do \
	    d=`dirname "$$file"` && mkdir -p "$(po/tmpdir)/$$d" || exit ; \
	    sed '1,/^<script\b/ s/.*// ; /^<\/script>/ s/.*//' "$$file" > "$(po/tmpdir)/$$file" || exit ; \
	  done < $>/po/poscan.tmp
	$Q $(po/xgettext_tmpdir) -f $>/po/poscan.tmp -o $@ -j -L Javascript
	$Q # process remaining files with automatic language detection
	$Q $(po/xgettext_tmpdir) -f $>/po/poscan.all -o $@ -j
	$Q # cleanup
	$Q rm -rf $(po/tmpdir)/ && rm -f $>/po/poscan.all $>/po/poscan.tmp
