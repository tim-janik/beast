# Build and upload development documentation

all: build-stamp

top_srcdir  = ../..
git_stamp   = $(top_srcdir)/.git/$(shell git symbolic-ref -q HEAD)
SED_VERSION = $(shell sed -ne "/^[ \t]*VERSION[ \t]*=/ { s/^[^=]*=[ \t]*//; p; q }" < $(top_srcdir)/Makefile)
SED_MKEVEN  = sed 's/1$$/0/;s/3$$/2/;s/5$$/4/;s/7$$/6/;s/9$$/8/'

build-stamp: $(git_stamp) doxygen.cfg
	rm -rf html/
	cd $(top_srcdir) \
	&& (cat docs/dev/doxygen.cfg \
	&& echo "STRIP_FROM_PATH = `pwd`" \
	&& echo "OUTPUT_DIRECTORY = docs/dev/html/" \
	&& echo -n "PROJECT_NUMBER = $(SED_VERSION)" \
	) | nice doxygen -
	mv html/html/ html/$(SED_VERSION)/
	ln -s $(SED_VERSION) html/latest
	touch $@

upload: all
	rsync -aHP --del '--filter=P /*/' html/  dev.testbit.eu:/srv/dev/html/beast/

clean:
	rm -f build-stamp
	rm -rf html/
