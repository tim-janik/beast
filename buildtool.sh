#!/bin/bash
# Copyright (C) 2015-2016 Tim Janik, MPL-2.0

set -e # abort on errors
SCRIPTNAME=`basename $0`;
function die { e="$1" ; shift ; [ -n "$*" ] && echo "$SCRIPTNAME: $*" >&2 ; exit "${e:-127}" ; } # die <exitcode> [message]

# == config ==
mkconfig()	# print shell variables describing package, version, commit id, monotonic revision
{
  set -e # abort on errors
  # ensure we're in the project directory and have full git history
  SCRIPTPATH=`readlink -f $0`
  SCRIPTDIR=`dirname "$SCRIPTPATH"`
  pushd "$SCRIPTDIR" >/dev/null 			# cd PACKAGE/
  gitdir=`git rev-parse --git-dir`
  test ! -s "$gitdir/shallow" || die 7 "missing history, run: git fetch --unshallow"
  # extract configure.ac:AC_INIT package and version
  test -r configure.ac || die 7 "missing configure.ac"
  test -z "$PACKAGE" &&
    PACKAGE=`sed -nr "/^AC_INIT\b/{ s/^[^(]*\([ 	]*([^,	 ]+).*/\1/; s/\[|]//g; p; }" configure.ac`
  test -z "$VERSION" &&
    VERSION=`sed -nr "/^AC_INIT\b/{ s/^[^,]*,[^0-9]*([0-9.]*).*/\1/; p; }" configure.ac`
  [[ $VERSION =~ ^[0-9.]+$ ]] || die 6 "failed to detect package version"
  # gather git bits
  TOTAL_COMMITS=`git rev-list --count HEAD` # count commits to provide a monotonically increasing revision
  REVISIONSUFFIX=
  test -z "$TRAVIS_JOB_NUMBER" || REVISIONSUFFIX="-0travis${TRAVIS_JOB_NUMBER/*./}"
  test -z "$BUILDREVISION" || REVISIONSUFFIX="-${BUILDREVISION}"
  test -n "$REVISIONSUFFIX" || REVISIONSUFFIX="-0anon" # avoid non-native-package-with-native-version
  VCSREVISION="+git$TOTAL_COMMITS$REVISIONSUFFIX"
  COMMITID=`git rev-parse HEAD`
  CHANGELOGMSG="Automatic snapshot, git commit $COMMITID"
  # print variables after all errors have been checked for
  cat <<-__EOF
	PACKAGE=$PACKAGE
	VERSION=$VERSION
	TOTAL_COMMITS=$TOTAL_COMMITS
	VCSREVISION=$VCSREVISION
	COMMITID=$COMMITID
	CHANGELOGMSG="$CHANGELOGMSG"
	__EOF
  popd >/dev/null					# cd OLDPWD
}


# == bintrayup ==
bintrayup() {
  set +x # avoid printing authentication secrets
  mkconfig >/dev/null # PACKAGE, VERSION, ...
  ACCNAME="$1"; PKGPATH="$2"; PKGDIST="$3" # BINTRAY_APITOKEN must be set by caller
  test -n "$ACCNAME" || die 1 "missing bintray account"
  test -n "$PKGPATH" || die 1 "missing package path"
  test -n "$PKGDIST" || die 1 "missing distribution"
  shift 3
  # create new bintray versoin
  REPOVERSION="$VERSION+git$TOTAL_COMMITS" # echo "REPOVERSION=$REPOVERSION"
  echo "  REMOTE  " "creating new version: $REPOVERSION"
  curl -d "{ \"name\": \"$REPOVERSION\", \"released\": \"`date -I`\", \"desc\": \"Automatic CI Build\" }" \
    -u"$ACCNAME:$BINTRAY_APITOKEN" "https://api.bintray.com/packages/$ACCNAME/$PKGPATH/versions" \
    -H"Content-Type: application/json" -f && EX=$? || EX=$?
  test $EX = 0 -o $EX = 22 # 22 indicates HTTP responses >= 400, the version likely already exists
  # upload individual files
  # NOTE: we cannot use "explode=1" b/c files may have different architectures, which
  # need to be passed corrctly in the upload to regenerate the Packages index.
  URL="https://api.bintray.com/content/$ACCNAME/$PKGPATH/$REPOVERSION"
  OPTS="deb_distribution=$PKGDIST;deb_component=main;explode=0;override=0;publish=1"
  ALLOK=0
  for F in "$@" ; do
    S="${F%.deb}"; A="${S##*_}"
    test ! -z "$A" || continue
    echo "  REMOTE  " "uploading: $F ($A)"
    curl -T "$F" -u"$ACCNAME:$BINTRAY_APITOKEN" "$URL/`basename $F`;$OPTS;deb_architecture=$A" -f && EX=$? || EX=$?
    ALLOK=$(($ALLOK + $EX))
  done
  test $ALLOK = 0 || die 2 "Some files failed to upload"
}

# == commands ==
[[ "$1" != config ]]	|| { shift; mkconfig "$@" ; exit $? ; }
[[ "$1" != bintrayup ]]	|| { shift; bintrayup "$@" ; exit $? ; }
