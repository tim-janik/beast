#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -Eeuo pipefail

SCRIPTNAME=${0#*/} ; die() { [ -z "$*" ] || echo "$SCRIPTNAME: $*" >&2; exit 9 ; }
# https://bintray.com/docs/api/

# Options and usage
DNLADD= KEEP= BINTRAY_PATH= DISTFILE= BINTRAY_VERSION="${BINTRAY_VERSION:-$(date -I)}"
while test $# -ne 0 ; do
  case "$1" in
    -d)			DNLADD=1 ;;
    -g)			BINTRAY_TAG=$(git rev-parse HEAD) || die "failed to find HEAD commit" ;
			BINTRAY_VERSION=$(git describe) || die "git failed to describe version" ;;
    -k)			shift ; KEEP="$1" ;;
    -v)			shift ; BINTRAY_VERSION="$1" ;;
    -h)                 echo "Usage: $SCRIPTNAME [-d] [-g] [-v VERSION] [-k N] owner/repo/package [DISTFILE]"
			echo "Upload DISTFILE to bintray.com at owner/repo/package"
			echo "  -h                    help message"
			echo "  -k N                  keep the highest N versions and prune the rest"
			echo "  -v VERSION            upload version for owner/repo/package"
			echo '  $AUTHSECRET_BINTRAY   authentication token for the bintray REST API'
			echo '  $BINTRAY_TAG          VCS tag (unless -g is given)'
			echo '  $BINTRAY_VERSION      upload version (unless -g/-v is given)'
			exit 0 ;;
    *)                  if   test -z "$BINTRAY_PATH" ; then BINTRAY_PATH="$1" ;
			elif test -z "$DISTFILE" ; then DISTFILE="$1" ;
			else die "unknown argument: '$1'" ;
			fi ;;
  esac
  shift
done
re="^([^/]+)/([^/]+)/([^/]+)$"
[[ "$BINTRAY_PATH" =~ $re ]] &&
  BINTRAY_USER="${BASH_REMATCH[1]}" &&
  BINTRAY_REPO="${BASH_REMATCH[2]}" &&
  BINTRAY_PACKAGE="${BASH_REMATCH[3]}" ||
    die "missing argument: owner/repo/package"
test -n "$BINTRAY_VERSION" || die "missing package version"
test -z "$DISTFILE" -o -e "$DISTFILE" || die "missing distribution file: '$DISTFILE'"

test -z "$DISTFILE" || echo "  INFO    " "$BINTRAY_USER / $BINTRAY_REPO / $BINTRAY_PACKAGE @ $BINTRAY_VERSION <- $DISTFILE"

# read AUTHSECRET_BINTRAY from ~/.secrets
test -n "${AUTHSECRET_BINTRAY:-}" || {
  AUTHSECRET_BINTRAY=$(sed -n '/^AUTHSECRET_BINTRAY=/{ s/^[^=]*=// ; p }' ~/.secrets) &&
    test -n "$AUTHSECRET_BINTRAY" ||
      die "missing Bintray authorization token"
}
CURL="curl -f -HAccept:application/json -u$BINTRAY_USER:$AUTHSECRET_BINTRAY"

# Create Version
DATA=$(cat <<EOF__
{
	"name":			"$BINTRAY_VERSION",
	"released":		"$(date -I)",
	"vcs_tag":		"${BINTRAY_TAG:-}"
}
EOF__
)

# Prune old versions to keep quota
test -z "$KEEP" || {
  URL="https://api.bintray.com/packages/$BINTRAY_USER/$BINTRAY_REPO/$BINTRAY_PACKAGE/"
  JSONRESPONSE=$($CURL -X GET -HContent-Type:application/json $URL -s) || die "failed to list old versions"
  ALLVERSIONS=( $(echo " $JSONRESPONSE" | sed -nr 's|.*"versions":\["([^]]*)"\].*|\1|p' | sed 's|","|\n|g') )
  SORTEDVERSIONS=( $(set -f && IFS=$'\n' && sort -rV <<<"${ALLVERSIONS[*]}") )
  for v in ${SORTEDVERSIONS[@]::$KEEP} ; do
    echo "  KEEP    " "$v"
  done
  for v in ${SORTEDVERSIONS[@]:$KEEP} ; do
    echo "  DELETE  " "$v"
    RET=$($CURL -X DELETE -HContent-Type:application/json "$URL/versions/$v" -s -w "%{http_code}" -o /dev/null) || :
    if test "${RET:0:2}" != 20 ; then
      echo "    FAIL  " "  $RET (deletion error)"
    else
      echo "      OK  " "  $RET (deleted)"
    fi
  done
}

# Delete existing version
false && {
  echo "  CLEAR   " "$BINTRAY_REPO $BINTRAY_PACKAGE $BINTRAY_VERSION"
  URL="https://api.bintray.com/packages/$BINTRAY_USER/$BINTRAY_REPO/$BINTRAY_PACKAGE/versions/$BINTRAY_VERSION"
  $CURL -sS -X DELETE -HContent-Type:application/json "$URL" -o /dev/null \
    && EX=$? || EX=$? ; test $EX = 0 -o $EX = 22 # may fail with HTTP responses >= 400
}

# Create version (yields "409 - Conflict" if existing)
test -n "$DISTFILE" && {
  echo "  CREATE  " "$BINTRAY_REPO $BINTRAY_PACKAGE $BINTRAY_VERSION"
  URL="https://api.bintray.com/packages/$BINTRAY_USER/$BINTRAY_REPO/$BINTRAY_PACKAGE/versions"
  RET=$($CURL -X POST -HContent-Type:application/json -d "$DATA" "$URL"  -s -w "%{http_code}" -o /dev/null) || :
  if test "${RET:0:3}" = 409 ; then
    echo "      OK  " "  $RET (version exists)"
  elif test "${RET:0:2}" = 20 ; then
    echo "      OK  " "  $RET (version created)"
  else
    echo "    FAIL  " "  $RET (not created)"
    exit 4	# Requested action not supported
  fi
}

# Upload file
test -n "$DISTFILE" && {
  URL="https://api.bintray.com/content/$BINTRAY_USER/$BINTRAY_REPO/$BINTRAY_PACKAGE/$BINTRAY_VERSION/${DISTFILE#*/}"
  echo "  UPLOAD  " "$BINTRAY_REPO $BINTRAY_PACKAGE $BINTRAY_VERSION: $DISTFILE"
  #CTYPE="-HContent-Type:application/octet-stream"
  $CURL -T "$DISTFILE" "$URL?publish=0&override=1" -o /dev/null # --progress-bar
}

# Publish version contents
test -n "$DISTFILE" && {
  URL="https://api.bintray.com/content/$BINTRAY_USER/$BINTRAY_REPO/$BINTRAY_PACKAGE/$BINTRAY_VERSION/publish"
  echo "  PUBLISH " "$BINTRAY_REPO $BINTRAY_PACKAGE $BINTRAY_VERSION"
  DATA='{ "publish_wait_for_secs": -1, "discard": "false" }'
  $CURL -sS -X POST -HContent-Type:application/json -d "$DATA" "$URL" -o /dev/null
}

# Add file to download list
test -n "$DISTFILE" -a "$DNLADD" = 1 && {
  URL="https://api.bintray.com/file_metadata/$BINTRAY_USER/$BINTRAY_REPO/${DISTFILE#*/}"
  echo "  DWNLD++ " "$BINTRAY_REPO: ${DISTFILE#*/}"
  sleep 10
  DATA='{ "list_in_downloads": "true" }'
  $CURL -sS -X PUT -HContent-Type:application/json -d "$DATA" "$URL" -o /dev/null
}
