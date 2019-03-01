# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/data/*.d)
CLEANDIRS += $(wildcard $>/data/)

# == data/ files ==
data/pc.dir	::= $(pkglibdir)/lib/pkgconfig
data/pc.files	::= $>/data/bse.pc
ALL_TARGETS	 += $(data/pc.files)

# == bse.pc ==
$>/data/bse.pc: $(config-stamps) data/Makefile.mk			| $>/data/
	$(QGEN)
	$Q echo '# bse.pc'							> $@.tmp
	$Q echo 'prefix=$(prefix)'						>>$@.tmp
	$Q echo 'libdir=$${prefix}/lib'						>>$@.tmp
	$Q echo 'pkglibdir=$${libdir}/beast-$(VERSION_MAJOR)-$(VERSION_MINOR)'	>>$@.tmp
	$Q echo 'includedir=$${pkglibdir}/include/'				>>$@.tmp
	$Q echo 'plugindir=$${pkglibdir}/plugins'				>>$@.tmp
	$Q echo 'driverdir=$${pkglibdir}/drivers'				>>$@.tmp
	$Q echo 'demodir=$${pkglibdir}/media/Demos'				>>$@.tmp
	$Q echo ''								>>$@.tmp
	$Q echo 'Name: Beast & Bse'						>>$@.tmp
	$Q echo 'Description: Beast - Music Synthesizer and Composer'		>>$@.tmp
	$Q echo 'Requires: glib-2.0 gobject-2.0'				>>$@.tmp
	$Q echo 'Version: $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO)'	>>$@.tmp
	$Q echo 'Libs: -L$${pkglibdir}/lib -lbse-$(VERSION_MAJOR)'		>>$@.tmp
	$Q echo 'Cflags: -I$${includedir}'					>>$@.tmp
	$Q mv $@.tmp $@

# == installation ==
$(call INSTALL_DATA_RULE, data/bse.pc, $(DESTDIR)$(data/pc.dir), $(data/pc.files))
