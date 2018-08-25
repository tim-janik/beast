#!/usr/bin/env python2
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
import sys, re

def unescape (xmlstr):
  xmlstr = xmlstr.replace ('&lt;', '<')
  xmlstr = xmlstr.replace ('&gt;', '>')
  xmlstr = xmlstr.replace ('&amp;', '&')
  return xmlstr

print '{',
for line in open (sys.argv[1]):
  dq = r'_[\w-]+="[^"]+"'
  sq = r"_[\w-]+='[^']+'"
  cm = r"<!--.*?-->"
  t = re.finditer (sq + '|' + dq + '|' + cm, line)
  l = list (t)
  for m in l:
    mtxt = m.group (0)
    if mtxt.startswith ('<!--'):
      if mtxt.endswith ('-->'):
        mtxt = mtxt[4:-3]
      print ' /*%s*/ ' % mtxt,
    elif mtxt.endswith ('"') or mtxt.endswith ('"'): # sq or dq
      key, val = mtxt.split ('=', 1)
      print ' /* %s= */ _(%s); ' % (key.lstrip ('_').replace('-','_'), unescape (val)),
  print ('' if l else ';')
print '}();',
