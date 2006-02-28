#!/usr/bin/env python2.4
#
# Doxer - Software documentation system
# Copyright (C) 2006 Tim Janik
#
# ScadParser.py - parser for source code analysis dumps
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
import os, sys, re, Config, Data, linkdict, DoxiParser
def debug (*args): Config.debug_print (*args)


#--- SCAD parser ---
class SCADParser:
  def __init__ (self):
    pass
  def parse_file (self, filename):
    fin = open (filename, "r")
    global_dict, local_dict = {}, {}
    exec (fin, global_dict, local_dict)
    fin.close()
    sfile = Data.SrcFile (os.path.abspath (filename))
    if local_dict.has_key ('functions'):
      for fhash in local_dict['functions']:
        sfile.add_member (self.parse_function (fhash))
    if local_dict.has_key ('structures'):
      for shash in local_dict['structures']:
        sfile.add_member (self.parse_structure (shash))
    if local_dict.has_key ('objects'):
      for ohash in local_dict['objects']:
        sfile.add_member (self.parse_object (ohash))
    return [ sfile ]
  def parse_parameter (self, ptuple):
    # (name, type, rest_string, init, ('blurb', 'file', 0, 'section'), label, group)
    p = Data.Parameter (ptuple[0])
    if len (ptuple) > 1:
      p.set_type (ptuple[1])
    if len (ptuple) > 2:
      p.set_argstring (ptuple[2])
    if len (ptuple) > 3:
      p.set_init (ptuple[3])
    if len (ptuple) > 4:
      p.set_doc (ptuple[4][0], ptuple[4][1], ptuple[4][2])
    if len (ptuple) > 5:
      p.set_label (ptuple[5])
    if len (ptuple) > 6:
      p.set_group (ptuple[6])
    return p
  def parse_function (self, fhash):
    f = Data.Function (fhash['name'])
    if fhash.has_key ('args'):
      for a in fhash['args']:
        p = self.parse_parameter (a)
        f.add_arg (p)
    if fhash.has_key ('return'):
      p = self.parse_parameter (fhash['return'])
      f.set_ret_arg (p)
    if fhash.has_key ('location'):
      loc = fhash['location'] # ('file', 0)
      f.set_location (loc[0], loc[1])
    else:
      f.set_location ('', 0)
    if fhash.has_key ('ellipsis'):
      f.set_ellipsis (fhash['ellipsis'])
    if fhash.has_key ('description'):
      desc = fhash['description']
      f.set_doc (desc[0], desc[1], desc[2])
    return f
  def parse_structure (self, shash):
    s = Data.Struct (shash['name'])
    if shash.has_key ('fields'):
      for a in shash['fields']:
        p = self.parse_parameter (a)
        s.add_member (p)
    if shash.has_key ('location'):
      loc = shash['location'] # ('file', 0)
      s.set_location (loc[0], loc[1])
    else:
      s.set_location ('', 0)
    if shash.has_key ('description'):
      desc = shash['description']
      s.set_doc (desc[0], desc[1], desc[2])
    if shash.has_key ('hint'):
      s.hint = shash['hint']
    return s
  def parse_channel (self, chash):
    c = Data.Channel (chash['name'])
    c.setup (chash.get ('id', 0), chash.get ('kind', ''), chash.get ('label', ''))
    if chash.has_key ('location'):
      loc = chash['location'] # ('file', 0)
      c.set_location (loc[0], loc[1])
    else:
      c.set_location ('', 0)
    if chash.has_key ('description'):
      desc = chash['description']
      c.set_doc (desc[0], desc[1], desc[2])
    return c
  def parse_signal (self, stuple):
    f = Data.Function (stuple[0])               # name
    p = Data.Parameter ('return', stuple[1])    # return type
    f.set_ret_arg (p)
    if len (stuple) >= 3 and stuple[2]:         # argument types
      n = 1
      for atype in stuple[2]:
        p = Data.Parameter ('arg%u' % n, atype)
        f.add_arg (p)
        n += 1
    return f
  def parse_object (self, ohash):
    f = Data.Object (ohash['name'])
    if ohash.has_key ('properties'):
      for a in ohash['properties']:
        p = self.parse_parameter (a)
        f.add_property (p)
    if ohash.has_key ('signals'):
      for a in ohash['signals']:
        p = self.parse_signal (a)
        f.add_signal (p)
    if ohash.has_key ('channels'):
      for ch in ohash['channels']:
        c = self.parse_channel (ch)
        f.add_channel (c)
    if ohash.has_key ('location'):
      loc = ohash['location'] # ('file', 0)
      f.set_location (loc[0], loc[1])
    else:
      f.set_location ('', 0)
    if ohash.has_key ('description'):
      desc = ohash['description']
      f.set_doc (desc[0], desc[1], desc[2])
    return f

#--- public API ---
def parse_file (filename):
  sp = SCADParser()
  return sp.parse_file (filename)
