#!/bin/bash

set -xe # be verbose and abort on errors

rm -rf autom4te.cache/ ./config.cache ./config.status

./version.sh || {
  echo "$0: error: a functional git checkout is required for non-tarball builds" >&2
  exit 7
}

if false ; then # the autoreconf overhead almost doubles autotools runtime
  autoreconf --verbose --force --symlink --install
else
  ${ACLOCAL:-aclocal}		--force				# --verbose
  ${LIBTOOLIZE:-libtoolize}	--force --install		# --verbose
  ${AUTOCONF:-autoconf}		--force				# --verbose
  ${AUTOHEADER:-autoheader}	--force				# --verbose
  ${AUTOMAKE:-automake}		--add-missing --force-missing	# --verbose
fi

test -n "$NOCONFIGURE" || "./configure" "$@"
