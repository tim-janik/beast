include $(top_srcdir)/Makefile.decl

AM_CXXFLAGS += $(SPECTMORPHGLUI_CFLAGS) $(SPECTMORPH_CFLAGS) $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DBSE_CONVENIENCE

bin_PROGRAMS = spectmorphoscgui

spectmorphoscgui_SOURCES = spectmorphoscgui.cc spectmorphoscgui.hh
spectmorphoscgui_LDADD   = ../../../bse/libbse-@MAJOR@.la $(LIBBSE_LIBS) $(SPECTMORPH_LIBS) $(SPECTMORPHGLUI_LIBS)
