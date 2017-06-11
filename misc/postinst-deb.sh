#!/bin/bash
set -e


set_perms() {
  USER="$1"; GROUP="$2"; MODE="$3"; FILE="$4"
  # https://www.debian.org/doc/debian-policy/ch-files.html#s10.9.1
  if ! dpkg-statoverride --list "$FILE" > /dev/null ; then
    chown "$USER:$GROUP" "$FILE"
    chmod "$MODE" "$FILE"
  fi
}


# https://www.debian.org/doc/debian-policy/ch-maintainerscripts.html#s-mscriptsinstact
case "$1" in
  configure)
    # https://www.debian.org/doc/debian-policy/ch-files.html#s-permissions-owners
    set_perms root root 4755 /opt/bin/beast	# wrapper which does renice -20
    # https://www.debian.org/doc/debian-policy/ch-sharedlibs.html#s-ldconfig
    ldconfig
    ;;
  abort-upgrade|abort-remove|abort-deconfigure)
    ;;
  *)
    # unkown action
    exit 1
    ;;
esac


exit 0
