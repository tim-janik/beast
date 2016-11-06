#!/bin/bash
set -e

# setup
SCRIPTNAME="$(basename "$0")" ; die() { e="$1"; shift; echo "$SCRIPTNAME: $*" >&2; exit "$e"; }
SCRIPTDIR="$(dirname "$(readlink -f "$0")")"

# Usage: bintrayup <TOKENFILE> <DIST> <ACCOUNTPATH> [packages...]
TOKENFILE="$1"		# e.g. ~/.bintray-apitoken
DIST="$2"		# e.g. debian:jessie
ACCOUNTPATH="$3"	# e.g. beast-team/stable/beast
shift 3 || :
DISTRELEASE="${DIST#*:}"
ACCNAME="${ACCOUNTPATH%%/*}"
PKGPATH="${ACCOUNTPATH#*/}"
test -n "$DISTRELEASE" || die 1 "missing distribution"
test -n "$ACCNAME" -o -n "$PKGPATH" || die 1 "invalid bintray account path"
test -r "$TOKENFILE" || die 1 "inaccesible token file: $TOKENFILE"
test -n "$1" || die 1 "missing deb files"

# extract metainfo
DEB_INFO=`dpkg-deb -I "$1"`
DEBVERSION=`echo " $DEB_INFO" | sed -n '/^ Version: /{s/.*: //;p;q}'`
test -n "$DEBVERSION" || die 2 "failed to identify Debian version from package: $1"

# create new bintray versoin
REPOVERSION="$DEBVERSION"
echo "  REMOTE  " "creating new version: $REPOVERSION"
curl -d "{ \"name\": \"$REPOVERSION\", \"released\": \"`date -I`\", \"desc\": \"Automatic CI Build\" }" \
  -u"$ACCNAME:`cat "$TOKENFILE"`" "https://api.bintray.com/packages/$ACCNAME/$PKGPATH/versions" \
  -H"Content-Type: application/json" -f && EX=$? || EX=$?
test $EX = 0 -o $EX = 22 # 22 indicates HTTP responses >= 400, the version likely already exists
# upload individual files
# NOTE: we cannot use "explode=1" b/c files may have different architectures, which
# need to be passed corrctly in the upload to regenerate the Packages index.
URL="https://api.bintray.com/content/$ACCNAME/$PKGPATH/$REPOVERSION"
OPTS="deb_distribution=$DISTRELEASE;deb_component=main;explode=0;override=0;publish=1"
ALLOK=0
for F in "$@" ; do
  S="${F%.deb}"; A="${S##*_}"
  test ! -z "$A" || continue
  echo "  REMOTE  " "uploading: $F ($A)"
  curl -T "$F" -u"$ACCNAME:`cat "$TOKENFILE"`" "$URL/`basename $F`;$OPTS;deb_architecture=$A" -f && EX=$? || EX=$?
  ALLOK=$(($ALLOK + $EX))
done
test $ALLOK = 0 || die 2 "Some files failed to upload"
