SUBDIRS = generated reference

public_docs = $(strip \
)
private_docs = $(strip \
	bse-categories.txt \
	interpolation.txt \
)
build_tools = $(strip \
	tags2doc.pl \
)

docsdir = ${beastdocsdir}
docs_DATA = ${public_docs}

EXTRA_DIST = ${public_docs} ${private_docs} $(build_tools)
