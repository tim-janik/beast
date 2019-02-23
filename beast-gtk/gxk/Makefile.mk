# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/beast-gtk/gxk/*.d)
CLEANDIRS += $(wildcard $>/beast-gtk/gxk/)
beast-gtk/gxk/rpath..bse ::= ../../bse

# == gxk/ files ==
beast-gtk/gxk/libgxk.sources ::= $(strip	\
	beast-gtk/gxk/glewidgets.c		\
	beast-gtk/gxk/gxkaction.cc		\
	beast-gtk/gxk/gxkassortment.cc		\
	beast-gtk/gxk/gxkauxwidgets.cc		\
	beast-gtk/gxk/gxkcanvas.cc		\
	beast-gtk/gxk/gxkcellrendererpopup.cc	\
	beast-gtk/gxk/gxkcompat.cc		\
	beast-gtk/gxk/gxkdialog.cc		\
	beast-gtk/gxk/gxkglobals.cc		\
	beast-gtk/gxk/gxkimagecache.cc		\
	beast-gtk/gxk/gxkled.cc			\
	beast-gtk/gxk/gxklistwrapper.cc		\
	beast-gtk/gxk/gxklogadjustment.cc	\
	beast-gtk/gxk/gxkmenubutton.cc		\
	beast-gtk/gxk/gxknotebook.cc		\
	beast-gtk/gxk/gxkparam.cc		\
	beast-gtk/gxk/gxkpolygon.cc		\
	beast-gtk/gxk/gxkrackeditor.cc		\
	beast-gtk/gxk/gxkrackitem.cc		\
	beast-gtk/gxk/gxkracktable.cc		\
	beast-gtk/gxk/gxkradget.cc		\
	beast-gtk/gxk/gxkradgetfactory.cc	\
	beast-gtk/gxk/gxkscrollcanvas.cc	\
	beast-gtk/gxk/gxksimplelabel.cc 	\
	beast-gtk/gxk/gxkspline.cc		\
	beast-gtk/gxk/gxkstatusbar.cc		\
	beast-gtk/gxk/gxkstock.cc		\
	beast-gtk/gxk/gxktexttools.cc		\
	beast-gtk/gxk/gxkutils.cc		\
)
beast-gtk/gxk/libgxk.headers ::= $(strip	\
	beast-gtk/gxk/glewidgets.h		\
	beast-gtk/gxk/gxk.hh			\
	beast-gtk/gxk/gxkaction.hh		\
	beast-gtk/gxk/gxkassortment.hh		\
	beast-gtk/gxk/gxkauxwidgets.hh		\
	beast-gtk/gxk/gxkcanvas.hh		\
	beast-gtk/gxk/gxkcellrendererpopup.hh	\
	beast-gtk/gxk/gxkcompat.hh		\
	beast-gtk/gxk/gxkdialog.hh		\
	beast-gtk/gxk/gxkglobals.hh		\
	beast-gtk/gxk/gxkimagecache.hh		\
	beast-gtk/gxk/gxkled.hh			\
	beast-gtk/gxk/gxklistwrapper.hh		\
	beast-gtk/gxk/gxklogadjustment.hh	\
	beast-gtk/gxk/gxkmenubutton.hh		\
	beast-gtk/gxk/gxknotebook.hh		\
	beast-gtk/gxk/gxkparam.hh		\
	beast-gtk/gxk/gxkpolygon.hh		\
	beast-gtk/gxk/gxkrackeditor.hh		\
	beast-gtk/gxk/gxkrackitem.hh		\
	beast-gtk/gxk/gxkracktable.hh		\
	beast-gtk/gxk/gxkradget.hh		\
	beast-gtk/gxk/gxkradgetfactory.hh	\
	beast-gtk/gxk/gxkscrollcanvas.hh	\
	beast-gtk/gxk/gxksimplelabel.hh		\
	beast-gtk/gxk/gxkspline.hh		\
	beast-gtk/gxk/gxkstatusbar.hh		\
	beast-gtk/gxk/gxkstock.hh		\
	beast-gtk/gxk/gxktexttools.hh		\
	beast-gtk/gxk/gxkutils.hh		\
)
beast-gtk/gxk/libgxk.deps ::= $(strip		\
	$>/beast-gtk/gxk/gxkgentypes.h		\
	$>/beast-gtk/gxk/gxkmarshal.h		\
)
beast-gtk/gxk/libgxk.cc.deps ::= $(strip	\
	$>/beast-gtk/gxk/gxkgentypes.cc		\
	$>/beast-gtk/gxk/gxkmarshal.cc		\
)

# == libgxk defs ==
beast-gtk/gxk/libgxk.a	     ::= $>/beast-gtk/gxk/libgxk-$(VERSION_MAJOR).$(VERSION_MINOR).a
beast-gtk/gxk/libgxk.objects ::= $(call SUBST_O, $(beast-gtk/gxk/libgxk.sources:%=$>/%))

# == gxktest defs ==
beast-gtk/gxk/gxktest		::= $>/beast-gtk/gxk/gxktest
beast-gtk/gxk/gxktest.sources	::= beast-gtk/gxk/gxktest.cc
beast-gtk/gxk/gxktest.objects	::= $(sort $(beast-gtk/gxk/gxktest.sources:%.cc=$>/%.o))

# == splinetest defs ==
beast-gtk/gxk/splinetest	  ::= $>/beast-gtk/gxk/splinetest
beast-gtk/gxk/splinetest.sources  ::= beast-gtk/gxk/splinetest.cc
beast-gtk/gxk/splinetest.objects  ::= $(sort $(beast-gtk/gxk/splinetest.sources:%.cc=$>/%.o))

# == libgxk rules ==
$(beast-gtk/gxk/libgxk.objects): $(beast-gtk/gxk/libgxk.deps) $(beast-gtk/gxk/libgxk.cc.deps)
$(beast-gtk/gxk/libgxk.objects): EXTRA_INCLUDES ::= -I$> -I$>/beast-gtk -Ibeast-gtk $(GTK_CFLAGS)
$(beast-gtk/gxk/libgxk.objects): EXTRA_DEFS ::= -DGXK_COMPILATION
$(call BUILD_STATIC_LIB, \
	$(beast-gtk/gxk/libgxk.a), \
	$(beast-gtk/gxk/libgxk.objects), \
	| $>/beast-gtk/gxk/)

# == gxktest rules ==
$(beast-gtk/gxk/gxktest.objects):	$(beast-gtk/gxk/libgxk.a)
$(beast-gtk/gxk/gxktest.objects):	EXTRA_INCLUDES ::= -I$> -I$>/beast-gtk -Ibeast-gtk $(GTK_CFLAGS)
$(call BUILD_PROGRAM, \
	$(beast-gtk/gxk/gxktest), \
	$(beast-gtk/gxk/gxktest.objects), \
	$(beast-gtk/gxk/libgxk.a) $(bse/libbse.so), \
	$(beast-gtk/gxk/libgxk.a) -lbse-$(VERSION_MAJOR) $(GTK_LIBS), \
	$(beast-gtk/gxk/rpath..bse))

# == splinetest rules ==
$(beast-gtk/gxk/splinetest.objects):	EXTRA_INCLUDES ::= -I$> -I$>/beast-gtk -Ibeast-gtk $(GTK_CFLAGS)
$(call BUILD_PROGRAM, \
	$(beast-gtk/gxk/splinetest), \
	$(beast-gtk/gxk/splinetest.objects), \
	$(beast-gtk/gxk/libgxk.a) $(bse/libbse.so), \
	$(beast-gtk/gxk/libgxk.a) -lbse-$(VERSION_MAJOR) $(GTK_LIBS), \
	$(beast-gtk/gxk/rpath..bse))

# == code generation ==
$>/beast-gtk/gxk/gxkgentypes.h: $(beast-gtk/gxk/libgxk.headers)			| $>/beast-gtk/gxk/
	$(QGEN)
	$Q $(GLIB_MKENUMS)	--fprod "\n/* --- @filename@ --- */" \
				--eprod "#define GXK_TYPE_@ENUMSHORT@\t    (gxk__type_id__@EnumName@)\n" \
				--eprod "extern GType gxk__type_id__@EnumName@;" \
				$(beast-gtk/gxk/libgxk.headers)		> $@.tmp
	$Q mv $@.tmp $@
$>/beast-gtk/gxk/gxkgentypes.cc: $(beast-gtk/gxk/libgxk.headers)		| $>/beast-gtk/gxk/
	$(QGEN)
	$Q $(GLIB_MKENUMS)	--fprod "\n/* --- @filename@ --- */\n" \
				--fprod "#include\t\"@filename@\"" \
				--vhead "\nGType gxk__type_id__@EnumName@ = 0;" \
				--vhead "\nstatic G@Type@Value @enum_name@_values[] = {" \
				--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
				--vtail "  { 0, NULL, NULL }\n};\n" \
				$(beast-gtk/gxk/libgxk.headers)		> $@.tmp
	$Q $(GLIB_MKENUMS)	--fhead "static const GxkTypeGenerated generated_type_entries[] = {" \
				--fprod "\n/* --- @filename@ --- */" \
				--eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &gxk__type_id__@EnumName@, @enum_name@_values }," \
				--ftail "\n};" \
				$(beast-gtk/gxk/libgxk.headers)		>>$@.tmp
	$Q mv $@.tmp $@
$>/beast-gtk/gxk/gxkmarshal.h: beast-gtk/gxk/gxkmarshal.list			| $>/beast-gtk/gxk/
	$(QGEN)
	$Q $(GLIB_GENMARSHAL) --prefix gxk_marshal --header beast-gtk/gxk/gxkmarshal.list	> $@.tmp
	$Q echo '#define gxk_marshal_VOID__POINTER      g_cclosure_marshal_VOID__POINTER'	>>$@.tmp
	$Q echo '#define gxk_marshal_VOID__UINT_POINTER g_cclosure_marshal_VOID__UINT_POINTER'	>>$@.tmp
	$Q echo '#define gxk_marshal_VOID__BOXED        g_cclosure_marshal_VOID__BOXED'		>>$@.tmp
	$Q echo '#define gxk_marshal_VOID__BOOLEAN      g_cclosure_marshal_VOID__BOOLEAN'	>>$@.tmp
	$Q echo '#define gxk_marshal_VOID__STRING       g_cclosure_marshal_VOID__STRING'	>>$@.tmp
	$Q echo '#define gxk_marshal_VOID__INT          g_cclosure_marshal_VOID__INT'		>>$@.tmp
	$Q echo '#define gxk_marshal_VOID__UINT         g_cclosure_marshal_VOID__UINT'		>>$@.tmp
	$Q mv $@.tmp $@
$>/beast-gtk/gxk/gxkmarshal.cc: beast-gtk/gxk/gxkmarshal.list			| $>/beast-gtk/gxk/
	$(QGEN)
	$Q $(GLIB_GENMARSHAL) --prefix gxk_marshal --body beast-gtk/gxk/gxkmarshal.list		> $@.tmp
	$Q mv $@.tmp $@
