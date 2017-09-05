include $(top_srcdir)/Makefile.decl

# FIXME: this is probably not the right place for -fPIC

AM_CXXFLAGS += $(SPECTMORPHGUI_CFLAGS) $(SPECTMORPH_CFLAGS) $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE -DQT_NO_EMIT -fPIC

bin_PROGRAMS = spectmorphoscgui

spectmorphoscgui_SOURCES = spectmorphoscgui.cc moc_spectmorphoscgui.cc spectmorphoscgui.hh
spectmorphoscgui_LDADD   = ../../../bse/libbse-@MAJOR@.la $(LIBBSE_LIBS) $(SPECTMORPH_LIBS) $(SPECTMORPHGUI_LIBS)

moc_spectmorphoscgui.cc: spectmorphoscgui.hh
	$(QT5_MOC) spectmorphoscgui.hh > moc_spectmorphoscgui.cc
