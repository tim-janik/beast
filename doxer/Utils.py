#!/usr/bin/env python2.4
#
# Doxer - Software documentation system
# Copyright (C) 2005-2006 Tim Janik
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

def template_copy (fin, fout, template_name, template_hook, variables = {}, reference_name = ''):
  key = '@' + template_name
  ref = reference_name and '@' + reference_name or ''
  for line in fin:
    if line.find (key) < 0 and (not ref or line.find (ref) < 0):
      fout.write (line)
      continue
    while line:
      if line[0] != '@':
        ch = line[0]
        line = line[1:]
        fout.write (ch)
        continue
      # before '@'
      mo = re.match (key + '{([^{}]*)}', line)
      if mo:
        arg = mo.group (1)
        template_hook (fout, template_name, arg)
        line = line[mo.end():]          # advance read pointer
        continue
      if ref:
        mo = re.match (ref + '{([^{}]*)}', line)
      if mo:
        arg = mo.group (1)
        if not variables.has_key (arg):
          raise RuntimeError ('Unknown identifier in: %s' % mo.group (0))
        fout.write (variables[arg])
        line = line[mo.end():]          # advance read pointer
        continue
      # no match
      chs = line[0:2]
      line = line[2:]
      fout.write (chs)                # write out '@' + escape character
