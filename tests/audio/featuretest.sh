#!/bin/sh
# 
# Copyright (C) 2004 Stefan Westerfeld, stefan@space.twc.de
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

# check for featuretest.sh options
while test $# -gt 0; do

    case "$1" in

	-h|--help)
	    echo "featuretest.sh - perform a comparision of the audio output a .bse"
	    echo "   file generates to a reference by extracting features"
	    echo " "
	    echo "featuretest.sh <bse-file> <feature-file> [ features... ] -- [ feature compare options...] "
	    echo " "
	    echo "options:"
	    echo "  -h, --help                show brief help"
		echo "  -v, --verbose             verbose (show commands executed)"
		echo "  -r, --reset-summary       reset summary of failed/passed tests"
		echo "  -s, --summary             show summary of failed/passed tests"
		echo
		echo "features:"
		echo "  features to be extracted"
		echo "  see bsefextract --help for a detailed list of possible features"
		echo " "
		echo "feature compare options:"
		echo "  --theshold=<threshold>    threshold in %"
		echo "  see bsefcompare --help for other comparision options"
	    exit 0
	    ;;

	-v|--verbose)
	    verbose=1
	    shift
	    ;;

	-r|--reset-summary)
		rm -f featuretest.status.sh
		exit 0
		;;
	-s|--summary)
		if test -f featuretest.status.sh; then
		  . featuretest.status.sh
		fi

		tests_passed=$(expr $tests_total - $tests_failed)
		echo "Summary: "
		echo "==================="
		echo "   $tests_passed tests passed"
		echo "   $tests_failed tests failed"
		echo "   $tests_total tests total"
		echo "==================="

		# set exit code: fail if at least one test failed
		if test $tests_failed -gt 0; then
		  exit 1
		else
		  exit 0
		fi
		;;
	--)
	    # no more features, get on with life
		shift
	    break
	    ;;

	*)	
		if test -z "$bsefile"; then
			bsefile="$1"
		elif test -z "$reference"; then
			reference="$1"
		else
			features="$features$1 "
		fi
		shift
		;;
    esac
done

# initialize variables
. beastconf.sh

cmpopts=$@
test_name=`echo $reference | sed s/.reference//g`
test_result=passed
test_indentation="        "
tests_failed=0
tests_total=0

if test "$verbose" = 1; then
  echo "test name:        $test_name"
  echo "bse file:         $bsefile"
  echo "reference file:   $reference"
  echo "features:         $features"
  echo "compare options:  $cmpopts"
fi

# verify arguments for sanity
if test -z "$bsefile"; then
  echo "invalid test: you must specify a <bse-file>"
  exit 1
fi
if test -z "$reference"; then
  echo "invalid test: you must specify a <feature-file>"
  exit 1
fi
if test -z "$features"; then
  echo "invalid test: you must specify which features to compare"
  exit 1
fi
if test -z "$cmpopts"; then
  echo "invalid test: you must specify feature compare options (at least --threshold=<threshold>)"
  exit 1
fi

# run test
echo "TEST $test_name:"
(
  # convert to wave file
  echo -n "$test_indentation"
  $top_builddir/shell/bsesh-$BSE_VERSION --bse-mixing-freq=48000 --bse-control-freq=1000 -p null -m null -s bsetowav.scm $bsefile $bsefile.wav || exit 1

  # extract features
  echo "${test_indentation}extracting features: $features"
  $top_builddir/tools/bsefextract --cut-zeros $features $bsefile.wav > $test_name.current || exit 1
  rm $bsefile.wav

  # compare with reference data
  if test -f $reference; then
    echo "${test_indentation}comparing features with options: $cmpopts"
    $top_builddir/tools/bsefcompare $cmpopts $test_name.current $reference > $bsefile.result || exit_code=1
    sed "s/^/$test_indentation/g" < $bsefile.result
    rm $bsefile.result
  else
    exit_code=1
    echo "${test_indentation}no reference data found in $reference (so no comparision possible)"
  fi
  exit $exit_code
) || test_result=failed
echo "TEST $test_name $test_result."

if test -f featuretest.status.sh; then
  . featuretest.status.sh
fi

if test $test_result = "failed"; then
  echo "# If you are sure the extracted features of this test run are correct, and"
  echo "# you want to make them the new reference data, you can do so by typing:"
  echo "mv $test_name.current $test_name.reference"
  tests_failed=`expr $tests_failed + 1`
fi
tests_total=`expr $tests_total + 1`
echo

# update feature test statistics
# 
# this breaks if feature tests are run parallel; however, you shouldn't
# do this anyway, as it messes up all output
(
  echo "tests_failed=$tests_failed"
  echo "tests_total=$tests_total"
) > featuretest.status.sh
