#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -Eeuo pipefail

SCRIPTNAME=${0##*/} ; die() { [ -z "$*" ] || echo "$SCRIPTNAME: $*" >&2; exit 128 ; }

# == defaults ==
DOCKEROPTIONS="--cap-add SYS_PTRACE"
LSAN0="-e ASAN_OPTIONS=detect_leaks=0"
WITH_CPPCHECK=
COMPILERCONF=
CONFIGUREOPTIONS=
QUICKCONF=
RULES=
WITH_CLANGTIDY=
MAKEOPTIONS=
EXEC_SHELL=

# == usage ==
usage() {
  #     12345678911234567892123456789312345678941234567895123456789612345678971234567898
  echo "Usage: $0 [cppcheck] [COMPILER] [RULE...]"
  echo "  clang         COMPILER: use clang/clang++"
  echo "  quick         COMPILER: use clang, disable debugging and optimizations"
  echo "  gcc           COMPILER: use gcc/g++"
  echo "  asan          COMPILER: enable gcc address sanitizer"
  echo "  lsan          COMPILER: enable gcc leak sanitizer"
  echo "  tsan          COMPILER: enable gcc thread sanitizer"
  echo "  ubsan         COMPILER: enable gcc undefined behaviour sanitizer"
  echo "  debug         COMPILER: use moderate optimizations, enable debugging"
  echo "  release       COMPILER: use optimized release compilation, disable debugging"
  echo "  all           RULE: build all sources"
  echo "  cppcheck      RULE: run cppcheck (recommended prior to compiling)"
  echo "  listhacks     RULE: find hack/bug notes in source code"
  echo "  listunused    RULE: find unused functions in source code"
  echo "  check         RULE: run check and installcheck"
  echo "  dist          RULE: build distribution tarball"
  echo "  distcheck     RULE: check distribution tarball"
  echo "  distcheck-po0 RULE: distcheck without check-update-po"
  echo "  appimage      RULE: build appimage"
  echo "  bintray       RULE: upload out/ contents to bintray"
  echo "  scan-build    RULE: run scan-build (includes 'all')"
  echo "  clang-tidy    RULE: run clang-tidy (requires generated sources)"
  echo "  mclean        RULE: run maintainer-clean"
  echo "  V=1           Enable verbose make rules"
  echo "  shell         Start shell into build environment"
  echo "  rshell        Start root shell into build environment"
}

# == parse args ==
test $# -ne 0 || { usage ; exit 0 ; }
while test $# -ne 0 ; do
  case "$1" in \
    gcc)       	COMPILERCONF=gcc ;;
    clang)     	COMPILERCONF=clang ;;
    asan|lsan|ubsan|tsan)
      :;	COMPILERCONF="${COMPILERCONF:-gcc}" CONFIGUREOPTIONS="$CONFIGUREOPTIONS MODE=$1" ;;
    quick)     	COMPILERCONF="${COMPILERCONF:-clang}" CONFIGUREOPTIONS="$CONFIGUREOPTIONS MODE=quick" ;;
    debug)   	CONFIGUREOPTIONS="$CONFIGUREOPTIONS MODE=debug" ;;
    release)   	CONFIGUREOPTIONS="$CONFIGUREOPTIONS MODE=release" ;;
    cppcheck|listhacks|listunused|scan-build|all|install|uninstall|installcheck|dist|distcheck|distcheck-po0|appimage|bintray|clean|clang-tidy)
      :;	RULES="$RULES $1" ;;
    check)   	RULES="$RULES root-check" ;;
    mclean)   	RULES="$RULES maintainer-clean" ;;
    V=1)	MAKEOPTIONS="$MAKEOPTIONS V=1" ;;
    shell) 	EXEC_SHELL=user ;;
    rshell) 	EXEC_SHELL=root ;;
    -h)		usage ; exit 0 ;;
    *)		die "invalid argument: $1" ;;
  esac
  shift
done

# == Decide Dockerfile base ==
DIST=timjanik/beast:cibase-200112-bionic

# == Prepare Build Environment ==
DOCKERFILE=misc/Dockerfile-cibuild
TUID=`id -u`
TGID=`id -g`
mkdir -p misc/.cicache/electron/
test ! -d ~/.electron/.       || cp --reflink=auto --preserve=timestamps ~/.electron/.       -r misc/.cicache/electron/
test ! -d ~/.cache/electron/. || cp --reflink=auto --preserve=timestamps ~/.cache/electron/. -r misc/.cicache/electron/
( set -x
  docker build -f "$DOCKERFILE" \
	 --build-arg DIST="$DIST" \
	 --build-arg USERGROUP="$TUID:$TGID" \
	 -t beast-cibuild misc/
)
rm -r misc/.cicache/
BEAST_USER_VOLUME="--user $TUID:$TGID -v `pwd`:/usr/src/beast/"

# == Keep interactive tty ==
tty >/dev/null && TI=-ti || TI=-t

# == Copy CWD to temporary volume ==
setup_BEAST_TEMP_VOLUME() {
  test -n "${TMPVOL:-}" || {
    TMPVOL=beast-cibuild-tmpvol
    trap "docker volume rm $TMPVOL >/dev/null" 0 HUP INT QUIT TRAP USR1 PIPE TERM ERR EXIT
    docker run $DOCKEROPTIONS -v `pwd`:/usr_src_beast/:ro -v $TMPVOL:/usr/src/beast/ $TI --rm beast-cibuild \
	   cp -a --reflink=auto /usr_src_beast/. /usr/src/beast/
  }
  BEAST_TEMP_VOLUME="-v $TMPVOL:/usr/src/beast/"
}

# == Shell ==
test -z "$EXEC_SHELL" || {
  BEAST_TEMP_VOLUME="$BEAST_USER_VOLUME"
  [[ "$EXEC_SHELL" =~ root ]] && setup_BEAST_TEMP_VOLUME
  ( set -x
    docker run $DOCKEROPTIONS $BEAST_TEMP_VOLUME $TI --rm beast-cibuild \
	   /bin/bash
  )
  exit $?
  # avoid 'exec' to carry out the setup_BEAST_TEMP_VOLUME trap
}

# == Configure ==
test -z "$COMPILERCONF" || (
  case $COMPILERCONF in \
    gcc)	CC=gcc CXX=g++ ;; \
    clang)	CC=clang CXX=clang++ ;; \
    *)		die "invalid compiler configuration: $COMPILERCONF" ;;
  esac
  set -x
  docker run $BEAST_USER_VOLUME $TI --rm $LSAN0 beast-cibuild \
	 nice make default prefix=/usr CC="$CC" CXX="$CXX" $CONFIGUREOPTIONS
)

# == make rules ==
for R in $RULES ; do
  BEAST_TEMP_VOLUME="$BEAST_USER_VOLUME"
  [[ $R =~ all ]] && PARALLEL="-j`nproc`" || PARALLEL=
  [[ $R =~ lsan ]] && LSAN0=
  [[ $R =~ ^distcheck-po0$ ]] && R="distcheck DISTCHECK_PO=0"
  [[ $R =~ ^root-check$ ]] && { R='check install installcheck uninstall' ; BEAST_TEMP_VOLUME= ; }
  test -n "$BEAST_TEMP_VOLUME" || setup_BEAST_TEMP_VOLUME
  ( set -x
    docker run $DOCKEROPTIONS $BEAST_TEMP_VOLUME $TI --rm $LSAN0 beast-cibuild \
	   make $MAKEOPTIONS $R $PARALLEL
  ) || exit $?
done

# == NOTES ==
# uses:
# misc/cibuild.sh quick cppcheck scan-build clang-tidy bintray
# misc/cibuild.sh clang all check
# misc/cibuild.sh gcc all distcheck appimage bintray
