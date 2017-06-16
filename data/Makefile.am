# BEAST & BSE
include $(top_srcdir)/Makefile.decl
AUTOMAKE_OPTIONS = -Wno-portability

private_images = $(strip	\
	beast-logo.png		\
	beast.png		\
)
public_images = $(strip         \
	beast-mime.png		\
	bse-mime.png		\
	beast-splash.png	\
)
images_DATA = ${public_images}
imagesdir = $(beastdatadir)/images
EXTRA_DIST += $(private_images) $(public_images)


# install pkg-config files
pkgconfigdir = $(bselibdir)/pkgconfig
pkgconfig_DATA = bse.pc # bse-@MAJOR@.pc
bse.pc: bse.pc.in $(top_builddir)/config.status
	$(AM_V_GEN)
	$(Q) cd $(top_builddir) && $(SHELL) ./config.status --file=$(subdir)/$@
EXTRA_DIST += bse.pc.in
CLEANFILES += bse.pc

# install related pixmaps
sharedpixmapsdir = $(beastsharedir)/pixmaps
sharedpixmaps_DATA = beast.png beast-audio-x-bse.png
beast-audio-x-bse.png: $(srcdir)/bse-mime.png
	cp $< $@
CLEANFILES += beast-audio-x-bse.png

# intltool-merge translation rules
PODEPS          = $(wildcard $(top_srcdir)/po/*.po)
INTLMERGE_CACHE = $(top_builddir)/po/intlmerge.tmpcache
$(INTLMERGE_CACHE): $(PODEPS)
	LC_ALL=C $(INTLMERGE) -d -u -c $(INTLMERGE_CACHE) $(top_srcdir)/po /dev/null /dev/null
%.desktop: %.desktop.in	$(INTLMERGE_CACHE)
	LC_ALL=C $(INTLMERGE) -d -u -c $(INTLMERGE_CACHE) $(top_srcdir)/po $< $@
%.xml: %.xml.in		$(INTLMERGE_CACHE)
	LC_ALL=C $(INTLMERGE) -x -u -c $(INTLMERGE_CACHE) $(top_srcdir)/po $< $@
%.keys: %.keys.in	$(INTLMERGE_CACHE)
	LC_ALL=C $(INTLMERGE) -k -u -c $(INTLMERGE_CACHE) $(top_srcdir)/po $< $@

# generate and install .desktop file, see: https://help.gnome.org/admin/system-admin-guide/stable/mime-types-custom.html
dotdesktopdir = $(beastsharedir)/applications
dotdesktop_in_files = beast.desktop.in
dotdesktop_DATA = $(dotdesktop_in_files:.desktop.in=.desktop)
beast.desktop: beast.desktop.in
CLEANFILES += beast.desktop
EXTRA_DIST += $(dotdesktop_in_files)

# generate and install .xml mime info file, from shared-mime-info-spec-0.12.html (2003-10-09)
# simple overview: https://help.gnome.org/admin/system-admin-guide/stable/mime-types-custom.html
# gvfs-info testfile.bse                # yields: standard::content-type: audio/x-bse
# gvfs-mime --query audio/x-bse	        # yields: Registered applications: beast.desktop
sharedmimedir = $(beastsharedir)/mime
sharedmimepackagedir = $(sharedmimedir)/packages
sharedmimepackage_in_files = beast.xml.in
sharedmimepackage_DATA = $(sharedmimepackage_in_files:.xml.in=.xml)
CLEANFILES += beast.xml
beast.xml: beast.xml.in
EXTRA_DIST += $(sharedmimepackage_in_files)

# ANCIENT: generate and install .mime and .keys files, from shared-mime-info-spec-0.11.html (2003-04-17)
mimedir = $(beastsharedir)/mime-info
mime_in_file = bse.keys.in bse.mime
mime_DATA = $(mime_in_file:.keys.in=.keys)
CLEANFILES += bse.keys
bse.keys: bse.keys.in
EXTRA_DIST += $(mime_in_file)

# ANCIENT: install .applications files, from "GNOME 2.4 Desktop System Administration Guide"
dotapplicationsdir = $(beastsharedir)/application-registry
dotapplications_DATA = beast.applications
EXTRA_DIST += $(dotapplications_DATA)
# mime_types=...,audio/mp3,audio/x-mp3,audio/mpg3,audio/x-mpg3,audio/mpeg3,audio/x-mpeg3
# mime_types=...,audio/mp2,audio/x-mp2,audio/mpg2,audio/x-mpg2,audio/mpeg2,audio/x-mpeg2
# mime_types=...,audio/mp1,audio/x-mp1,audio/mpg1,audio/x-mpg1,audio/mpeg1,audio/x-mpeg1
# mime_types=...,audio/mpeg,audio/x-mpeg

# $(call INSTALL_SYMLINK, TARGET, LINKNAME) - install symbolic link to target file
INSTALL_SYMLINK = ( d=$$(dirname $(2)) && set -x && rm -f $(2) && $(MKDIR_P) "$$d" && ln -s $(1) $(2) )

# Create links into standard filesystem hierarchy (FHS) and update system databases
# * https://www.freedesktop.org/wiki/Howto_desktop_files/
# * https://www.freedesktop.org/Standards/shared-mime-info-spec
# TODO: ../debian/copyright *should* be moved into ../docs/ and installed from there
EXTRA_DIST += ../debian/copyright
install-data-hook:
	@set -x && $(INSTALL_DATA) ../debian/copyright '$(DESTDIR)$(beastdocdir)/copyright'
	@$(call INSTALL_SYMLINK, '$(beastmandir)/man1/beast.1', '$(DESTDIR)$(mandir)/man1/beast.1')
	@$(call INSTALL_SYMLINK, '$(beastmandir)/man1/bsewavetool.1', '$(DESTDIR)$(mandir)/man1/bsewavetool.1')
	@$(call INSTALL_SYMLINK, '$(beastmandir)/man5/bse.5', '$(DESTDIR)$(mandir)/man5/bse.5')
	@$(call INSTALL_SYMLINK, '$(sharedpixmapsdir)/beast.png', '$(DESTDIR)$(datarootdir)/pixmaps/beast.png')
	@$(call INSTALL_SYMLINK, '$(sharedpixmapsdir)/beast-audio-x-bse.png', '$(DESTDIR)$(datarootdir)/pixmaps/beast-audio-x-bse.png')
	@$(call INSTALL_SYMLINK, '$(dotdesktopdir)/beast.desktop', '$(DESTDIR)$(datarootdir)/applications/beast.desktop')
	@$(call INSTALL_SYMLINK, '$(dotapplicationsdir)/beast.applications', '$(DESTDIR)$(datarootdir)/application-registry/beast.applications')
	@$(call INSTALL_SYMLINK, '$(sharedmimepackagedir)/beast.xml', '$(DESTDIR)$(datarootdir)/mime/packages/beast.xml')
	@$(call INSTALL_SYMLINK, '$(beastbindir)/beast', '$(DESTDIR)$(bindir)/beast')
	@set -x && test -z '$(UPDATE_DESKTOP_DATABASE)' || $(UPDATE_DESKTOP_DATABASE) '$(DESTDIR)${dotdesktopdir}';
	@set -x && test -z '$(UPDATE_MIME_DATABASE)' || $(UPDATE_MIME_DATABASE) '$(DESTDIR)${sharedmimedir}';
beastmandir = ${beastsharedir}/man

uninstall-local: uninstall-sharedmimepackageDATA
	rm -f '$(DESTDIR)$(beastdocdir)/copyright'
	rm -f '$(DESTDIR)$(mandir)/man1/beast.1'
	rm -f '$(DESTDIR)$(mandir)/man1/bsewavetool.1'
	rm -f '$(DESTDIR)$(mandir)/man5/bse.5'
	rm -f '$(DESTDIR)$(datarootdir)/pixmaps/beast.png'
	rm -f '$(DESTDIR)$(datarootdir)/pixmaps/beast-audio-x-bse.png'
	rm -f '$(DESTDIR)$(datarootdir)/applications/beast.desktop'
	rm -f '$(DESTDIR)$(datarootdir)/application-registry/beast.applications'
	rm -f '$(DESTDIR)$(datarootdir)/mime/packages/beast.xml'
	rm -f '$(DESTDIR)$(bindir)/beast'
	@set -x && test -z '$(UPDATE_DESKTOP_DATABASE)' || $(UPDATE_DESKTOP_DATABASE) '$(DESTDIR)${dotdesktopdir}';
	@set -x && test -z '$(UPDATE_MIME_DATABASE)' || $(UPDATE_MIME_DATABASE) '$(DESTDIR)${sharedmimedir}';
