#!/bin/sh

die() { echo $1; exit 1; }

DIR=/usr/src/beast

[ "$1" == "" ] && die "need version"
TNAME=gsl-$1.tar.gz

cd $DIR/gsl || die "cd failed"
make clean || die "make clean failed"
cd ../docs/reference || die "cd failed"
make gsl.3 || die "make gsl.3 failed"
cd ../.. || die "cd failed"
cp -a docs/reference/gsl.3 gsl/ || die "cp failed"
tar zcvf $TNAME gsl/gsl* || die "tar failed"
echo "-----------------------------------------------------"
echo "$TNAME is ready for distribution"
echo "-----------------------------------------------------"
