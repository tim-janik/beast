# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/drivers/*.d)
CLEANDIRS += $(wildcard $>/drivers/)
drivers/rpath..libbse ::= ../lib

# == alsamidi defs ==
drivers/alsamidi.so		::= $>/drivers/alsamidi.so
drivers/alsamidi.sources	::= drivers/bsemididevice-alsa.cc
drivers/alsamidi.objects	::= $(call BUILDDIR_O, $(drivers/alsamidi.sources))

# == install rules ==
$(call INSTALL_BIN_RULE, plugins/drivers, \
	$(DESTDIR)$(pkglibdir)/plugins, \
	$(drivers/alsamidi.so))

# == alsamidi rules ==
$(drivers/alsamidi.objects): $(bse/libbse.deps) | $>/drivers/
$(drivers/alsamidi.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_SHARED_LIB, \
	$(drivers/alsamidi.so), \
	$(drivers/alsamidi.objects), \
	$(lib/libbse.so) | $>/drivers/, \
	-lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS) $(ALSA_LIBS), \
	$(drivers/rpath..libbse))
