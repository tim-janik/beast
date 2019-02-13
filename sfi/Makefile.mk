# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/sfi/*.d)
CLEANDIRS += $(wildcard $>/sfi/)


# == sfidl ==
sfi/sfidl	  ::= $>/sfi/sfidl
ALL_TARGETS	   += $(sfi/sfidl)
sfi/sfidl.sources ::= sfi/sfidl.cc
sfi/sfidl.objects ::= $(sort $(sfi/sfidl.sources:%.cc=$>/%.o))
$(sfi/sfidl.objects):	| $>/sfi/
$(sfi/sfidl.objects): EXTRA_DEFS ::= -DG_LOG_DOMAIN=\"SFI\" -DG_DISABLE_CONST_RETURNS -DPARANOID -DSFIDL_INTERNALS
$(sfi/sfidl.objects): EXTRA_INCLUDES ::= $(GLIB_CFLAGS)
sfi/sfidl.cc.FLAGS  = -O0
# BIN linking
$(sfi/sfidl): $(sfi/sfidl.objects) $(MAKEFILE_LIST)
	$(QECHO) LD $@
	$Q $(CXX) $(CXXSTD) -fPIC -o $@ $(sfi/sfidl.objects) -Wl,--no-undefined $(GLIB_LIBS)
