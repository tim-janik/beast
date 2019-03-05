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
	  -std=gnu++14 \
	  -I . \
	  -I $> \
	  -I sfi \
	  -I beast-gtk \
	  -I $>/beast-gtk \
	  -I $>/plugins \
	  -I external/v8pp/ \
	  -I ebeast/node_modules/node-gyp/cache/.node-gyp/iojs-*/src/ \
	  -I ebeast/node_modules/node-gyp/cache/.node-gyp/iojs-*/deps/v8/include/ \
	  -DBSE_COMPILATION \
	  -DGXK_COMPILATION \
	  -D__TOPDIR__=\"`pwd`\" \
	  `pkg-config --cflags libgnomecanvas-2.0`			> $>/misc/clang-tidy/clang-tidy.raw
	$Q sed "s,^`pwd`/,," $>/misc/clang-tidy/clang-tidy.raw		> $>/misc/clang-tidy/clang-tidy.log
	$Q rm -f $>/misc/clang-tidy/clang-tidy.raw  $>/misc/tmpls.all  $>/misc/tmpls.cchh  $>/misc/tmpls.clangtidy
	misc/blame-lines -b $>/misc/clang-tidy/clang-tidy.log
CLANG_TIDY_GLOB := "^(aidacc|bse|plugins|drivers|beast-gtk|ebeast|tools|launchers)/.*\.(cc|hh)$$"
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
appimage: VERSION_LONG != ./version.sh -l
appimage: all $>/misc/appaux/appimagetool/AppRun				| $>/misc/bin/
	$(QGEN)
	$Q echo "  CHECK   " "for AppImage build with --prefix=/usr"
	$Q test '$(prefix)' = '/usr' || { echo "prefix=$(prefix)" >&2 ; false ; }
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
	$Q LD_LIBRARY_PATH=$(APPDIR2)/usr/lib/:$(APPDIR2)/usr/bundle/ $>/misc/appaux/linuxdeploy/AppRun \
		$(if $(findstring 1, $(V)), -v1, -v2) \
		--appdir=$(APPDIR2) \
		-l /usr/lib/x86_64-linux-gnu/libXss.so.1 \
		-l /usr/lib/x86_64-linux-gnu/libgconf-2.so.4 \
		-l /usr/lib/x86_64-linux-gnu/libXtst.so.6 \
		-i $(APPDIR2)/usr/images/beast.png \
		-e $(APPDIR2)/usr/bin/beast \
		--custom-apprun=misc/AppRun
	@: # Create AppImage executable
	@echo '  RUN     ' appimagetool ...
	$Q ARCH=x86_64 $>/misc/appaux/appimagetool/AppRun --comp=xz -n $(if $(findstring 1, $(V)), -v) $(APPDIR2)
	$Q rm -fr $(APPDIR) $(APPDIR2)
	$Q mv BEAST-x86_64.AppImage $>/misc/bin/beast-$(VERSION_LONG)-x64.AppImage
	$Q ls -l -h --color=auto $>/misc/bin/beast-*-x64.AppImage
.PHONY: appimage

# == bintray ==
BINTRAY_KEEPS            = 99
USE_BINTRAY_API_KEY_FILE = $(shell test -z "$$BINTRAY_API_KEY" && echo " -b .bintray_api_key")
bintray: VERSION_LONG != ./version.sh -l
bintray:		# upload $>/misc/ contents to bintray
	@echo '  UPLOAD  ' 'https://bintray.com/beast-team/'
	@: # upload beast-*-x64.AppImage if it exists
	$Q test -x "$>/misc/bin/beast-$(VERSION_LONG)-x64.AppImage" || exit 0 ; \
		misc/bintray.sh beast-team/testing/Beast-AppImage -k $(BINTRAY_KEEPS) -g -v $(VERSION_LONG) $(USE_BINTRAY_API_KEY_FILE) \
		  -d $>/misc/bin/beast-$(VERSION_LONG)-x64.AppImage
	@: # upload tarballs of existing log directories
	$Q for d in cppcheck scan-build clang-tidy hacks unused asan ; do \
		(set -- $>/misc/$$d/*.* ; test -r "$$1") || continue ; \
		tar cJf $>/misc/$$d-$(VERSION_LONG).tar.xz -C $>/misc/ $$d/ && \
		misc/bintray.sh beast-team/testing/Reports -k $(BINTRAY_KEEPS) -g -v $(VERSION_LONG) $(USE_BINTRAY_API_KEY_FILE) \
		  $>/misc/$$d-$(VERSION_LONG).tar.xz || exit 1 ; \
	done
.PHONY: bintray
# Kill old versions: misc/bintray.sh beast-team/testing/Repository -k 0 $(USE_BINTRAY_API_KEY_FILE)
