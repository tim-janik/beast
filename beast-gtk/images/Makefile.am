# BEAST - Bedevilled Audio System
# Copyright (C) 2002 Tim Janik
#
## Makefile.am for BEAST images

public_images = @STRIP_BEGIN@ \
	beast-mime.png \
	beast-splash.png \
@STRIP_END@
private_images = @STRIP_BEGIN@ \
	beast-logo.png \
	beast.png \
@STRIP_END@

imagesdir = ${beastimagesdir}
images_DATA = ${public_images}
EXTRA_DIST += ${public_images} ${private_images}

gen_sources           = xgen-bstih
CLEANFILES           += $(gen_sources) beast-images.h
noinst_DATA           = beast-images.h

beast-images.h:	Makefile
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
