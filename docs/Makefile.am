SUBDIRS = generated

texi_docs = $(strip						\
	WRITING.3.texi			beast-index.texi	\
	quickstart.texi			plugin-devel.texi	\
	release-notes.texi		faq.texi		\
	sfidl.1.texi			sfidl-manual.texi	\
	architecture.texi					\
)
texi_images = $(strip		\
	module-voices.png	\
	beast-components.png	\
	beast-progs.png		\
)
EXTRA_DIST = $(strip		\
	beast-components.dia	\
	beast-progs.dia		\
	module-voices.dia	\
	bse-categories.txt	\
	interpolation.txt	\
	${texi_docs}		\
	$(texi_images)		\
)

docimages_DATA = ${texi_images}
docimagesdir = ${beastimagesdir}

site site-upgrade site-update site-preview site-update-con site-update-img site-update-nav:
	$(MAKE) -C generated/ $(AM_MAKEFLAGS) $@

overview-de.texi: linuxtag-2003.texi
	recode latin1..utf8 <$< >$@
