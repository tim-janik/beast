# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/beast-gtk/*.d)
CLEANDIRS += $(wildcard $>/beast-gtk/)
beast-gtk/rpath..bse ::= ../bse

# == beast-gtk/ files ==
beast-gtk/beast.sources ::= $(strip		\
	beast-gtk/bstapp.cc			\
	beast-gtk/bstasciipixbuf.cc		\
	beast-gtk/bstauxdialogs.cc		\
	beast-gtk/bstbseutils.cc		\
	beast-gtk/bstbuseditor.cc		\
	beast-gtk/bstbusmixer.cc		\
	beast-gtk/bstbusview.cc			\
	beast-gtk/bstcanvaslink.cc		\
	beast-gtk/bstcanvassource.cc		\
	beast-gtk/bstcluehunter.cc		\
	beast-gtk/bstdbmeter.cc			\
	beast-gtk/bstdial.cc			\
	beast-gtk/bsteventroll.cc		\
	beast-gtk/bsteventrollctrl.cc		\
	beast-gtk/bstfiledialog.cc		\
	beast-gtk/bstgconfig.cc			\
	beast-gtk/bstgrowbar.cc			\
	beast-gtk/bstitemseqdialog.cc		\
	beast-gtk/bstitemview.cc		\
	beast-gtk/bstkeybindings.cc		\
	beast-gtk/bstknob.cc			\
	beast-gtk/bstlogadjustment.cc		\
	beast-gtk/bstmain.cc			\
	beast-gtk/bstmenus.cc			\
	beast-gtk/bstmsgabsorb.cc		\
	beast-gtk/bstparam.cc			\
	beast-gtk/bstparamview.cc		\
	beast-gtk/bstpartdialog.cc		\
	beast-gtk/bstpartview.cc		\
	beast-gtk/bstpatterncolumns.cc		\
	beast-gtk/bstpatternctrl.cc		\
	beast-gtk/bstpatternview.cc		\
	beast-gtk/bstpianoroll.cc		\
	beast-gtk/bstpianorollctrl.cc		\
	beast-gtk/bstplayback.cc		\
	beast-gtk/bstpreferences.cc		\
	beast-gtk/bstprofiler.cc		\
	beast-gtk/bstprojectctrl.cc		\
	beast-gtk/bstqsampler.cc		\
	beast-gtk/bstsampleeditor.cc		\
	beast-gtk/bstscrollgraph.cc		\
	beast-gtk/bstsegment.cc			\
	beast-gtk/bstsequence.cc		\
	beast-gtk/bstservermonitor.cc		\
	beast-gtk/bstskinconfig.cc		\
	beast-gtk/bstsnetrouter.cc		\
	beast-gtk/bstsnifferscope.cc		\
	beast-gtk/bstsoundfontpresetview.cc	\
	beast-gtk/bstsoundfontview.cc		\
	beast-gtk/bstsplash.cc			\
	beast-gtk/bstsupershell.cc		\
	beast-gtk/bsttrackroll.cc		\
	beast-gtk/bsttrackrollctrl.cc		\
	beast-gtk/bsttracksynthdialog.cc	\
	beast-gtk/bsttrackview.cc		\
	beast-gtk/bsttreestores.cc		\
	beast-gtk/bstusermessage.cc		\
	beast-gtk/bstutils.cc			\
	beast-gtk/bstwaveeditor.cc		\
	beast-gtk/bstwaveview.cc		\
	beast-gtk/bstxframe.cc			\
	beast-gtk/bstxkb.cc			\
	beast-gtk/bstzoomedwindow.cc		\
)
beast-gtk/beast.headers ::= $(strip		\
	beast-gtk/bstapp.hh			\
	beast-gtk/bstasciipixbuf.hh		\
	beast-gtk/bstauxdialogs.hh		\
	beast-gtk/bstbseutils.hh		\
	beast-gtk/bstbuseditor.hh		\
	beast-gtk/bstbusmixer.hh		\
	beast-gtk/bstbusview.hh			\
	beast-gtk/bstcanvaslink.hh		\
	beast-gtk/bstcanvassource.hh		\
	beast-gtk/bstcluehunter.hh		\
	beast-gtk/bstdbmeter.hh			\
	beast-gtk/bstdefs.hh			\
	beast-gtk/bstdial.hh			\
	beast-gtk/bsteventroll.hh		\
	beast-gtk/bsteventrollctrl.hh		\
	beast-gtk/bstfiledialog.hh		\
	beast-gtk/bstgconfig.hh			\
	beast-gtk/bstgrowbar.hh			\
	beast-gtk/bstitemseqdialog.hh		\
	beast-gtk/bstitemview.hh		\
	beast-gtk/bstkeybindings.hh		\
	beast-gtk/bstknob.hh			\
	beast-gtk/bstlogadjustment.hh		\
	beast-gtk/bstmenus.hh			\
	beast-gtk/bstmsgabsorb.hh		\
	beast-gtk/bstparam.hh			\
	beast-gtk/bstparamview.hh		\
	beast-gtk/bstpartdialog.hh		\
	beast-gtk/bstpartview.hh		\
	beast-gtk/bstpatterncolumns.hh		\
	beast-gtk/bstpatternctrl.hh		\
	beast-gtk/bstpatternview.hh		\
	beast-gtk/bstpianoroll.hh		\
	beast-gtk/bstpianorollctrl.hh		\
	beast-gtk/bstplayback.hh		\
	beast-gtk/bstpreferences.hh		\
	beast-gtk/bstprofiler.hh		\
	beast-gtk/bstprojectctrl.hh		\
	beast-gtk/bstqsampler.hh		\
	beast-gtk/bstsampleeditor.hh		\
	beast-gtk/bstscrollgraph.hh		\
	beast-gtk/bstsegment.hh			\
	beast-gtk/bstsequence.hh		\
	beast-gtk/bstservermonitor.hh		\
	beast-gtk/bstskinconfig.hh		\
	beast-gtk/bstsnetrouter.hh		\
	beast-gtk/bstsnifferscope.hh		\
	beast-gtk/bstsoundfontpresetview.hh	\
	beast-gtk/bstsoundfontview.hh		\
	beast-gtk/bstsplash.hh			\
	beast-gtk/bstsupershell.hh		\
	beast-gtk/bsttrackroll.hh		\
	beast-gtk/bsttrackrollctrl.hh		\
	beast-gtk/bsttracksynthdialog.hh	\
	beast-gtk/bsttrackview.hh		\
	beast-gtk/bsttreestores.hh		\
	beast-gtk/bstusermessage.hh		\
	beast-gtk/bstutils.hh			\
	beast-gtk/bstwaveeditor.hh		\
	beast-gtk/bstwaveview.hh		\
	beast-gtk/bstxframe.hh			\
	beast-gtk/bstxkb.hh			\
	beast-gtk/bstzoomedwindow.hh		\
)
beast-gtk/beast.deps ::= $(strip		\
	$>/beast-gtk/bstapi_interfaces.hh	\
	$>/beast-gtk/bstmarshal.h		\
	$>/beast-gtk/bstoldbseapi.h		\
)
beast-gtk/beast.cc.deps ::= $(strip		\
	$>/beast-gtk/bstmarshal.cc		\
	$>/beast-gtk/bstoldbseapi.cc		\
)

# == beast defs ==
beast-gtk/beast			::= $>/beast-gtk/beast
beast-gtk/beast.objects		::= $(call SUBST_O, $(addprefix $>/, $(beast-gtk/beast.sources)))

# == subdirs ==
include beast-gtk/gxk/Makefile.mk
include beast-gtk/icons/Makefile.mk

# == beast rules ==
$(beast-gtk/beast.objects): $(beast-gtk/icons/icon.deps) $(beast-gtk/icons/icon.cc.deps)
$(beast-gtk/beast.objects): $(beast-gtk/beast.deps) $(beast-gtk/beast.cc.deps)
$(beast-gtk/beast.objects): EXTRA_INCLUDES ::= -I$> -I$>/beast-gtk -Ibeast-gtk $(GTK_CFLAGS)
$(beast-gtk/beast.objects): $(bse/libbse.deps) | $>/beast-gtk/
$(beast-gtk/beast.objects): $(beast-gtk/gxk/libgxk.deps)
$(call BUILD_PROGRAM, \
	$(beast-gtk/beast), \
	$(beast-gtk/beast.objects), \
	$(beast-gtk/gxk/libgxk.a) $(bse/libbse.so), \
	$(beast-gtk/gxk/libgxk.a) -lbse-$(VERSION_MAJOR) $(GTK_LIBS) $(XKB_LIBS), \
	$(beast-gtk/rpath..bse))

# == bstmarshal ==
$>/beast-gtk/bstmarshal.h: beast-gtk/bstmarshal.list			| $>/beast-gtk/
	$(QGEN)
	$Q $(GLIB_GENMARSHAL) --prefix bst_marshal --header beast-gtk/bstmarshal.list		> $@.tmp
	$Q mv $@.tmp $@
$>/beast-gtk/bstmarshal.cc: beast-gtk/bstmarshal.list			| $>/beast-gtk/
	$(QGEN)
	$Q $(GLIB_GENMARSHAL) --prefix bst_marshal --body   beast-gtk/bstmarshal.list		> $@.tmp
	$Q mv $@.tmp $@

# == bstapi.idl ==
beast-gtk/bstapi.idl.outputs ::= $>/beast-gtk/bstapi_interfaces.hh $>/beast-gtk/bstapi_interfaces.cc $>/beast-gtk/bstapi_handles.hh $>/beast-gtk/bstapi_handles.cc
$(call MULTIOUTPUT, $(beast-gtk/bstapi.idl.outputs)): beast-gtk/bstapi.idl	$(aidacc/aidacc) bse/AuxTypes.py	| $>/beast-gtk/
	$(QECHO) GEN $(beast-gtk/bstapi.idl.outputs) # aidacc generates %_interfaces.{hh|cc} %_handles.{hh|cc} from %.idl, and the real MULTIOUTPUT target name looks wierd
	$Q cp $< $>/beast-gtk/
	$Q cd $>/beast-gtk/ && \
	  $(abspath $(aidacc/aidacc)) -x CxxStub -x $(abspath bse/AuxTypes.py) -G strip-path=$(abspath $>)/ $(<F)
	$Q cd $>/beast-gtk/ && \
	  sed '1i#define _(x) x' -i bstapi_interfaces.cc && sed '1i#undef _' -i bstapi_interfaces.cc

# == bstoldbseapi.h ==
$>/beast-gtk/bstoldbseapi.h:  $(wildcard bse/*.idl) $>/bse/bsehack.idl					|$>/beast-gtk/	# $(sfi/sfidl)
	$(QGEN)
	$Q $(sfi/sfidl) $(sfi/sfidl.includes) -I$>/bse --client-c --header --prefix beast_ bse/bse.idl	> $@.tmp
	$Q mv $@.tmp $@
$>/beast-gtk/bstoldbseapi.cc: $(wildcard bse/*.idl) $>/bse/bsehack.idl $>/beast-gtk/bstoldbseapi.h	|$>/beast-gtk/	# $(sfi/sfidl)
	$(QGEN)
	$Q echo -e "/* #include \"beast-gtk/bstoldbseapi.h\" */\n"					> $@.tmp
	$Q $(sfi/sfidl) $(sfi/sfidl.includes) -I$>/bse --client-c --source --prefix beast_ bse/bse.idl	>>$@.tmp
	$Q mv $@.tmp $@
