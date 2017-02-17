# BEAST & BSE
include $(top_srcdir)/Makefile.decl

CLEANDIRS     =


# == electron ==
npm:
	npm install --save electron		# ./node_modules/electron/dist/electron
	npm install --save-dev node-gyp		# builds build/Release/v8bse.node
node_modules/: npm
clean-node_modules:
	rm -rf node_modules/

# == clean ==
clean-local: cleandirs # clean-node_modules
cleandirs:
	rm -rf $(CLEANDIRS)

EXTRA_DIST += $(strip \
)
