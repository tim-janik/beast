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
import os, Config
def debug (*args): Config.debug_print (*args)

# --- command flags ---
SECTIONED, SCOPED, CLAIM_PARA, SPAN_LINE, SPAN_WORD, QUOTABLE, NESTING = 1, 2, 4, 8, 16, 32, 64
custom_command_flags_dict = {
  # "sectioned":        SECTIONED,
  "scoped":             SCOPED | SPAN_LINE,
  "span_line":          SPAN_LINE,
  "claim_para":         CLAIM_PARA,
  "span_word":          SPAN_WORD,
  "quotable":           QUOTABLE,
  "nesting":            NESTING,
}
SPAN_LINE_MASK = SPAN_LINE | SECTIONED

# --- token tree nodes ---
class TextNode:
  anchor = ''
  numbering = ''
  _id = ''
  _class = ''
  group = ''
  align = ''
  span = ''
  title_span = ''
  body_span = ''
  hidden = False
  generate_toc = False
  def __init__ (self, fname, fline, name = '', flags = 0):
    self.fname = fname
    self.fline = fline
    self.name = name
    self.arg = []
    self.title = []
    self.contents = []
    self.flags = flags
  def clone_macro (self):
    n = TextNode (self.fname, self.fline, self.name)
    n.__dict__.update (self.__dict__)
    return n
  def tostring (self):
    desc = "%s:" % TextNode, "name=%s" % self.name, "arg=%s" % self.arg, "title=%s" % self.title, "contents=%s" % self.contents
    return "%s" % (desc,)
  def descendants (self):
    return self.IterDescendants (self)
  class IterDescendants:
    def __iter__ (self):
      return self
    def __init__ (self, node):
      self.tlist = []
      if node:
        self.feed_node (node)
    def feed_node (self, node):
      self.tlist = list (node.arg) + list (node.title) + list (node.contents) + self.tlist
    def next (self):
      while self.tlist:
        v = self.tlist.pop (0)
        if isinstance (v, TextNode):
          self.feed_node (v)
          return v
      raise StopIteration()
TextBlock = TextNode

# --- source code nodes ---
class Documentable:
  name = None
  doc = []
  loc_file = None
  loc_line = 0
  hint = ''
  def set_name (self, name):
    self.name = name
  def set_doc (self, source, fname, fline, sectionname = ''):
    if source or sectionname:
      self.doc = [ source, fname, fline, sectionname ]
    else:
      self.doc = []
  def set_location (self, file, line):
    self.loc_file = file
    self.loc_line = line
  def location (self):
    return (self.loc_file, self.loc_line)

class Parameter (Documentable):
  init = None
  argstring = ''
  default = ''
  label = ''
  group = ''
  isconst = False
  def __init__ (self, name, type = None):
    self.name = name
    self.type = type
  def set_type (self, type):
    self.type = type
  def set_const (self, arg):
    self.isconst = bool (arg)
  def set_label (self, label):
    self.label = label
  def set_init (self, initializer):
    self.init = initializer
  def set_argstring (self, astring):
    self.argstring = astring
  def set_group (self, group):
    self.group = group
  def set_default (self, default):
    self.default = default

class SrcFile (Documentable):
  preserve_extension = True
  def __init__ (self, fullpath):
    assert os.path.isabs (fullpath)
    self.name = fullpath
    self.set_location = None
    self.members = []
    self.html_file = None
    self.doxi_file = None
  def add_member (self, mem):
    assert mem.set_location and mem.loc_file != None # check for proper Documentable
    self.members += [ mem ]
  def list_members (self, classinfo = None):
    r = []
    for m in self.members:
      if not classinfo or isinstance (m, classinfo):
        r += [ m ]
    return r

class Typedef (Documentable):
  argstring = ''
  def __init__ (self, name, type = None):
    self.name = name
    self.type = type
  def set_type (self, type, argstring = ''):
    self.type = type
    if argstring:
      self.argstring = argstring

class Macro (Documentable):
  args = ()
  doc_args = ()
  has_ellipsis = False
  is_constant = False
  def __init__ (self, name):
    self.name = name
  def add_arg (self, param):
    self.args = list (self.args) + [ param ]
  def add_doc_arg (self, name, text):
    self.doc_args = list (self.doc_args) + [ (name, text) ]
  def set_is_constant (self, toggle):
    self.is_constant = toggle

class Enum (Documentable):
  values = ()
  def __init__ (self, name):
    self.name = name
  def add_value (self, param):
    self.values = list (self.values) + [ param ]

class Function (Documentable):
  ret_arg = Parameter ('RETURNS', 'void')
  args = ()
  doc_args = ()
  has_ellipsis = False
  isstatic = False
  isvirtual = False
  isinline = False
  isexplicit = False
  isconst = False
  def __init__ (self, name):
    self.name = name
  def set_flags (self, isstatic, isvirtual, isinline, isexplicit, isconst):
    self.isstatic, self.isvirtual, self.isinline, self.isexplicit, self.isconst = (
      bool (isstatic), bool (isvirtual), bool (isinline), bool (isexplicit), bool (isconst))
  def add_arg (self, param):
    self.args = list (self.args) + [ param ]
  def add_arg_docu (self, name, text):
    self.doc_args = list (self.doc_args) + [ (name, text) ]
  def set_ret_arg (self, param):
    self.ret_arg = param
    self.ret_arg.name = 'RETURNS'
  def set_ellipsis (self, toggle):
    self.has_ellipsis = toggle
  def arg_name_set (self):
    return set ([ a.name for a in self.args ])

class Struct (Documentable):
  members = ()
  methods = ()
  derived = ()
  def __init__ (self, name):
    self.name = name
  def add_member (self, param):
    self.members = list (self.members) + [ param ]
  def add_method (self, func):
    self.methods = list (self.methods) + [ func ]
  def add_derived (self, cls):
    self.derived = list (self.derived) + [ cls ]
  def isclass (self):
    return self.methods != ()

class Channel (Documentable):
  id = 0
  label = ''
  kind = ''
  def __init__ (self, name):
    self.name = name
  def setup (self, id, kind, label):
    self.id = id
    self.kind = kind
    self.label = label

class Object (Documentable):
  properties = []
  signals = []
  channels = []
  def __init__ (self, name):
    self.name = name
  def add_property (self, param):
    self.properties = list (self.properties) + [ param ]
  def list_properties (self):
    return self.properties[:]
  def add_signal (self, func):
    self.signals = list (self.signals) + [ func ]
  def list_signals (self):
    return self.signals[:]
  def add_channel (self, ch):
    self.channels = list (self.channels) + [ ch ]
  def list_channels (self):
    return self.channels[:]

# --- XML transformation ---
class XML:
  def serialize_node (root, ofile):
    assert os.path.isabs (ofile)
    debug ("generating: %s..." % ofile)
    fout = open (ofile, "w")
    print >>fout, '<?xml version="1.0" encoding="UTF-8"?>'
    xml_string = XML.transform_node (root)
    print >>fout, xml_string
    fout.close()
  serialize_node = staticmethod (serialize_node)
  def transform_node (text_node):
    assert isinstance (text_node, TextNode)
    result = ''
    result += '<' + text_node.name + XML.text_node_attributes (text_node) + '>'
    result += '<arg>' + XML.transform_list (text_node.arg) + '</arg>'
    if text_node.title:
      result += '<title>' + XML.transform_list (text_node.title) + '</title>'
    if text_node.contents:
      result += XML.transform_list (text_node.contents)
    result += '</' + text_node.name + '>'
    return result
  transform_node = staticmethod (transform_node)
  def transform_list (list):
    result = ''
    for elem in list:
      if isinstance (elem, str):
        result += XML.escape (elem)
      else:
        result += XML.transform_node (elem)
    return result
  transform_list = staticmethod (transform_list)
  def text_node_attributes (text_node):
    return ''
  text_node_attributes = staticmethod (text_node_attributes)
  def escape (string):
    output = ''
    for ch in string:
      if ch == '"':
        output += '&quot;'
      elif ch == '&':
        output += '&amp;'
      elif ch == '<':
        output += '&lt;'
      elif ch == '>':
        output += '&gt;'
      else:
        output += ch
    return output
  escape = staticmethod (escape)
