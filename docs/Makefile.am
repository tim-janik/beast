SUBDIRS = generated reference

texi_docs = $(strip		\
	faq.texi		\
	quickstart.texi		\
	release-notes.texi	\
	teximacros.texi		\
	beast-index.texi	\
)
private_docs = $(strip		\
	bse-categories.txt	\
	interpolation.txt	\
)

docsdir = ${beastdocsdir}
docs_DATA = # ${public_docs}

EXTRA_DIST = ${texi_docs} ${private_docs}
