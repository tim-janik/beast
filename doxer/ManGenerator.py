#!/usr/bin/env python2.4
#
# Doxer - Software documentation system
# Copyright (C) 2006 Tim Janik
# Copyright (C) 2007 Stefan Westerfeld
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
import os, sys, re, Config, Data, DoxiParser, warnings
from Config import doxer_warn_if_reached

def debug (*args): Config.debug_print (*args)

# --- ManOStream ---
class ManOStream:
  def __init__ (self):
    self.output = ''
    self.inline = 0
    self.tag_stack = []
    self.toggle_spaces = 0
    self.preformatted = 0
    self.skiptext = 0
    self.raw_entities = 0
    self.output_marks = set ()
  def __lshift__ (self, other):
    self.put (other)
  def output_has_tag (self, mark):
    return mark in self.output_marks
  def output_add_tag (self, mark):
    self.output_marks.add (mark)
  def output_changed (self):
    self.output_marks = set ()
  def push (self, tag, args = ''):
    if not self.skiptext:
      self.output += '<%s' % tag
      if args:
        if args[0] != ' ':
          self.output += ' '
        self.output += args
      self.output += '>'
      self.output_changed()
    self.tag_stack += [ tag ]
  def single (self, tagstring, need_slash = True):
    if need_slash:
      assert tagstring.find ('/>') >= 0
    else:
      assert tagstring.find ('>') >= 0
    if not self.skiptext:
      self.output += tagstring
      self.output_changed()
  def pop (self, tag = None):
    if tag:
      if tag != self.tag_stack[-1]:
        raise RuntimeError ('Tag mismatch: %s != %s' % (tag, self.tag_stack[-1]))
    lasttag = self.tag_stack.pop()
    if not self.skiptext:
      self.output += '</%s>' % lasttag
      self.output_changed()
  def append_raw (self, raw_string):
    self.output += raw_string
    self.output_changed()
  def putc (self, chr):
    assert len (chr) == 1
    if self.skiptext:
      return
    if chr in ' \t':
      # collapse all spaces
      if self.toggle_spaces:
        self.output += chr
      elif not self.output[-1:] in ' \n':
        self.output += ' '
      self.output_changed()
      return
    if self.raw_entities:
      self.output += chr
      self.output_changed()
      return
    if chr == '-':
      self.output += '\\';
    self.output += chr
    self.output_changed()
  def put (self, string):
    for ch in string:
      self.putc (ch)
  def push_many (self, tuple):
    for tag in tuple:
      self.push (tag)
  def pop_many (self, tuple):
    for tag in tuple:
      self.pop (tag)
  def isopen (self, tag):
    return len (self.tag_stack) and tag == self.tag_stack[-1]
  def find_open (self, taglist):
    for tag in self.tag_stack:
      if tag in taglist:
        return tag
    return None
  def open_inline (self, string = ''):
    self.inline += 1
  def close_inline (self):
    if not self.inline:
      raise RuntimeError ("Out of inline scope")
    self.inline -= 1
  def open_block (self, string = ''):
    if self.inline:
      raise RuntimeError ("Inside inline scope at: <%s>" % string)
  def close_block (self):
    pass
  def quote_string (string):
    result = ''
    trans = { '"': '\\"', '\n': '\\\n' }
    for c in string:
      d = trans.get (c)
      result += d and d or c
    return result
  quote_string = staticmethod (quote_string)

# --- node transform flags ---
NODE_ARG, NODE_TITLE, NODE_CONTENTS = (1, 2, 4)
NODE_TEXT = NODE_TITLE | NODE_CONTENTS
NODE_ALL = NODE_ARG | NODE_TITLE | NODE_CONTENTS

# --- User Errors ---
class SemanticError (RuntimeError): pass

# --- helpers ---
def ensure_list (value):
  if isinstance (value, list) or isinstance (value, tuple):
    return value
  if not value:
    return []
  return [ value ]

# --- man generator ---
class ManGenerator:
  def __init__ (self, manstream):
    self.mstream = manstream
    self.call_stack = []
    self.list_item_open = False
  # transformation functions
  def lookup_handler (self, tagname, tagdict):
    handler = tagdict.get (tagname)
    if not handler:
      return None
    data = None
    if isinstance (handler, tuple):
      if len (handler) > 1:
        data = handler[1]
      handler = handler[0]
    # turn unbound handler into a bound method
    return (handler.__get__ (self), data)
  def transform_node_expandable (self, node, keyword_dict, caller):
    if node.name != 'doxer_args':
      self.transform_node (node, keyword_dict)
    else:
      token_list = DoxiParser.expand_doxer_args (DoxiParser.plain_text_from_token_list (node.arg), caller)
      for tok in token_list:
        self.transform_any (tok, keyword_dict)
  def transform_text (self, string, keyword_dict):
    self.mstream << string
  def transform_multi_lines (self, string, keyword_dict):
    if (self.mstream.preformatted or                    # don't split preformatted text
        string.find ('\n') < 0 or                       # need newlines for splitting
        not re.search (r'\S', string)):                 # only split around non whitespace
      self.transform_text (string, keyword_dict)
      return
    self.transform_text (string, keyword_dict)
    return
  def transform_node (self, node, keyword_dict):
    self.call_stack += [ node ]
    name = node.name
    handlerb = self.lookup_handler (name, self.block_tagdict)
    handleri = handlerb and None or self.lookup_handler (name, self.inline_tagdict)
    if handlerb:
      self.mstream.open_block (node.name)
      handlerb[0] (handlerb[1], self.transform_any_contents, node, keyword_dict)
      self.mstream.close_block ()
    elif handleri:
      self.mstream.open_inline (node.name)
      handleri[0] (handleri[1], self.transform_any_contents, node, keyword_dict)
      self.mstream.close_inline ()
    else:
      raise SemanticError ('%s:%u: Undefined text markup: %s' % (node.fname, node.fline, node.name))
    an = self.call_stack.pop()
    assert an == node
  def transform_contents (self, node, keyword_dict, node_flags = NODE_ALL):
    foreign_node = not node in self.call_stack
    if foreign_node:
      self.call_stack += [ node ]
    if node_flags & NODE_ARG:
      for el in node.arg:
        self.transform_any (el, keyword_dict)
    if node_flags & NODE_TITLE:
      for el in node.title:
        self.transform_any (el, keyword_dict)
    if node_flags & NODE_CONTENTS:
      for el in node.contents:
        self.transform_any (el, keyword_dict)
    if foreign_node:
      an = self.call_stack.pop()
      assert an == node
  def transform_any_contents (self, node_or_str_or_list, keyword_dict, node_flags = NODE_ALL):
    if isinstance (node_or_str_or_list, str):
      self.transform_multi_lines (node_or_str_or_list, keyword_dict)
    elif isinstance (node_or_str_or_list, list) or isinstance (node_or_str_or_list, tuple):
      for element in node_or_str_or_list:
        self.transform_any (element, keyword_dict)
    elif node_or_str_or_list:
      self.transform_contents (node_or_str_or_list, keyword_dict, node_flags)
  def transform_any (self, token, keyword_dict):
    if isinstance (token, str):
      self.transform_multi_lines (token, keyword_dict)
    elif token:
      self.transform_node (token, keyword_dict)
  # block tag handlers
  def doxi_document (self, data, transformer, node, keyword_dict):
    env = node.root.variables
    mtitle = env.get ('man-title') or env.get ('title')
    msection = env.get ('man-section')
    mdate = env.get ('man-date')
    msource = env.get ('man-source')
    mmanual = env.get ('man-manual')
    self.mstream << '.\\" generator: doxer.py %s\n' % Config.VERSION
    for tag in ('author', 'description', 'keywords'):
      content = env.get (tag)
      if content:
        self.mstream << '.\\" %s: %s\n' % (tag, content)
    isonow = DoxiParser.datetime_isoformat (DoxiParser.datetime_parse (env['today']))
    self.mstream << '.\\" generation: %s\n' % isonow
    self.mstream << '.TH "' + mtitle + '" "' + msection + '" "' + mdate + '" "' + msource + '" "' + mmanual + '"\n'
    transformer (node, keyword_dict)
  def doxer_group (self, dummy, transformer, node, keyword_dict):
    transformer (node, keyword_dict)
  def token_list_is_empty (self, l):
    for el in l:
      if isinstance (el, Data.TextNode):
        return False
      if el.strip():
        return False
    return True
  def section (self, dummy, transformer, node, keyword_dict):
    if node.hidden or self.token_list_is_empty (node.title):
      return
    # process args
    tokens = node.arg
    prefix = node.numbering and ('%s ' % node.numbering) or ''
    align = node.align and (' align="%s"' % node.align) or ''
    id_stmt = node._id and (' id="%s"' % ManOStream.quote_string (node._id)) or ''
    span_stmt = node.span and (' class="%s"' % ManOStream.quote_string (node.span)) or ''
    # write out section title
    hx = { 0: 'SH', 1: 'SH', 2: 'SS' }.get (node.level, 'SS')
    self.mstream << '\n.' + hx + '\n'
    self.mstream << prefix
    transformer (node, keyword_dict, NODE_TITLE)
    self.mstream << '\n'
    # write out section contents
    if node.contents or node.generate_toc:
      self.mstream << '\n\n.PP\n'
      transformer (node, keyword_dict, NODE_CONTENTS)
  def template_hook (self, data, transformer, node, keyword_dict):
    if not self.filler:
      raise SemanticError ('%s:%u: Template macro used in non-template context' % (node.fname, node.fline))
    class RawStream:
      def __init__ (self, hstream, root):
        self.mstream = hstream
        self.section_dict = {}
        for element in root.contents:
          if isinstance (element, Data.TextNode) and element.group:
            self.section_dict[element.group] = element
      def write (self, x):
        self.mstream.append_raw (x)
      def __lshift__ (self, other):
        self.mstream.append_raw (other)
      def insert_section (self, section_name):
        section = self.section_dict.get (section_name, [])
        oldhidden = section.hidden
        section.hidden = False
        transformer ([ section ], keyword_dict, NODE_TEXT)
        section.hidden = oldhidden
    debug ("Filling template...")
    rs = RawStream (self.mstream, node.root)
    self.filler (rs)
  # block tag => handler dictionary
  block_tagdict = {
    'doxi-document'             : doxi_document,
    'doxer-group'               : doxer_group,
    'doxer_start_section'       : ( section, 0, ),
    'doxer_template_hook'       : template_hook,
  }
  # inline tag handlers
  def doxer_visibility (self, data, transformer, node, keyword_dict):
    enable = node.name == 'doxer_hidden'
    if node.arg and isinstance (node.arg[0], str):
      match = re.search (r'([-+]?[0-9]+)', node.arg[0])
      if match:
        enable = int (match.group (0))
    if ((enable and node.name == 'doxer_visible') or
        (not enable and node.name == 'doxer_hidden')):
      transformer (node, keyword_dict, NODE_TEXT)
  def pass_through (self, data, transformer, node, keyword_dict):
    transformer (node, keyword_dict, NODE_TEXT)
  def pass_block (self, data, transformer, node, keyword_dict):
    self.mstream << '\n'
    transformer (node, keyword_dict, NODE_TEXT)
    self.mstream << '\n\n.PP\n'
  def doxer_list_get_innermost_type (self):
    ltype = 'none'
    for node in reversed (self.call_stack):
      if node.name == 'doxer_list':
        tokens = node.arg
        while tokens:
          arg, tokens = DoxiParser.split_arg_from_token_list (tokens)
          arg = DoxiParser.plain_text_from_token_list (arg).strip()
          if   arg == 'BULLET':     ltype = 'bullet'
          elif arg == 'bullet':     ltype = 'bullet'
          elif arg == 'none':       ltype = 'none'
          else:
            print >> sys.stderr, "%s:%u: warning: manual page backend does not support list type: %s" % (node.fname, node.fline, arg)
        return ltype
    doxer_warn_if_reached() # this function is designed to be called in doxer_list contexts only
    return ltype
  def doxer_list_close_item (self):
    if self.list_item_open and self.doxer_list_get_innermost_type() == 'bullet':
      self.mstream << '\n.LP\n'
    self.list_item_open = False
  def doxer_list (self, data, transformer, node, keyword_dict):
    old_list_item_open = self.list_item_open
    self.list_item_open = False
    self.mstream << '\n'
    transformer (node, keyword_dict, NODE_TEXT)
    self.doxer_list_close_item()
    self.mstream << '\n\n.PP\n'
    self.list_item_open = old_list_item_open
  def doxer_item (self, data, transformer, node, keyword_dict):
    assert (len (self.call_stack) >= 2) # item can only occur inside lists and deflists
    if self.call_stack[-2].name == 'doxer_list':
      self.doxer_list_close_item()
      self.list_item_open = True
      list_type = self.doxer_list_get_innermost_type()
      if list_type   == 'none':   self.mstream << '\n.TP\n'
      elif list_type == 'bullet': self.mstream << '\n.IP " *" 3\n'
      else:                       doxer_warn_if_reached()
      transformer (node, keyword_dict, NODE_TEXT)
    else:                               # doxer_deflist
      self.mstream << '\n.TP\n'
      transformer (node, keyword_dict)
  def bold (self, tagname, transformer, node, keyword_dict):
    self.mstream << r'\fB'
    transformer (node, keyword_dict)
    self.mstream << r'\fP'
  def italic (self, tagname, transformer, node, keyword_dict):
    self.mstream << r'\fI'
    transformer (node, keyword_dict)
    self.mstream << r'\fP'
  def doxer_uri (self, longform, transformer, node, keyword_dict):
    def convert_uri (uri):
      if uri[:4] == 'man:':
        uri = uri[4:]
      return uri
    if not node.arg or not isinstance (node.arg[0], str):
      raise SemanticError ("%s:%u: Tag '%s' requires a URI" % (node.fname, node.fline, node.name))
    token_list = node.arg[:]
    atext = ''
    while token_list and isinstance (token_list[0], str):
      atext += token_list.pop (0)
    cpos = atext.find (',')
    if cpos >= 0:
      uri = atext[:cpos].strip()
      atext = atext[cpos+1:].strip()
      uri = convert_uri (uri)
    else:
      uri = ""
      atext = convert_uri (atext)
    if atext:
      token_list = [ atext ] + token_list
    uri = self.mstream.quote_string (uri)
    saved_arg = node.arg # temporary args shift
    node.arg = token_list
    transformer (node, keyword_dict)
    if uri and longform:
      self.mstream << ' (' + self.mstream.quote_string (uri) + ')'
    node.arg = saved_arg
  def doxer_raw (self, data, transformer, node, keyword_dict):
    atext = DoxiParser.plain_text_from_token_list (node.arg)
    cpos = atext.find (',')
    if cpos >= 0:
      kind = atext[:cpos].strip()
      atext = atext[cpos+1:].strip()
      if kind.strip() == 'man':
        self.mstream.raw_entities += 1
        self.mstream.put (atext)
        self.mstream.raw_entities -= 1
  def linebreak (self, data, transformer, node, keyword_dict):
    if (node.arg != [ 'automatic_newline' ] or  # collapse consecutive automatic breaks
        not self.mstream.output_has_tag ('br')):
      self.mstream << '\n.br\n' # FIXME: untested
      self.mstream.output_add_tag ('br')
  # inline tag => handler dictionary
  inline_tagdict = {
    'doxer_bold'        : bold,
    'doxer_italic'      : italic,
    'doxer_fontsize'    : pass_through,
    'doxer_newline'     : linebreak,
    'doxer_visible'     : doxer_visibility,
    'doxer_hidden'      : doxer_visibility,
    'doxer_list'        : doxer_list,
    'doxer_deflist'     : pass_block,
    'doxer_item'        : doxer_item,
    'doxer_table'       : pass_block,
    'doxer_row'         : pass_through,
    'doxer_cell'        : doxer_item,
    'doxer_uri'         : ( doxer_uri, 0),
    'doxer_longuri'     : ( doxer_uri, 1),
    'doxer_image'       : pass_through,
    'doxer_anchor'      : pass_through,
    'doxer_raw'         : doxer_raw,
  }

def process_root (root, filler = None):
  hs = ManOStream ()
  hgen = ManGenerator (hs)
  hgen.filler = filler
  try:
    hgen.transform_any (root, {})
    # here, we're catching exceptions raised by the semantic parser
  except SemanticError, ex:
    print >>sys.stderr, str (ex)
    if Config.debug:
      raise
    # exit silently if not debugging
    os._exit (1)
  return hs.output
