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

. beastconf.sh
tests_total=0
tests_failed=0
for test_file in "$@"
do
  echo "TEST $test_file:"
  test_result=passed
  test_indentation="        "
  (
    . $test_file
    if test -z "$filename"; then
      echo "invalid test $test_file: must contain a filename=<bse_file> line"
      exit 1
    fi
    if test -z "$features"; then
      echo "invalid test $test_file: must contain a features=<desired_features> line"
      exit 1
    fi
    if test -z "$threshold"; then
      echo "invalid test $test_file: must contain a threshold=<desired_threshold> line"
      exit 1
    fi
    echo -n "$test_indentation"
    $top_builddir/shell/bsesh-$BSE_VERSION -s bsetowav.scm $filename $filename.wav || exit 1
    echo "${test_indentation}extracting features: $features"
    $top_builddir/tools/bsefextract $features $filename.wav > $test_file.current || exit 1
    rm $filename.wav
    if test -f $test_file.reference; then
      echo "${test_indentation}comparing features with threshold: $threshold"
      $top_builddir/tools/bsefcompare --threshold=$threshold $test_file.current $test_file.reference > $filename.result || exit_code=1
      sed "s/^/$test_indentation/g" < $filename.result
      rm $filename.result
    else
      exit_code=1
      echo "${test_indentation}no reference data found in $test_file.reference (so no comparision possible)"
    fi
    exit $exit_code
  ) || test_result=failed
  echo "TEST $test_file $test_result."
  if test $test_result = "failed"; then
    echo "# If you are sure the extracted features of this test run are correct, and"
    echo "# you want to make them the new reference data, you can do so by typing:"
    echo "mv $test_file.current $test_file.reference"
    tests_failed=$(expr $tests_failed + 1)
  fi
  tests_total=$(expr $tests_total + 1)
  echo
done

tests_passed=$(expr $tests_total - $tests_failed)
echo "Summary: "
echo "==================="
echo "   $tests_passed tests passed"
echo "   $tests_failed tests failed"
echo "   $tests_total tests total"
echo "==================="

if test $tests_failed -gt 0; then
  exit 1
else
  exit 0
fi
