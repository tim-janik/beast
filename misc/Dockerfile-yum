# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == Distribution preparation ==
ARG DIST
FROM $DIST

# Provide /bin/retry
RUN \
  echo -e '#!/bin/bash\n"$@" || { sleep 15 ; "$@" ; } || { sleep 90 ; "$@" ; }' > /bin/retry && chmod +x /bin/retry

# Upgrade packages
RUN retry yum -y upgrade

# Install dependencies
# electron needs libXss.so.1 and libgconf-2.so.4
RUN retry yum -y install \
    findutils file bzip2 \
    gcc-c++ git libtool gettext-devel perl-XML-Parser \
    alsa-lib-devel flac-devel libgnomecanvas-devel libvorbis-devel libmad-devel \
    pandoc python3-pandocfilters doxygen graphviz \
    npm ImageMagick libXScrnSaver GConf2 \
    libsndfile-devel cmake

# Build stripped down fluidsynth version without drivers
RUN mkdir -p /tmp/fluid/build && cd /tmp/fluid/ && \
  curl -sfSOL https://github.com/FluidSynth/fluidsynth/archive/v2.0.4.tar.gz && \
  tar xf v2.0.4.tar.gz && rm v2.0.4.tar.gz && cd build && \
  cmake ../fluidsynth-2.0.4 \
	-DCMAKE_INSTALL_PREFIX=/usr/ \
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
  make && make install && cd / && rm -rf /tmp/fluid/

# Setup build environment and provide the git repositories
COPY ./.git/ /tmp/beast.git
RUN \
  mkdir -p /opt/src/ && \
  cd /opt/src/ && \
  git clone /tmp/beast.git
WORKDIR /opt/src/beast

# Configure source tree, possibly using compiler wrapper
RUN make default prefix=/opt

# Build sources in parallel
RUN nice make -j`nproc`

# Validate Build
RUN make check
RUN make install
RUN make installcheck
RUN make dist
RUN make uninstall

# docker build --build-arg DIST=fedora:27 -f misc/Dockerfile-yum -t beast-fedora-27 .
