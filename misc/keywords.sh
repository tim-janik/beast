#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -Eeuo pipefail

SCRIPTNAME=${0#*/} ; die() { [ -z "$*" ] || echo "$SCRIPTNAME: $*" >&2; exit 128 ; }

# == Defaults ==
ERRORS='ALERT|ATTENTION|BUG|DANGER|ERR|FIX'
WARNINGS='CAUTION|DEPRECAT|HACK|NOTE|NOTICE|PERF|TBD|TODO|WARN'
EXTENSIONS='cc|hh|md|js|vue'
GITLS=false
GREP_ERRWARN=false
SED_COLOR=false

# == Usage ==
usage() {
  echo "Usage: $0 [FLAGS] [FILES...]"
  echo "Extract keywords from source code"
  echo "  -c            colorize messages with keywords and line numbers"
  echo "  -g            grep input files for C/C++ comments with known keywords"
  echo "  -l            automatically create file list with 'git ls-tree'"
}

# == parse args ==
test $# -ne 0 || { usage ; exit 0 ; }
while test $# -ne 0 ; do
  case "$1" in \
    -c)		SED_COLOR=true ;;
    -g)		GREP_ERRWARN=true ;;
    -l)		GITLS=true ;;
    -h)		usage ; exit 0 ;;
    --)		shift ; break ;;	# keep file arguments
    *)		break ;;		# keep file arguments
  esac
  shift
done
$GITLS && {
  set -- $( git ls-tree -r --name-only HEAD | grep -v '^external/' | grep -E "\.($EXTENSIONS)$" )
}

# == Error/warning comments ==
$GREP_ERRWARN && {
  egrep -n "(/\*.*|//.*|^\s\*+\s*)\b($ERRORS|$WARNINGS)" "$@" || :
}

# == Colorize keywords and line numbers ==
$SED_COLOR && {
  sed -r \
      -e "/^[^:]+:[0-9]+:/ { s/^([^:]+:[0-9]+): ?/\x1b[1m\1:\x1b[22m /" \
      -e "s/<([^()<>@]+@[^()<>@]+\.[^()<>@]+)>/<\x1b[32;1m\1\x1b[39;22m>/gI" \
      -e "s/\b($ERRORS)(\w*)/\x1b[31;1m\1\2\x1b[39;22m/gI" \
      -e "s/\b($WARNINGS)(\w*)/\x1b[35;1m\1\2\x1b[39;22m/gI }" \
      "$@" || :
}

exit 0
