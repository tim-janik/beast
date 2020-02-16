# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/misc/*.d)
CLEANDIRS += $(wildcard $>/misc/)

# BEAST misc/ - CI/CD rules
#
# This Makefile diverts from the conventional misc/ prefixing,
# because it serves as an extension of the toplevel makefile
# rules to provides continuous integration/development rules.

# == cppcheck ==
cppcheck:								| $>/misc/cppcheck/
	$(QGEN)
	$Q export OUTDIR=$>/misc/ && set -x && misc/run-cppcheck.sh
	$Q mv $>/misc/cppcheck.err $>/misc/cppcheck/cppcheck.log
	misc/blame-lines -b $>/misc/cppcheck/cppcheck.log
.PHONY: cppcheck
# Note, 'cppcheck' can be carried out before the sources are built

# == hacks ==
listhacks:								| $>/misc/hacks/
	$(QGEN)
	misc/keywords.sh -g -l >$>/misc/hacks/hacks.log
	misc/blame-lines -b $>/misc/hacks/hacks.log
	misc/keywords.sh -c $>/misc/hacks/hacks.blame > $>/misc/hacks/hacks.vt
.PHONY: listhacks
# Note, 'listhacks' can be carried out before the sources are built

# == unused ==
listunused:								| $>/misc/unused/
	$(QGEN)
	$Q export OUTDIR=$>/misc/ && set -x && misc/run-cppcheck.sh -u
	$Q grep -E '\b(un)?(use|reach)' $>/misc/cppcheck.err >$>/misc/unused/unused.log
	$Q rm -f $>/misc/cppcheck.err
	misc/blame-lines -b $>/misc/unused/unused.log
.PHONY: listunused
# Note, 'listunused' requires a successfuly built source tree.

# == scan-build ==
scan-build:								| $>/misc/scan-build/
	$(QGEN)
	$Q rm -rf $>/misc/scan-tmp/ && mkdir -p $>/misc/scan-tmp/
	$Q echo "  CHECK   " "for CXX to resemble clang++"
	$Q $(CXX) --version | grep '\bclang\b'
	scan-build -o $>/misc/scan-tmp/ --use-cc "$(CC)" --use-c++ "$(CXX)" $(MAKE) -j`nproc`
	$Q shopt -s nullglob ; \
	      for r in $>/misc/scan-tmp/20??-??-??-*/report-*.html ; do \
		D=$$(sed -nr '/<!-- BUGDESC/ { s/^<!-- \w+ (.+) -->/\1/    ; p }' $$r) && \
		F=$$(sed -nr '/<!-- BUGFILE/ { s/^<!-- \w+ ([^ ]+) -->/\1/ ; p }' $$r) && \
		L=$$(sed -nr '/<!-- BUGLINE/ { s/^<!-- \w+ ([^ ]+) -->/\1/ ; p }' $$r) && \
		echo "$$F:$$L: $$D" | sed 's,^/usr/src/beast/,,' ; \
	      done > $>/misc/scan-build/scan-build.log
	$Q shopt -s nullglob ; \
	      for d in $>/misc/scan-tmp/20??-??-??-*/ ; do \
		rm -rf $>/misc/scan-build/html/ ; \
		mv -v "$$d" $>/misc/scan-build/html/ || exit 1 ; \
		break ; \
	      done
	$Q rm -rf $>/misc/scan-tmp/
	misc/blame-lines -b $>/misc/scan-build/scan-build.log
.PHONY: scan-build
# Note, 'make scan-build' requires 'configure CC=clang CXX=g++' to generate any reports.

# == clang-tidy ==
clang-tidy:								| $>/misc/clang-tidy/
	$(QGEN)
	$Q git ls-tree -r --name-only HEAD				> $>/misc/tmpls.all
	$Q egrep $(CLANG_TIDY_GLOB) < $>/misc/tmpls.all			> $>/misc/tmpls.cchh
	$Q egrep -vf misc/clang-tidy.ignore  $>/misc/tmpls.cchh		> $>/misc/tmpls.clangtidy
	clang-tidy `cat $>/misc/tmpls.clangtidy` -- \
	  -std=gnu++17 \
	  -I . \
	  -I $> \
	  -I sfi \
	  -I $>/plugins \
	  -I external/ \
	  -I ebeast/node_modules/node-gyp/cache/.node-gyp/iojs-*/src/ \
	  -I ebeast/node_modules/node-gyp/cache/.node-gyp/iojs-*/deps/v8/include/ \
	  -DBSE_COMPILATION \
	  -DGXK_COMPILATION \
	  -D__TOPDIR__=\"`pwd`\" \
	  `$(PKG_CONFIG) --cflags glib-2.0`				> $>/misc/clang-tidy/clang-tidy.raw
	$Q sed "s,^`pwd`/,," $>/misc/clang-tidy/clang-tidy.raw		> $>/misc/clang-tidy/clang-tidy.log
	$Q rm -f $>/misc/clang-tidy/clang-tidy.raw  $>/misc/tmpls.all  $>/misc/tmpls.cchh  $>/misc/tmpls.clangtidy
	misc/blame-lines -b $>/misc/clang-tidy/clang-tidy.log
CLANG_TIDY_GLOB := "^(aidacc|bse|plugins|drivers|ebeast|tools|launchers)/.*\.(cc|hh)$$"
.PHONY: clang-tidy
# Note, 'make clang-tidy' requires a successfuly built source tree.

# == appimage tools ==
$>/misc/appaux/appimagetool/AppRun:					| $>/misc/appaux/
	$(QGEN) # Fetch and extract AppImage tools
	$Q cd $>/misc/appaux/ && \
		curl -sfSOL https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage && \
		curl -sfSOL https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage && \
		chmod +x linuxdeploy-x86_64.AppImage appimagetool-x86_64.AppImage && \
		rm -rf squashfs-root linuxdeploy appimagetool && \
		./linuxdeploy-x86_64.AppImage  --appimage-extract && mv -v squashfs-root/ ./linuxdeploy && \
		./appimagetool-x86_64.AppImage --appimage-extract && mv -v squashfs-root/ ./appimagetool && \
		rm linuxdeploy-x86_64.AppImage appimagetool-x86_64.AppImage

# == appimage ==
APPDIR  = $>/appdir/
APPDIR2 = $>/appdir2/
appimage: all $>/misc/appaux/appimagetool/AppRun				| $>/misc/bin/
	$(QGEN)
	@$(eval distversion != ./version.sh -l)
	$Q echo "  CHECK   " "for AppImage build with prefix=/usr"
	$Q test '$(prefix)' = '/usr' || { echo "$@: assertion failed: prefix=$(prefix)" >&2 ; false ; }
	@: # AppDir installation
	@echo '  INSTALL ' AppImage files
	$Q rm -fr $(APPDIR) $(APPDIR2) && \
		make install DESTDIR=$(APPDIR) $(if $(findstring 1, $(V)), , >/dev/null)
	@: # Populate AppDir, linuxdeploy expects libraries under usr/lib, binaries under usr/bin, etc
	@: # We achieve that by treating the beast-$MAJOR-$MINOR installation prefix as /usr/.
	@: # Also, we hand-pick extra libs for ebeast to keep the AppImage small.
	$Q mkdir $(APPDIR2)
	$Q cp -a $(APPDIR)/usr/lib/beast-* $(APPDIR2)/usr
	$Q rm -f BEAST-x86_64.AppImage
	@echo '  RUN     ' linuxdeploy ...
	$Q if test -e /usr/lib64/libc_nonshared.a ; \
	   then LIB64=/usr/lib64/ ; \
	   else LIB64=/usr/lib/x86_64-linux-gnu/ ; fi \
	   && LD_LIBRARY_PATH=$(APPDIR2)/usr/lib/:$(APPDIR2)/usr/bundle/ $>/misc/appaux/linuxdeploy/AppRun \
		$(if $(findstring 1, $(V)), -v1, -v2) \
		--appdir=$(APPDIR2) \
		-l $$LIB64/libXss.so.1 \
		-l $$LIB64/libXtst.so.6 \
		-i $(APPDIR2)/usr/images/beast.png \
		-e $(APPDIR2)/usr/electron/ebeast \
		--custom-apprun=misc/AppRun
	@: # 'linuxdeploy -e usr/electron/ebeast' copies it to usr/bin/ebeast
	$Q rm -f $(APPDIR2)/usr/lib/libffmpeg.so \
	         $(APPDIR2)/usr/bin/ebeast	# remove bogus leftovers from linuxdeploy -e
	@: # Create AppImage executable
	@echo '  RUN     ' appimagetool ...
	$Q ARCH=x86_64 $>/misc/appaux/appimagetool/AppRun --comp=xz -n $(if $(findstring 1, $(V)), -v) $(APPDIR2)
	$Q mv BEAST-x86_64.AppImage $>/beast-$(distversion)-x64.AppImage
	$Q ls -l -h --color=auto $>/beast-*-x64.AppImage
.PHONY: appimage

# == bintray ==
BINTRAY_KEEPS   = 52
BINTRAY_KEYARGS = --skip -b .bintray_api_key
bintray:		# upload $>/misc/ contents to bintray
	@echo '  UPLOAD  ' 'https://bintray.com/beast-team/'
	@$(eval distversion != ./version.sh -l)
	@: # upload beast-*-x64.AppImage if it exists
	$Q test -x "$>/beast-$(distversion)-x64.AppImage" || exit 0 ; \
		misc/bintray.sh beast-team/testing/Beast-AppImage -k $(BINTRAY_KEEPS) -g -v $(distversion) $(BINTRAY_KEYARGS) \
		  -d $>/beast-$(distversion)-x64.AppImage
	@: # upload tarballs of existing log directories
	$Q for d in cppcheck scan-build clang-tidy hacks unused asan ; do \
		(set -- $>/misc/$$d/*.* ; test -r "$$1") || continue ; \
		tar cJf $>/misc/$$d-$(distversion).tar.xz -C $>/misc/ $$d/ && \
		misc/bintray.sh beast-team/testing/Reports -k $(BINTRAY_KEEPS) -g -v $(distversion) $(BINTRAY_KEYARGS) \
		  $>/misc/$$d-$(distversion).tar.xz || exit 1 ; \
	done
.PHONY: bintray
# Kill old versions: misc/bintray.sh beast-team/testing/Repository -k 0 $(BINTRAY_KEYARGS)

# == release-news ==
release-news:
	git log --first-parent --date=short --pretty='%s    # %cd %an %h%d%n%w(0,4,4)%b' --reverse HEAD "`./version.sh --last`^!" | \
	  sed -e '/^\s*Signed-off-by:.*<.*@.*>/d' -e '/^\s*$$/d'

# == release-increment ==
release-commits:
	$(QGEN)
	@ # Ensure master is on a *.0-alpha tag
	$(Q) test `git rev-parse --abbrev-ref HEAD` = master || { set -x; test `git rev-parse --abbrev-ref HEAD` = master ; }
	$(Q) LAST_TAG=`./version.sh --last` \
	&& EXPECTED_TAG=`git describe --match '[0-9]*.[0-9]*.0-alpha' --abbrev=0 --first-parent HEAD` \
	&& set -x \
	&& test "$$LAST_TAG" == "$$EXPECTED_TAG"
	@ # NEWS: turn alpha into release version, remove alpha-warning
	$(Q) RELEASE_DATE=`date +%Y-%m-%d` \
	&& $(NEWS_2R) < NEWS.md > NEWS.md.tmp \
	&& ! cmp -s NEWS.md NEWS.md.tmp || (echo 'NEWS: edit failed' >&2 ; false ) \
	&& mv NEWS.md.tmp NEWS.md \
	&& head -vn1 NEWS.md
	@ # Commit and tag release
	$(Q) RELEASE_TAG=`./version.sh --last | sed 's/-alpha$$//'` \
	&& set -x \
	&& git commit -s -m "Release $$RELEASE_TAG" NEWS.md \
	&& git tag -a "$$RELEASE_TAG" -m "`git log -1 --pretty=%s`"
	@ # NEWS: add alpha + warning, commit and tag
	$(Q) LAST_TAG=`./version.sh --last` \
	&& ALPHA_TAG=`python3 -c "l = '$$LAST_TAG'.split ('.'); l[1] = str (int (l[1]) + 1); print ('.'.join (l))"`-alpha \
	&& $(NEWS_2A) < NEWS.md > NEWS.md.tmp \
	&& ! cmp -s NEWS.md NEWS.md.tmp || (echo 'NEWS: edit failed' >&2 ; false ) \
        && mv NEWS.md.tmp NEWS.md \
        && head -vn1 NEWS.md \
	&& set -x \
	&& git commit -s -m "NEWS: now working on $$ALPHA_TAG" NEWS.md \
	&& git tag -a "$$ALPHA_TAG" -m "`git log -1 --pretty=%s`"
# NEWS EDIT for RELEASE
NEWS_2R_IN  = r'^(\#\# Beast [0-9.]+)-alpha:\n*\*Note:[^*]*\*\n*'
NEWS_2R_OUT = r'\\1:\t\t\t\t\t($$RELEASE_DATE)\n\n'
NEWS_2R     = python3 -c "import sys, re; sys.stdout.write (re.sub ($(NEWS_2R_IN), $(NEWS_2R_OUT), sys.stdin.read(), re.M))"
# NEWS EDIT for ALPHA
NEWS_2A_OUT = '\#\# Beast $$ALPHA_TAG:\n\n' + \
	      '*Note: This is the master branch of Beast, which contains alpha version software.\n' + \
	      'Beware of file format instabilities and report any bugs you find in it.*\n\n\n'
NEWS_2A     = python3 -c "import sys, re; sys.stdout.write (re.sub (r'^', $(NEWS_2A_OUT), sys.stdin.read(), re.M))"
