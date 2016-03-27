#!/bin/bash
# Copyright (C) 2015-2016 Tim Janik, MPL-2.0

set -e # abort on errors
SCRIPTNAME=`basename $0`;
function die { e="$1" ; shift ; [ -n "$*" ] && echo "$SCRIPTNAME: $*" >&2 ; exit "${e:-127}" ; } # die <exitcode> [message]

# == config ==
mkconfig() # print shell variables describing package, version, commit id, monotonic revision
{
  set -e # abort on errors
  REVISIONSUFFIX="$1" # may be empty
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
  test -z "$UPSVERSION" &&
    UPSVERSION=`sed -nr "/^AC_INIT\b/{ s/^[^,]*,[^0-9]*([0-9.]*).*/\1/; p; }" configure.ac`
  [[ $UPSVERSION =~ ^[0-9.]+$ ]] || die 6 "failed to detect package version"
  [[ $UPSVERSION =~ [13579]$ ]] && DEVELOPMENT=true || DEVELOPMENT=false # spot unreleased odd devel versions
  # commit ID and changelog message
  COMMITID=`git rev-parse HEAD`
  if $DEVELOPMENT ; then
    CHANGELOGMSG="Development snapshot, git commit $COMMITID"
  else
    CHANGELOGMSG="Release snapshot, git commit $COMMITID"
  fi
  # upstream version details
  TOTAL_COMMITS=`git rev-list --count HEAD` # count commits to provide a monotonically increasing revision
  if $DEVELOPMENT ; then
    UPSDETAIL="~git$TOTAL_COMMITS" # sort *before* UPSDETAIL="" (pre-release candidates)
  else
    UPSDETAIL="+git$TOTAL_COMMITS" # sort *after* UPSDETAIL="" (post release patchlevel)
  fi
  # build revision parts
  REVISIONSUFFIX="-${REVISIONSUFFIX:-0.1local}" # avoid non-native-package-with-native-version
  test -z "$TRAVIS_JOB_NUMBER" || REVISIONSUFFIX="$REVISIONSUFFIX~travis${TRAVIS_JOB_NUMBER/*./}"
  BUILDREV="$REVISIONSUFFIX"
  # complate deb package versioning
  DEBVERSION="$UPSVERSION$UPSDETAIL$BUILDREV"
  # print variables after all errors have been checked for
  cat <<-__EOF
	PACKAGE=$PACKAGE
	UPSVERSION=$UPSVERSION
	UPSDETAIL=$UPSDETAIL
	BUILDREV=$BUILDREV
	DEVELOPMENT=$DEVELOPMENT
	TOTAL_COMMITS=$TOTAL_COMMITS
	COMMITID=$COMMITID
	CHANGELOGMSG="$CHANGELOGMSG"
	DEBVERSION=$DEBVERSION
	__EOF
  popd >/dev/null					# cd OLDPWD
}

# == bintrayup ==
bintrayup() # Usage: bintrayup <bintrayaccount> <packagepath> <packagedistribution> [packages...]
{
  set +x # avoid printing authentication secrets
  mkconfig >/dev/null # PACKAGE, UPSVERSION, ...
  ACCNAME="$1"; PKGPATH="$2"; PKGDIST="$3" # BINTRAY_APITOKEN must be set by caller
  test -n "$ACCNAME" || die 1 "missing bintray account"
  test -n "$PKGPATH" || die 1 "missing package path"
  test -n "$PKGDIST" || die 1 "missing distribution"
  shift 3
  # create new bintray versoin
  REPOVERSION="$DEBVERSION" # echo "REPOVERSION=$REPOVERSION"
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

# == applyenv ==
applyenv() # Usage: applyenv <inputfile> [inputargs...]
{
  # NOTE: avoid using unprefixed environment variables, they could become output substitutions
  applyenv__input___="$1"; shift
  test -r "$applyenv__input___" || { echo "$0: failed to read: $applyenv__input___"; exit 1; }
  applyenv__sedprogram___=""
  for applyenv__word___ in \
    $(grep -oE '@[0123456789abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ]+@' "$applyenv__input___" | sort | uniq)
  do
    applyenv__varname___=${applyenv__word___//@/} # word=@EXAMPLE@ varname=EXAMPLE
    if [[ ${!applyenv__varname___} == *\|* ]]; then
      echo "$0: detected '|', skipping variable: \$$applyenv__varname___" >&2
    elif test "$applyenv__word___" == "@0@"; then
      applyenv__sedprogram___="$applyenv__sedprogram___; s|$applyenv__word___|${applyenv__input___}|g"
    else
      applyenv__sedprogram___="$applyenv__sedprogram___; s|$applyenv__word___|${!applyenv__varname___}|g"
    fi
  done
  sed "$applyenv__sedprogram___" "$applyenv__input___"
}


# == commands ==
[[ "$1" != config ]]	|| { shift; mkconfig "$@" ; exit $? ; }
[[ "$1" != bintrayup ]]	|| { shift; bintrayup "$@" ; exit $? ; }
[[ "$1" != applyenv ]]	|| { shift; applyenv "$@" ; exit $? ; }
