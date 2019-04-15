# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == Distribution preparation ==
FROM ubuntu:bionic
ENV DEBIAN_FRONTEND noninteractive

# Use BASH(1) as shell, affects the RUN commands below
RUN ln -sf bash /bin/sh && ls -al /bin/sh

# Keep all steps + cleanups in one layer to reduce the resulting size
RUN \
  \
  echo '## Provide /bin/retry' && \
  echo -e '#!/bin/bash\n"$@" || { sleep 15 ; "$@" ; } || { sleep 90 ; "$@" ; }' > /bin/retry && chmod +x /bin/retry && \
  \
  echo '## Upgrade packages' && \
  retry apt-get update && retry apt-get -y upgrade && \
  \
  echo '## Build stripped down fluidsynth version without drivers' && \
  retry apt-get install -y build-essential curl cmake libglib2.0-dev && \
  mkdir -p /tmp/fluid/build && cd /tmp/fluid/ && \
  curl -sfSOL https://github.com/FluidSynth/fluidsynth/archive/v2.0.4.tar.gz && \
  tar xf v2.0.4.tar.gz && rm v2.0.4.tar.gz && cd build && \
  cmake ../fluidsynth-2.0.4 \
	-DCMAKE_INSTALL_PREFIX=/usr/ \
	-DLIB_SUFFIX="/`dpkg-architecture -qDEB_HOST_MULTIARCH`" \
	-Denable-libsndfile=1 \
	-Denable-dbus=0 \
	-Denable-ipv6=0 \
	-Denable-jack=0 \
	-Denable-midishare=0 \
	-Denable-network=0 \
	-Denable-oss=0 \
	-Denable-pulseaudio=0 \
	-Denable-readline=0 \
	-Denable-alsa=0 \
	-Denable-lash=0 && \
  make && make install && cd / && rm -rf /tmp/fluid/ && \
  apt-get --purge remove -y cmake && \
  \
  echo '## Provide Beast dependencies, add libXss.so for electron' && \
  retry apt-get install -y \
	build-essential automake autoconf autoconf-archive libtool \
	curl doxygen graphviz texlive-binaries git libxml2-utils \
	cppcheck clang clang-tidy clang-tools python3-pandocfilters \
	gawk python2.7-dev python3 \
	libxml2-dev libpango1.0-dev libgnomecanvas2-dev \
	libasound2-dev libflac-dev libvorbis-dev libmad0-dev libgnomecanvas2-dev \
	imagemagick libgconf-2-4 nodejs npm libxss1 && \
  \
  echo '## Provide recent Pandoc' && \
  retry curl -ROL 'https://github.com/jgm/pandoc/releases/download/2.7.2/pandoc-2.7.2-1-amd64.deb' && \
  dpkg -i pandoc-2.7.2-1-amd64.deb && \
  rm pandoc-2.7.2-1-amd64.deb && \
  \
  echo '## Pre-fetch electron download' && \
  mkdir -p /root/.electron/ && cd /root/.electron/ && \
  curl -ROL 'https://github.com/electron/electron/releases/download/v2.0.18/electron-v2.0.18-linux-x64.zip' && \
  \
  echo '## Cleanup' && \
  apt-get --purge remove -y cmake python2.7-dev && \
  apt-get autoremove -y && \
  apt-get clean && rm -rf /var/lib/apt/lists/* && \
  :
