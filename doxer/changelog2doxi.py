#!/usr/bin/env python2.4
#
# Copyright (C) 2006 Tim Janik
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
import os, sys, re

def compile_number ():
  # number exponent
  expo = r'([eE][+-]?[0-9]+)'
  expoP = expo + '?'    # possible exponent
  # floating point numbers
  fpattern = (r' [0-9]+ \. [0-9]+' + expoP + ' |' +
              r' [0-9]+ \. ?     ' + expo  + ' |' +
              #' [0-9]+ \.       '         + ' |' +
              r'        \. [0-9]+' + expoP + '  ')
  # integer/hex numbers
  ipattern = (r'        [0-9]+       [LlUu]? |' +
              r' 0 [xX] [A-Fa-f0-9]+          ')
  # match mixed-case symbols
  pat = re.compile (r'(?<=[^\w.+-]) [+-]? (' + fpattern + '|' + ipattern + ') (?=[^\w.+-]|$)', re.X)
  return (pat, '@doxer_span{ChangeLog-number,', '}')

def compile_macro ():
  # match upper-case macros
  prefix = r'(SFI|GSL|BSE|BST|GXK|GTK|GDK|GNOME|G)_'
  pattern = r'[*0-9A-Za-z?][*0-9A-Za-z_?]*(\(\)){0,1}'
  pat = re.compile ('(?<=\W)' + prefix + pattern)
  return (pat, '@doxer_span{ChangeLog-macro,', '}')

def compile_func ():
  # match lower-case function symbols
  prefix = r'(sfi|gsl|bse|bst|gxk|gtk|gdk|gnome|g)_'
  pattern = r'[*0-9A-Za-z?][*0-9A-Za-z_?]*(\(\)){0,1}'
  pat = re.compile ('(?<=\W)' + prefix + pattern)
  return (pat, '@doxer_span{ChangeLog-function,', '}')

def compile_symbol ():
  # match mixed-case symbols
  prefix  = r'(Sfi|Gsl|Bse|Bst|Gxk|Gtk|Gdk|Gnome)'
  pattern = r'[*0-9A-Z?][*0-9A-Za-z_?]*(\(\)){0,1}'
  glib_pattern = 'G[*A-Z?]+[*a-z?][*0-9A-Za-z_?]*'
  pat = re.compile ('(?<=\W)(' + prefix + pattern + '|' + glib_pattern + ')')
  return (pat, '@doxer_span{ChangeLog-symbol,', '}')

def compile_uri ():
  # match white-space enclosed URIs
  ws_uri = r'(?<=\s)(https?|ftp|mailto):[^,\s]+[^,\s:.;]'
  # match brace/parenthesis enclosed URIs
  bp_uri = r'(https?|ftp|mailto):[^,\s{}[\]()]+[^,\s{}[\]():.;]'
  pat = re.compile (ws_uri + '|' + bp_uri)
  return (pat, '@uri{',  '}')

def compile_dqstring ():
  pat = re.compile (r'"([^\\"]|\\.)*"', re.X)
  return (pat, '@doxer_span{ChangeLog-string,', '}')

def markup_next (string, patlist):
  if not patlist:
    return string
  pat = patlist[0]
  mo = pat[0].search (string)
  if mo:
    s, e = mo.start(), mo.end()
    part1 = string[:s]
    match = string[s:e]
    part3 = string[e:]
    if part1:
      part1 = markup_next (part1, patlist[1:])
    part2 = pat[1] + match + pat[2]
    if part3:
      part3 = markup_next (part3, patlist)
    return part1 + part2 + part3
  return markup_next (string, patlist[1:])

global text_pattern_list
text_pattern_list = []

def markup_text (string):
  global text_pattern_list
  if not text_pattern_list:
    for pc in [ compile_dqstring, compile_uri, compile_symbol, compile_func, compile_macro, compile_number ]:
      text_pattern_list += [ pc() ]
  return markup_next (string, text_pattern_list)

def markup_line (line):
  # escape special chars
  line = re.sub (r'([{}@])', r'@\1', line)
  # match statements
  if (re.match (r'^[0-9A-Za-z_]', line)):       # start of ChangeLog entry
    line = re.sub (r'<[^>]*>', '', line)        # strip email adresses
    line = '@doxer_start_section{level=3,title-span=ChangeLog-title,body-span=ChangeLog-body} ' + line
  elif (re.match (r'^\s+\*\s+[^:]+:(?=(\s|$))', line)): # ChangeLog file entry
    mo = re.search (r'^(\s+\*\s+)([^:\s]+)', line)
    e = mo.end()
    fstring = line[:e]
    rest = line[e:]
    fstring = re.sub (r'^(\s+\*\s+)([^:\s]+)', r'\1@doxer_span{ChangeLog-file,\2}', fstring)
    rest = markup_text (rest)
    line = fstring + rest
  elif (re.match (r'^\s+', line)):              # text line, preceeded by spaces
    line = markup_text (line)
  else:                                         # unknown format
    line = line
  # strip newlines
  while line and line[-1] in '\n\r':
    line = line[:-1]
  # explicit newline markup
  line += '@*\n'
  return line

for line in sys.stdin:
  line = markup_line (line)
  sys.stdout.write (line)
