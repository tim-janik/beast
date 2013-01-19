# BSE-Plugins - Better Sound Engine dynamic Plugins
include $(top_srcdir)/Makefile.decl

icon_pattern = *.c *.png *.xcf
icon_list = 

# create icon list
Makefile.icons: $(icon_pattern) Makefile.am
	cd $(srcdir) \
	&& echo "## Generated data (by make $@)" > $@ \
	&& for f in $(icon_pattern) ; do \
	    echo "icon_list += $$f" >>$@ ; \
	done \
	&& echo "## Generated data ends here" >> $@

EXTRA_DIST += $(strip \
	$(icon_list) \
)

# generate C files:
# $ gdk-pixbuf-csource --name=adsr_icon adsr.png >adsr.c

## include generated icon list Makefile
include Makefile.icons
