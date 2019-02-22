# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/drivers/*.d)
CLEANDIRS += $(wildcard $>/drivers/)

# == alsapcm defs ==
alsapcm.so			  ::= alsapcm.so
alsapcm.soname			  ::= $(alsapcm.so)
drivers/alsapcm.sofile		  ::= $>/drivers/$(alsapcm.so)
ALL_TARGETS			   += $(drivers/alsapcm.sofile)
drivers/alsapcm.sources		  ::= drivers/bsepcmdevice-alsa.cc
drivers/alsapcm.objects		  ::= $(sort $(drivers/alsapcm.sources:%.cc=$>/%.o))
$(drivers/alsapcm.sofile).LDFLAGS ::= -shared -Wl,-soname,$(alsapcm.soname)

# == alsamidi defs ==
alsamidi.so			::= alsamidi.so
alsamidi.soname			::= $(alsamidi.so)
drivers/alsamidi.sofile		::= $>/drivers/$(alsamidi.so)
ALL_TARGETS			 += $(drivers/alsamidi.sofile)
drivers/alsamidi.sources	::= drivers/bsemididevice-alsa.cc
drivers/alsamidi.objects	::= $(sort $(drivers/alsamidi.sources:%.cc=$>/%.o))

# == alsapcm rules ==
$(drivers/alsapcm.objects): $(bse/libbse.deps) | $>/drivers/
$(drivers/alsapcm.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(eval $(call LINKER, $(drivers/alsapcm.sofile), $(drivers/alsapcm.objects), $(bse/libbse.sofiles), -lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS) $(ALSA_LIBS), ../bse))

# == alsamidi rules ==
$(drivers/alsamidi.objects): $(bse/libbse.deps) | $>/drivers/
$(drivers/alsamidi.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(drivers/alsamidi.sofile).LDFLAGS ::= -shared -Wl,-soname,$(alsamidi.soname)
$(eval $(call LINKER, $(drivers/alsamidi.sofile), $(drivers/alsamidi.objects), $(bse/libbse.sofiles), -lbse-$(VERSION_MAJOR) $(BSEDEPS_LIBS) $(ALSA_LIBS), ../bse))

