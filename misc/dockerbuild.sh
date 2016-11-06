#!/bin/bash
set -e

# setup
SCRIPTNAME="$(basename "$0")" ; die() { e="$1"; shift; echo "$SCRIPTNAME: $*" >&2; exit "$e"; }
SCRIPTDIR="$(dirname "$(readlink -f "$0")")"
PROJECT=`basename "$(git rev-parse --show-toplevel)"`

# parse args
test $# -gt 0 || die 1 "Usage: dockerbuild.sh <DIST> [INTENT]"
DIST=debian:jessie
INTENT=distcheck
test $# -lt 1 || { DIST="$1"; shift; }
test $# -lt 1 || { INTENT="$1"; shift; }
DISTRELEASE="${DIST#*:}"

# determine bintray repo for packaging
case `$SCRIPTDIR/mkbuildid.sh -p` in			# similar to 'git describe'
  *-g*)         BINTRAY_REPO=beast-team/devel ;;        # work in progress
  *)            BINTRAY_REPO=beast-team/deb ;;          # on release tag
esac

# configure Dockerfile
export PROJECT DIST INTENT DISTRELEASE BINTRAY_REPO TRAVIS_JOB_NUMBER
misc/applyenv.sh misc/Dockerfile-apt.in > Dockerfile

# provide a full .git clone (which might be a worktree pointer)
rm -rf tmp-bintray-token ./tmp-mirror.git/
git clone --mirror .git tmp-mirror.git

# store BINTRAY_APITOKEN for the docker image, while this is still recorded
# in images, it's not automatically spilled into all RUN environments
(umask 0377 && echo "$BINTRAY_APITOKEN" >tmp-bintray-token)

# forward http_proxy if set
BUILD_ARG_HTTP_PROXY=
test -z "$http_proxy" || BUILD_ARG_HTTP_PROXY="--build-arg=http_proxy=$http_proxy"

# build project in docker container
docker build $BUILD_ARG_HTTP_PROXY -t $PROJECT .
echo -e "OK, EXAMINE:\n  docker run -ti --rm $PROJECT /bin/bash"

# cleanup
rm -rf tmp-bintray-token ./tmp-mirror.git/
