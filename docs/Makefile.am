SUBDIRS = generated

texi_docs = $(strip							\
	WRITING.3.texi		beast-index.texi	faq.texi	\
	quickstart.texi		release-notes.texi			\
)
extra_docs = $(strip					\
	bse-categories.txt	interpolation.txt	\
)

EXTRA_DIST = ${texi_docs} ${extra_docs}

site site-upgrade site-update site-preview site-update-con site-update-img site-update-nav:
	$(MAKE) -C generated/ $(AM_MAKEFLAGS) $@

overview-de.texi: linuxtag-2003.texi
	recode latin1..utf8 <$< >$@
