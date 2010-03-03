#!/usr/bin/env python
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
import array
from math import sin, pi, atan

def pseudo_square (sample_rate, freq):
  out = array.array ('f')
  samples = int (sample_rate / freq)
  for i in range (samples):
    x = sin (i * pi * 2.0 / samples)
    x = atan (x * 10) / (pi / 2)
    out.append (x)
  print sample_rate * 1.0 / samples
  return out

def pseudo_saw (sample_rate, freq):
  out = array.array ('f')
  samples = int (sample_rate / freq)
  for i in range (samples):
    phase = i * 1.0 / samples
    if phase < 0.9:     # ramp up
      x = phase / 0.9   # range from  0.0 .. 1.0
      x = 2 * x - 1     # range from -1.0 .. 1.0
    else:
      x = phase - 0.9   # range from 0.0 .. 0.1
      x = x / 0.1       # range from 0.0 .. 1.0
      x = -2 * x + 1    # range from 1.0 .. -1.0
    out.append (x)
  return out

def interleave (left, right):
  assert len (left) == len (right)

  out = array.array ('f')
  for i in range (len (left)):
    out.append (left[i])
    out.append (right[i])
  return out

def repeat (block, n):
  out = array.array ('f')
  for i in range (n):
    for j in range (len (block)):
      out.append (block[j])
  return out

out_psq = pseudo_square (48000, 440)
out_psq.tofile (open ("pseudo_square_440", "w"))

out_psaw = pseudo_saw (48000, 440)
out_psaw.tofile (open ("pseudo_saw_440", "w"))

#out_psq_psaw = repeat (interleave (out_psq, out_psaw), 50)
out_psq_psaw = interleave (out_psq, out_psaw)
out_psq_psaw.tofile (open ("pseudo_stereo_440", "w"))
