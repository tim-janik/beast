# BEAST - Bedevilled Audio System
# Copyright (C) 2007 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.

IMPORT_TARGETS = $(strip			\
	beastdocs.css				\
	Beast-Quickstart			\
)
IMPORT_LINKS = 						\
	"http://testbit.eu/wiki/Beast-Quickstart"	\
	"http://testbit.eu/beastdocs.css"

CHROME_ARGS = --header "X-Theme: chrome=off style=beastdocs.css"

update:
	rm -f ${IMPORT_TARGETS}
	for fi in ${IMPORT_LINKS} ; do echo " $$fi" ; done	\
	| wget ${CHROME_ARGS} --convert-links -i -
