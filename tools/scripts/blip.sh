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
rm pseudo_square.bsewave
bsewavetool create pseudo_square.bsewave 1
bsewavetool add-raw-chunk pseudo_square.bsewave -f 440.366972477 -F float -R 48000 pseudo_square_440
LOOP_END=108
bsewavetool xinfo pseudo_square.bsewave -f440.36 loop-type=jump loop-start=0 loop-end=$LOOP_END loop-count=1000000

rm pseudo_saw.bsewave
bsewavetool create pseudo_saw.bsewave 1
bsewavetool add-raw-chunk pseudo_saw.bsewave -f 440.366972477 -F float -R 48000 pseudo_saw_440
LOOP_END=108
bsewavetool xinfo pseudo_saw.bsewave -f440.36 loop-type=jump loop-start=0 loop-end=$LOOP_END loop-count=1000000

rm pseudo_stereo.bsewave
bsewavetool create pseudo_stereo.bsewave 2
bsewavetool add-raw-chunk pseudo_stereo.bsewave -f 440.366972477 -F float -R 48000 pseudo_stereo_440
LOOP_END=216
bsewavetool xinfo pseudo_stereo.bsewave -f440.36 loop-type=jump loop-start=0 loop-end=$LOOP_END loop-count=1000000
bsewavetool xinfo pseudo_stereo.bsewave --wave play-type=adsr-wave-2
