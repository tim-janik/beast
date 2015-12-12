#!/bin/bash
# Copyright (C) 2015 Tim Janik / MPL-2.0

set -e

# == usage ==
usage() {
  cat <<-__EOF
	Usage:
	  taptool.sh [options] [--] <TESTPROGRAM>
	Options:
	  --color {auto|never|always}	enable colorization
	  --test-name <NAME>		test name used in reports
	  --log-file <PATH>		capture output in a log file
	  --trs-file <PATH>		produce automake test result file
	__EOF
  # [--enable-hard-errors {yes|no}] # <- used by automake
}

# == process options ==
unset TEST_NAME LOG_FILE TRS_FILE
COLOR=auto; EXPECT_FAILURE=no ENABLE_HARD_ERRORS=yes
while test $# -gt 0 ; do
  case "$1" in
    --test-name)	TEST_NAME="$2"; shift ;; # automake
    --log-file)		LOG_FILE="$2"; shift ;;
    --trs-file)		TRS_FILE="$2"; shift ;;
    --color)		COLOR="$2"; shift ;;
    --color-tests)	COLOR=always; test "$2" = "yes" || COLOR=never; shift ;; # automake
    --expect-failure)	EXPECT_FAILURE="$2"; shift ;; # automake
    --enable-hard-errors) ENABLE_HARD_ERRORS="$2"; shift ;; # automake
    --help)		usage >&2 ; exit 0 ;;
    --) 		shift; break ;;
    -*)			usage >&2 ; exit 127 ;; # invalid option
    *) 			break ;;
  esac
  shift
done
test $# -gt 0 || { usage >&2 ; exit 127 ; }

if test -n "$TRS_FILE" -a "$ENABLE_HARD_ERRORS" = yes ; then
  # this option combination is used by automake and automake wants
  # make rule continuation even after failing tests.
  kind_exit() { exit 0 ; }
else
  kind_exit() { exit "${1:-127}" ; }
fi

if test ! -z "$LOG_FILE" ; then
  exec &> >(tee "$LOG_FILE")
fi

# == color setup ==
case "$COLOR" in
  true|always)	COLOR=always ;;
  false|never)	COLOR=never ;;
  *)		COLOR=always
		test -t 1 &&
		  [[ `tput setaf 1 2>/dev/null` =~ $'\e[31m' ]] ||
		  COLOR=never ;;
esac
if test $COLOR = always ; then
  color_green=$(tput setaf 2)		# green
  color_pass=$(tput bold; tput setaf 7) # white
  color_skip=$(tput setaf 4)		# blue
  color_warn=$(tput setaf 3)		# brown
  color_info=$(tput setaf 6)		# cyan
  color_fail=$(tput bold; tput setaf 1)	# red
  color_error=$(tput setaf 5)		# magenta
  color_reset=$(tput sgr0)		# reset all
else
  unset color_green color_pass color_skip color_warn color_info color_fail color_reset
fi

# == tmpfile for global data ==
TMPFILE=`mktemp "/tmp/taptool$$XXXXXX"` || exit 127 # failed to create temporary file?
trap 'rm -rf "$TMPFILE"' 0 HUP QUIT TRAP USR1 PIPE TERM

# == reference handling ==
RAPICORN_OUTPUT_TEST_REF=`basename "$1"`.outref
RAPICORN_OUTPUT_TEST_LOG=`basename "$1"`.output
check_reference_output() {
  R="$RAPICORN_OUTPUT_TEST_REF"
  L="$RAPICORN_OUTPUT_TEST_LOG"
  check="$TEST_NAME: match output: $RAPICORN_OUTPUT_TEST_REF"
  test ! \( -s "$L" -o -s "$R" \) || {
    test -e "$R" && OLD="$R" || OLD=/dev/null
    test -e "$L" && NEW="$L" || NEW=/dev/null
    if cmp -s "$OLD" "$NEW" ; then
      printf "%s  PASS%s     $check\n" "$color_pass" "$color_reset"
      NP=$[1 + $NP]
    elif test "$FORCECHECKUPDATE" = true ; then
      cp "$L" "$R"
      printf "%s  FORCED   $check%s\n" "$color_warn" "$color_reset"
      NX=$[1 + $NX]
    else
      printf "%s  FAIL     $check%s\n" "$color_fail" "$color_reset"
      NB=$[1 + $NB]
      if test -x /usr/bin/git ; then
	git --no-pager diff -p --no-index --color=$COLOR -- "$OLD" "$NEW"
      else
        diff -up "$OLD" "$NEW" || :
      fi
      return 1 # keep $L around
    fi
  }
  rm -f "$L"
}
export RAPICORN_OUTPUT_TEST_LOG

# == process and check the test ouput ==
check_test_output() {
  set -e # catch errors other than the pipeline errors

  # == output processing ==
  BAILOUT=; ERROR=0; NP=0; NS=0; NX=0; NB=0; NTESTS=
  while test -z "$BAILOUT" && IFS=  read -r line ; do
    if   printf "%s" "$line" | grep -Piq '^1\.\.[0-9]+\b' ; then
      NTESTS=`printf "%s" "$line" | sed -r 's/1\.\.([0-9]+).*/\1/'`
      printf "%s  TESTS    %u tests expected%s\n" "" $NTESTS ""
    elif printf "%s" "$line" | grep -Piq '^\s*(not\s+ok\b|ok\b).*#\s*SKIP\b|^\s*skip\b' ; then
      printf "%s\n" "$line" | sed -r "s/^\s*(not\s+ok\b|ok\b|skip\b:?)\s*-?\s*(.*)/$color_skip  SKIP     \2$color_reset/i"
      NS=$[1 + $NS]
    elif printf "%s" "$line" | grep -Piq '^\s*(ok\b).*#\s*TODO\b|^\s*xpass\b' ; then
      printf "%s\n" "$line" | sed -r "s/^\s*(ok\b|xpass\b:?)\s*-?\s*(.*)/$color_pass  XPASS$color_reset    \2/i"
      NP=$[1 + $NP]
    elif printf "%s" "$line" | grep -Piq '^\s*(ok|pass)\b' ; then
      printf "%s\n" "$line" | sed -r "s/^\s*(ok\b|pass\b:?)\s*-?\s*(.*)/$color_pass  PASS$color_reset     \2/i"
      NP=$[1 + $NP]
    elif printf "%s" "$line" | grep -Piq '^\s*todo\b' ; then
      printf "%s\n" "$line" | sed -r "s/^\s*(todo\b:?)\s*-?\s*(.*)/$color_warn  TODO     \2$color_reset/i"
      NX=$[1 + $NX]
    elif printf "%s" "$line" | grep -Piq '^\s*fixme\b' ; then
      printf "%s\n" "$line" | sed -r "s/^\s*(fixme\b:?)\s*-?\s*(.*)/$color_warn  FIXME    \2$color_reset/i"
      NX=$[1 + $NX]
    elif printf "%s" "$line" | grep -Piq '^\s*(not\s+ok\b).*#\s*TODO\b|^\s*xfail\b' ; then
      printf "%s\n" "$line" | sed -r "s/^\s*(not\s+ok\b|xfail\b:?)\s*-?\s*(.*)/$color_warn  XFAIL    \2$color_reset/i"
      NX=$[1 + $NX]
    elif printf "%s" "$line" | grep -Piq '^\s*(not\s+ok\b|fail\b:?)' ; then
      printf "%s\n" "$line" | sed -r "s/^\s*(not\s+ok\b|fail\b:?)\s*-?\s*(.*)/$color_fail  FAIL     \2$color_reset/i"
      NB=$[1 + $NB]
    elif printf "%s" "$line" | grep -Piq '^\s*error\b:?' ; then
      printf "%s\n" "$line" | sed -r "s/^\s*(error\b:?)\s*-?\s*(.*)/$color_fail  ERROR    \2$color_reset/i"
      NB=$[1 + $NB]
    elif printf "%s" "$line" | grep -Piq '^\s*Bail\s+out\b' ; then
      printf "%s  BAILOUT  %s%s\n" "$color_fail" "$line" "$color_reset"
      BAILOUT=true
    else
      printf "%s\n" "$line"
    fi
  done

  # == check output ==
  check_reference_output || {
    test $ERROR != 0 || ERROR=4
  }

  # == results ==
  NK=$[$NP + $NS]
  SEEN=$[$NK + $NX + $NB]
  test -z "$BAILOUT" || ERROR=3
  test 0 = $NK || {
    printf "%s  RESULT   $TEST_NAME: passing tests: %u/%u%s\n" "$color_green" $NK $SEEN "$color_reset"
  }
  test 0 = $NX || {
    printf "%s  RESULT   $TEST_NAME: expected failing tests remaining: %u%s\n" "$color_warn" $NX "$color_reset"
  }
  test 0 = $NB || {
    printf "%s  ERROR    $TEST_NAME: broken tests encountered: %u%s\n" "$color_error" $NB "$color_reset"
    test $ERROR != 0 || ERROR=1
  }
  test "$BAILOUT" = true -o -z "$NTESTS" -o "$NTESTS" = $SEEN || {
    printf "%s  ERROR    $TEST_NAME: processed %u/%u tests, mismatch: %+d%s\n" "$color_error" $SEEN $NTESTS $[$SEEN - $NTESTS] "$color_reset"
    test $ERROR != 0 || ERROR=2
  }

  # == generate trs ==
  test -z "$TRS_FILE" || {
    if   test 0 = "$ERROR" ; then
      echo ":global-test-result: PASS"
    elif test 1 = "$ERROR" ; then
      echo ":global-test-result: FAIL"
    else # 3, 4
      echo ":global-test-result: ERROR"
    fi
    for ((i=1; i<=$NP; i++)) ; do echo ":test-result: PASS"; done
    for ((i=1; i<=$NS; i++)) ; do echo ":test-result: SKIP"; done
    for ((i=1; i<=$NX; i++)) ; do echo ":test-result: XFAIL"; done
    for ((i=1; i<=$NB; i++)) ; do echo ":test-result: FAIL"; done
  } > "$TRS_FILE"

  # == exit status ==
  echo "$ERROR" > $TMPFILE
  exit 0 # don't clutter "$1" exit status
}

# == test invocation ==
test ! -z "$TEST_NAME" || TEST_NAME="${1#./}"
printf "%s  START%s    $TEST_NAME: testing...\n" "$color_pass" "$color_reset"
rm -f "$RAPICORN_OUTPUT_TEST_LOG"
set +e -o pipefail # catch pipeline errors
"$@" | check_test_output

# check exit status from $1, possible b/c the output-processing exit status is in $TMPFILE,
# and pipefail then retains the "$1" exit status.
TEST_EXIT=$?
test $TEST_EXIT = 0 || {
  printf "%s  ERROR    $TEST_NAME: non-zero exit status: %s%s\n" "$color_error" "$TEST_EXIT" "$color_reset"
  test -z "$TRS_FILE" ||
    echo ":test-result: ERROR" >> "$TRS_FILE"
  kind_exit $TEST_EXIT
}

# now exit according to output-processing exit status
TEST_EXIT=`cat $TMPFILE 2>/dev/null`
kind_exit ${TEST_EXIT:-127}
