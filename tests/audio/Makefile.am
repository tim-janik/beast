FEATURE_TESTS = test-bseadder
CLEANFILES = *.current *.bse.wav xgen-bp beastconf.sh featuretest.status.sh

noinst_build_sources += beastconf.sh
beastconf.sh:
	cd . \
	&& echo "# Generated data from $< (by make $@)" > xgen-bp \
	&& echo >> xgen-bp \
	&& echo "BSE_VERSION=$(BSE_VERSION)" >> xgen-bp \
	&& echo "top_builddir=$(top_builddir)" >> xgen-bp \
	&& chmod 755 xgen-bp \
	&& (cp xgen-bp $@) \
	&& rm -f xgen-bp

all: beastconf.sh

test: reset-summary $(FEATURE_TESTS)
	@featuretest.sh --summary
	@featuretest.sh --reset-summary

reset-summary:
	@featuretest.sh --reset-summary

test-bseadder: beastconf.sh
	@featuretest.sh bseadder.bse bseadder.reference \
	               --avg-spectrum --avg-energy \
	            -- --threshold=99.99
