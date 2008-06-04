# BEAST - Bedevilled Audio System
# Copyright (C) 2007 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

IMPORT_TARGETS = $(strip			\
	beastdocs.css				\
	Beast-Quickstart			\
)
EXTRA_DIST += $(strip		\
	style-append.css	\
)
IMPORT_LINKS = 						\
	"http://testbit.eu/wiki/Beast-Quickstart"	\
	"http://testbit.eu/beastdocs.css"

CHROME_ARGS = --header "X-Theme: chrome=off style=beastdocs.css"

import-docs:
	rm -f ${IMPORT_TARGETS}
	for fi in ${IMPORT_LINKS} ; do echo " $$fi" ; done	\
	| wget ${CHROME_ARGS} --convert-links -i -
	cat ${srcdir}/style-append.css >> beastdocs.css
	touch stamp-import-docs
.PHONY: import-docs

dist-hook:
	find stamp-import-docs -mtime -4 | fgrep -q stamp \
	|| { echo "docs/imports/: Last import is older than 3 days, please run: make import-docs -C docs/imports/" >&2 ; exit 1 ; }
