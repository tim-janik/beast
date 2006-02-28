#!/usr/bin/env python2.4
#
# Doxer - Software documentation system
# Copyright (C) 2005-2006 Tim Janik
#
# Code2Doxi.py - generate doxer markup from source code representation tree
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

##
# 12.02.2006    allmost complete rewrite
# 21.08.2005    generate index page
# 20.08.2005    implemented typedef formatting
# 19.08.2005    implemented function pointer variable formatting
# 18.08.2005    added struct formatting
# 16.08.2005    major refactoring, formed objects
# 15.08.2005    generate multiple pages
# 14.08.2005    support cross-link dictionary, lots of markup fixes
# 13.08.2005    basic html formatting in place
# 12.08.2005    initial version


#--- link markup ---
class CodeMarkupCore:
  def __init__ (self):
    self.index_lookup = None
  def set_index_handler (self, handler):
    self.index_lookup = handler
  def mspace (self, n_spaces = 1):
    return '@doxer_monospace{' + ' ' * n_spaces + '}'
  def linkdict_markup (self, string, custom_lookup, fallback_lookup):
    # perform keyword-based linkdict link-markup
    tlist = linkdict.markup_tuples (string, custom_lookup, fallback_lookup)
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
    outstring = ''
    for tok in jlist:
      if isinstance (tok, tuple):
        outstring += '@doxer_uri{' + tok[0] + ','
        outstring += tok[1]
        outstring += '}'
      else:
        outstring += tok
    return outstring
  def link_markup (self, text, fallback_lookup = None):
    return self.linkdict_markup (text, self.index_lookup, fallback_lookup)
  def markup_syntax (self, text):
    return self.link_markup (text, linkdict.lookup_ctype)
  def markup_type_unanchored (self, name):
    return '@doxer_span{doxer-style-type,@doxer_monospace{' +name + '}}'
  def markup_type (self, name):
    return '@doxer_span{doxer-style-type,@doxer_monospace{' + self.link_markup (name, linkdict.lookup_ctype) + '}}'
  def markup_structure (self, name, anchor):
    if anchor:
      return '@doxer_uri{#' + anchor + ',' + self.markup_type_unanchored (name) + '}'
    else:
      return '@doxer_uri{nolink:,' + self.markup_type_unanchored (name) + '}'
  def markup_callable (self, name, anchor):
    if anchor:
      return '@doxer_uri{#' + anchor + ',' + self.markup_callable_unanchored (name) + '}'
    else:
      return '@doxer_uri{nolink:,' + self.markup_callable_unanchored (name) + '}'
  def markup_callable_unanchored (self, name):
    return '@doxer_span{doxer-style-callable,@doxer_monospace{' + name + '}}'
  def markup_variable (self, name):
    emphasize = True
    if emphasize:
      return '@doxer_span{doxer-style-variable,@doxer_monospace{@doxer_italic{' + name + '}}}'
    else:
      return '@doxer_span{doxer-style-variable,@doxer_monospace{' + name + '}}'
  def canonicalize_anchor (self, str):
    return str
  def construct_object_anchor_detailed (self, details, *args):
    anchor = ''
    strip_enum_names = 'EV' in details
    for obj in args:
      filler = anchor and '::' or ''
      if not strip_enum_names or not isinstance (obj, Data.Enum):
        anchor += filler + self.canonicalize_anchor (obj.name)
      else:
        ename = obj.name;
        cpos = ename.rfind (':')
        if cpos >= 0:
          ename = ename[:cpos]
          anchor += filler + self.canonicalize_anchor (ename)
    return anchor
  def construct_object_anchor (self, *args):
    return self.construct_object_anchor_detailed ("", *args)
  def construct_enum_value_anchor (self, *args):
    return self.construct_object_anchor_detailed (",EV,", *args)

#--- Doxi file writer ---
class SrcFileWriter (CodeMarkupCore):
  def __init__ (self, indexer, top_webdir, fout):
    CodeMarkupCore.__init__ (self)
    self.fout = fout
    if indexer:
      def index_lookup (keyword):
        ltup = indexer.index_lookup (keyword, top_webdir)
        if ltup:
          return ltup[1]
        return None
      self.set_index_handler (index_lookup)
  def start_description_section (self, name, hint, anchor, location, doctuple, fout):
    if hint:
      hint = ' (' + hint + ')'
    else:
      hint = ''
    print >>fout, '@doxer_start_section{level=3} @doxer_anchor{' + anchor + '} ' + name + hint
    if location[0] and location[1]:
      print >>fout, '@doxer_title_tail ' + '%s:%d' % (location[0], location[1])
    elif doctuple and doctuple[1] and doctuple[2]:
      print >>fout, '@doxer_title_tail ' + '%s:%d' % (doctuple[1], doctuple[2])
    elif location[0]:
      print >>fout, '@doxer_title_tail ' + location[0]
    elif doctuple and doctuple[1]:
      print >>fout, '@doxer_title_tail ' + doctuple[1]
    print >>fout, '@dnl'
  def cmp_object_by_name (self, a, b):
    return cmp (a.name, b.name)
  def split_typed_identifier (self, name, type, argstring = ''):
    t, t2, m, n, n2, a = type.strip(), '', '', name.strip(), '', argstring.strip()
    special_chars = set ('*&()')
    # seperate plain type name
    while t and (t[-1] in special_chars):
      m = t[-1] + m
      t = t[:-1]
    # check for function variables
    i = m.find ('(')
    if i >= 0:
      t2 = m[:i].strip()
      m = m[i:].strip()
    # check for function variables
    if a and a[0] == ')':
      n2 = a[0]
      a = a[1:].strip()
    return (t, t2, m, n, n2, a)
  def write_variable_list (self, indent, name_type_argstring_anchor_list, is_decl = False):
    maxwidth_t, maxwidth_n, maxwidth_a = 0, 0, 0
    # measure type string widths
    for nta in name_type_argstring_anchor_list:
      t, t2, m, n, n2, a = self.split_typed_identifier (nta[0], nta[1], nta[2])
      maxwidth_t = max (maxwidth_t, len (t) + len (t2) + len (m))
      maxwidth_n = max (maxwidth_n, len (n) + len (n2))
    # add params
    text = ""
    for nta in name_type_argstring_anchor_list:
      if text:
        text += (',', ';')[is_decl] + '\n' + indent;
      t, t2, m, n, n2, a = self.split_typed_identifier (nta[0], nta[1], nta[2])
      text += self.markup_type (t) + t2
      text += ' ' * (1 + maxwidth_t - (len (t) + len (t2) + len (m))) + m
      hi = len (nta) > 3 and nta[3].rfind ('#') or -1
      if hi >= 0:       # found '#'
        text += '@doxer_anchor{' + nta[3][hi+1:] + '}'                          # anchor
        text += '@doxer_uri{' + nta[3] + ',' + self.markup_variable (n) + '}'   # link
      else:
        text += self.markup_variable (n)
      text += n2
      if a:
        text += ' ' * (1 + maxwidth_n - (len (n) + len (n2))) + self.markup_syntax (a)
    if text:
      return ('', indent)[is_decl] + text + ('', ';\n')[is_decl]
    else:
      return ''
  def print_documentables (self, section_title, documentable_list, fout):
    need_title = True
    for d in documentable_list:
      if len (d.doc) >= 3:
        if need_title:
          print >>fout, '@doxer_flush_parameters{}@*'                   # flush left over parameters
          if section_title:
            print >>fout, '@* @doxer_bold{' + section_title + '}'
          need_title = False
        print >>fout, '@doxer_line %u "%s"' % (int (d.doc[2]), d.doc[1])
        print >>fout, '@doxer_parameter %s ' % d.name
        print >>fout, d.doc[0] + '\n'
    if not need_title:
      print >>fout, '@doxer_flush_parameters{}'
  def write_structure_synopsis (self, struct, fout):
    struct_anchor = self.construct_object_anchor (struct)
    print >>fout, '@doxer_row'
    print >>fout, '@doxer_cell{top} ' + self.markup_type ('struct') + self.mspace (1)           # return type column
    marked_structure = self.markup_structure (struct.name, struct_anchor)                       
    print >>fout, '@doxer_cell{top} ' + marked_structure + ';'                                  # structure name
    print >>fout, '@doxer_done'
  def write_structure_description (self, struct, fout):
    struct_anchor = self.construct_object_anchor (struct)
    self.start_description_section (struct.name, struct.hint, struct_anchor, struct.location(), struct.doc, fout)
    print >>fout, '@doxer_div{doxer-style-prototype}'
    print >>fout, '@doxer_table{noframe}'
    print >>fout, '@doxer_row'
    print >>fout, '@doxer_cell '
    # print preformatted prototype
    print >>fout, '@doxer_preformatted{' + self.markup_type ('struct') + " " + self.markup_structure (struct.name, struct_anchor)
    print >>fout, "{"
    name_type_argstring_anchor_list = [(field.name, field.type, field.argstring, self.construct_object_anchor (struct, field)) for field in struct.members]
    variable_blurb = self.write_variable_list ('  ', name_type_argstring_anchor_list, True)
    if variable_blurb:
      fout.write (variable_blurb)
    print >>fout, '};}' # closes @doxer_preformatted{}
    # close prototype scopes
    print >>fout, '@doxer_done'
    print >>fout, '@doxer_done'
    print >>fout, '@doxer_done'
    print >>fout, '@dnl'
    # print docu
    if len (struct.doc) >= 3:
      print >>fout, '@doxer_line %u "%s"' % (int (struct.doc[2]), struct.doc[1])
      print >>fout, struct.doc[0]
    # print arg docu
    self.print_documentables ('', struct.members, fout)
    # finish structure description
    # print >>fout, '\n\n'
    print >>fout, '@doxer_flush_parameters{}'
    print >>fout, '@doxer_hseparator{}'
  def write_function_proto (self, func, with_anchor, fout, anchor_parent = None, prefix = ''):
    ## self.write_function_proto (func, True, fout, obj, '::')
    if anchor_parent:
      func_anchor = self.construct_object_anchor (anchor_parent, func)
    else:
      func_anchor = self.construct_object_anchor (func)
    # open row
    print >>fout, '@doxer_row'
    # return type
    rindent = anchor_parent and self.mspace (2) or ''
    print >>fout, '@doxer_cell{top} ' + rindent + self.markup_type (func.ret_arg.type) + self.mspace (1)
    # function name
    with_anchor = with_anchor and '@doxer_anchor{' + func_anchor + '}' or ''
    prefix += with_anchor
    print >>fout, '@doxer_cell{top} ' + prefix + self.markup_callable (func.name, func_anchor) + self.mspace (1)
    # print args
    name_type_argstring_anchor_list = [(arg.name, arg.type, arg.argstring) for arg in func.args]
    indent = ' ' # align with first arg after '('
    args = self.write_variable_list (' ', name_type_argstring_anchor_list)
    if func.has_ellipsis:
      if args:
        args += ",\n" + indent
      args += self.markup_type ("...")
    print >>fout, '@doxer_cell{top} @doxer_preformatted{' + '(' + args + ');' + '}'
    # close row
    print >>fout, '@doxer_done'
  def write_function_proto_short (self, func, with_anchor, fout, anchor_parent = None, prefix = ''):
    if anchor_parent:
      func_anchor = self.construct_object_anchor (anchor_parent, func)
    else:
      func_anchor = self.construct_object_anchor (func)
    print >>fout, '@doxer_row'
    rindent = anchor_parent and '@doxer_monospace{  }' or ''
    msp = self.mspace (1)
    print >>fout, '@doxer_cell{top} ' + rindent + self.markup_type (func.ret_arg.type) + msp    # return type
    with_anchor = with_anchor and '@doxer_anchor{' + func_anchor + '}' or ''
    prefix += with_anchor
    marked_callable = self.markup_callable (func.name, func_anchor)
    print >>fout, '@doxer_cell{top} ' + prefix + marked_callable + self.mspace (1)              # function name
    print >>fout, '@doxer_cell{top} ',                                                          # arg list
    print >>fout, '@doxer_uri{#' + func_anchor + ',',   # arg list link
    # arg list
    result = '('
    nth = 0
    for arg in func.args:
      if nth: result += ', '
      result += self.markup_variable (arg.name) 				                # arguments
      nth += 1
    if (func.has_ellipsis):
      if nth: result += ', '
      result += '...'
    result += ');}'                                     # arg list link done
    print >>fout, result
    print >>fout, '@doxer_done'
  def write_function_synopsis (self, func, fout, anchor_parent = None, prefix = ''):
    return self.write_function_proto_short (func, False, fout, anchor_parent, prefix)
  def write_function_description (self, func, fout):
    func_anchor = self.construct_object_anchor (func)
    self.start_description_section (func.name, func.hint, func_anchor, func.location(), func.doc, fout)
    print >>fout, '@doxer_div{doxer-style-prototype}'
    print >>fout, '@doxer_table{noframe}'
    # function prototype
    if False:
      print >>fout, '@doxer_row'
      print >>fout, '@doxer_cell '
      # print preformatted prototype
      frtype = func.ret_arg.type
      str1 = frtype + " " + func.name + " ("
      indent = ' ' * (len (str1))
      str1 = self.markup_type (frtype) + " " + self.markup_callable (func.name, func_anchor) + " ("
      name_type_argstring_anchor_list = [(arg.name, arg.type, arg.argstring) for arg in func.args]
      str2 = self.write_variable_list (indent, name_type_argstring_anchor_list)
      if func.has_ellipsis:
        if str2:
          str2 += ",\n" + indent
        str2 += self.markup_type ("...")
      pretty_proto = str1 + str2 + ");"
      print >>fout, '@doxer_preformatted{' + pretty_proto + '}'
      print >>fout, '@doxer_done'
    else:
      # self.write_function_proto_short (func, True, fout, obj, '::')
      self.write_function_proto (func, True, fout)
    # close prototype scopes
    print >>fout, '@doxer_done'
    print >>fout, '@doxer_done'
    print >>fout, '@dnl'
    # print docu
    if len (func.doc) >= 3:
      print >>fout, '@doxer_line %u "%s"' % (int (func.doc[2]), func.doc[1])
      print >>fout, func.doc[0]
    # print arg docu
    self.print_documentables ('', func.args, fout)
    # finish function description
    # print >>fout, '\n\n'
    print >>fout, '@doxer_flush_parameters{}'
    print >>fout, '@doxer_hseparator{}'
  def write_channel_proto (self, channel, with_anchor, fout, anchor_parent = None, prefix = ''):
    if anchor_parent:
      channel_anchor = self.construct_object_anchor (anchor_parent, channel)
    else:
      channel_anchor = self.construct_object_anchor (channel)
    print >>fout, '@doxer_row'
    tindent = anchor_parent and '@doxer_monospace{  }' or ''
    msp =  self.mspace (1)
    print >>fout, '@doxer_cell{top} ' + tindent + self.markup_type (channel.kind) + msp         # return type column
    marked_channel = self.markup_variable (channel.name)
    marked_channel = '@doxer_uri{#' + channel_anchor + ',' + marked_channel + ';}'
    with_anchor = with_anchor and '@doxer_anchor{' + channel_anchor + '}' or ''
    print >>fout, '@doxer_cell{top} ' + with_anchor + prefix + marked_channel                   # structure name
    print >>fout, '@doxer_done'
  def write_param_proto (self, param, with_anchor, fout, anchor_parent = None, prefix = ''):
    if anchor_parent:
      param_anchor = self.construct_object_anchor (anchor_parent, param)
    else:
      param_anchor = self.construct_object_anchor (param)
    print >>fout, '@doxer_row'
    tindent = anchor_parent and '@doxer_monospace{  }' or ''
    t, t2, m, n, n2, a = self.split_typed_identifier (param.name, param.type, param.argstring)
    tname = tindent + self.markup_type (t) + t2
    vname = ''
    if m[0:1] == '(':   # we do different spacings for function variables
      vname = m
    else:
      tname += m
    print >>fout, '@doxer_cell{top} ' + tname + self.mspace (1)                                 # type
    vname += prefix
    vname += '@doxer_uri{#' + param_anchor + ',' + self.markup_variable (n) + n2
    if not a:
      vname += ';'
    vname += '}'
    with_anchor = with_anchor and '@doxer_anchor{' + param_anchor + '}' or ''
    print >>fout, '@doxer_cell{top} ' + with_anchor + vname                                     # variable name
    if a:
      print >>fout, '@doxer_cell{top} ' + self.markup_syntax (a + ';')                          # argstring
    print >>fout, '@doxer_done'
  def write_object_synopsis (self, obj, fout):
    obj_anchor = self.construct_object_anchor (obj)
    list_channels, list_properties, list_signals = False, False, True
    # lists
    channels = list_channels and obj.list_channels() or []
    properties = list_properties and obj.list_properties() or []
    signals = list_signals and obj.list_signals() or []
    quick_return = not (channels or properties or signals)
    # object header
    print >>fout, '@doxer_row'
    print >>fout, '@doxer_cell{top} ' + self.markup_type ('class') + self.mspace (1)            # return type column
    marked_object = self.markup_structure (obj.name, obj_anchor)
    if quick_return:
      print >>fout, '@doxer_cell{top} ' + marked_object + ';'                                   # object name
      print >>fout, '@doxer_done'
      return None
    print >>fout, '@doxer_cell ' + marked_object + '@doxer_monospace{ }@{'                      # object name
    print >>fout, '@doxer_done'
    # list channels
    if channels:
      print >>fout, '@doxer_row\n@doxer_cell @doxer_monospace{ }Channels:\n@doxer_done'
      for channel in channels:
        self.write_channel_proto (channel, fout, obj, '::')
    # list properties
    if properties:
      print >>fout, '@doxer_row\n@doxer_cell @doxer_monospace{ }Properties:\n@doxer_done'
      for prop in properties:
        self.write_param_proto (prop, fout, obj, '::')
    # list signals
    if signals:
      print >>fout, '@doxer_row\n@doxer_cell @doxer_monospace{ }Signals:\n@doxer_done'
      for func in obj.list_signals():
        self.write_function_synopsis (func, fout, obj, '::')
    # object footer
    print >>fout, '@doxer_row'
    print >>fout, '@doxer_cell @};'                                                             # return type column
    print >>fout, '@doxer_done'
  def write_object_description (self, obj, fout):
    obj_anchor = self.construct_object_anchor (obj)
    with_anchor = True
    self.start_description_section (obj.name, '', obj_anchor, obj.location(), obj.doc, fout)
    print >>fout, '@doxer_div{doxer-style-prototype}'
    print >>fout, '@doxer_table{noframe}'
    space1 = '@doxer_monospace{ }'
    # object header
    print >>fout, '@doxer_row'
    with_anchor = with_anchor and '@doxer_anchor{' + obj_anchor + '}' or ''
    class_proto = self.markup_type ('class') + space1 + with_anchor
    class_proto += self.markup_structure (obj.name, obj_anchor)
    print >>fout, '@doxer_cell{colspan=3} ' + class_proto                                       # object name
    print >>fout, '@doxer_done'
    print >>fout, '@doxer_row\n@doxer_cell @{\n@doxer_done'
    # print channels
    channels = obj.list_channels()
    if channels:
      print >>fout, '@doxer_row\n@doxer_cell @doxer_monospace{ }Channels:\n@doxer_done'
      for channel in channels:
        self.write_channel_proto (channel, True, fout, obj, '::')
    # print properties
    properties = obj.list_properties()
    if properties:
      print >>fout, '@doxer_row\n@doxer_cell @doxer_monospace{ }Properties:\n@doxer_done'
      for prop in properties:
        self.write_param_proto (prop, True, fout, obj, '::')
    # print signals
    signals = obj.list_signals()
    if signals:
      print >>fout, '@doxer_row\n@doxer_cell @doxer_monospace{ }Signals:\n@doxer_done'
      for func in obj.list_signals():
        ## self.write_function_proto_short (func, True, fout, obj, '::')
        self.write_function_proto (func, True, fout, obj, '::')
    # object footer
    print >>fout, '@doxer_row'
    print >>fout, '@doxer_cell @};'                                                             # type column
    print >>fout, '@doxer_done'
    # close scopes
    print >>fout, '@doxer_done'
    print >>fout, '@doxer_done'
    print >>fout, '@dnl'
    # print docu
    if len (obj.doc) >= 3:
      print >>fout, '@doxer_line %u "%s"' % (int (obj.doc[2]), obj.doc[1])
      print >>fout, obj.doc[0]
    # print property docu
    self.print_documentables ('Channels:', channels, fout)
    # print channel docu
    self.print_documentables ('Properties:', properties, fout)
    # print signal docu
    self.print_documentables ('Signals:', signals, fout)
    # finish structure description
    # print >>fout, '\n\n'
    print >>fout, '@doxer_flush_parameters{}'
    print >>fout, '@doxer_hseparator{}'
  def write_file (self, sfile):
    fout = self.fout
    def cmp_typedefs (a, b):
      aa, ba = None, None
      try:      aa = a.argstring
      except:   pass
      try:      ba = b.argstring
      except:   pass
      # sort typedefs with argstrings first
      if bool (ba) ^ bool (aa):
        return (-1, +1)[bool (aa)]
      return cmp (a.name, b.name)
    # prepare docu lists
    typedefs = sfile.list_members (Data.Typedef)
    typedefs.sort (cmp_typedefs)
    enums = sfile.list_members (Data.Enum)
    enums.sort (self.cmp_object_by_name)
    structs = sfile.list_members (Data.Struct)
    structs.sort (self.cmp_object_by_name)
    objects = sfile.list_members (Data.Object)
    objects.sort (self.cmp_object_by_name)
    functions = sfile.list_members (Data.Function)
    functions.sort (self.cmp_object_by_name)
    # page intro
    print >>fout, "@doxer_start_section{level=0} @doxer_center %s" % sfile.name
    print >>fout, ""
    # SYNOPSIS
    print >>fout, "@doxer_start_section{level=2} @doxer_underline{SYNOPSIS}"
    print >>fout, ""
    print >>fout, "@doxer_div{doxer-style-synopsis}"
    print >>fout, "@doxer_table{noframe}"
    n = 0
    # list structures
    if n and structs:
      print >>fout, "@doxer_row\n@cell @doxer_monospace{    }\n@doxer_done"     # empty row
    for struct in structs:
      self.write_structure_synopsis (struct, fout)
      n += 1
    # list objects
    if n and objects:
      print >>fout, "@doxer_row\n@cell @doxer_monospace{    }\n@doxer_done"     # empty row
    for obj in objects:
      self.write_object_synopsis (obj, fout)
      n += 1
    # list functions
    if n and functions:
      print >>fout, "@doxer_row\n@cell @doxer_monospace{    }\n@doxer_done"     # empty row
    for func in functions:
      self.write_function_synopsis (func, fout)
      n += 1
    # close SYNOPSIS
    print >>fout, "@doxer_done"
    print >>fout, "@doxer_done"
    # DESCRIPTION
    print >>fout, "@*"
    print >>fout, "@doxer_start_section{level=2} @doxer_underline{DESCRIPTION}"
    print >>fout, ""
    print >>fout, "@doxer_div{doxer-style-description}"
    # list structures
    for struct in structs:
      self.write_structure_description (struct, fout)
    # list objects
    for obj in objects:
      self.write_object_description (obj, fout)
    # list functions
    for func in functions:
      self.write_function_description (func, fout)
    # close DESCRIPTION
    print >>fout, "@doxer_done"

#--- Doxi index writer ---
class DoxiIndexWriter (CodeMarkupCore):
  def __init__ (self, indexer, top_webdir, fout):
    CodeMarkupCore.__init__ (self)
    self.indexer = indexer
    self.top_webdir = top_webdir
    self.fout = fout
  def file_index_list (self):
    def cmp_tuple0 (a, b):
      res = cmp (a[0].lower(), b[0].lower())
      if res:
        return res
      return cmp (a[0], b[0])
    slist = self.indexer.file_index_list()[:]
    slist.sort (cmp_tuple0)
    dlist = []
    last = ""
    for ele in slist:
      sname, hurl, sfile = ele[0], ele[1], ele[2]
      letter = sname[0].upper()
      if last != letter:
        if last:
          dlist += [ (None, None, None) ]       # empty row
        last = letter
        dlist += [ (last, None, None) ]         # letter 'X' row
      if (self.top_webdir and hurl and
          not re.match (r'\w+:', hurl)):
        hurl = os.path.join (self.top_webdir, hurl)
      dlist += [ (sname, hurl, sfile) ]         # index entry
    return dlist
  def write_file_index (self, n_columns):
    fout = self.fout
    # file table title
    print >>fout, "@*"
    print >>fout, "@doxer_start_section{level=1} @doxer_center %s" % "Files"
    # file table
    print >>fout, "@doxer_div{doxer-style-index-body}"
    print >>fout, "@doxer_table{noframe}"
    # write index elements
    element_list = self.file_index_list()
    self.write_hwrap_table (n_columns, element_list)
    # close file table
    print >>fout, "@doxer_done"
    print >>fout, "@doxer_done"
  def alpha_index_list (self):
    def cmp_tuple0 (a, b):
      res = cmp (a[0].lower(), b[0].lower())
      if res:
        return res
      return cmp (a[0], b[0])
    slist = self.indexer.link_index_list()[:]
    slist.sort (cmp_tuple0)
    dlist = []
    last = ""
    for ele in slist:
      ename, eurl, etype = ele
      letter = ename[0].upper()
      if last != letter:
        if last:
          dlist += [ (None, None, None) ]       # empty row
        last = letter
        dlist += [ (last, None, None) ]         # letter 'X' row
      if (self.top_webdir and eurl and
          not re.match (r'\w+:', eurl)):
        eurl = os.path.join (self.top_webdir, eurl)
      dlist += [ (ename, eurl, etype) ]         # index entry
    return dlist
  def write_alpha_index (self, n_columns):
    fout = self.fout
    # alpha table title
    print >>fout, "@*"
    print >>fout, "@doxer_start_section{level=1} @doxer_center %s" % "Identifiers"
    # alpha table
    print >>fout, "@doxer_div{doxer-style-index-body}"
    print >>fout, "@doxer_table{noframe}"
    # write index elements
    element_list = self.alpha_index_list()
    self.write_hwrap_table (n_columns, element_list)
    # close alpha table
    print >>fout, "@doxer_done"
    print >>fout, "@doxer_done"
  def markup_section_title (self, title):
    return '@doxer_span{doxer-style-index-title,@doxer_bold{@doxer_monospace{    ' + title + '    }}}'
  def write_hwrap_table (self, n_columns, element_list):
    fout = self.fout
    n = 0
    for ele in element_list:
      name, url, edummy = ele
      if not name:
        continue                        # ignore empty elements
      if url:
        if n == 0:
          print >>fout, '@doxer_row'
        n += 1
        print >>fout, '@doxer_cell @doxer_uri{' + url + ', ' + name + '}'
        if n == n_columns:
          print >>fout, '@doxer_done'   # close row
          n = 0
      else:
        if n:
          print >>fout, '@doxer_done'   # close row
          n = 0
        print >>fout, '@doxer_row'
        print >>fout, '@doxer_raw{html,<td colspan="%u"><center><big>}' % n_columns + self.markup_section_title (name) + '@doxer_raw{html,</big></center></td>}'
        print >>fout, '@doxer_done'   # close row
    if n:
      print >>fout, '@doxer_done'   # close row
      n = 0
    assert n == 0

#--- public API ---
class DoxiWriter (CodeMarkupCore):
  def __init__ (self):
    self.link_index = {}
    self.file_index = {}
  def write_file (self, sfile, top_webdir, custom_lookup, fout):
    sfile_writer = SrcFileWriter (self, top_webdir, fout)
    sfile_writer.write_file (sfile)
  def write_index_page (self, dfile, top_webdir, fout):
    hpage = DoxiIndexWriter (self, top_webdir, fout)
    hpage.write_file_index (4)
    hpage.write_alpha_index (2)
  def index_add_file (self, sfile, hfile):
    huri = hfile
    bname = os.path.basename (sfile.name)
    self.file_index[bname] = (bname, huri, sfile)
    self.index_add_link (os.path.basename (sfile.name), huri, 'file')
    for m in sfile.list_members():
      self.index_add_link (m.name, huri + '#' + self.construct_object_anchor (m))
      if isinstance (m, Data.Enum):
        for v in m.values:
          self.index_add_link (v.name, huri + '#' + self.construct_enum_value_anchor (m, v))
      # add to global index
      #self.index_add (enum.name, self.doc_url (enum), "enum")
      #self.index_add (v.name, value_anchor, "enum-value")
      #self.index_add (struct.name, self.doc_url (struct), "struct")
      #self.index_add (func.name + "()", self.doc_url (func), "function")
      # self.index_add (typedef.name, self.doc_url (typedef), "typedef")
  def file_index_list (self):
    return self.file_index.values()
  def index_add_link (self, element_title, eurl, etype = None):
    self.link_index[element_title] = (element_title, eurl, etype)
  def link_index_list (self):
    return self.link_index.values()
  def index_lookup (self, title, top_webdir):
    ele = self.link_index.get (title)
    if top_webdir and ele and ele[1] and not re.match (r'\w+:', ele[1]):
      ename, eurl, etype = ele
      eurl = os.path.join (top_webdir, eurl)
      ele = (ename, eurl, etype)
    return ele
          
