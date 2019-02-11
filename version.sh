#!/usr/bin/env bash
set -Eeuo pipefail

# Usage: version.sh [-s]		# print project version
SHORT=false LONG=false RDATE=false
while test $# -ne 0 ; do
  case "$1" in
    -s)			SHORT=true ;;
    -l)			LONG=true ;;
    -d)			RDATE=true ;;
    *)                  : ;;
  esac
  shift
done
UNOFFICIAL=snapshot

# Print short or long version and exit successfully
exit_with_version() { # exit_with_version <version> <buildid> <releasedate>
  if $SHORT ; then
    echo "$1"
  elif $LONG ; then
    echo "$2"
  elif $RDATE ; then
    echo "$3"
  else
    echo "$@"
  fi
  exit 0
}

# Determine version from Git if ./.git is present
COMMIT_DATE=$(git log -1 --format='%ci' 2>/dev/null) &&
  LAST_TAG=$(git describe --match '[0-9]*.[0-9]*.*[0-9a]' --abbrev=0 --first-parent 2>/dev/null ||
	     git describe --match v'[0-9]*.[0-9]*.*[0-9a]' --abbrev=0 --first-parent) &&
  BUILD_ID=$(git describe --match "$LAST_TAG" --abbrev=8 --long --dirty) &&
  test -n "$BUILD_ID" &&
  exit_with_version "$LAST_TAG" "$BUILD_ID" "$COMMIT_DATE"

# Determine version from NEWS file
test -r NEWS &&
  NEWS_VERSION="$(sed -nr '/^##\s.*[0-9]+\.[0-9]+\.[0-9]/ { s/.*\s([0-9]+\.[0-9]+\.[0-9]+[a-z0-9A-Z-]+)\b.*/\1/; p; q }' NEWS)" &&
  NEWS_DATE="$(stat -c %y NEWS | sed 's/\.[0-9]\+//')" ||
    NEWS_VERSION=

# Determine version via 'git archive' (see man gitattributes / export-subst)
GITARCHIVE_REFS='$Format:%D,$'
GITARCHIVE_DATE='$Format:%ci$'
[[ "$GITARCHIVE_REFS$GITARCHIVE_DATE" =~ '$'Format:% ]] && LAST_TAG= ||
    LAST_TAG=$(echo " $GITARCHIVE_REFS" | tr , '\n' | \
		 sed -nr '/\btag: [0-9]\.[0-9]+\.[0-9]+/ { s/^[^0-9]*\b([0-9]+\.[0-9]+\.[0-9]+[a-z0-9A-Z-]+).*/\1/; p; q }')
test -n "$LAST_TAG" && {
  # FIXME: LAST_TAG == NEWS is release
  if test "$LAST_TAG" = "$NEWS_VERSION" ; then
    exit_with_version "$LAST_TAG" "$LAST_TAG-tarball" "$GITARCHIVE_DATE" # release version
  else
    exit_with_version "$LAST_TAG" "$LAST_TAG-$UNOFFICIAL" "$GITARCHIVE_DATE"
  fi
}

# Use NEWS_VERSION as last resort
test -n "$NEWS_VERSION" &&
  exit_with_version "$NEWS_VERSION" "$NEWS_VERSION-$UNOFFICIAL" "$NEWS_DATE"

echo "$0: ERROR: Failed to find project version, use a valid git repository or release tarball" >&2
exit 97
