#!/bin/bash

set -xe # be verbose and abort on errors

rm -rf autom4te.cache/ config.cache

autoreconf -vfsi

./configure --enable-devel-mode=yes "$@"
