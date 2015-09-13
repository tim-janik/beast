#!/bin/bash

CONFIGURE_OPTIONS=

echo "$0: Cleaning configure caches..."
rm -rf autom4te.cache/
rm -f  config.cache

# automake *requires* ChangeLog
echo "$0: Enforce ChangeLog presence"
test -e ChangeLog || TZ=GMT0 touch ChangeLog -t 190112132145.52

echo "$0: autoreconf -vfsi -Wno-portability"
autoreconf -vfsi -Wno-portability || exit $?

echo "$0: ./configure $CONFIGURE_OPTIONS $@"
./configure $CONFIGURE_OPTIONS "$@" || exit $?
