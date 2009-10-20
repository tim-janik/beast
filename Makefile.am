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

# === ChangeLog ===
LAST_COMMITID = ce584d04999a7fb9393e1cfedde2048ba73e8878
ChangeLog:	$(shell ls "$${GIT_DIR:-.git}/packed-refs" "$${GIT_DIR:-.git}/HEAD" "$${GIT_DIR:-.git}/`git symbolic-ref -q HEAD || echo HEAD`" 2>/dev/null )
	: # Generate ChangeLog with -prefixed records
	git log --pretty='format:%ad %an 	# %H (%cn)%n%n%s%n%n%b' \
		${LAST_COMMITID}..HEAD		 > xgen-$(@F)
	: # Tab-indent ChangeLog, except for record start
	sed 's/^/	/;s/^	//;/^[ 	]*<unknown>$$/d' -i xgen-$(@F)
	: # Kill trailing whitespaces, compress multiple trailing newlines
	$(PYTHON) -c "import sys, re; sys.stdout.write ( \
	  re.compile (r'\n\n\n(?=^\w)', re.M).sub ('\n\n', \
	  re.compile ('^[	 ]+$$', re.M).sub ('', \
	  sys.stdin.read())))" < xgen-$(@F) > xgen2-$(@F)
	cp xgen2-$(@F) $@ && rm -f xgen-$(@F) xgen2-$(@F)
noinst_DATA += ChangeLog
EXTRA_DIST  += ChangeLog

# fixup automake-1.9.6 distuninstallcheck:-rule reporting files created by update-mime-database(1)
filter_stale_uninstalled      = | egrep -v '^.*/share/mime/(subclasses|globs|magic|XMLnamespaces|aliases)$$'
filter_stale_buildfiles       = | egrep -v '^./report.out$$'
distuninstallcheck_listfiles  = find . -type f -print 		# automake-1.9 setting
distuninstallcheck_listfiles += $(filter_stale_uninstalled)	# amend by required filtering
distcleancheck_listfiles      = find . -type f -print		# automake-1.9 setting
distcleancheck_listfiles     += $(filter_stale_buildfiles)	# amend by required filtering
# fixup automake-1.9.6 distcheck:-rule, which attempts to rebuild a dist tarball from within the test-build
# also, bsescm currently needs install: before check:
distcheck: dist
	case '$(DIST_ARCHIVES)' in \
	*.tar.gz*) \
	  GZIP=$(GZIP_ENV) gunzip -c $(distdir).tar.gz | $(am__untar) ;;\
	*.tar.bz2*) \
	  bunzip2 -c $(distdir).tar.bz2 | $(am__untar) ;;\
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
	dc_install_base=`$(am__cd) $(distdir)/_inst && pwd | sed -e 's,^[^:\\/]:[\\/],/,'` \
	  && dc_destdir="$${TMPDIR-/tmp}/am-dc-$$$$/" \
	  && cd $(distdir)/_build \
	  && ../configure --srcdir=.. --prefix="$$dc_install_base" \
	    $(DISTCHECK_CONFIGURE_FLAGS) \
	  && $(MAKE) $(AM_MAKEFLAGS) \
	  && $(MAKE) $(AM_MAKEFLAGS) dvi \
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
	  && echo "DISABLED: $(MAKE) $(AM_MAKEFLAGS) dist" \
	  && rm -rf $(DIST_ARCHIVES) \
	  && $(MAKE) $(AM_MAKEFLAGS) distcleancheck
	$(am__remove_distdir)
	@(echo "$(distdir) archives ready for distribution: "; \
	  list='$(DIST_ARCHIVES)'; for i in $$list; do echo $$i; done) | \
	  sed -e '1{h;s/./=/g;p;x;}' -e '$${p;x;}'
# remove non-distributed po files from distributed POTFILES.in
dist-hook:
	sed 's,^\(drivers/\),# \1,' < $(distdir)/po/POTFILES.in > $(distdir)/po/POTFILES.tmp
	mv $(distdir)/po/POTFILES.tmp $(distdir)/po/POTFILES.in
