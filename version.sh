#!/usr/bin/env bash
set -Ee -o pipefail

# Usage: git-version.sh [-s]		# print project version

if   test " $1" = " -s" ; then	# short
  FORMAT="--abbrev=0"
elif test " $1" = " -l" ; then	# long, clean
  FORMAT="--abbrev=7 --long"
else				# default
  FORMAT="--abbrev=7 --dirty"
fi

# Find version in toplevel project directory
cd "$(dirname "$0")"

# If present, git knows the canonical version
if test -d ${GIT_DIR:-.git} -o -f .git ; then
  # Find most recent annotated version tag, excluding tags merged
  # through branches, i.e. find tag through --first-parent
  TRUNKTAG=$(git describe --match '[0-9]*.[0-9]*.*[0-9a]' --abbrev=0 --first-parent)
  # Append comit hash and *full* commit count to the tag, to generate
  # monotonically increasing versions, i.e. *avoid* using --first-parent
  VERSION=$(git describe --match "$TRUNKTAG" $FORMAT)
  rm -f git-version.cache	# the cache file maybe stale if git works
  echo "$VERSION"
  exit 0
fi

# Fallback to tarball version
if test -f git-version.cache ; then
  cat git-version.cache
  exit 0
fi

# Give up
echo "$0: missing version information" >&2
exit 97
