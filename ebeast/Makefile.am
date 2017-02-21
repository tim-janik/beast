# BEAST & BSE
include $(top_srcdir)/Makefile.decl

SUBDIRS = v8bse .

EXTRA_DIST += $(strip \
)

# == all: ==
all:
	@echo "$$(basename $$(pwd))/: experimental code, not built by default. Usage:"
	@echo "	make npm	# install electron and build tools locally"
	@echo "	make app	# build the v8 Bse module and app files"
	@echo "	make run	# run the electron app in the current directory"
	@echo "	make clean	# remove build files"
	@echo "	make npmclean	# remove npm installations and make clean"
