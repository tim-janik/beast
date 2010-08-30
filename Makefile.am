# BEAST/BSE - Bedevilled Audio System / Bedevilled Sound Engine
# Copyright (C) 1998-2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = . data birnet sfi bse plugins shell beast-gtk launchers library tools po tests doxer docs web

noinst_DATA =

# require automake 1.9
AUTOMAKE_OPTIONS = 1.9 dist-bzip2 no-dist-gzip

site site-preview site-update site-update-all site-update-pages site-update-galleries:
	@$(MAKE) -C web/ $(AM_MAKEFLAGS) $@

# extra dependencies
configure: birnet/acbirnet.m4 birnet/configure.inc

CLEANFILES += $(strip		\
	intltool-extract	\
	intltool-merge		\
	intltool-update		\
)

EXTRA_DIST += $(strip		\
	TODO			\
	COPYING.GPL		\
	COPYING.LGPL		\
	po-helper.sh.in		\
	topconfig.h		\
	intltool-extract.in     \
	intltool-merge.in       \
	intltool-update.in	\
)

# check: message
ALL_CHECKED_MSG = "All package tests passed"
check: check-recursive
	@echo $(ALL_CHECKED_MSG) | sed 's/./=/g'
	@echo $(ALL_CHECKED_MSG)
	@echo $(ALL_CHECKED_MSG) | sed 's/./=/g'

# --- ChangeLog ---
ChangeLog:	$(shell $(srcdir)/mkrelease.sh commit-stamps)
	$(srcdir)/mkrelease.sh ChangeLog \
	  -R ce584d04999a7fb9393e1cfedde2048ba73e8878..HEAD
noinst_DATA += ChangeLog
EXTRA_DIST  += ChangeLog mkrelease.sh

# --- release-news ---
release-news:
	$(srcdir)/mkrelease.sh news
.PHONY: release-news

# --- release-upload ---
release-upload: ChangeLog
	$(srcdir)/mkrelease.sh upload -U localhost:tmp -E configure.in:BSE_MICRO_VERSION
.PHONY: release-upload

# --- dist fixes ---
uninstall_filter = $(strip	\
	./share/mime/subclasses		./share/mime/icons	./share/mime/generic-icons \
	./share/mime/globs		./share/mime/types	./share/mime/XMLnamespaces \
	./share/mime/mime.cache		./share/mime/magic	./share/mime/globs2 \
	./share/mime/treemagic		./share/mime/aliases	\
)
#distuninstallcheck_listfiles = find . -type f -print	# original automake-1.11 setting
distuninstallcheck_listfiles  = find . -type f $(patsubst ./%, ! -path \*/%, $(uninstall_filter)) -print
clean_filter = $(strip		\
	./report.out		\
)
#distcleancheck_listfiles     = find . -type f -print	# original automake-1.11 setting
distcleancheck_listfiles      = find . -type f $(patsubst ./%, ! -path \*/%, $(clean_filter)) -print

# extend automake-1.11 distcheck rule to make report.
distcheck: dist
	case '$(DIST_ARCHIVES)' in \
	*.tar.gz*) \
	  GZIP=$(GZIP_ENV) gunzip -c $(distdir).tar.gz | $(am__untar) ;;\
	*.tar.bz2*) \
	  bunzip2 -c $(distdir).tar.bz2 | $(am__untar) ;;\
	*.tar.lzma*) \
	  unlzma -c $(distdir).tar.lzma | $(am__untar) ;;\
	*.tar.xz*) \
	  xz -dc $(distdir).tar.xz | $(am__untar) ;;\
	*.tar.Z*) \
	  uncompress -c $(distdir).tar.Z | $(am__untar) ;;\
	*.shar.gz*) \
	  GZIP=$(GZIP_ENV) gunzip -c $(distdir).shar.gz | unshar ;;\
	*.zip*) \
	  unzip $(distdir).zip ;;\
	esac
	chmod -R a-w $(distdir); chmod a+w $(distdir)
	mkdir $(distdir)/_build
	mkdir $(distdir)/_inst
	chmod a-w $(distdir)
	test -d $(distdir)/_build || exit 0; \
	dc_install_base=`$(am__cd) $(distdir)/_inst && pwd | sed -e 's,^[^:\\/]:[\\/],/,'` \
	  && dc_destdir="$${TMPDIR-/tmp}/am-dc-$$$$/" \
	  && am__cwd=`pwd` \
	  && $(am__cd) $(distdir)/_build \
	  && ../configure --srcdir=.. --prefix="$$dc_install_base" \
	    $(DISTCHECK_CONFIGURE_FLAGS) \
	  && $(MAKE) $(AM_MAKEFLAGS) \
	  && $(MAKE) $(AM_MAKEFLAGS) dvi \
	  && $(MAKE) $(AM_MAKEFLAGS) check \
	  && $(MAKE) $(AM_MAKEFLAGS) install \
	  && $(MAKE) $(AM_MAKEFLAGS) report \
	  && $(MAKE) $(AM_MAKEFLAGS) installcheck \
	  && $(MAKE) $(AM_MAKEFLAGS) uninstall \
	  && $(MAKE) $(AM_MAKEFLAGS) distuninstallcheck_dir="$$dc_install_base" \
	        distuninstallcheck \
	  && chmod -R a-w "$$dc_install_base" \
	  && ({ \
	       (cd ../.. && umask 077 && mkdir "$$dc_destdir") \
	       && $(MAKE) $(AM_MAKEFLAGS) DESTDIR="$$dc_destdir" install \
	       && $(MAKE) $(AM_MAKEFLAGS) DESTDIR="$$dc_destdir" uninstall \
	       && $(MAKE) $(AM_MAKEFLAGS) DESTDIR="$$dc_destdir" \
	            distuninstallcheck_dir="$$dc_destdir" distuninstallcheck; \
	      } || { rm -rf "$$dc_destdir"; exit 1; }) \
	  && rm -rf "$$dc_destdir" \
	  && $(MAKE) $(AM_MAKEFLAGS) dist \
	  && rm -rf $(DIST_ARCHIVES) \
	  && $(MAKE) $(AM_MAKEFLAGS) distcleancheck \
	  && cd "$$am__cwd" \
	  || exit 1
	$(am__remove_distdir)
	@(echo "$(distdir) archives ready for distribution: "; \
	  list='$(DIST_ARCHIVES)'; for i in $$list; do echo $$i; done) | \
	  sed -e 1h -e 1s/./=/g -e 1p -e 1x -e '$$p' -e '$$x'
# remove non-distributed po files from distributed POTFILES.in
dist-hook:
	sed 's,^\(drivers/\),# \1,' < $(distdir)/po/POTFILES.in > $(distdir)/po/POTFILES.tmp
	mv $(distdir)/po/POTFILES.tmp $(distdir)/po/POTFILES.in
