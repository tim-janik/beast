# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/sfi/*.d)
CLEANDIRS += $(wildcard $>/sfi/)

# == sfidl defs ==
sfi/sfidl	  ::= $>/sfi/sfidl
sfi/sfidl.sources ::= sfi/sfidl.cc
sfi/sfidl.objects ::= $(call BUILDDIR_O, $(sfi/sfidl.sources))
sfi/sfidl.cc.FLAGS  = -O0

# == sfidl rules ==
$(sfi/sfidl.objects):	| $>/sfi/
$(sfi/sfidl.objects): EXTRA_DEFS ::= -DG_LOG_DOMAIN=\"SFI\" -DG_DISABLE_CONST_RETURNS -DPARANOID -DSFIDL_INTERNALS
$(sfi/sfidl.objects): EXTRA_INCLUDES ::= $(GLIB_CFLAGS)
$(call BUILD_PROGRAM, \
	$(sfi/sfidl), \
	$(sfi/sfidl.objects), \
	, \
	$(GLIB_LIBS))

# == testsfidl defs ==
sfi/testsfidl		::= $>/sfi/testsfidl
sfi/testsfidl.sources	  = sfi/testsfidl.cc
sfi/testsfidl.objects	  = $(call BUILDDIR_O, $(sfi/testsfidl.sources))
sfi/testsfidl.cc.FLAGS	  = -O0

# == testsfidl rules ==
$(sfi/testsfidl.objects): EXTRA_INCLUDES ::= -I$> $(GLIB_CFLAGS)
$(call BUILD_PROGRAM, \
	$(sfi/testsfidl), \
	$(sfi/testsfidl.objects), \
	, \
	$(GLIB_LIBS))

# == sfi-check ==
sfi-check: FORCE	| $(sfi/testsfidl)
	$(QECHO) RUN $@
	$Q $(sfi/testsfidl)
check: sfi-check
