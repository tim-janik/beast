#!/bin/sh

PACKAGE="bse"
PATH="$PATH:.."

echo "Calling intltool-update for you ..."
intltool-update --gettext-package $PACKAGE $*
