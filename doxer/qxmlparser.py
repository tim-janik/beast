#!/usr/bin/env python2.4
#
# Doxer - Software documentation system
# Copyright (C) 2005-2006 Tim Janik
#
# qxmlparser.py - parse doxygen XML into SrcFile and Docu structures
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
import os, sys, re, libxml2
import Config, Data, linkdict
def debug (*args): Config.debug_print (*args)

##
# 03.09.2005    added support for an external comment dictionary
# 02.09.2005    ported to Config module, modularized code
# 20.08.2005    implemented typedef parsing
# 19.08.2005	implemented function pointer variables
# 18.08.2005	added struct parsing
# 17.08.2005	added enum definition and value parsing
# 16.08.2005	major refactoring, formed objects
# 13.08.2005	basic function parsing in place
# 12.08.2005	initial version

# --- xmlNode functions ---
def node_first_child (node):
  return node.children			# return first child
# iterator for all descendants
def node_descendants_subtree (node):
  return node.children.__iter__()	# libxml2 depth-first iterator
# find named descendant, including self
def node_find (node, name):
  if node.name == name:
    return node
  return node_find_descendant (node, name)
def node_find_descendant (node, name):
  for child in node_descendants_subtree (node) or []:
    if child.name == name:
      return child
  return None
# retrive text from specific text element
def node_get_leaf_text (node, name):
  node = node_find (node, name)
  if node and not node.isText():
    node = node_first_child (node)
  if node and node.isText():
    return node.content
  raise RuntimeError ("Failed to find leaf node with text:", name)
# try to find named text element
def node_find_text (node, name):
  node = node_find (node, name)
  if node:
    return node.content
  return ""
# iterator for immediate children
class node_children:
  def __iter__ (self):
    return self
  def __init__ (self, node):
    self.node = None
    if node:
      self.node = node_first_child (node)
  def next (self):
    if self.node:
      value = self.node
      self.node = self.node.next
      return value
    raise StopIteration()
# iterator for node + descendants
class node_subtree:
  def __iter__ (self):
    return self
  def __init__ (self, node):
    self.node = node
    self.iter = None
  def next (self):
    if self.node:
      value = self.node
      self.iter = node_descendants_subtree (self.node)
      self.node = None  # this works for siblings of node, because self.iter is libxml2 depth-first iterator
      return value
    if not self.iter:
      raise StopIteration()
    return self.iter.next()
# iterator for named descendants
class node_descendants_by_name:
  def __iter__ (self):
    return self
  def __init__ (self, node, name):
    self.iter = None
    self.name = name
    if node:
      self.iter = node.children.__iter__()      # libxml2 depth-first iterator
  def next (self):
    while True:
      if not self.iter:
        raise StopIteration()
      value = self.iter.next()
      if value.name == self.name:
        return value
    # loop end, check next child

# --- utility functions ---
def canonify_type (typename):
  string = typename;
  ignore_keywords = 'G_BEGIN_DECLS|G_GNUC_[A-Z_]+'
  string = re.sub (r'\b(' + ignore_keywords + r')\b', ' ', string) # workaround parsing oddities
  string = re.sub (' +', ' ', string)
  string = re.sub (' \*', '*', string)
  string = re.sub ('\* ', '*', string)
  string = re.sub ('@', '$', string)    # anonymous types
  if string.find ('GNUC') >= 0:
    print "TYPE:", string
  return string

# --- Doxygen XML Parser ---
class DoxygenXMLParser:
  def __init__ (self):
    self.files = []
    self.file_dict = {}
    self.comment_dict = {}
    self.doc_queue = []
  def add_file_member (self, dx):
    assert dx.loc_file
    file = self.file_dict.get (dx.loc_file)
    if not file:
      file = Data.SrcFile (dx.loc_file)
      self.file_dict[file.name] = file
      self.files += [ file ]
    file.add_member (dx)
  # --- FIXME: legacy code ---
  def parse_xml_description (self, xdef, desc_tags = [ "briefdescription", "description", "detaileddescription", "inbodydescription" ], skip_tags = []):
    node_map = dict ( [(dtag, None) for dtag in desc_tags] )
    desc_set = set (desc_tags)
    # collect all desc_tags in xdef ancestry
    for dnode in node_subtree (xdef):
      if dnode.name in desc_set:
        node_map[dnode.name] = dnode
        desc_set.remove (dnode.name)
        if not desc_set:
          break
    # select first non-empty description in order
    for dtag in desc_tags:
      dd_node = node_map[dtag]
      if dd_node and dd_node.content.strip():
        break
    else:
      return None
    # copy the whole detaileddescription subtree
    dd_node = dd_node.copyNode (1)
    # unlink tags from skip_list
    skip_list = []
    skip_set = set (skip_tags)
    for child in node_descendants_subtree (dd_node):
      if child.name in skip_set:
        skip_list += [ child ]
    for child in skip_list:
      child.unlinkNode()
    for child in skip_list:
      child.freeNode()
    return dd_node
  # --- comment dictionary ---
  def comment_dict_expand (self, desc_node):
    if desc_node:
      string = desc_node.content.strip()
      if string[:9] == '____doxer':
        result = self.comment_dict.get (string)
        if result:
          (text, fname, fline) = result
          text = re.compile (r'^[ \t]*\*[ \t]?', re.M).sub ('', text) # strip " * " prefix after newline
          while text and not text[0].strip():
            if text[0] == '\n':
              fline += 1        # count lines when stripping head
            text = text[1:]
          text = text.strip()   # strip tail
          return (text, fname, fline)
    return ()
  def comment_dict_load (self, filename):
    debug ("Loading comment dictionary:", filename)
    import cPickle
    fin = open (filename, 'r')
    dict = cPickle.load (fin)
    fin.close()
    self.comment_dict.update (dict)
  # --- parse descriptions ---
  def description_parse_and_set (self, xdef, obj, search_subtree = False, desc_tags = [ 'detaileddescription', 'description', 'briefdescription', 'inbodydescription' ], skip_tags = []):
    node_map = dict ( [(dtag, None) for dtag in desc_tags] )
    desc_set = set (desc_tags)
    # collect all desc_tags in xdef subtree
    if search_subtree:
      for dnode in node_subtree (xdef):
        if dnode.name in desc_set:
          node_map[dnode.name] = dnode
          desc_set.remove (dnode.name)
          if not desc_set:
            break
    else:
      for dnode in node_children (xdef):
        if dnode.name in desc_set:
          node_map[dnode.name] = dnode
          desc_set.remove (dnode.name)
          if not desc_set:
            break
    # select first non-empty description in order
    for dtag in desc_tags:
      dd_node = node_map[dtag]
      if dd_node and dd_node.content.strip():
        break
    else:
      return None
    # copy the whole detaileddescription subtree
    dd_node = dd_node.copyNode (1)
    # unlink tags from skip_list
    skip_list = []
    skip_set = set (skip_tags)
    for child in node_descendants_subtree (dd_node):
      if child.name in skip_set:
        skip_list += [ child ]
    for child in skip_list:
      child.unlinkNode()
    for child in skip_list:
      child.freeNode()
    # assign description
    if dd_node.content:
      xcontent = self.comment_dict_expand (dd_node)
      if xcontent:
        obj.set_doc (xcontent[0], xcontent[1], xcontent[2], '')
        if obj.doc:
          self.doc_queue += [ obj.doc ]
  # --- parse location ---
  def location_parse_and_set (self, xdef, obj): # parse file content location
    loc = xdef.xpathEval ("location")
    if len (loc) == 1:
      file = loc[0].prop ("file")
      if file:
        line = 0
        try:
          line = int (loc[0].prop ("line"))
        except: pass
        if line >= 1:
          # assign location
          obj.set_location (file, line)
          return
    raise RuntimeError ("failed to parse file locaiton")
  # --- macro parser ---
  def parse_define (self, mdef, rcontainer):
    # parse macro name
    name = node_get_leaf_text (mdef, "name")
    # create macro
    dm = Data.Macro (name)
    self.location_parse_and_set (mdef, dm)
    rcontainer.add_file_member (dm)
    # parse description
    self.description_parse_and_set (mdef, dm)
    # parse arguments
    args = mdef.xpathEval ("param")
    if len (args) == 1 and args[0].content == "":
      # print "#define", name + "()"
      pass
    elif [] == args:
      # print "#define", name
      dm.set_is_constant (True)
    else:
      # print "#define", name, "(",
      for a in args:
        # print p.content,
        pass      # FIXME: parse macro arg
      # print ")"
  # --- Typedef parser ---
  def parse_typedef (self, mdef, rcontainer):
    # parse typedef name
    name = node_get_leaf_text (mdef, "name")
    name = canonify_type (name)
    # create typedef
    dt = Data.Typedef (name)
    self.location_parse_and_set (mdef, dt)
    rcontainer.add_file_member (dt)
    # parse description
    self.description_parse_and_set (mdef, dt)
    # parse type
    ttype = node_find_text (mdef, "type")
    ttype = canonify_type (ttype)
    argstring = node_find_text (mdef, "argsstring")
    argstring = canonify_type (argstring)
    dt.set_type (ttype, argstring)
    # fixup typedef (for "typedef struct Foo Foo;", we actually get name=Foo, type=Foo)
    if dt.type.strip() == dt.name.strip():
      # try to fix missing structure prefix
      definition = node_find_text (mdef, "definition")
      if re.search (r'\bstruct\b', definition):
        dt.type = 'struct ' + dt.type
      elif re.search (r'\bunion\b', definition):
        dt.type = 'union ' + dt.type
      elif re.search (r'\bclass\b', definition):
        dt.type = 'class ' + dt.type
  # --- parse enum ---
  def parse_enum (self, mdef, rcontainer):
    # parse enum type name
    name = node_get_leaf_text (mdef, "name")
    name = canonify_type (name)
    # create enum
    de = Data.Enum (name)
    self.location_parse_and_set (mdef, de)
    rcontainer.add_file_member (de)
    # parse description
    self.description_parse_and_set (mdef, de)
    # parse values
    for vnode in node_descendants_by_name (mdef, "enumvalue"):
      vname = node_get_leaf_text (vnode, "name")
      dv = Data.Parameter (vname)
      self.location_parse_and_set (mdef, dv)
      de.add_value (dv)
      # parse description
      self.description_parse_and_set (vnode, dv)
      # parse initializer
      initializer = node_find_text (vnode, "initializer")
      initializer = initializer.strip()
      if initializer:
        dv.set_init (initializer)
  # --- variable parser ---
  def parse_field (self, mdef, compound):
    # parse variable name
    name = node_get_leaf_text (mdef, "name")
    if name[0] == '@':
      return # ignore numbered field names, generated for e.g. anon structures
    # create variable
    dv = Data.Parameter (name)
    self.location_parse_and_set (mdef, dv)
    compound.add_member (dv)
    # parse description
    self.description_parse_and_set (mdef, dv)
    # parse type
    dv.set_type (canonify_type (mdef.xpathEval ("type")[0].content))
    dv.set_const (re.search (r'\bconst\b[^*&]*$', dv.type))
    # parse remaining argstring
    argstring = node_find_text (mdef, "argsstring")
    argstring = canonify_type (argstring)
    dv.set_argstring (argstring)
  # --- parse functions ---
  def parse_global_function (self, cdef, rcontainer):
    # parse function name
    name = cdef.xpathEval ("name")[0].content
    # create function
    df = Data.Function (name)
    self.location_parse_and_set (cdef, df)
    rcontainer.add_file_member (df)
    # standard function stuff
    self.parse_function_def (df, cdef)
  def parse_class_method (self, cdef, compound):
    # parse method name
    name = cdef.xpathEval ("name")[0].content
    # create function
    df = Data.Function (name)
    self.location_parse_and_set (cdef, df)
    compound.add_method (df)
    # standard function stuff
    self.parse_function_def (df, cdef)
  def parse_function_def (self, df, cdef):
    # determine kind
    df.set_flags (cdef.prop ('static') == 'yes',
                  cdef.prop ('virt') == 'virtual',
                  cdef.prop ('inline') == 'yes',
                  cdef.prop ('explicit') == 'yes',
                  cdef.prop ('const') == 'yes')
    # parse description
    self.description_parse_and_set (cdef, df, search_subtree = True, skip_tags = [ "parameterlist", "simplesect" ])
    # parse return type
    dr = Data.Parameter ('', canonify_type (cdef.xpathEval ("type")[0].content))
    df.set_ret_arg (dr)
    # parse return type description
    for child in node_descendants_by_name (cdef, "simplesect"):
      if child.prop ('kind') == 'return':
        self.description_parse_and_set (child, dr, search_subtree = True, desc_tags = [ 'simplesect' ])
        break
    # parse arguments
    for a in cdef.xpathEval ("param"):
      atype = node_find_text (a, "type")
      atype = canonify_type (atype)
      if atype == "...":
        df.set_ellipsis (True)
        continue
      try:
        aname = node_get_leaf_text (a, "declname")
      except:
        aname = ''
      if aname:
        # create argument
        da = Data.Parameter (aname, atype)
        try:    dval = node_get_leaf_text (a, 'defval')
        except: dval = ''
        if dval:
          da.set_default (dval)
        df.add_arg (da)
    # parse documentation arguments
    plist = node_find (cdef, "parameterlist")
    if plist:
      for pitem in node_descendants_subtree (plist):
        if pitem.name == "parameteritem":
          print "qxmlparser.py: Debug1: ",pitem.name
          aname = node_get_leaf_text (pitem, "parametername")
          if not aname:
            raise RuntimeError ("failed to find unique argument name")
          # create argument
          da = Data.Parameter (aname)
          print "qxmlparser.py: Debug2: "
          df.add_doc_arg (da)
          self.description_parse_and_set (pitem, da, search_subtree = True, desc_tags = [ "parameterdescription" ])
  # --- struct parser ---
  def parse_struct (self, cdef, rcontainer):
    # parse struct name
    name = node_get_leaf_text (cdef, "compoundname")
    # create struct
    ds = Data.Struct (name)
    self.location_parse_and_set (cdef, ds)
    rcontainer.add_file_member (ds)
    # parse description
    self.description_parse_and_set (cdef, ds)
    # parse derived classes list
    for ddef in node_descendants_by_name (cdef, "derivedcompoundref"):
      dname = node_find_text (ddef, 'derivedcompoundref')
      protection = ddef.prop ('prot')
      isvirtual  = ddef.prop ('virtual')
      ds.add_derived (dname)
    # parse methods and variable members
    for mdef in node_descendants_by_name (cdef, "memberdef"):
      kind = mdef.prop ('kind');
      if kind == 'variable':
        self.parse_field (mdef, ds)
      elif kind == 'function':
        self.parse_class_method (mdef, ds)
      else:
        print "qxmlparser.py: Unhandled: %s:" % kind, name + "::" + node_get_leaf_text (mdef, "name")
  # --- parse dirs and files ---
  def parse_dir (self, cdef, dummy):           # parse XML dir (compounddef)
    name = node_get_leaf_text (cdef, "compoundname");
  def parse_file_or_namespace (self, cdef, dummy): # parse XML file/namespace (compounddef)
    name = node_get_leaf_text (cdef, "compoundname");
    file_desc = self.parse_xml_description (cdef)
    if file_desc:
      debug ('Ignoring file description:', file_desc.content)
    # parse members: define, struct, typedef, variable, function
    for node in cdef.xpathEval ('sectiondef/memberdef'):
      kind = node.prop ("kind");
      { "define"	: self.parse_define,
        "enum"  	: self.parse_enum,
        "typedef"	: self.parse_typedef,
        "function"      : self.parse_global_function,
        "variable"      : self.parse_export_variable,
      } [kind] (node, self)
  def parse_export_variable (self, mdef, dummy): # parse XML variable (memberdef)
    name = node_get_leaf_text (mdef, "name")
  # --- parse a doxygen XML tree ---
  def parse_doxygen_xml (self, docroot, docname):
    for cdef in docroot.xpathEval ('//compounddef'):
      kind = cdef.prop ("kind");
      try:
        parser = {
          "file"        : self.parse_file_or_namespace,
          "namespace"   : self.parse_file_or_namespace,
          "dir"         : self.parse_dir,
          "struct"      : self.parse_struct,
          "class"       : self.parse_struct,
        } [kind]
      except:
        def print_compound (node, dummy):
          print "qxmlparser.py: Unhandled: %s:" % kind, node_get_leaf_text (node, "compoundname"), "\t\t\t(" + docname + ")"
        parser = print_compound
      parser (cdef, self)

# --- parser wrapper ---
def parse_tree (xmldir):
  xml_files = os.listdir (xmldir)
  dparser = DoxygenXMLParser()
  # parse comment database files
  for xfile in xml_files:
    if xfile[:14] == '.qcomment.dump':
      dparser.comment_dict_load (os.path.join (xmldir, xfile))
  # parse XML input files
  for xfile in xml_files:
    if xfile[:14] == '.qcomment.dump':
      continue
    xfile = os.path.join (xmldir, xfile)
    debug ('XML-Parsing:', xfile)
    xmldoc = libxml2.parseFile (xfile)
    dparser.parse_doxygen_xml (xmldoc, xfile)
    xmldoc.freeDoc()
  return dparser.files, dparser.doc_queue
