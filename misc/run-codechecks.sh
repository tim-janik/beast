#!/bin/bash
set -Eeuo pipefail

SCRIPTNAME=${0#*/} ; die() { [ -z "$*" ] || echo "$SCRIPTNAME: $*" >&2; exit 127 ; }

# beast/ need sto be CWD
test -e bse/bsemain.cc || die "script needs to run in beast/ root"

# base image, TODO: test smaller distributions
DIST=ubuntu:bionic
INTENT=cppcheck,scan-build,clang-tidy,unused,fi\xmes

# input files for clang-tidy to ignore
cat >.ls.ignore << \__EOF__
aidacc/CxxStub-.*\.cc
beast-gtk/bstparam-.*\.cc
beast-gtk/gxk/gxkparam-.*\.cc
beast-gtk/gxk/gxkrackcovers\.cc
bse/bseapi-inserts.hh
bse/bsegenclosures.hh
bse/bseincluder\.hh
bse/bsepcmmodule\.cc
bse/gslincluder\.hh
bse/gsloscillator-aux\.cc
bse/gslwaveosc-aux\.cc
bse/tests/filtercatalog\.cc
drivers/bse-portaudio/bsepcmdevice-portaudio\.cc
plugins/bsefirfilter\.hh
plugins/bsesimpleadsr-aux\.cc
plugins/evaluator/.*\.cc
plugins/evaluator/.*\.hh
sfi/gbsearcharray.hh
sfi/private.hh
sfi/sfidl-.*\.cc
__EOF__

# Dockerfile to run various llvm based checks on the beast sources
sed "s/@DIST@/$DIST/g ; s/@INTENT@/$INTENT/g" >Dockerfile << \__EOF__
FROM @DIST@

# use BASH(1) as shell
RUN ln -sf bash /bin/sh && ls -al /bin/sh

# provide 'retry', 'intent' and 'wrapclang'
RUN echo -e '#!/bin/bash\n"$@" || { sleep 10 ; "$@" ; } || { sleep 90 ; "$@" ; }' > /bin/retry && \
    echo -e '#!/bin/sh\ncase "$INTENT" in *$1*) echo true ;; *) echo "exit 0" ;; esac' > /bin/intent && \
    echo -e '#!/bin/bash\nexec $WRAPPER ${0##*/wrap} "$@"' > /bin/wrapclang && ln -s wrapclang /bin/wrapclang++ && \
    chmod +x /bin/retry /bin/intent /bin/wrapclang /bin/wrapclang++

# fetch updates and build-essential
RUN retry apt-get update && retry apt-get -y upgrade && retry apt-get install -y clang clang-tidy clang-tools cppcheck \
    build-essential devscripts lintian automake autoconf autoconf-archive libtool intltool \
    doxygen graphviz texlive-binaries pandoc git libxml2-utils \
    \
    gawk libxml-parser-perl \
    libpango1.0-dev python2.7-dev libxml2-dev libreadline6-dev libreadline-dev \
    libasound2-dev libflac-dev libvorbis-dev libmad0-dev libgnomecanvas2-dev libfluidsynth-dev \
    imagemagick npm libgconf-2-4 nodejs \
    \
    bison flex cython xvfb \
    libpango1.0-dev python2.7-dev python-all-dev python-enum34 \
    \
    python3

# prepare /usr/src/beast
COPY ./.git/ /tmp/beast.git
RUN cd /usr/src/ && git clone /tmp/beast.git
WORKDIR /usr/src/beast
RUN mkdir ./codechecks/
COPY ./.ls.ignore ./

# determine source file set
RUN git ls-tree -r --name-only HEAD >.ls.all && \
    egrep '^(beast-gtk|bse|drivers|ebeast|plugins|sfi)/.*\.(cc|hh)$' <.ls.all >.ls.cchh && \
    egrep '^(beast-gtk|bse|drivers|ebeast|plugins|sfi)/.*\.(js|vue)$' <.ls.all >.ls.jsvue

# configure source tree, possibly using compiler wrapper
RUN ./autogen.sh --prefix=/usr CC=wrapclang CXX=wrapclang++

ENV INTENT "@INTENT@"

# regular cppcheck run doesn't need any built sources
RUN `intent cppcheck` && \
    misc/run-cppcheck.sh && \
    mv -v cppcheck.err ./codechecks/cppcheck.log

# build source tree, use scan-build as compiler wrapper if intended
RUN `intent scan-build` && \
    export WRAPPER='scan-build -o /usr/src/beast/codechecks/html/ -disable-checker deadcode.DeadStores' && \
    nice make -j`nproc`

# build index of scan-build reports (strip top_srcdir)
RUN test -e ./codechecks/html/ || exit 0; \
    for r in ./codechecks/html/*/report-*.html ; do \
      D=$(sed -nr '/<!-- BUGDESC/ { s/^<!-- \w+ (.+) -->/\1/ ; p }' $r) && \
      F=$(sed -nr '/<!-- BUGFILE/ { s/^<!-- \w+ ([^ ]+) -->/\1/ ; p }' $r) && \
      L=$(sed -nr '/<!-- BUGLINE/ { s/^<!-- \w+ ([^ ]+) -->/\1/ ; p }' $r) && \
      echo "$F:$L: $D" | sed 's,^/usr/src/beast/,,' ; \
    done >./codechecks/scan-build.log

# with all generated sources in place, run clang-tidy if intended (strip top_srcdir)
RUN `intent clang-tidy` && \
    clang-tidy `egrep -vf .ls.ignore .ls.cchh` -- \
	-std=gnu++14 \
	-I. \
	-Isfi \
	-Ibeast-gtk \
	-DBSE_COMPILATION \
	-DGXK_COMPILATION \
	-D__TOPDIR__=\"`pwd`\" \
	`pkg-config --cflags libgnomecanvas-2.0` \
	>./codechecks/clang-tidy.raw && \
    sed 's,^/usr/src/beast/,,' ./codechecks/clang-tidy.raw >./codechecks/clang-tidy.log && \
    rm -f ./codechecks/clang-tidy.raw

# with all generated sources in place, use cppcheck to find unused functions
RUN `intent unused` && \
    misc/run-cppcheck.sh -u && \
    mv -v cppcheck.err ./codechecks/unused.log

# with all generated sources in place, use cppcheck to find unused functions
RUN `intent fi\xmes` && \
    misc/keywords.sh -g $(cat .ls.cchh .ls.jsvue) >./codechecks/fi\xmes.log

# blame error origins
RUN misc/blame-lines -b codechecks/*.log

__EOF__

# build container and run checks
docker build -t beast-codechecks .

# extract reports
rm -fr ./codechecks/
mkdir -p codechecks/
docker run -v `pwd`/codechecks/:/codechecks -ti --rm beast-codechecks bash -c "chown $UID.$UID -R ./codechecks/ && cp -a ./codechecks/* /codechecks/"

# show report
ls -ald codechecks/*
wc codechecks/*.*
