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
import os, sys, re, Config, Data, linkdict, DoxiParser
def debug (*args): Config.debug_print (*args)

# --- User Errors ---
class SemanticError (RuntimeError): pass

# --- HtmlOStream ---
class HtmlOStream:
  def __init__ (self):
    self.output = ''
    self.output_list = []
    self.inline = 0
    self.tag_stack = []
    self.toggle_spaces = 0
    self.preformatted = 0
    self.skiptext = 0
    self.raw_entities = 0
    self.output_marks = set ()
    self.in_block_count = 0
  def joined_output (self):
    return "".join (self.output_list + [ self.output ])
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
    if self.toggle_spaces and chr == ' ':
      # produce ' ' and '&nbsp;' in alternating order to avoid browsers
      # collapsing multiple spaces. give precedence to ' ' to preserve
      # line breaking functionality
      if self.output[-1:] in (' ', '>'):       # self.output[-6:] != '&nbsp;':
        self.output += '&nbsp;'
        self.output_changed()
        return
    if self.raw_entities:
      self.output += chr
      self.output_changed()
      return
    if len (self.output) > 256:
      # shorten the string, so appending doesn't take too long for big output files
      self.output_list += [ self.output ]
      self.output = ""
    self.output += self.quote_string (chr)
    self.output_changed()
  def put (self, string):
    #if not self.skiptext and not self.toggle_spaces:
    #  if self.raw_entities or not re.search (r'["&<>]', string):
    #    self.output += string
    #    self.output_changed()
    #    return
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
  def open_block (self, string, fname, fline):
    if self.inline:
      raise SemanticError ('%s:%u: Inside inline scope at: %s' % (fname, fline, string))
    self.in_block_count += 1
  def close_block (self):
    if self.in_block_count < 1:
      raise RuntimeError ("No block to close")
    self.in_block_count -= 1
  def in_block (self):
    return in_block_count > 0
  def quote_string (string, allow_breaks = False):
    result = ''
    trans = { '"': '&quot;', '&': '&amp;', '<': '&lt;', '>': '&gt;' }
    for c in string:
      d = trans.get (c)
      result += d and d or c
    if allow_breaks:
      #result = re.sub (r'/\b', '/&shy;', result) # not widely supported
      #result = re.sub (r'/\b', '/&#8203;', result) # zero width space forces placeholder in many fonts
      #result = re.sub (r'/\b', '<span style="white-space:normal">/</span>', result) # browsers still want a space to break
      result = re.sub (r'/\b', '/<wbr/>', result) # 'wbr' instead of 'wbr/' screws konquerors parsing
    return result
  quote_string = staticmethod (quote_string)

# --- node transform flags ---
NODE_ARG, NODE_TITLE, NODE_CONTENTS = (1, 2, 4)
NODE_TEXT = NODE_TITLE | NODE_CONTENTS
NODE_ALL = NODE_ARG | NODE_TITLE | NODE_CONTENTS

# --- helpers ---
def ensure_list (value):
  if isinstance (value, list) or isinstance (value, tuple):
    return value
  if not value:
    return []
  return [ value ]

# --- html generator ---
class HtmlGenerator:
  def __init__ (self, htmlstream, custom_lookup = None):
    self.hstream = htmlstream
    self.call_stack = []
    self.block_linkdict = 0
    self.custom_lookup = custom_lookup
    self.number_pattern = self.compile_number()
  def compile_number (self):
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
    return (pat, 'span', 'class="doxer-style-autonumber"')
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
  def transform_node_expandable (self, node, environment, caller):
    if node.name != 'doxer_args':
      self.transform_node (node, environment)
    else:
      token_list = DoxiParser.expand_doxer_args (DoxiParser.text_from_token_list (node.arg), caller)
      for tok in token_list:
        self.transform_any (tok, environment)
  def transform_text (self, string, environment):
    if self.block_linkdict:
      self.hstream << string
    else:
      def hstream_markup (string, patlist):
        if not patlist:
          if string:
            self.hstream << string
          return
        pat = patlist[0]
        mo = pat[0].search (string)
        if mo:
          s, e = mo.start(), mo.end()
          part1 = string[:s]
          match = string[s:e]
          part3 = string[e:]
          if part1:
            hstream_markup (part1, patlist[1:])
          self.hstream.push (pat[1], pat[2])
          self.hstream << match
          self.hstream.pop (pat[1])
          if part3:
            hstream_markup (part3, patlist)
          return
        hstream_markup (string, patlist[1:])
        return
      # perform keyword-based linkdict link-markup
      tlist = linkdict.markup_tuples (string, self.custom_lookup)
      # re-join word-seperated token list
      jlist = [ '' ]
      for tok in tlist:
        if isinstance (tok, str):
          if isinstance (jlist[-1], str):
            jlist[-1] += tok                                    # re-join consequtive strings
          elif tok[:2] == '()' and isinstance (jlist[-1], tuple):
            jlist[-1] = (jlist[-1][0], jlist[-1][1] + tok[:2])  # re-append '()' to marked-up functions
            if tok[2:]:
              jlist += [ tok[2:] ]
          else:
            jlist += [ tok ]
        else:
          jlist += [ tok ]
      if not jlist[0]:
        jlist.pop (0)
      # write out tuples and markup ordinary strings
      for tok in jlist:
        if isinstance (tok, tuple):
          self.hstream.push ('a', 'href="%s"' % tok[0])
          self.hstream << tok[1]
          self.hstream.pop ('a')
        else:
          hstream_markup (tok, [ self.number_pattern ])
  def transform_multi_lines (self, string, environment):
    if (self.hstream.preformatted or                    # don't split preformatted text
        string.find ('\n') < 0 or                       # need newlines for splitting
        not re.search (r'\S', string)):                 # only split around non whitespace
      self.transform_text (string, environment)
      return
    self.transform_text (string, environment)
  def transform_node (self, node, environment):
    self.call_stack += [ node ]
    name = node.name
    handlerb = self.lookup_handler (name, self.block_tagdict)
    handleri = handlerb and None or self.lookup_handler (name, self.inline_tagdict)
    if handlerb:
      self.hstream.open_block (node.name, node.fname, node.fline)
      handlerb[0] (handlerb[1], self.transform_any_contents, node, environment)
      self.hstream.close_block ()
    elif handleri:
      self.hstream.open_inline (node.name)
      handleri[0] (handleri[1], self.transform_any_contents, node, environment)
      self.hstream.close_inline ()
    else:
      raise SemanticError ('%s:%u: Undefined text markup: %s' % (node.fname, node.fline, node.name))
    an = self.call_stack.pop()
    assert an == node
  def transform_contents (self, node, environment, node_flags = NODE_ALL):
    foreign_node = not node in self.call_stack
    if foreign_node:
      self.call_stack += [ node ]
    if node_flags & NODE_ARG:
      for el in node.arg:
        self.transform_any (el, environment)
    if node_flags & NODE_TITLE:
      for el in node.title:
        self.transform_any (el, environment)
    if node_flags & NODE_CONTENTS:
      for el in node.contents:
        self.transform_any (el, environment)
    if foreign_node:
      an = self.call_stack.pop()
      assert an == node
  def transform_any_contents (self, node_or_str_or_list, environment, node_flags = NODE_ALL):
    if isinstance (node_or_str_or_list, str):
      self.transform_multi_lines (node_or_str_or_list, environment)
    elif isinstance (node_or_str_or_list, list) or isinstance (node_or_str_or_list, tuple):
      for element in node_or_str_or_list:
        self.transform_any (element, environment)
    elif node_or_str_or_list:
      self.transform_contents (node_or_str_or_list, environment, node_flags)
  def transform_any (self, token, environment):
    if isinstance (token, str):
      self.transform_multi_lines (token, environment)
    elif token:
      self.transform_node (token, environment)
  # block tag handlers
  def write_meta (self, node, environment):
    self.hstream.single ('<meta http-equiv="Content-Type" content="text/html; charset=utf-8">\n', False)
    self.hstream.single ('<meta name="generator" content="doxer.py %s">\n' % Config.VERSION, False)
    for tag in ('author', 'description', 'keywords'):
      content = environment.get (tag)
      if content:
        self.hstream.single ('<meta name="%s" content="%s">\n' % (tag, HtmlOStream.quote_string (str (content))), False)
    isonow = DoxiParser.datetime_isoformat (DoxiParser.datetime_parse (environment['today']))
    self.hstream.single ('<meta name="date" content="%s">\n' % isonow, False)
    content = environment.get ('html-base-uri')
    if content:
      content = content.strip()
    if content:
      self.hstream.single ('<base href="%s"/>\n' % HtmlOStream.quote_string (content), False)
    content = environment.get ('html-stylesheets')
    for sc in ensure_list (content):
      title = re.sub (r'\s*<[^>]*>\s*', '', sc, 1)
      clist = re.findall (r"<([^>]*)", sc)
      if not clist:
        raise SemanticError ('%s:%u: Missing stylesheet specification (e.g. <sheet.css>) in: %s' % (node.fname, node.fline, sc))
      self.hstream.single ('<link rel="stylesheet" type="text/css" title="%s" href="%s">\n'
                           % (HtmlOStream.quote_string (title), HtmlOStream.quote_string (clist and clist or '')),
                           False)
    content = environment.get ('title')
    if content:
      self.hstream.single ('<title>%s</title>\n' % HtmlOStream.quote_string (str (content)), False)
  def manpage_header (self, data, transformer, node, environment):
    mtitle = environment.get ('man-title') or environment.get ('title')
    msection = environment.get ('man-section')
    mdate = environment.get ('man-date')
    msource = environment.get ('man-source')
    mmanual = environment.get ('man-manual')
    self.hstream.push ('table', 'width="100%" border="0" cellspacing="0" cellpadding="0" rules="none" class="doxer-style-table"')
    self.hstream.put ('\n')
    self.hstream.push ('tr')
    self.hstream.put ('\n')
    self.hstream.push ('td', 'width="33%" valign="top" align="left"')
    self.hstream.push ('h3')
    self.hstream << "%s(%s)" % (mtitle, msection)
    self.hstream.pop ('h3')
    self.hstream.pop ('td')
    self.hstream.push ('td', 'width="34%" valign="top" align="center"')
    self.hstream.push ('h3')
    self.hstream << mmanual
    self.hstream.pop ('h3')
    self.hstream.pop ('td')
    self.hstream.push ('td', 'width="33%" valign="top" align="right"')
    self.hstream.push ('h3')
    self.hstream << "%s(%s)" % (mtitle, msection)
    self.hstream.pop ('h3')
    self.hstream.pop ('td')
    self.hstream.pop ('tr')
    self.hstream.pop ('table')
  def manpage_footer (self, data, transformer, node, environment):
    mtitle = environment.get ('man-title') or environment.get ('title')
    msection = environment.get ('man-section')
    mdate = environment.get ('man-date')
    msource = environment.get ('man-source')
    mmanual = environment.get ('man-manual')
    self.hstream.push ('table', 'width="100%" border="0" cellspacing="0" cellpadding="0" rules="none" class="doxer-style-table"')
    self.hstream.put ('\n')
    self.hstream.push ('tr')
    self.hstream.put ('\n')
    self.hstream.push ('td', 'width="33%" valign="top" align="left"')
    self.hstream.push ('h3')
    self.hstream << msource
    self.hstream.pop ('h3')
    self.hstream.pop ('td')
    self.hstream.push ('td', 'width="34%" valign="top" align="center"')
    self.hstream.push ('h3')
    self.hstream << mdate
    self.hstream.pop ('h3')
    self.hstream.pop ('td')
    self.hstream.push ('td', 'width="33%" valign="top" align="right"')
    self.hstream.push ('h3')
    self.hstream << "%s(%s)" % (mtitle, msection)
    self.hstream.pop ('h3')
    self.hstream.pop ('td')
    self.hstream.pop ('tr')
    self.hstream.pop ('table')
  def doxi_document (self, data, transformer, node, environment):
    # html boilerplate code
    self.hstream.single ('<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">\n', False)
    self.hstream.push ('html')
    self.hstream.put ('\n')
    self.hstream.push ('head')
    self.hstream.put ('\n')
    self.write_meta (node, environment)
    self.hstream.pop ('head')
    self.hstream.put ('\n')
    content = environment.get ('html-classes')
    dclasses = " ".join (ensure_list (content))
    dclasses = dclasses.strip()
    self.hstream.push ('body', dclasses and 'class="%s"' % dclasses or '')
    self.hstream.put ('\n')
    # sort-seperate sections by areas
    area_list = {}
    for i in ('header', 'lheader', 'cheader', 'rheader', 'left', 'content', 'right', 'lfooter', 'cfooter', 'rfooter', 'footer'):
      area_list[i] = []
    for element in node.contents:
      if isinstance (element, Data.TextNode):
        alist = area_list.get (element.group, area_list['content'])
        alist += [ element ]
      else:
        area_list['content'] += [ element ]
    cs = 'width="99%"'  # expand content column
    # due to gecko layout bugs, we write out seperate tables instead of rows here
    for table in ( [ [ ('header',) ] ],
                   self.manpage_header,
                   [ [ ('lheader',), ('cheader',),    ('rheader',) ],
                     [ ('left',),    ('content', cs), ('right',)   ],
                     [ ('lfooter',), ('cfooter',),    ('rfooter',) ] ],
                   self.manpage_footer,
                   [ [ ('footer',) ] ] ):
      if callable (table):      # manpage hooks
        if Config.manpage:
          table (data, transformer, node, environment)
        continue
      self.hstream.push ('table', 'width="100%" border="0" cellspacing="0" cellpadding="0" rules="none" class="doxer-style-table"')
      self.hstream.put ('\n')
      for row in table:
        self.hstream.push ('tr')
        self.hstream.put ('\n')
        for cell in row:
          args = ''
          for arg in cell[1:]:
            args += ' ' + arg
          self.hstream.push ('td', args + ' valign="top" id="%s"' % HtmlOStream.quote_string (cell[0]))
          alist = area_list.get (cell[0], []);
          transformer (alist, environment)
          self.hstream.pop ('td')
          self.hstream.put ('\n')
        self.hstream.pop ('tr')
        self.hstream.put ('\n')
      self.hstream.pop ('table')
      self.hstream.put ('\n')
    self.hstream.pop ('body')
    self.hstream.put ('\n')
    self.hstream.pop ('html')
  def doxer_group (self, dummy, transformer, node, environment):
    asbox = True # forces all sections into one non-floating box
    self.hstream.push ('div', 'id="%s"' % HtmlOStream.quote_string (node._id))
    # print "group:", 'id="%s"' % HtmlOStream.quote_string (node._id)
    if asbox:
      self.hstream.push ('table')
      self.hstream.push ('tr')
      self.hstream.push ('td')
    transformer (node, environment)
    if asbox:
      self.hstream.pop ('td')
      self.hstream.pop ('tr')
      self.hstream.pop ('table')
    self.hstream.pop ('div')
  def token_list_is_empty (self, l):
    for el in l:
      if isinstance (el, Data.TextNode):
        return False
      if el.strip():
        return False
    return True
  def scan_center (self, token_list):
    # somewhat of a hack, needed because '<center>' is a block tag in HTML
    for tok in token_list:
      if isinstance (tok, Data.TextNode):
        if tok.name == 'doxer_center':
          return True
    return False
  def find_children (self, node, name, node_flags = NODE_ALL, depth = 9999999999999999):
    if depth < 0:
      return []
    if node.name == name:
      return [ node ]
    depth -= 1
    if depth < 0:
      return []
    lists = []
    if node_flags & NODE_ARG:
      lists += [ node.arg ]
    if node_flags & NODE_TITLE:
      lists += [ node.title ]
    if node_flags & NODE_CONTENTS:
      lists += [ node.contents ]
    result = []
    for li in lists:
      for el in li:
        if isinstance (el, Data.TextNode):
          result += self.find_children (el, name, node_flags, depth)
    return result
  def section (self, dummy, transformer, node, environment):
    if node.hidden:
      return
    # process args
    tokens = node.arg
    prefix = node.numbering and ('%s ' % node.numbering) or ''
    align = node.align and (' align="%s"' % node.align) or ''
    id_stmt = node._id and (' id="%s"' % HtmlOStream.quote_string (node._id)) or ''
    span_stmt = node.span and (' class="%s"' % HtmlOStream.quote_string (node.span)) or ''
    title_class  = 'section-title section%d-title' % node.level
    title_class += node.title_span and (' %s' % node.title_span) or ''
    body_class   = 'section-body section%d-body' % node.level
    body_class  += node.body_span and (' %s' % node.body_span) or ''
    # write out section skeleton
    self.hstream.put ('\n')
    self.hstream.push ('div', align + span_stmt)
    # write out section title
    title_tails = self.find_children (node, 'doxer_title_tail', NODE_TEXT, 2)
    if title_tails or prefix.strip() or not self.token_list_is_empty (node.title):
      if Config.manpage:
        hx = { 0: 'h1', 1: 'h1' }.get (node.level, 'h3')
      else:
        hx = { 0: 'h1', 1: 'h1', 2: 'h2', 3: 'h3', 4: 'h4', 5: 'h5', 6: 'h6' }.get (node.level, 'h6')
      self.hstream.put ('\n')
      if title_tails:
        self.hstream.push ('table', 'width="100%" border="0" rules="none" class="section-title"')
        self.hstream.push ('tr')
        self.hstream.push ('td', 'align="left"')
      self.hstream.open_inline (node.name)
      alignment = ''
      if self.scan_center (node.title):
        alignment = ' align="center"'
      self.hstream.push (hx, alignment + ' class="%s"' % HtmlOStream.quote_string (title_class))
      if node.anchor:
        self.hstream.push ('a', 'name="%s"' % HtmlOStream.quote_string (node.anchor))
        self.hstream.pop ('a')
      if node.level == 0:
        self.hstream.push ('u')
      self.hstream << prefix
      transformer (node, environment, NODE_TITLE)
      if node.level == 0:
        self.hstream.pop ('u')
      self.hstream.pop (hx)
      self.hstream.close_inline ()
      if title_tails:
        self.hstream.pop ('td')
        self.hstream.push ('td', 'align="center"')
        self.hstream.pop ('td')
        self.hstream.push ('td', 'align="right"')
        env = {}
        env.update (environment)
        env.update ({ 'doxer_title_tail' : True })
        transformer (title_tails, env, NODE_TITLE) # title_tails is a list
        self.hstream.pop ('td')
        self.hstream.pop ('tr')
        self.hstream.pop ('table')
      self.hstream.put ('\n')
    # write out section contents
    if node.contents or node.generate_toc:
      self.hstream.put ('\n')
      self.hstream.push ('div', 'class="%s"' % HtmlOStream.quote_string (body_class))
      transformer (node, environment, NODE_CONTENTS)
      self.doxer_flush_parameters (None, transformer, node, environment)
      if node.generate_toc:
        self.list_sections (dummy, transformer, node, environment)
      self.hstream.pop ('div')
    self.hstream.pop ('div')
    self.hstream.put ('\n')
  def list_sections (self, dummy, transformer, node, environment):
    def empty_section (s):
      if s.hidden:
        return True
      for el in s.title:
        if isinstance (el, Data.TextNode):
          return False
        if el.strip():
          return False
      return True
    if not node.section_list:
      return
    need_break = 0
    for s in node.section_list:
      if empty_section (s):
        continue
      if need_break:
        self.hstream.single ('<br/>\n')
      need_break = 1
      prefix = s.numbering
      level = 0
      try: level += s.level
      except: pass
      for i in range (level):
        self.hstream.toggle_spaces += 1
        self.hstream.put ('    ')
        self.hstream.toggle_spaces -= 1
      if s.toc_anchor:
        self.hstream.push ('a', 'href="#%s"' % s.toc_anchor)
      if prefix.strip():
        self.hstream << prefix
        self.hstream.raw_entities += 1
        self.hstream << ' &middot; '
        self.hstream.raw_entities -= 1
      transformer (s, environment, NODE_TITLE)
      if s.toc_anchor:
        self.hstream.pop ('a')
  def template_hook (self, data, transformer, node, environment):
    if not self.filler:
      raise SemanticError ('%s:%u: Template macro used in non-template context' % (node.fname, node.fline))
    class RawStream:
      def __init__ (self, hstream, root):
        self.hstream = hstream
        self.section_dict = {}
        for element in root.contents:
          if isinstance (element, Data.TextNode) and element.group:
            self.section_dict[element.group] = element
      def write (self, x):
        self.hstream.append_raw (x)
      def __lshift__ (self, other):
        self.hstream.append_raw (other)
      def insert_section (self, section_name):
        section = self.section_dict.get (section_name, [])
        oldhidden = section.hidden
        section.hidden = False
        # transformer == transform_any_contents
        transformer ([ section ], environment, NODE_TEXT)
        section.hidden = oldhidden
    debug ("Filling template...")
    rs = RawStream (self.hstream, node.root)
    self.filler (rs)
  def doxer_definition (self, data, transformer, node, environment):
    if not self.hstream.isopen ('dl'):
      self.hstream.push ('dl')
    if not self.token_list_is_empty (node.title):
      self.hstream.push ('dt', 'style="white-space:nowrap"')
      transformer (node, environment, NODE_TITLE)
      self.hstream.pop ('dt')
    if not self.token_list_is_empty (node.contents):
      self.hstream.push ('dd')
      transformer (node, environment, NODE_CONTENTS)
      self.hstream.pop ('dd')
    if not node.has_para_sibling:
      self.hstream.pop ('dl')
  def doxer_flush_parameters (self, data, transformer, node, environment):
    if not node.flags & Data.SECTIONED:
      if self.call_stack[-2].flags & Data.SECTIONED:
        node = self.call_stack[-2]
    if not node.flags & Data.SECTIONED or not node.__dict__.has_key ('parameters'):
      return
    self.hstream.push_many (('table', 'tr', 'td'))      # this outer table is needed to avoid horizontal table expansion
    self.hstream.put ('\n')
    self.hstream.push ('div', 'class="doxer-style-parameters"')
    self.hstream.push ('table')
    self.hstream.put ('\n')
    for param in node.parameters:
      self.hstream.push ('tr')
      self.hstream.push ('td', 'valign="top"')
      transformer (param, environment, NODE_TITLE)
      self.hstream << ':'
      self.hstream.pop ('td')
      self.hstream.push ('td')
      self.hstream << ' '
      self.hstream.pop ('td')
      self.hstream.push ('td')
      transformer (param, environment, NODE_CONTENTS)
      self.hstream.pop ('td')
      self.hstream.pop ('tr')
      self.hstream.put ('\n')
    self.hstream.pop ('table')
    self.hstream.pop ('div')
    self.hstream.pop_many (('td', 'tr', 'table'))
    del node.parameters
  def doxer_parameter (self, data, transformer, node, environment):
    if self.call_stack[-2].flags & Data.SECTIONED:
      parent = self.call_stack[-2]
      if not parent.__dict__.has_key ('parameters'):
        parent.parameters = []
      parent.parameters += [ node ]
    else:
      self.doxer_definition (data, transformer, node, environment)
  def doxer_table (self, data, transformer, node, environment):
    variant = 'border="1" rules="all"'
    spacing, size = '', ''
    tokens = node.arg
    while tokens:
      arg, tokens = DoxiParser.split_arg_from_token_list (tokens)
      arg = DoxiParser.text_from_token_list (arg).strip()
      if   arg == 'noframe':    variant = ' border="0" rules="none"'
      elif arg == 'normal':     variant = ' border="1" rules="all"'
      elif arg == 'bigframe':   variant = ' border="3"' # some browsers render no rules bigger than rules=none
      elif arg == 'expand':     variant = ' width="100%"'
      else:
        raise SemanticError ("%s:%u: Unknown table variant: %s" % (node.fname, node.fline, arg))
    self.hstream.push ('table', variant + spacing)
    transformer (node, environment, NODE_TEXT)
    if self.hstream.isopen ('td'):
      self.hstream.pop ('td')
    if self.hstream.isopen ('tr'):
      self.hstream.pop ('tr')
    self.hstream.pop ('table')
  def doxer_row (self, data, transformer, node, environment):
    if self.hstream.isopen ('td'):
      self.hstream.pop ('td')
    if self.hstream.isopen ('tr'):
      self.hstream.pop ('tr')
    self.hstream.push ('tr')
    transformer (node, environment)
  def doxer_cell (self, data, transformer, node, environment):
    if self.hstream.isopen ('td'):
      self.hstream.pop ('td')
    halign, valign, colspan = "", "", ""
    tokens = node.arg
    while tokens:
      arg, tokens = DoxiParser.split_arg_from_token_list (tokens)
      arg = DoxiParser.text_from_token_list (arg).strip()
      if   arg == 'left':       halign = ' align="left"'
      elif arg == 'center':     halign = ' align="center"'
      elif arg == 'right':      halign = ' align="right"'
      elif arg == 'top':        valign = ' valign="top"'
      elif arg == 'middle':     valign = ' valign="middle"'
      elif arg == 'bottom':     valign = ' valign="bottom"'
      elif re.match (r'colspan=[0-9]+$', arg):  colspan = ' colspan=\"' + arg[8:] + '"'
      else:
        raise SemanticError ("%s:%u: Unknown cell variant: %s" % (node.fname, node.fline, arg))
    self.hstream.push ('td', halign + valign + colspan)
    transformer (node, environment, NODE_TEXT)
  def css_divspan (self, tag, transformer, node, environment):
    if node.arg and isinstance (node.arg[0], str):
      match = re.match (r'([-0-9A-Za-z_]+)(?:$|,(.*))', node.arg[0])
      if match:
        cssclass, atext = match.groups()
        self.hstream.push (tag, 'class="%s"' % HtmlOStream.quote_string (cssclass))
        saved_arg = node.arg # temporary args shift
        node.arg = (atext and [ atext ] or []) + node.arg[1:]
        transformer (node, environment)
        node.arg = saved_arg
        self.hstream.pop (tag)
        return
    raise SemanticError ("%s:%u: Tag '%s' requires class argument" % (node.fname, node.fline, node.name))
  def doxer_ignore (self, tag, transformer, node, environment):
    if environment['doxer_title_tail']:
      transformer (node, environment)
  # block tag => handler dictionary
  block_tagdict = {
    'doxi-document'             : doxi_document,
    'doxer-group'               : doxer_group,
    'doxer_start_section'       : ( section, 0, ),
    'doxer_template_hook'       : template_hook,
    'doxer_definition'          : doxer_definition,
    'doxer_parameter'           : doxer_parameter,
    'doxer_flush_parameters'    : doxer_flush_parameters,
    'doxer_title_tail'          : doxer_ignore,
    'doxer_div'                 : (css_divspan, 'div'),
    'doxer_table'               : doxer_table,
    'doxer_row'                 : doxer_row,
    'doxer_cell'                : doxer_cell,
  }
  # inline tag handlers
  def doxer_visibility (self, data, transformer, node, environment):
    enable = node.name == 'doxer_hidden'
    if node.arg and isinstance (node.arg[0], str):
      match = re.search (r'([-+]?[0-9]+)', node.arg[0])
      if match:
        enable = int (match.group (0))
    if ((enable and node.name == 'doxer_visible') or
        (not enable and node.name == 'doxer_hidden')):
      transformer (node, environment, NODE_TEXT)
  def doxer_list (self, data, transformer, node, environment):
    ltag, ltype = 'ul', ''
    tokens = node.arg
    while tokens:
      arg, tokens = DoxiParser.split_arg_from_token_list (tokens)
      arg = DoxiParser.text_from_token_list (arg).strip()
      ltag = 'ul'
      if   arg == 'BULLET':     ltag, ltype = 'ul', ''
      elif arg == 'bullet':     ltag, ltype = 'ul', ''
      elif arg == 'CIRCLE':     ltag, ltype = 'ul', 'type="circle"'
      elif arg == 'circle':     ltag, ltype = 'ul', 'type="circle"'
      elif arg == 'SQUARE':     ltag, ltype = 'ul', 'type="square"'
      elif arg == 'square':     ltag, ltype = 'ul', 'type="square"'
      elif arg == 'DISC':       ltag, ltype = 'ul', 'type="disc"'
      elif arg == 'disc':       ltag, ltype = 'ul', 'type="disc"'
      elif arg == 'ARABIC':     ltag, ltype = 'ol', ''
      elif arg == 'arabic':     ltag, ltype = 'ol', ''
      elif arg == 'ALPHA':      ltag, ltype = 'ol', 'type="A"'
      elif arg == 'alpha':      ltag, ltype = 'ol', 'type="a"'
      elif arg == 'ROMAN':      ltag, ltype = 'ol', 'type="I"'
      elif arg == 'roman':      ltag, ltype = 'ol', 'type="i"'
      elif arg == 'none':       ltag, ltype = 'ul', 'style="text-indent:-1em;list-style-position:outside;list-style-type:none"'
      else:
        raise SemanticError ("%s:%u: Unknown list type: %s" % (node.fname, node.fline, arg))
    self.hstream.push (ltag, ltype)
    transformer (node, environment, NODE_TEXT)
    if self.hstream.isopen ('li'):
      self.hstream.pop ('li')
      self.hstream << '\n'
    self.hstream.pop (ltag)
    self.hstream << '\n'
  def doxer_deflist (self, data, transformer, node, environment):
    self.hstream.push ('dl')
    transformer (node, environment, NODE_TEXT)
    if self.hstream.isopen ('dd'):
      self.hstream.pop ('dd')
    self.hstream.pop ('dl')
    self.hstream << '\n'
  def doxer_item (self, data, transformer, node, environment):
    ptag = self.hstream.find_open (('ul', 'ol', 'dl'))
    assert len (ptag) > 0
    if ptag == 'dl':
      if self.hstream.isopen ('dd'):
        self.hstream.pop ('dd')
        self.hstream << '\n'
      self.hstream.push ('dt', 'style="white-space:nowrap"')
      transformer (node, environment, NODE_TITLE)
      self.hstream << '\n'      # this newline was omitted when parsing @doxer_item in SPAN_LINE mode
      transformer (node, environment, NODE_CONTENTS)
      self.hstream.pop ('dt')
      self.hstream << '\n'
      self.hstream.push ('dd')
    else:
      if self.hstream.isopen ('li'):
        self.hstream.pop ('li')
        self.hstream << '\n'
      self.hstream.push ('li')
      transformer (node, environment, NODE_TITLE)
      self.hstream << '\n'      # this newline was omitted when parsing @doxer_item in SPAN_LINE mode
      transformer (node, environment, NODE_CONTENTS)
  def center (self, tagname, transformer, node, environment):
    # unfortunately "<center>" is a block element that can't go into inline elements
    if self.hstream.inline == 1:
      self.hstream.push ('center')
    transformer (node, environment)
    if self.hstream.inline == 1:
      self.hstream.pop ('center')
  def simple (self, tagname, transformer, node, environment):
    self.hstream.push (tagname)
    transformer (node, environment)
    self.hstream.pop (tagname)
  def monospace (self, data, transformer, node, environment):
    self.hstream.push ('tt')
    self.hstream.toggle_spaces += 1
    transformer (node, environment)
    self.hstream.toggle_spaces -= 1
    self.hstream.pop ('tt')
  def preformatted (self, data, transformer, node, environment):
    self.hstream.push ('pre')
    self.hstream.preformatted += 1
    self.hstream.toggle_spaces += 1
    transformer (node, environment)
    self.hstream.toggle_spaces -= 1
    self.hstream.preformatted -= 1
    self.hstream.pop ('pre')
  def uri_quote_string (string, allow_breaks = False):
    result = ''
    trans = { '"': '%22', '&': '%26', '<': '%3C', '>': '%3E', '%': '%25', ',': '%2E' }
    for c in string:
      d = trans.get (c)
      result += d and d or c
    if allow_breaks:
      #result = re.sub (r'/\b', '/&shy;', result) # not widely supported
      #result = re.sub (r'/\b', '/&#8203;', result) # zero width space forces placeholder in many fonts
      #result = re.sub (r'/\b', '<span style="white-space:normal">/</span>', result) # browsers still want a space to break
      result = re.sub (r'/\b', '/<wbr/>', result) # 'wbr' instead of 'wbr/' screws konquerors parsing
    return result
  uri_quote_string = staticmethod (uri_quote_string)
  def doxer_uri (self, longform, transformer, node, environment):
    def check_uri (uri):
      if uri[:7] == 'nolink:':
        return False
      return True
    if not node.arg or not isinstance (node.arg[0], str):
      raise SemanticError ("%s:%u: Tag '%s' requires a URI" % (node.fname, node.fline, node.name))
    token_list = node.arg[:]
    atext = ''
    while token_list and isinstance (token_list[0], str):
      atext += token_list.pop (0)
    cpos = atext.find (',')
    if cpos >= 0:
      uri = atext[:cpos].strip()
      with_href = check_uri (uri)
      atext = atext[cpos+1:].strip()
    else:
      uri = atext.strip()
      with_href = check_uri (uri)
      atext = uri
    if atext:
      token_list = [ atext ] + token_list
    uri = self.hstream.quote_string (uri)
    if with_href:
      self.hstream.push ('a', 'href="%s"' % uri)
    else:
      self.hstream.push ('u', 'class="doxer-style-nolink"')
    saved_arg = node.arg # temporary args shift
    node.arg = token_list
    transformer.im_self.block_linkdict += 1
    transformer (node, environment)
    transformer.im_self.block_linkdict -= 1
    if longform:
      self.hstream.raw_entities += 1
      self.hstream << ' (%s)' % self.hstream.quote_string (uri, True)
      self.hstream.raw_entities -= 1
    node.arg = saved_arg
    if with_href:
      self.hstream.pop ('a')
    else:
      self.hstream.pop ('u')
  def doxer_image (self, data, transformer, node, environment):
    if not node.arg or not isinstance (node.arg[0], str):
      raise SemanticError ("%s:%u: Tag '%s' requires a URI" % (node.fname, node.fline, node.name))
    atext = node.arg[0]
    cpos = atext.find (',')
    if cpos >= 0:
      uri = atext[:cpos].strip()
      atext = atext[cpos+1:].strip()
    else:
      uri = atext.strip()
      atext = uri
    uri = self.uri_quote_string (uri)
    alt = DoxiParser.text_from_token_list ((atext and [ atext ] or []) + node.arg[1:]).strip()
    if alt:
      self.hstream.single ('<img src="%s" alt="%s"/>' % (uri, HtmlOStream.quote_string (alt)))
    else:
      self.hstream.single ('<img src="%s"/>' % uri)
  def fontsize (self, data, transformer, node, environment):
    if not node.arg or not isinstance (node.arg[0], str):
      raise SemanticError ("%s:%u: Tag '%s' requires size argument" % (node.fname, node.fline, node.name))
    atext = node.arg[0]
    cpos = atext.find (',')
    if cpos >= 0:
      fsize = atext[:cpos].strip()
      atext = atext[cpos+1:]
    else:
      fsize = atext.strip()
      atext = ''
    if not re.match ("[+-][0-5]$", fsize):
      raise SemanticError ("%s:%u: Invalid font size: '%s'" % (node.fname, node.fline, fsize))
    self.hstream.push ('font', 'size="%s"' % HtmlOStream.quote_string (fsize))
    # temporary node args shift
    savedarg = node.arg
    if atext:
      node.arg = [ atext ] + node.arg[1:]
    else:
      node.arg = node.arg[1:]
    transformer (node, environment)
    node.arg = savedarg
    self.hstream.pop ('font')
  def doxer_anchor (self, data, transformer, node, environment):
    if node.anchor:
      self.hstream.push ('a', 'name="%s"' % HtmlOStream.quote_string (node.anchor))
      self.hstream.pop ('a')
  def doxer_raw (self, data, transformer, node, environment):
    atext = DoxiParser.text_from_token_list (node.arg, False, True)
    cpos = atext.find (',')
    if cpos >= 0:
      kind = atext[:cpos].strip()
      atext = atext[cpos+1:].strip()
      if kind.strip() == 'html':
        self.hstream.raw_entities += 1
        self.hstream.put (atext)
        self.hstream.raw_entities -= 1
  def doxer_hseparator (self, data, transformer, node, environment):
    self.hstream.single ("<hr/>")
  def linebreak (self, data, transformer, node, environment):
    if self.hstream.preformatted:
      self.hstream << '\n'
    else:
      if (node.arg != [ 'automatic_newline' ] or        # collapse consecutive automatic breaks
          not self.hstream.output_has_tag ('br')):
        self.hstream.single ('<br/>\n')
        #self.hstream.single ('<br doxer-file="%s" doxer-line="%u"/>\n' % (node.fname, node.fline))
        self.hstream.output_add_tag ('br')
  # inline tag => handler dictionary
  inline_tagdict = {
    'doxer_center'      : center,
    'doxer_bold'        : (simple, 'b'),
    'doxer_italic'      : (simple, 'i'),
    'doxer_underline'   : (simple, 'u'),
    'doxer_monospace'   : monospace,
    'doxer_preformatted': preformatted,
    'doxer_fontsize'    : fontsize,
    'doxer_span'        : (css_divspan, 'span'),
    'doxer_newline'     : linebreak,
    'doxer_visible'     : doxer_visibility,
    'doxer_hidden'      : doxer_visibility,
    'doxer_list'        : doxer_list,
    'doxer_deflist'     : doxer_deflist,
    'doxer_item'        : doxer_item,
    'doxer_uri'         : ( doxer_uri, 0),
    'doxer_longuri'     : ( doxer_uri, 1),
    'doxer_image'       : doxer_image,
    'doxer_anchor'      : doxer_anchor,
    'doxer_raw'         : doxer_raw,
    'doxer_hseparator'  : doxer_hseparator,
  }

def process_root (root, filler = None, custom_lookup = None):
  hs = HtmlOStream ()
  hgen = HtmlGenerator (hs, custom_lookup)
  hgen.filler = filler
  environment = {}
  environment.update (root.variables)
  env_defaults = {
    'doxer_title_tail' : 0,
  }
  environment.update (env_defaults)
  try:
    hgen.transform_any (root, environment)
    # here, we're catching exceptions raised by the semantic parser
  except SemanticError, ex:
    print >>sys.stderr, str (ex)
    if Config.debug:
      raise
    # exit silently if not debugging
    os._exit (1)
  return hs.joined_output()
