#!/usr/bin/env python3
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"""
Generate C source code from a binary file.
"""
import sys, re, zlib

class Printer:
  def print_data (self, data):
    self.pad = False
    self.lines = []
    self.s = '"'
    self.pos = -2
    for c in data:
      self.print_char (c)
    self.lines.append (self.s + '"')
    return '  ' + '\n  '.join (self.lines)
  def print_char (self, o):
    if self.pos >= 70-1:
      self.lines.append (self.s + '"')
      self.s = '"'
      self.pos = 1
      self.pad = False
    need_pad = False
    if o in (ord ('\\'), ord ('"')):
      self.s += '\\%c' % o
      self.pos += 2
    elif o == ord ('\n'):
      self.s += '\\n'
      self.pos += 2
    elif o < 32 or o > 126 or o == ord ('?'):
      self.s += '\\%o' % o
      self.pos += 1 + (o > 63) + (o > 7) + 1
      need_pad = not (o > 63)
    elif self.pad and o >= ord ('0') and o <= ord ('9'):
      self.s += '""%c' % o
      self.pos += 3
    else:
      self.s += '%c' % o
      self.pos += 1
    self.pad = need_pad

prefix = 'PACKRES_'
with_resource_entry = True

def print_file (filename, strip_prefix = ''):
  stripped_filename = filename
  if strip_prefix:
    stripped_filename = re.sub (strip_prefix, '', filename)
  ident = re.sub ('[^a-z0-9_A-Z]', '_', stripped_filename)
  idd, ide = prefix + ident, prefix + ident
  if with_resource_entry:
    idd += '_'
  f = open (filename, 'rb')
  raw = f.read()
  f.close()
  del f
  l = len (raw)
  data = zlib.compress (raw, 9)
  # two things to consider for using the compressed data:
  # 1) For the reader code, compressed size + 1 must be smaller than the original data size to identify compressed data.
  # 2) Using compressed data requires runtime unpacking overhead and extra dynamic memory allocation.
  # So it should provide a *significant* benefit if it's used.
  j = len (data)
  if j + 1 >= l or j > 0.9 * l:
    data = raw  # skip compression
  p = Printer()
  out = p.print_data (data)
  print ('static const char %s[] __attribute__ ((__aligned__ (16))) =' % idd)
  print (out + '; // %u + 1' % len (data))
  if with_resource_entry:
    print ('static const LocalResourceEntry %s = {' % ide)
    print ('  "%s", %u,' % (stripped_filename, l))
    print ('  %s, sizeof %s' % (idd, idd))
    print ('};')


if __name__ == "__main__":
  # parse args
  strip_prefix = ''
  files = []
  i = 1
  while i < len (sys.argv):
    if sys.argv[i] == '-s':
      i += 1
      strip_prefix = sys.argv[i]
    else:
      files.append (sys.argv[i])
    i += 1

  # process files
  for f in files:
    print()
    print_file (f, strip_prefix)
