# BEAST/BSE - Better Audio System / Better Sound Engine
include $(top_srcdir)/Makefile.decl

TAGFILES    = tagfile-rapicorn.xml
MANMANS	    = beast.1 bse.5 bsescm.1 bsewavetool.1 sfidl.1
HTMLMANS    = beast.1.html bse.5.html bsescm.1.html bsewavetool.1.html sfidl.1.html
EXTRA_DIST += $(TAGFILES) $(MANMANS)
# HTMLMANS are packaged as part of the doxygen docs

import: check-tools
	$(AM_V_GEN)
	$(Q) rm -f $(TAGFILES) $(HTMLMANS) $(MANMANS)
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory $(TAGFILES) $(HTMLMANS) $(MANMANS)
.PHONY: import

# = Check Tools =
CHECK_WGET = { command -v wget >/dev/null && { echo "1.10.0" ; wget --version 2>&1 | sed 's/[^0-9]*// ; 1q' ; } | sort -VC ; }
CHECK_WH2M = { command -v wikihtml2man >/dev/null && { echo "11.09.1" ; wikihtml2man --version 2>&1 | sed 's/\([^0-9]\|l2m\)*// ; 1q' ; } | sort -VC ; }
check-tools:
	$(Q) ${CHECK_WGET} || { echo "$@: failed to detect recent version: wget"; exit 1; }
	$(Q) ${CHECK_WH2M} || { echo "$@: failed to detect recent version: wikihtml2man"; exit 1; }

# = Tagfile Imports =
tagfile-rapicorn.xml:
	$(Q) wget --progress=dot --output-document=- http://dev.testbit.eu/rapicorn/latest/tagfile.xml > xtmp-${@F}
	$(Q) mv xtmp-${@F} $@

# = MANMANS =
%.1 %.5:
	$(AM_V_GEN)
	$(Q) wikihtml2man "http://testbit.eu/$(@F)?action=render" > xtmp-$(@F)
	$(Q) mv xtmp-$(@F) $@
	$(Q) rm -rf xtmp-$(@F)

# = HTMLMANS =
HEADER= $$'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">\n<html>\n' \
	$$'<head>\n' \
	$$'<title>'"$$TITLE"$$'</title>\n' \
	$$'<style type="text/css">\n' \
	$$'body { font-family: "DejaVu Sans", "Bitstream Vera Sans", Corbel, Verdana, "Verdana Ref", sans-serif; }\n' \
	$$'.tocnumber { display: none; }\n' \
	$$'body > p, body > dl, body > ul, body > ol { margin-left: 8em; }\n' \
	$$'</style></head>\n<body>\n' \
	$$'<a href="index.html">Return to Index</a><hr>'
FOOTER= $$'<hr>\n</body>\n</html>'
STEMTMP = xtmp-$(*F)
%.1.html %.5.html:
	$(AM_V_GEN)
	$(Q) rm -rf $(STEMTMP)
	$(Q) mkdir $(STEMTMP)
	$(Q) : # wget -k needs the final target name
	$(Q) cd $(STEMTMP) && wget -nv -k --no-cache "http://testbit.eu/$(@F:.html=)?action=render" -O $(@F)
	$(Q) TITLE="$(@F:.html=)" && echo ${HEADER} > $(STEMTMP)/header
	$(Q) echo ${FOOTER} > $(STEMTMP)/footer
	$(Q) cat $(STEMTMP)/header $(STEMTMP)/$(@F) $(STEMTMP)/footer > $(STEMTMP)/combined
	$(Q) mv $(STEMTMP)/combined $@
	$(Q) rm -rf $(STEMTMP)
