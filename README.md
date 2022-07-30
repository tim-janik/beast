BEAST & BSE
===========

[![License LGPL-2.1+](https://testbit.eu/~timj/pics/license-lgpl-2-1+.svg)](https://github.com/tim-janik/beast/blob/master/COPYING)
[![Chat](https://testbit.eu/~timj/pics/chat-beast-f7b.svg)](https://chat.mibbit.com/?server=irc.gimp.org&channel=%23beast)
[![Build Status](https://travis-ci.org/tim-janik/beast.svg)](https://travis-ci.org/tim-janik/beast)
[![Download](https://api.bintray.com/packages/beast-team/testing/Beast-AppImage/images/download.svg)](https://bintray.com/beast-team/testing/Beast-AppImage/_latestVersion)


# DISCONTINUED

Further Beast development has been halted, our development efforts now concentrate on [Anklang](github.com/tim-janik/anklang/) which is in many way a modern and better reimplementation of the Beast feature set. Modules and functionality of Beast are incrementally integrated into Anklang.

# DESCRIPTION

Beast is a digital synthesizer and music creation system. It has support
for Linux Audio Plugins
([LADSPA](http://wikipedia.org/wiki/LADSPA)),
multi-track editing, unlimited undo, real-time synthesis, MIDI and various
free audio codecs.
Bse is the Beast Sound Engine, a library providing the synthesis and
audio functions used by Beast.

* For a full description, visit the project page:
	http://beast.testbit.eu

* To submit bug reports and feature requests, visit:
	https://github.com/tim-janik/beast/issues


# REQUIREMENTS

Beast is a soft realtime application which needs elevated CPU scheduling
priorities to avoid audio glitches and drop outs. For this purpose, it
installs a small uid wrapper which acquires nice level -20 for the
synthesis threads and then immediately drops privileges.
For Linux kernels of the 2.6.x series and later, this enables the
low-latency scheduling behavior needed to avoid audio artefacts.

In order to build release tarballs, `GnomeCanvas`, `Ogg/Vorbis`,
`libflac` and `npm` and `libfluidsynth` are required.
Support for MP3 files is optional and requires `libmad` (MPEG audio
decoder library) when compiling Beast.
Compilation requires at least `g++-8.3.0` or `clang++-9` and a recent
Linux distribution like Ubuntu-18.04.


# INSTALLATION

Beast needs GNU Make to build under Linux:

	make			# build all components
	make check		# run unit and audio tests
	make install		# install under $(prefix)
	make installcheck	# run installation tests

The build can be configured with:

	make default prefix=... CXX=... CC=... CXXFLAGS=...
	make help		# brief description about build targets

Note that some Beast versions provide a realtime scheduling wrapper in
the launchers/ directory which has to be installed with permissions of
the root user to work properly. Realtime scheduling priority helps to
avoid drop outs during audio playback.


# BINARY PACKAGES

New source code pushed to the Beast repository is automatically built
and tested via Travis-CI. Ever so often we create release candidate
packages and later release packages after a stabilization phase.
Some of the release candidates and the stable versions are provided
as binary packages. Refer to the website for download links:

	https://beast.testbit.org/#downloads
