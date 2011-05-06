# Build and upload development documentation

all: build-stamp

top_srcdir  = ../..
git_stamp   = $(top_srcdir)/.git/$(shell git symbolic-ref -q HEAD)
SED_VERSION = $(shell sed -ne "/^[ \t]*VERSION[ \t]*=/ { s/^[^=]*=[ \t]*//; p; q }" < $(top_srcdir)/Makefile)
SED_MKEVEN  = sed 's/1$$/0/;s/3$$/2/;s/5$$/4/;s/7$$/6/;s/9$$/8/'

DOC_DIRS   = birnet/ sfi/ bse/ plugins/ plugins/freeverb/ beast-gtk/ beast-gtk/gxk/ launchers/ shell/ tools/
DOCU_GLOBS = *.idl *.h *.H *.hh *.proc
DOXY_GLOBS = $(DOCU_GLOBS) *.c *.C *.cc
SRCTREE    = srctree

build-stamp: $(git_stamp) doxygen.cfg
	rm -rf html/ $(SRCTREE)/
	mkdir -p html/ $(SRCTREE)/
	@echo "### Copying Source..."
	CWD=`pwd` \
	&& for dir in $(DOC_DIRS) ; do \
	  (mkdir -p $$CWD/$(SRCTREE)/$$dir/ \
	   && cd $(top_srcdir)/$$dir \
	   && find . -maxdepth 1 -type f \( $(patsubst %,-name '%' -o, $(DOXY_GLOBS)) -false \) \
	      -exec cp {} $$CWD/$(SRCTREE)/$$dir/{} \; ) \
	  done
	@echo "### Identifying Documentation files..."
	cd $(SRCTREE)/ \
	&& find . -type f \( $(patsubst %,-name '%' -o, $(DOCU_GLOBS)) -false \) \
	   -exec sed  '1,+0s,^,/** @file {} */,' {} --in-place \;
	@echo "### Doxygen..."
	(cat doxygen.cfg \
	&& echo "STRIP_FROM_PATH  = `pwd`/$(SRCTREE)" \
	&& echo "INPUT		  = $(SRCTREE)/" \
	&& echo "OUTPUT_DIRECTORY = html/" \
	&& echo -n "PROJECT_NUMBER = $(SED_VERSION)" \
	) | nice doxygen -
	rm -rf $(SRCTREE)/
	mv html/html/ html/$(SED_VERSION)/
	ln -s $(SED_VERSION) html/latest
	touch $@

upload: all
	rsync -aHP --del '--filter=P /*/' html/  dev.testbit.eu:/srv/dev/html/beast/

clean:
	rm -f build-stamp
	rm -rf $(SRCTREE)/ html/
