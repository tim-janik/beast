#!/bin/sh

PACKAGE="beast"
PATH="$PATH:.."

echo "Calling intltool-update for you ..."
intltool-update --gettext-package $PACKAGE $*
