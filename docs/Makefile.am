public_docs = $(strip \
	bse-heart.txt \
	bse-networks.txt \
	faq.txt \
)
private_docs = $(strip \
	bse-categories.txt \
	bse-data-v1.txt \
	bse-typesystem.txt \
)
build_tools = $(strip \
	faq.src \
	text.1.faq \
	text.2.faq \
	makefaq.py \
)
built_sources = $(strip \
	faq.txt \
)

docsdir = ${beastdocsdir}
docs_DATA = ${public_docs}


faq.txt: @MISSING_PYTHON@ @MISSING_FOLD@ faq.src
	python makefaq.py | fold -s > faq.txt

MAINTAINERCLEANFILES += $(built_sources)

EXTRA_DIST = ${public_docs} ${private_docs} $(build_tools)
