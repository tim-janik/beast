# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/data/*.d)
CLEANDIRS += $(wildcard $>/data/)

# == data/ files ==
#data/desktop.inputs		::= data/beast.desktop.in
data/desktop.files		::= $>/data/beast.desktop
ALL_TARGETS			 += $(data/desktop.files)
#data/mimepkgs.inputs 		::= data/beast.xml.in
data/mimepkgs.files		::= $>/data/beast.xml
ALL_TARGETS			 += $(data/mimepkgs.files)
#data/mimeinfo.inputs 		::= data/bse.keys.in data/bse.mime
data/mimeinfo.files		::= $>/data/bse.keys $>/data/bse.mime
ALL_TARGETS			 += $(data/mimeinfo.files)
#data/applications.inputs	::= data/bse.keys.in data/bse.mime
data/applications.files		::= $>/data/beast.applications
ALL_TARGETS			 += $(data/applications.files)
#data/pc.inputs			::= data/bse.pc.in
data/pc.files			::= $>/data/bse.pc
ALL_TARGETS			 += $(data/pc.files)

# == installation rules ==
$(call INSTALL_DATA_RULE, data/desktop.files,      $(DESTDIR)$(pkgsharedir)/applications,         $(data/desktop.files))
$(call INSTALL_DATA_RULE, data/mimepkgs.files,     $(DESTDIR)$(pkgsharedir)/mime/packages,        $(data/mimepkgs.files))
$(call INSTALL_DATA_RULE, data/mimeinfo.files,     $(DESTDIR)$(pkgsharedir)/mime-info,            $(data/mimeinfo.files))
$(call INSTALL_DATA_RULE, data/applications.files, $(DESTDIR)$(pkgsharedir)/application-registry, $(data/applications.files))
$(call INSTALL_DATA_RULE, data/pc.files,           $(DESTDIR)$(pkglibdir)/lib/pkgconfig,          $(data/pc.files))

# == install symlinks ==
# Create links from $(prefix) paths according to the FHS, into $(pkglibdir), see also:
# * https://www.freedesktop.org/wiki/Howto_desktop_files/
# * https://www.freedesktop.org/Standards/shared-mime-info-spec
# Note, AppImage builds depend on the installdir hierarchy to match the symlinks hierarchy
data/install-prefix-symlinks: $(images/img.files) FORCE
	$(QECHO) INSTALL $@
	$Q $(call INSTALL_SYMLINK, '$(pkglibdir)/images/beast.png',                          '$(DESTDIR)$(datadir)/pixmaps/beast.png')
	$Q $(call INSTALL_SYMLINK, '$(pkglibdir)/images/bse-mime.png',                       '$(DESTDIR)$(datadir)/pixmaps/beast-audio-x-bse.png')
	$Q $(call INSTALL_SYMLINK, '$(pkgsharedir)/applications/beast.desktop',              '$(DESTDIR)$(datadir)/applications/beast.desktop')
	$Q $(call INSTALL_SYMLINK, '$(pkgsharedir)/mime/packages/beast.xml',                 '$(DESTDIR)$(datadir)/mime/packages/beast.xml')
	$Q $(call INSTALL_SYMLINK, '$(pkgsharedir)/mime-info/bse.keys',                      '$(DESTDIR)$(datadir)/mime-info/bse.keys')
	$Q $(call INSTALL_SYMLINK, '$(pkgsharedir)/mime-info/bse.mime',                      '$(DESTDIR)$(datadir)/mime-info/bse.mime')
	$Q $(call INSTALL_SYMLINK, '$(pkgsharedir)/application-registry/beast.applications', '$(DESTDIR)$(datadir)/application-registry/beast.applications')
	$Q $(call INSTALL_SYMLINK, '$(pkglibdir)/bin/beast',                                 '$(DESTDIR)$(bindir)/beast')
install: data/install-prefix-symlinks
data/uninstall-prefix-symlinks: FORCE
	$(QECHO) REMOVE $@
	$Q rm -f '$(DESTDIR)$(datadir)/pixmaps/beast.png'
	$Q rm -f '$(DESTDIR)$(datadir)/pixmaps/beast-audio-x-bse.png'
	$Q rm -f '$(DESTDIR)$(datadir)/applications/beast.desktop'
	$Q rm -f '$(DESTDIR)$(datadir)/mime/packages/beast.xml'
	$Q rm -f '$(DESTDIR)$(datadir)/mime-info/bse.keys'
	$Q rm -f '$(DESTDIR)$(datadir)/mime-info/bse.mime'
	$Q rm -f '$(DESTDIR)$(datadir)/application-registry/beast.applications'
	$Q rm -f '$(DESTDIR)$(bindir)/beast'
uninstall: data/uninstall-prefix-symlinks

# == update Desktop/Mime caches after installation ==
data/install.dbupdates: install--data/desktop.files install--data/mimeinfo.files install--data/mimepkgs.files
	$(QECHO) RUN $@
	$Q test ! -x '$(UPDATE_DESKTOP_DATABASE)' || $(UPDATE_DESKTOP_DATABASE) '$(DESTDIR)$(datadir)/applications/'
	$Q test ! -x '$(UPDATE_MIME_DATABASE)' || \
	  XDG_DATA_DIRS="$$XDG_DATA_DIRS:$(DESTDIR)$(datadir)" \
	  $(UPDATE_MIME_DATABASE) '$(DESTDIR)$(datadir)/mime'
install: data/install.dbupdates
# We need to temporarily set XDG_DATA_DIRS to shut up update-mime-database about custom search paths
data/uninstall.dbupdates: uninstall--data/desktop.files uninstall--data/mimeinfo.files uninstall--data/mimepkgs.files
	$(QECHO) RUN $@
	$Q test ! -x '$(UPDATE_DESKTOP_DATABASE)' || $(UPDATE_DESKTOP_DATABASE) '$(DESTDIR)$(datadir)/applications/' || :
	$Q test ! -x '$(UPDATE_MIME_DATABASE)' || \
	  XDG_DATA_DIRS="$$XDG_DATA_DIRS:$(DESTDIR)$(datadir)" \
	  $(UPDATE_MIME_DATABASE) '$(DESTDIR)$(datadir)/mime' || :
uninstall: data/uninstall.dbupdates

# == i18n merge rules ==
# .desktop file, see: https://help.gnome.org/admin/system-admin-guide/stable/mime-types-custom.html
$>/data/%.desktop: data/%.desktop.in		$(INTLTOOL_MERGE) $(INTLTOOL_MERGE_CACHE)	| $>/data/
	$(QGEN)
	$Q LC_ALL=C $(INTLTOOL_MERGE) -u -d -q -c $(INTLTOOL_MERGE_CACHE) po/ $< $@
# .xml mime info file, from shared-mime-info-spec-0.12.html (2003-10-09)
# simple overview: https://help.gnome.org/admin/system-admin-guide/stable/mime-types-custom.html
# gvfs-info testfile.bse                # yields: standard::content-type: audio/x-bse
# gvfs-mime --query audio/x-bse	        # yields: Registered applications: beast.desktop
$>/data/%.xml: data/%.xml.in			$(INTLTOOL_MERGE) $(INTLTOOL_MERGE_CACHE)	| $>/data/
	$(QGEN)
	$Q LC_ALL=C $(INTLTOOL_MERGE) -u -x -q -c $(INTLTOOL_MERGE_CACHE) po/ $< $@
# ANCIENT, .keys files, from shared-mime-info-spec-0.11.html (2003-04-17)
$>/data/%.keys: data/%.keys.in			$(INTLTOOL_MERGE) $(INTLTOOL_MERGE_CACHE)	| $>/data/
	$(QGEN)
	$Q LC_ALL=C $(INTLTOOL_MERGE) -u -k -q -c $(INTLTOOL_MERGE_CACHE) po/ $< $@
# ANCIENT: .mime, from shared-mime-info-spec-0.11.html (2003-04-17)
$>/data/%.mime: data/%.mime									| $>/data/
	$(QGEN)
	$Q cp -L $< $@
# ANCIENT: install .applications files, from "GNOME 2.4 Desktop System Administration Guide"
$>/data/%.applications: data/%.applications							| $>/data/
	$(QGEN)
	$Q cp -L $< $@

# == bse.pc rule ==
$>/data/bse.pc: $(config-stamps) data/Makefile.mk						| $>/data/
	$(QGEN)
	$Q echo '# bse.pc'							> $@.tmp
	$Q echo 'prefix=$(prefix)'						>>$@.tmp
	$Q echo 'libdir=$${prefix}/lib'						>>$@.tmp
	$Q echo 'pkglibdir=$${libdir}/beast-$(VERSION_MAJOR)-$(VERSION_MINOR)'	>>$@.tmp
	$Q echo 'includedir=$${pkglibdir}/include/'				>>$@.tmp
	$Q echo 'plugindir=$${pkglibdir}/plugins'				>>$@.tmp
	$Q echo 'demodir=$${pkglibdir}/media/Demos'				>>$@.tmp
	$Q echo ''								>>$@.tmp
	$Q echo 'Name: Beast & Bse'						>>$@.tmp
	$Q echo 'Description: Beast - Music Synthesizer and Composer'		>>$@.tmp
	$Q echo 'Requires: glib-2.0 gobject-2.0'				>>$@.tmp
	$Q echo 'Version: $(VERSION_M.M.M)'					>>$@.tmp
	$Q echo 'Libs: -L$${pkglibdir}/lib -lbse-$(VERSION_MAJOR)'		>>$@.tmp
	$Q echo 'Cflags: -I$${includedir}'					>>$@.tmp
	$Q echo '$@ : $(config-calc-hash.dep)'					> $@.d
	$Q mv $@.tmp $@
