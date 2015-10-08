#!/bin/bash

echo "$0: Cleaning configure caches..."
rm -rf autom4te.cache/
rm -f  config.cache

echo "$0: autoreconf -vfsi -Wno-portability"
autoreconf -vfsi -Wno-portability || exit $?

echo "$0: ./configure $*"
./configure "$@" || exit $?
