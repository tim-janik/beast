#!/bin/bash

SOURCE="$1"
[ -z "$SOURCE" ] && { echo "usage: $0 <inputfile> [maximum]"; exit 1; }
MAXIMUM="$2"
[ -z "$MAXIMUM" ] && MAXIMUM="999999"

function lineecho() {
	# comment out the following line to get real line number information
	echo "# $1 \"$2\""
	echo # eat line 0
}

# Signal3
if test 3 -le $MAXIMUM ; then
	lineecho 0 $SOURCE
	cat <$SOURCE
fi

# Signal0
[ 0 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\20/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\21/g;
	s/, typename A1, typename A2, typename A3//g;		# either front comma
	s/typename A1, typename A2, typename A3,\?//g;		# or (maybe) back comma
	s/, A1, A2, A3//g;
	s/A1, A2, A3,\?//g;
	s/, a1, a2, a3//g;
	s/a1, a2, a3,\?//g;
	s/, A1 a1, A2 a2, A3 a3//g;
	s/A1 a1, A2 a2, A3 a3,\?//g;
	s/A1 m_a1; A2 m_a2; A3 m_a3;//g;
	s/, m_a1 (a1), m_a2 (a2), m_a3 (a3)//g;
	s/m_a1 (a1), m_a2 (a2), m_a3 (a3),\?//g;
	s/, m_a1, m_a2, m_a3//g;
	s/m_a1, m_a2, m_a3,\?//g;
EOT
  lineecho 99000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal1
[ 1 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\21/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\22/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1/g;
	s/\(, \)\?A1, A2, A3/\1A1/g;
	s/\(, \)\?a1, a2, a3/\1a1/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1/g;
EOT
  lineecho 10000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal2
[ 2 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\22/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\23/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2/g;
EOT
  lineecho 20000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal4
[ 4 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\25/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\24/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4/g;
EOT
  lineecho 40000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal5
[ 5 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\25/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\26/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5/g;
EOT
  lineecho 50000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal6
[ 6 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\26/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\27/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6/g;
EOT
  lineecho 60000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal7
[ 7 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\27/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\28/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7/g;
EOT
  lineecho 70000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal8
[ 8 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\28/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\29/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8/g;
EOT
  lineecho 80000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal9
[ 9 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\29/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\210/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8, m_a9/g;
EOT
  lineecho 90000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal10
[ 10 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\210/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\211/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10;/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8, m_a9, m_a10/g;
EOT
  lineecho 100000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal11
[ 11 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\211/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\212/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8, m_a9, m_a10, m_a11/g;
EOT
  lineecho 110000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal12
[ 12 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\212/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\213/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11; A12 m_a12/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11), m_a12 (a12)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8, m_a9, m_a10, m_a11, m_a12/g;
EOT
  lineecho 120000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal13
[ 13 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\213/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\214/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11; A12 m_a12; A13 m_a13/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11), m_a12 (a12), m_a13 (a13)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8, m_a9, m_a10, m_a11, m_a12, m_a13/g;
EOT
  lineecho 130000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal14
[ 14 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\214/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\215/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13, A14 a14/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11; A12 m_a12; A13 m_a13; A14 m_a14/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11), m_a12 (a12), m_a13 (a13), m_a14 (a14)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8, m_a9, m_a10, m_a11, m_a12, m_a13, m_a14/g;
EOT
  lineecho 140000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal15
[ 15 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\215/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\216/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13, A14 a14, A15 a15/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11; A12 m_a12; A13 m_a13; A14 m_a14; A15 m_a15/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11), m_a12 (a12), m_a13 (a13), m_a14 (a14), m_a15 (a15)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8, m_a9, m_a10, m_a11, m_a12, m_a13, m_a14, m_a15/g;
EOT
  lineecho 150000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal16
[ 16 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\216/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\217/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15, typename A16/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13, A14 a14, A15 a15, A16 a16/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11; A12 m_a12; A13 m_a13; A14 m_a14; A15 m_a15; A16 m_a16/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11), m_a12 (a12), m_a13 (a13), m_a14 (a14), m_a15 (a15), m_a16 (a16)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8, m_a9, m_a10, m_a11, m_a12, m_a13, m_a14, m_a15, m_a16/g;
EOT
  lineecho 160000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# Signal17
[ 17 -le $MAXIMUM ] && {
  cat >xgen-signals.sed <<EOT
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\217/g;
	s/\b\([A-Za-z0-9]*Trampoline\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)4\b/\1\218/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15, typename A16, typename A17/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13, A14 a14, A15 a15, A16 a16, A17 a17/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11; A12 m_a12; A13 m_a13; A14 m_a14; A15 m_a15; A16 m_a16; A17 m_a17/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11), m_a12 (a12), m_a13 (a13), m_a14 (a14), m_a15 (a15), m_a16 (a16), m_a17 (a17)/g;
	s/\(, \)\?m_a1, m_a2, m_a3/\1m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8, m_a9, m_a10, m_a11, m_a12, m_a13, m_a14, m_a15, m_a16, m_a17/g;
EOT
  lineecho 170000 $SOURCE
  sed -f xgen-signals.sed <$SOURCE
}

# cleanup
rm -f xgen-signals.sed
