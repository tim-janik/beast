BEAST & BSE
===========

This is the development version of Beast & Bse. BEAST is the BSE
Equipped Audio Synthesizer and Tracker, and BSE is the Beast Sound Engine.
Beast is a music composition and modular synthesis application, that is
released as Free Software under the GNU LGPL and runs under Unix.

Beast is a soft realtime application which needs elevated CPU scheduling
priorities to avoid audio glitches and drop outs. For this purpose, it
installs a small uid wrapper which acquires nice level -20 for the
synthesis threads and then immediately drops privileges.
For Linux kernels of the 2.6.x series and later, this enables the
low-latency scheduling behavior needed to avoid audio artefacts,
this kernel series is therefore recommended for operating Beast.

In order to build release tarballs, Rapicorn, the GnomeCanvas,
Guile, the Ogg/Vorbis Codec libraries and libflac are required.
Support for MP3 files is optional and requires libmad (MPEG audio
decoder library) when compiling Beast.

The web site, download and forums for Beast are:
  http://beast.testbit.eu/
  http://beast.testbit.eu/beast-ftp/
  http://mail.gnome.org/mailman/listinfo/beast/ (beast@gnome.org)
  #beast on GimpNet (irc.gimp.org:6666)

Beast also supports the LADSPA plugin API, so various kinds of third
party plugins can be loaded and executed by Beast. LADSPA plugins are
available from:
	http://www.ladspa.org
Beast will look for LADSPA plugins under lib/ladspa/ inside it's own
installation prefix, and in the directories listed in $LADSPA_PATH.


Installation
============

Note that Beast has to be fully installed to function properly, and that
the binaries built in the launchers/ directory have to be installed with
permissions of the root user.
For generic installation instructions, see the file 'INSTALL'.
