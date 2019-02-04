#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -Eeuo pipefail

SCRIPTNAME=${0##*/} ; die() { [ -z "$*" ] || echo "$SCRIPTNAME: $*" >&2; exit 128 ; }

# == defaults ==
WITNESSLOG=

# == usage ==
usage() {
  echo "Usage: $0 <logfile> [cibuildcommands...]"
}

# == parse args ==
test $# -ne 0 -a "${1:-}" != -h || { usage ; exit 0 ; }
WITNESSLOG="$1" ; shift

# == prepare log ==
mkdir -p "$(dirname "$WITNESSLOG" )"
touch $WITNESSLOG

# == cibuild.sh ==
EXITSTATUS=
for cmd in "$@" ; do
  (set -x ; misc/cibuild.sh "$cmd" ) >>$WITNESSLOG 2>&1 ||
    EXITSTATUS="${EXITSTATUS:-$?}"
done

# == exit ==
exit "${EXITSTATUS:-0}"
