#!/usr/bin/env bash
set -Ee -o pipefail

# Usage: git-version.sh [-s]		# print project version

FORMAT="--abbrev=7 --dirty"
test " $1" != " -s" || FORMAT="--abbrev=0"

# Find version in toplevel project directory
cd "$(dirname "$0")"

# If present, git knows the canonical version
if test -d ${GIT_DIR:-.git} -o -f .git ; then
  VERSION=$(git describe --match '[0-9]*.[0-9]*.*[0-9a]' --first-parent $FORMAT)
  rm -f git-version.cache	# Cache file is stale if git works
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
