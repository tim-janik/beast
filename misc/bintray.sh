#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -Eeuo pipefail

SCRIPTNAME=${0##*/} ; die() { [ -z "$*" ] || echo "$SCRIPTNAME: $*" >&2; exit 9 ; }
# https://bintray.com/docs/api/

# Options and usage
DNLADD= KEEP= BINTRAY_PATH= DISTFILE= PUBLISH_VERSION=
BINTRAY_API_KEY_FILE=
BINTRAY_API_KEY="${BINTRAY_API_KEY:-}"
BINTRAY_VERSION="${BINTRAY_VERSION:-$(date -I)}"
while test $# -ne 0 ; do
  case "$1" in
    -b)			shift ; BINTRAY_API_KEY_FILE="$1" ;;
    -d)			DNLADD=1 ;;
    -g)			BINTRAY_TAG=$(git rev-parse HEAD) || die "failed to find HEAD commit" ;
			BINTRAY_VERSION=$(git describe) || die "git failed to describe version" ;
			PUBLISH_VERSION=true ;;
    -k)			shift ; KEEP="$1" ;;
    -v)			shift ; BINTRAY_VERSION="$1" ;
			PUBLISH_VERSION=true ;;
    -h)                 echo "Usage: $SCRIPTNAME [-d] [-g] [-v VERSION] [-k N] owner/repo/package [DISTFILE]"
			echo "Prune old versions if -k is given, create VERSION for owner/repo/package,"
			echo "upload DISTFILE and publish VERSION at bintray.com."
			echo "  -b <api-key-file>     read $BINTRAY_API_KEY from a file"
			echo "  -d                    add file to download list"
			echo "  -g                    publish Git TAG and VERSION"
			echo "  -h                    help message"
			echo "  -k <N>                keep the highest N versions and prune the rest"
			echo "  -v <VERSION>          publish version for owner/repo/package"
			echo '  $BINTRAY_API_KEY      authentication token for the bintray REST API'
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
test -z "$DISTFILE" || PUBLISH_VERSION=true

test -z "$DISTFILE" || echo "  INFO    " "$BINTRAY_USER / $BINTRAY_REPO / $BINTRAY_PACKAGE @ $BINTRAY_VERSION <- $DISTFILE"

# == BINTRAY_API_KEY ==
test -z "$BINTRAY_API_KEY_FILE" ||
  BINTRAY_API_KEY="$(cat "$BINTRAY_API_KEY_FILE")"
test -n "$BINTRAY_API_KEY" ||
  die "missing Bintray authorization token"
CURL="curl -f -HAccept:application/json -u$BINTRAY_USER:$BINTRAY_API_KEY"

# Prune old versions to keep quota
if test -n "$KEEP" ; then
  URL="https://api.bintray.com/packages/$BINTRAY_USER/$BINTRAY_REPO/$BINTRAY_PACKAGE/"
  JSONRESPONSE=$($CURL -X GET -HContent-Type:application/json $URL -sS) || die "failed to list old versions"
  ALLVERSIONS=( $(echo " $JSONRESPONSE" | sed -nr 's|.*"versions":\["([^]]*)"\].*|\1|p' | sed 's|","|\n|g') )
  SORTEDVERSIONS=( $(set -f && IFS=$'\n' && sort -rV <<<"${ALLVERSIONS[*]}") )
  for v in ${SORTEDVERSIONS[@]::$KEEP} ; do
    echo "  KEEP    " "$v"
  done
  for v in ${SORTEDVERSIONS[@]:$KEEP} ; do
    echo "  DELETE  " "$v"
    RET=$($CURL -X DELETE -HContent-Type:application/json "$URL/versions/$v" -sS -w "%{http_code}" -o /dev/null) || :
    if test "${RET:0:2}" != 20 ; then
      echo "    FAIL  " "  $RET (deletion error)"
    else
      echo "      OK  " "  $RET (deleted)"
    fi
  done
fi

# Delete existing version
if false ; then
  echo "  CLEAR   " "$BINTRAY_REPO $BINTRAY_PACKAGE $BINTRAY_VERSION"
  URL="https://api.bintray.com/packages/$BINTRAY_USER/$BINTRAY_REPO/$BINTRAY_PACKAGE/versions/$BINTRAY_VERSION"
  $CURL -sS -X DELETE -HContent-Type:application/json "$URL" -o /dev/null \
    && EX=$? || EX=$? ; test $EX = 0 -o $EX = 22 # may fail with HTTP responses >= 400
fi

# Create version (yields "409 - Conflict" if existing)
DATA=$(cat <<EOF__
{ "name":		"$BINTRAY_VERSION",
  "released":		"$(date -I)",
  "vcs_tag":		"${BINTRAY_TAG:-}" }
EOF__
)
if test -n "$PUBLISH_VERSION" ; then
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
fi

# Upload file
if test -n "$DISTFILE" ; then
  URL="https://api.bintray.com/content/$BINTRAY_USER/$BINTRAY_REPO/$BINTRAY_PACKAGE/$BINTRAY_VERSION/${DISTFILE##*/}"
  echo "  UPLOAD  " "$BINTRAY_REPO $BINTRAY_PACKAGE $BINTRAY_VERSION: $DISTFILE"
  #CTYPE="-HContent-Type:application/octet-stream"
  $CURL -T "$DISTFILE" "$URL?publish=0&override=1" -o /dev/null # --progress-bar
fi

# Publish version contents
if test -n "$PUBLISH_VERSION" ; then
  URL="https://api.bintray.com/content/$BINTRAY_USER/$BINTRAY_REPO/$BINTRAY_PACKAGE/$BINTRAY_VERSION/publish"
  echo "  PUBLISH " "$BINTRAY_REPO $BINTRAY_PACKAGE $BINTRAY_VERSION"
  DATA='{ "publish_wait_for_secs": -1, "discard": "false" }'
  $CURL -sS -X POST -HContent-Type:application/json -d "$DATA" "$URL" -o /dev/null
fi

# Add file to download list
if test -n "$DISTFILE" -a "$DNLADD" = 1 ; then
  URL="https://api.bintray.com/file_metadata/$BINTRAY_USER/$BINTRAY_REPO/${DISTFILE##*/}"
  echo "  DWNLD++ " "$BINTRAY_REPO: ${DISTFILE##*/}"
  sleep 10
  DATA='{ "list_in_downloads": "true" }'
  $CURL -sS -X PUT -HContent-Type:application/json -d "$DATA" "$URL" -o /dev/null
fi
