#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

ERRORS='ALERT|ATTENTION|BUG|DANGER|ERR|FIX'
WARNINGS='CAUTION|DEPRECAT|HACK|NOTE|NOTICE|PERF|TBD|TODO|WARN'

# grep input files for C/C++ comments with known keywords
[ "$1" == -g ] && {
  shift
  egrep -n "(/\*.*|//.*|^\s\*+\s*)\b($ERRORS|$WARNINGS)" "$@"
}

# colorize messages with keywords and line numbers
[ "$1" == -c ] && {
  shift
  sed -r "/^[^:]+:[0-9]+:/ { s/^([^:]+:[0-9]+): ?/\x1b[1m\1:\x1b[22m / ; s/\b($ERRORS)(\w*)/\x1b[31;1m\1\2\x1b[39;22m/gI ; s/\b($WARNINGS)(\w*)/\x1b[35;1m\1\2\x1b[39;22m/gI }" "$@"
}

true
