#!/usr/bin/env python3
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"""
Pandoc filter to convert all level 2+ headers to paragraphs
with emphasized text.
"""
from pandocfilters import toJSONFilter, Emph, Strong, SmallCaps, Para, Header

def behead (key, value, format_, meta):
  if key == 'Header':
    if value[0] >= 2:
      return Para ([ SmallCaps (value[2]) ])
    else:
      return Para ([ Strong (value[2]) ])
    # alt: increase heading levels
    level, content, attr = value
    # content[1] += [ 'unnumbered' ] # causes "Duplicate identifier" warnings
    return Header (min (6, level + 2), content, attr)

if __name__ == "__main__":
  toJSONFilter (behead)
