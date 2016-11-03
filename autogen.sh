#!/bin/bash

set -xe # be verbose and abort on errors

rm -rf autom4te.cache/ config.cache

autoreconf --verbose --force --symlink --install

test -n "$NOCONFIGURE" || "./configure" "$@"
