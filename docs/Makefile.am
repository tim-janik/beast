SUBDIRS = utils generated

texi_docs = $(strip								\
	WRITING.3.texi		teximacros.texi		beast-index.texi	\
	faq.texi		quickstart.texi		release-notes.texi	\
	site-contact.texi	site-download.texi	site-resources.texi	\
	site-documentation.texi	site-news.texi		site-about.texi		\
	site-development.texi	site-historic.texi	site-oldnews.texi	\
	site-links-related.texi	site-links-synthesis.texi \
)
extra_docs = $(strip					\
	site-navigation.xml				\
	bse-categories.txt	interpolation.txt	\
)

EXTRA_DIST = ${texi_docs} ${extra_docs}
