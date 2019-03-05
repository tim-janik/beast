# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/misc/*.d)
CLEANDIRS += $(wildcard $>/misc/)

# BEAST misc/ - CI/CD rules
#
# This Makefile diverts from the conventional misc/ prefixing,
# because it serves as an extension of the toplevel makefile
# rules to provides continuous integration/development rules.

# == cppcheck ==
cppcheck:
	$(QGEN)
	$Q mkdir -p $>/misc/cppcheck/
	$Q export OUTDIR=$>/misc/ && set -x && misc/run-cppcheck.sh
	$Q mv $>/misc/cppcheck.err $>/misc/cppcheck/cppcheck.log
	misc/blame-lines -b $>/misc/cppcheck/cppcheck.log
.PHONY: cppcheck
# Note, 'cppcheck' can be carried out before the sources are built

# == hacks ==
listhacks:
	$(QGEN)
	$Q mkdir -p $>/misc/hacks/
	misc/keywords.sh -g -l >$>/misc/hacks/hacks.log
	misc/blame-lines -b $>/misc/hacks/hacks.log
	misc/keywords.sh -c $>/misc/hacks/hacks.blame > $>/misc/hacks/hacks.vt
.PHONY: listhacks
# Note, 'listhacks' can be carried out before the sources are built

# == unused ==
listunused:
	$(QGEN)
	$Q mkdir -p $>/misc/unused/
	$Q export OUTDIR=$>/misc/ && set -x && misc/run-cppcheck.sh -u
	$Q grep -E '\b(un)?(use|reach)' $>/misc/cppcheck.err >$>/misc/unused/unused.log
	$Q rm -f $>/misc/cppcheck.err
	misc/blame-lines -b $>/misc/unused/unused.log
.PHONY: listunused
# Note, 'listunused' requires a successfuly built source tree.

# == scan-build ==
scan-build:
	$(QGEN)
	$Q rm -rf $>/misc/scan-tmp/
	$Q mkdir -p $>/misc/scan-build/ $>/misc/scan-tmp/
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
clang-tidy:
	$(QGEN)
	$Q mkdir -p $>/misc/clang-tidy/
	$Q git ls-tree -r --name-only HEAD >tmpls.all
	$Q egrep $(CLANG_TIDY_GLOB) <tmpls.all >tmpls.cchh
	$Q egrep -vf misc/clang-tidy.ignore tmpls.cchh >tmpls.clangtidy
	clang-tidy `cat tmpls.clangtidy` -- \
	  -std=gnu++14 \
	  -I . \
	  -I sfi \
	  -I beast-gtk \
	  -I external/v8pp/ \
	  -I ebeast/node_modules/node-gyp/cache/.node-gyp/iojs-*/src/ \
	  -I ebeast/node_modules/node-gyp/cache/.node-gyp/iojs-*/deps/v8/include/ \
	  -DBSE_COMPILATION \
	  -DGXK_COMPILATION \
	  -D__TOPDIR__=\"`pwd`\" \
	  `pkg-config --cflags libgnomecanvas-2.0` \
	  > $>/misc/clang-tidy/clang-tidy.raw
	$Q sed "s,^`pwd`/,," $>/misc/clang-tidy/clang-tidy.raw >$>/misc/clang-tidy/clang-tidy.log
	$Q rm -f $>/misc/clang-tidy/clang-tidy.raw tmpls.all tmpls.cchh tmpls.clangtidy
	misc/blame-lines -b $>/misc/clang-tidy/clang-tidy.log
CLANG_TIDY_GLOB := "^(aidacc|bse|plugins|drivers|beast-gtk|ebeast|tools|launchers)/.*\.(cc|hh)$$"
.PHONY: clang-tidy
# Note, 'make clang-tidy' requires a successfuly built source tree.

# == appimage tools ==
cached/appimagetool/AppRun:
	@: # Fetch and extract AppImage tools
	$Q mkdir -p cached/
	$Q cd cached/ && \
		curl -sfSOL https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage && \
		curl -sfSOL https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage && \
		chmod +x linuxdeploy-x86_64.AppImage appimagetool-x86_64.AppImage && \
		rm -rf squashfs-root linuxdeploy appimagetool && \
		./linuxdeploy-x86_64.AppImage  --appimage-extract && mv -v squashfs-root/ ./linuxdeploy && \
		./appimagetool-x86_64.AppImage --appimage-extract && mv -v squashfs-root/ ./appimagetool && \
		rm linuxdeploy-x86_64.AppImage appimagetool-x86_64.AppImage
CLEANDIRS += cached

# == appimage ==
APPDIR  = $(abs_top_srcdir)/appdir/
APPDIR2 = $(abs_top_srcdir)/appdir2/
appimage: all cached/appimagetool/AppRun
	$(QGEN)
	$Q mkdir -p $>/misc/bin/
	$Q echo "  CHECK   " "for AppImage build with --prefix=/usr"
	$Q grep '^prefix = /usr\b' Makefile
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
	$Q LD_LIBRARY_PATH=$(APPDIR2)/usr/lib/:$(APPDIR2)/usr/bundle/ cached/linuxdeploy/AppRun \
		$(if $(findstring 1, $(V)), -v1, -v2) \
		--appdir=$(APPDIR2) \
		-l /usr/lib/x86_64-linux-gnu/libXss.so.1 \
		-l /usr/lib/x86_64-linux-gnu/libgconf-2.so.4 \
		-l /usr/lib/x86_64-linux-gnu/libXtst.so.6 \
		-e $(APPDIR2)/usr/bin/beast-*.* \
		--custom-apprun=misc/AppRun
	@: # Create AppImage executable
	@echo '  RUN     ' appimagetool ...
	$Q ARCH=x86_64 cached/appimagetool/AppRun --comp=xz -n $(if $(findstring 1, $(V)), -v) $(APPDIR2)
	$Q rm -fr $(APPDIR) $(APPDIR2)
	$Q mv BEAST-x86_64.AppImage $>/misc/bin/beast-$(clean_version).x64.AppImage
	$Q ls -l -h --color=auto $>/misc/bin/beast-*.x64.AppImage
.PHONY: appimage
CLEANDIRS += $(APPDIR) $(APPDIR2)

# == bintray ==
BINTRAY_KEEPS            = 99
USE_BINTRAY_API_KEY_FILE = $(shell test -z "$$BINTRAY_API_KEY" && echo " -b .bintray_api_key")
bintray:		# upload $>/misc/ contents to bintray
	@echo '  UPLOAD  ' 'https://bintray.com/beast-team/'
	@: # upload beast-*.x64.AppImage if it exists
	$Q test -x "$>/misc/bin/beast-$(clean_version).x64.AppImage" || exit 0 ; \
		misc/bintray.sh beast-team/testing/Beast-AppImage -k $(BINTRAY_KEEPS) -g -v $(clean_version) $(USE_BINTRAY_API_KEY_FILE) \
		  -d $>/misc/bin/beast-$(clean_version).x64.AppImage
	@: # upload tarballs of existing log directories
	$Q for d in cppcheck scan-build clang-tidy hacks unused asan ; do \
		(set -- $>/misc/$$d/*.* ; test -r "$$1") || continue ; \
		tar cJf $>/misc/$$d-$(clean_version).tar.xz -C $>/misc/ $$d/ && \
		misc/bintray.sh beast-team/testing/Reports -k $(BINTRAY_KEEPS) -g -v $(clean_version) $(USE_BINTRAY_API_KEY_FILE) \
		  $>/misc/$$d-$(clean_version).tar.xz || exit 1 ; \
	done
.PHONY: bintray
# Kill old versions: misc/bintray.sh beast-team/testing/Repository -k 0 $(USE_BINTRAY_API_KEY_FILE)
