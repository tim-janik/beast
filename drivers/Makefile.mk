# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/drivers/*.d)
CLEANDIRS += $(wildcard $>/drivers/)
drivers/rpath..bse ::= ../bse

# == alsapcm defs ==
drivers/alsapcm.so		::= $>/drivers/alsapcm.so
drivers/alsapcm.sources		::= drivers/bsepcmdevice-alsa.cc
drivers/alsapcm.objects		::= $(sort $(drivers/alsapcm.sources:%.cc=$>/%.o))

# == alsamidi defs ==
drivers/alsamidi.so		::= $>/drivers/alsamidi.so
drivers/alsamidi.sources	::= drivers/bsemididevice-alsa.cc
drivers/alsamidi.objects	::= $(sort $(drivers/alsamidi.sources:%.cc=$>/%.o))

# == alsapcm rules ==
$(drivers/alsapcm.objects): $(bse/libbse.deps) | $>/drivers/
$(drivers/alsapcm.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_SHARED_LIB, \
	$(drivers/alsapcm.so), \
	$(drivers/alsapcm.objects), \
	$(bse/libbse.sofiles) | $>/drivers/, \
	-lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS) $(ALSA_LIBS), \
	$(drivers/rpath..bse))

# == alsamidi rules ==
$(drivers/alsamidi.objects): $(bse/libbse.deps) | $>/drivers/
$(drivers/alsamidi.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_SHARED_LIB, \
	$(drivers/alsamidi.so), \
	$(drivers/alsamidi.objects), \
	$(bse/libbse.sofiles) | $>/drivers/, \
	-lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS) $(ALSA_LIBS), \
	$(drivers/rpath..bse))
