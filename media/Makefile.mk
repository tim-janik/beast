# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/media/*.d)
CLEANDIRS += $(wildcard $>/media/)

media/subdir_names   = Demos Instruments
media/subdir_links ::= $(addprefix $>/media/, $(media/subdir_names))

# == building ==
ALL_TARGETS += $(media/subdir_links)
$(media/subdir_links):	| $>/media/
	$(QGEN)
	$Q rm -f $@
	$Q ln -s ../$(build2srcdir)/media/$(notdir $@) $@

# == installation ==
media/install: $(media/subdir_links) FORCE
	@$(QECHO) INSTALL '$(DESTDIR)$(pkglibdir)/media/...'
	$Q $(INSTALL) -d '$(DESTDIR)$(pkglibdir)/media/'
	$Q : \
	  $(foreach DIR, $(media/subdir_names), \
	    && rm -f -r '$(DESTDIR)$(pkglibdir)/media/$(DIR)' \
	    && $(CP) -a -L $>/media/$(DIR) '$(DESTDIR)$(pkglibdir)/media/')
install: media/install
media/uninstall: FORCE
	@$(QECHO) REMOVE '$(DESTDIR)$(pkglibdir)/media/...'
	$Q : \
	  $(foreach DIR, $(media/subdir_names), \
	    && rm -f -r '$(DESTDIR)$(pkglibdir)/media/$(DIR)')
uninstall: media/uninstall

