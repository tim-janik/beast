SUBDIRS = generated reference

public_docs = $(strip \
)
private_docs = $(strip \
	bse-heart.txt \
	bse-networks.txt \
	bse-categories.txt \
	bse-data-v1.txt \
	bse-typesystem.txt \
	interpolation.txt \
	release-notes.tmpl \
)
build_tools = $(strip \
	tags2doc.pl \
)

docsdir = ${beastdocsdir}
docs_DATA = ${public_docs}

EXTRA_DIST = ${public_docs} ${private_docs} $(build_tools)
