# BEAST - Bedevilled Audio System
# Copyright (C) 2002-2003 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

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
imagesdir = ${beastimagesdir}
EXTRA_DIST += $(private_images) $(public_images)

# generate image header files
noinst_build_sources = beast-images.h
beast-images.h:	Makefile ${public_images}
	cd . \
	&& echo "/* Generated data from $< (by make $@) */" > xgen-bstih \
	&& echo >> xgen-bstih \
	&& echo "#define BST_SPLASH_IMAGE    \"beast-splash.png\"" >> xgen-bstih \
	&& echo "#define BST_SPLASH_WIDTH    (300)" >> xgen-bstih \
	&& echo "#define BST_SPLASH_HEIGHT   (350)" >> xgen-bstih \
	&& echo >> xgen-bstih \
	&& echo "/* Generated data ends here */" >> xgen-bstih \
	&& (cp xgen-bstih $@) \
	&& rm -f xgen-bstih


# install pkg-config files
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = bse.pc # sfi.pc
$(pkgconfig_DATA): $(top_builddir)/config.status
CLEANFILES += bse.pc sfi.pc
EXTRA_DIST += bse.pc.in # sfi.pc.in

# install .applications files
dotapplicationsdir = $(datadir)/application-registry
dotapplications_DATA = beast.applications
EXTRA_DIST += $(dotapplications_DATA)

# install related pixmaps
sharedpixmapsdir = $(datadir)/pixmaps
sharedpixmaps_DATA = beast.png beast-audio-x-bse.png
beast-audio-x-bse.png: $(srcdir)/bse-mime.png
	cp $< $@
CLEANFILES += beast-audio-x-bse.png
EXTRA_DIST += $(sharedpixmaps_DATA)

# generate and install .desktop file
# FIXME: INTLTOOL_DESKTOP_RULE
%.desktop: %.desktop.in
	$(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; \
	LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache \
			$(top_srcdir)/po $< $@
dotdesktopdir = $(datadir)/applications
dotdesktop_in_files = beast.desktop.in
dotdesktop_DATA = $(dotdesktop_in_files:.desktop.in=.desktop)
beast.desktop: beast.desktop.in
CLEANFILES += beast.desktop
EXTRA_DIST += $(dotdesktop_in_files)

# generate and install .xml mime info file
# FIXME: INTLTOOL_XML_RULE
%.xml: %.xml.in
	$(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; \
	LC_ALL=C $(INTLTOOL_MERGE) -x -u -c $(top_builddir)/po/.intltool-merge-cache \
		$(top_srcdir)/po $< $@
sharedmimedir = $(datadir)/mime
sharedmimepackagedir = $(sharedmimedir)/packages
sharedmimepackage_in_files = beast.xml.in
sharedmimepackage_DATA = $(sharedmimepackage_in_files:.xml.in=.xml)
CLEANFILES += beast.xml
beast.xml: beast.xml.in
EXTRA_DIST += $(sharedmimepackage_in_files)

# generate and install .mime and .keys files
# FIXME: INTLTOOL_KEYS_RULE
%.keys: %.keys.in
	$(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; \
	LC_ALL=C $(INTLTOOL_MERGE) -k -u -c $(top_builddir)/po/.intltool-merge-cache \
		$(top_srcdir)/po $< $@
mimedir = $(datadir)/mime-info
mime_in_file = bse.keys.in bse.mime
mime_DATA = $(mime_in_file:.keys.in=.keys)
CLEANFILES += bse.keys
bse.keys: bse.keys.in
EXTRA_DIST += $(mime_in_file)

# update mime database according to: http://www.freedesktop.org/Standards/shared-mime-info-spec
install-data-hook:
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)${sharedmimedir}";
uninstall-local: uninstall-sharedmimepackageDATA
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)${sharedmimedir}";


noinst_DATA = $(noinst_build_sources)
CLEANFILES += $(noinst_build_sources)
noinst_build_sources += config-paths.h
config-paths.h: $(top_builddir)/config.status Makefile
	cd . \
	&& echo "/* Generated data from $< (by make $@) */" > xgen-cph \
	&& echo >> xgen-cph \
	&& echo "#define SFIDL_PATH_STDINC        \"${sfidlstdincdir}\"" >> xgen-cph \
	&& echo "#define BSE_PATH_BINARIES        \"${bindir}\"" >> xgen-cph \
	&& echo "#define BSE_PATH_LADSPA          \"${bseladspapath}\"" >> xgen-cph \
	&& echo "#define BSE_PATH_PLUGINS         \"${bseplugindir}\"" >> xgen-cph \
	&& echo "#define BSE_PATH_DRIVERS         \"${bsedriverdir}\"" >> xgen-cph \
	&& echo "#define BSE_PATH_DEMOS           \"${bsedemodir}\"" >> xgen-cph \
	&& echo "#define BSE_PATH_SAMPLES         \"${bsesampledir}:${bseunversionedsampledir}\"" >> xgen-cph \
	&& echo "#define BSE_PATH_EFFECTS         \"${bseeffectdir}\"" >> xgen-cph \
	&& echo "#define BSE_PATH_INSTRUMENTS     \"${bseinstrumentdir}\"" >> xgen-cph \
	&& echo "#define BSE_PATH_SCRIPTS         \"${bsescriptdir}\"" >> xgen-cph \
	&& echo "#define BSE_PATH_USER_DATA(dir)  \"${bseuserdatadir}\" dir" >> xgen-cph \
	&& echo "#define BST_PATH_BINARIES        \"${bindir}\"" >> xgen-cph \
	&& echo "#define BST_PATH_DOCS            \"${beastdocsdir}\"" >> xgen-cph \
	&& echo "#define BST_PATH_IMAGES          \"${beastimagesdir}\"" >> xgen-cph \
	&& echo "#define BST_PATH_LOCALE          \"${beastlocaledir}\"" >> xgen-cph \
	&& echo "#define BST_PATH_KEYS            \"${beastkeysdir}\"" >> xgen-cph \
	&& echo "#define BST_PATH_SKINS           \"${beastskinsdir}\"" >> xgen-cph \
	&& echo "#define BST_PATH_PREFIX          \"${prefix}\"" >> xgen-cph \
	&& echo >> xgen-cph \
	&& echo "/* Generated data ends here */" >> xgen-cph \
	&& (cp xgen-cph $@) \
	&& rm -f xgen-cph
