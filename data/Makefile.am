# BEAST - Bedevilled Audio System
# Copyright (C) 2002-2003 Tim Janik
#
## Makefile.am for BEAST data files

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

# generate image header files
noinst_build_sources += beast-images.h
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
pkgconfig_DATA = sfi.pc bse.pc
$(pkgconfig_DATA): $(top_builddir)/config.status
CLEANFILES += bse.pc sfi.pc

# install .applications files
dotapplicationsdir = $(datadir)/application-registry
dotapplications_DATA = beast.applications

# install related pixmaps
sharedpixmapsdir = $(datadir)/pixmaps
sharedpixmaps_DATA = beast.png beast-audio-x-bse.png
beast-audio-x-bse.png: $(srcdir)/bse-mime.png
	cp $< $@

# generate and install .desktop file
@INTLTOOL_DESKTOP_RULE@
dotdesktopdir = $(datadir)/applications
dotdesktop_in_files = beast.desktop.in
dotdesktop_DATA = $(dotdesktop_in_files:.desktop.in=.desktop)
CLEANFILES += beast.desktop
beast.desktop: beast.desktop.in

# generate and install .xml mime info file
@INTLTOOL_XML_RULE@
sharedmimedir = $(datadir)/mime
sharedmimepackagedir = $(sharedmimedir)/packages
sharedmimepackage_in_files = beast.xml.in
sharedmimepackage_DATA = $(sharedmimepackage_in_files:.xml.in=.xml)
CLEANFILES += beast.xml
beast.xml: beast.xml.in

# generate and install .mime and .keys files
@INTLTOOL_KEYS_RULE@
mimedir = $(datadir)/mime-info
mime_in_file = bse.keys.in bse.mime
mime_DATA = $(mime_in_file:.keys.in=.keys)
CLEANFILES += bse.keys
bse.keys: bse.keys.in

# update mime database according to: http://www.freedesktop.org/Standards/shared-mime-info-spec
install-data-hook:
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)${sharedmimedir}";
uninstall-local: uninstall-sharedmimepackageDATA
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)${sharedmimedir}";


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
	&& echo "#define BSE_PATH_SAMPLES         \"${bsesampledir}\"" >> xgen-cph \
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
#	&& (cmp -s xgen-cph $@ || cp xgen-cph $@)


noinst_DATA = $(noinst_build_sources)
CLEANFILES += $(noinst_build_sources)


EXTRA_DIST += $(strip           \
	${public_images}	\
	${private_images}	\
	${dotapplications_DATA}	\
	${dotdesktop_in_files}	\
	${mime_in_file}		\
        sfi.pc.in               \
        bse.pc.in               \
)
