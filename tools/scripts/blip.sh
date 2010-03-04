#!/bin/bash
#
# Copyright (C) 2010 Stefan Westerfeld, stefan@space.twc.de
#
# This software is provided "as is"; redistribution and modification
# is permitted, provided that the following disclaimer is retained.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# In no event shall the authors or contributors be liable for any
# direct, indirect, incidental, special, exemplary, or consequential
# damages (including, but not limited to, procurement of substitute
# goods or services; loss of use, data, or profits; or business
# interruption) however caused and on any theory of liability, whether
# in contract, strict liability, or tort (including negligence or
# otherwise) arising in any way out of the use of this software, even
# if advised of the possibility of such damage.
#

#
# a test signal generator for beast
#
# shell script to convert the output of blip.py to .bsewave files
#
blip.py || { echo "creation failed" ; exit 1; }
rm pseudo-square.bsewave
bsewavetool create pseudo-square.bsewave 1
bsewavetool add-raw-chunk pseudo-square.bsewave -f 440.366972477 -F float -R 48000 pseudo-square-440
LOOP_END=108
bsewavetool xinfo pseudo-square.bsewave -f440.36 loop-type=jump loop-start=0 loop-end=$LOOP_END loop-count=1000000

rm pseudo-saw.bsewave
bsewavetool create pseudo-saw.bsewave 1
bsewavetool add-raw-chunk pseudo-saw.bsewave -f 440.366972477 -F float -R 48000 pseudo-saw-440
LOOP_END=108
bsewavetool xinfo pseudo-saw.bsewave -f440.36 loop-type=jump loop-start=0 loop-end=$LOOP_END loop-count=1000000

rm pseudo-stereo.bsewave
bsewavetool create pseudo-stereo.bsewave 2
bsewavetool add-raw-chunk pseudo-stereo.bsewave -f 440.366972477 -F float -R 48000 pseudo-stereo-440
LOOP_END=216
bsewavetool xinfo pseudo-stereo.bsewave -f440.36 loop-type=jump loop-start=0 loop-end=$LOOP_END loop-count=1000000
bsewavetool xinfo pseudo-stereo.bsewave --wave play-type=adsr-wave-2

rm pseudo-square-env.bsewave
bsewavetool create pseudo-square-env.bsewave 1
bsewavetool add-raw-chunk pseudo-square-env.bsewave -f 440.366972477 -F float -R 48000 pseudo-square-env-440
bsewavetool xinfo pseudo-square-env.bsewave --wave play-type=plain-wave-1
bsewavetool oggenc pseudo-square-env.bsewave

rm pseudo-stereo-env.bsewave
bsewavetool create pseudo-stereo-env.bsewave 2
bsewavetool add-raw-chunk pseudo-stereo-env.bsewave -f 440.366972477 -F float -R 48000 pseudo-stereo-env-440
bsewavetool xinfo pseudo-stereo-env.bsewave --wave play-type=plain-wave-2
bsewavetool oggenc pseudo-stereo-env.bsewave
