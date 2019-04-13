#!/usr/bin/env python3
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"""
Pandoc filter to increase heading level, convert all level 3+
headers to Strong text and level 2 headers to SmallCaps.
"""
from pandocfilters import toJSONFilter, Emph, Strong, SmallCaps, Para, Header

def behead (key, value, format_, meta):
  if key == 'Header':
    if value[0] >= 3:
      return Para ([ Strong (value[2]) ])
    elif value[0] >= 2:
      return Para ([ SmallCaps (value[2]) ])
    # increase heading levels
    level, content, attr = value
    # content[1] += [ 'unnumbered' ] # causes "Duplicate identifier" warnings
    return Header (min (6, level + 2), content, attr)

if __name__ == "__main__":
  toJSONFilter (behead)
