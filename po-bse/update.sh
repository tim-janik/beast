#!/bin/sh

[ -f POTFILES.in -a -f ../po-helper.sh ] || {
	echo "missing POTFILES.in or ../po-helper.sh, aborting..."
	exit 1
}

# define *_GETTEXT_DOMAIN
. ../po-helper.sh

PACKAGE="$BSE_GETTEXT_DOMAIN"
PATH="$PATH:.."

echo "intltool-update --gettext-package $PACKAGE $*"
intltool-update --gettext-package $PACKAGE $*
