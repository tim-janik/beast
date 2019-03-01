# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/data/*.d)
CLEANDIRS += $(wildcard $>/data/)

# == data/ files ==
data/pc.dir			::= $(pkglibdir)/lib/pkgconfig
data/pc.files			::= $>/data/bse.pc
ALL_TARGETS			 += $(data/pc.files)
#data/desktop.inputs		::= data/beast.desktop.in
data/desktop.dir		::= $(pkglibdir)/share/applications
data/desktop.files		::= $>/data/beast.desktop
ALL_TARGETS			 += $(data/desktop.files)
data/sharemime.dir		::= $(pkglibdir)/share/mime
#data/mimepkgs.inputs 		::= data/beast.xml.in
data/mimepkgs.dir		::= $(data/sharemime.dir)/packages
data/mimepkgs.files		::= $>/data/beast.xml
ALL_TARGETS			 += $(data/mimepkgs.files)
#data/mimeinfo.inputs 		::= data/bse.keys.in data/bse.mime
data/mimeinfo.dir		::= $(pkglibdir)/share/mime-info
data/mimeinfo.files		::= $>/data/bse.keys $>/data/bse.mime
ALL_TARGETS			 += $(data/mimeinfo.files)
#data/applications.inputs	::= data/bse.keys.in data/bse.mime
data/applications.dir		::= $(pkglibdir)/share/application-registry
data/applications.files		::= $>/data/beast.applications
ALL_TARGETS			 += $(data/applications.files)

# == installation rules ==
$(call INSTALL_DATA_RULE, data/pc.files,           $(DESTDIR)$(data/pc.dir),           $(data/pc.files))
$(call INSTALL_DATA_RULE, data/desktop.files,      $(DESTDIR)$(data/desktop.dir),      $(data/desktop.files))
$(call INSTALL_DATA_RULE, data/mimeinfo.files,     $(DESTDIR)$(data/mimeinfo.dir),     $(data/mimeinfo.files))
$(call INSTALL_DATA_RULE, data/mimepkgs.files,     $(DESTDIR)$(data/mimepkgs.dir),     $(data/mimepkgs.files))
$(call INSTALL_DATA_RULE, data/applications.files, $(DESTDIR)$(data/applications.dir), $(data/applications.files))

# == update Desktop/Mime caches after installation ==
data/install.dbupdates: install--data/desktop.files install--data/mimeinfo.files install--data/mimepkgs.files
	$(QECHO) RUN $@
	$Q test -z '$(UPDATE_DESKTOP_DATABASE)' || $(UPDATE_DESKTOP_DATABASE) '$(DESTDIR)$(data/desktop.dir)'
	$Q test -z '$(UPDATE_MIME_DATABASE)' || \
	  XDG_DATA_DIRS="$$XDG_DATA_DIRS:$(DESTDIR)$(data/sharemime.dir)/.." \
	  $(UPDATE_MIME_DATABASE) '$(DESTDIR)$(data/sharemime.dir)'
install: data/install.dbupdates
# We need to temporarily set XDG_DATA_DIRS to shut up update-mime-database about custom search paths
data/uninstall.dbupdates: uninstall--data/desktop.files uninstall--data/mimeinfo.files uninstall--data/mimepkgs.files
	$(QECHO) RUN $@
	$Q test -z '$(UPDATE_DESKTOP_DATABASE)' || $(UPDATE_DESKTOP_DATABASE) '$(DESTDIR)$(data/desktop.dir)'
	$Q test -z '$(UPDATE_MIME_DATABASE)' || \
	  XDG_DATA_DIRS="$$XDG_DATA_DIRS:$(DESTDIR)$(data/sharemime.dir)/.." \
	  $(UPDATE_MIME_DATABASE) '$(DESTDIR)$(data/sharemime.dir)'
uninstall: data/uninstall.dbupdates

# == i18n merge rules ==
# .desktop file, see: https://help.gnome.org/admin/system-admin-guide/stable/mime-types-custom.html
$>/data/%.desktop: data/%.desktop.in		$(INTLTOOL_MERGE) $(INTLTOOL_MERGE_CACHE)
	$(QGEN)
	$Q LC_ALL=C $(INTLTOOL_MERGE) -u -d -q -c $(INTLTOOL_MERGE_CACHE) po/ $< $@
# .xml mime info file, from shared-mime-info-spec-0.12.html (2003-10-09)
# simple overview: https://help.gnome.org/admin/system-admin-guide/stable/mime-types-custom.html
# gvfs-info testfile.bse                # yields: standard::content-type: audio/x-bse
# gvfs-mime --query audio/x-bse	        # yields: Registered applications: beast.desktop
$>/data/%.xml: data/%.xml.in			$(INTLTOOL_MERGE) $(INTLTOOL_MERGE_CACHE)
	$(QGEN)
	$Q LC_ALL=C $(INTLTOOL_MERGE) -u -x -q -c $(INTLTOOL_MERGE_CACHE) po/ $< $@
# ANCIENT, .keys files, from shared-mime-info-spec-0.11.html (2003-04-17)
$>/data/%.keys: data/%.keys.in			$(INTLTOOL_MERGE) $(INTLTOOL_MERGE_CACHE)
	$(QGEN)
	$Q LC_ALL=C $(INTLTOOL_MERGE) -u -k -q -c $(INTLTOOL_MERGE_CACHE) po/ $< $@
# ANCIENT: .mime, from shared-mime-info-spec-0.11.html (2003-04-17)
$>/data/%.mime: data/%.mime
	$(QGEN)
	$Q cp -L $< $@
# ANCIENT: install .applications files, from "GNOME 2.4 Desktop System Administration Guide"
$>/data/%.applications: data/%.applications
	$(QGEN)
	$Q cp -L $< $@

# == bse.pc rule ==
$>/data/bse.pc: $(config-stamps) data/Makefile.mk			| $>/data/
	$(QGEN)
	$Q echo '# bse.pc'							> $@.tmp
	$Q echo 'prefix=$(prefix)'						>>$@.tmp
	$Q echo 'libdir=$${prefix}/lib'						>>$@.tmp
	$Q echo 'pkglibdir=$${libdir}/beast-$(VERSION_MAJOR)-$(VERSION_MINOR)'	>>$@.tmp
	$Q echo 'includedir=$${pkglibdir}/include/'				>>$@.tmp
	$Q echo 'plugindir=$${pkglibdir}/plugins'				>>$@.tmp
	$Q echo 'driverdir=$${pkglibdir}/drivers'				>>$@.tmp
	$Q echo 'demodir=$${pkglibdir}/media/Demos'				>>$@.tmp
	$Q echo ''								>>$@.tmp
	$Q echo 'Name: Beast & Bse'						>>$@.tmp
	$Q echo 'Description: Beast - Music Synthesizer and Composer'		>>$@.tmp
	$Q echo 'Requires: glib-2.0 gobject-2.0'				>>$@.tmp
	$Q echo 'Version: $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO)'	>>$@.tmp
	$Q echo 'Libs: -L$${pkglibdir}/lib -lbse-$(VERSION_MAJOR)'		>>$@.tmp
	$Q echo 'Cflags: -I$${includedir}'					>>$@.tmp
	$Q mv $@.tmp $@
