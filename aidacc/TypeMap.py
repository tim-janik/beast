# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"""AidaTypeMap - Binary Type Map generator for Aida

More details at https://rapicorn.testbit.org/
"""
import Decls, struct

class Encoder:
  def __init__ (self, node_offset = 16, list_offset = 24, string_offset = 32):
    self.node0 = node_offset
    self.nodes = ''
    self.nodem = []
    self.list0 = list_offset
    self.lists = ''
    self.listm = []
    self.string0 = string_offset
    self.strings = ''
    self.stringm = {}
    if 0: # debug segment offsets
      import sys
      print >>sys.stderr, "AidaTypeMap:Encoder: segment offsets:", self.node0, self.list0, self.string0
  def segment_offsets (self):
    return (self.node0, self.list0, self.string0)
  def segment_lengths (self):
    return (len (self.nodes), len (self.lists), len (self.strings))
  def segment_strings (self):
    return (self.nodes, self.lists, self.strings)
  def node_index (self, node):
    assert isinstance (node, tuple) and len (node) == 4
    for t in self.nodem:
      if t[0] == node:
        return t[1] # idx
    s = self.encode_fields (node)
    idx = self.node0 + len (self.nodes)
    self.nodes += s
    self.nodem += [ (node, idx) ]
    return idx
  def list_index (self, lst):
    assert isinstance (lst, list)
    for t in self.listm:
      if t[0] == lst:
        return t[1] # idx
    s = self.encode_unsigned (len (lst))
    for v in lst:
      s += self.encode_value (v)
    idx = self.list0 + len (self.lists)
    self.lists += s
    self.listm += [ (lst, idx) ]
    return idx
  def string_index (self, string):
    assert isinstance (string, str)
    idx = self.stringm.get (string, None)
    if idx == None:
      s = self.encode_string (string)
      idx = self.string0 + len (self.strings)
      self.strings += s
      self.stringm[string] = idx
    return idx
  def encode_fields (self, fields):
    assert isinstance (fields, tuple)
    s = ''
    for f in fields:
      s += self.encode_value (f)
    return s
  def encode_value (self, v):
    n = None
    if isinstance (v, (int, long)):
      n = v
    elif isinstance (v, str):
      n = self.string_index (v)
    elif isinstance (v, list):
      n = self.list_index (v)
    elif isinstance (v, tuple):
      n = self.node_index (v)
    else:
      raise TypeError ('unencodable value: ' + repr (v))
    return self.encode_unsigned (n)
  @staticmethod
  def encode_string (string):
    r = (len (string) + 1) % 4
    r = 4 - r if r else 0
    return Encoder.encode_unsigned (len (string)) + string + '\0' + r * ' '
  @staticmethod
  def encode_unsigned (num):
    assert num >= 0 and num <= 0xffffffff
    return struct.pack ('<I', num)      # little endian uint
  @staticmethod
  def encode_double (num):
    return struct.pack ('<d', num)      # little endian double

def encode_type_map (nodes):
  def pad (string, sz = 8, c = '\0'):
    l = sz - len (string) % sz
    return string + c * (l % sz)
  def align (l, sz = 8):
    return ((l + sz - 1) / sz) * sz
  def header (sz, ooo, idx):
    s = 'AidaTypeMap\0\0\0\0\0'
    s += Encoder.encode_unsigned (sz) + '\0\0\0\0' + '\0\0\0\0' + '\0\0\0\0'
    s += Encoder.encode_unsigned (ooo[0])
    s += Encoder.encode_unsigned (ooo[1])
    s += Encoder.encode_unsigned (ooo[2]) + '\0\0\0\0'
    s += Encoder.encode_unsigned (idx) + '\0\0\0\0' + '\0\0\0\0' + '\0\0\0\0'
    return s
  assert isinstance (nodes, list)
  hl = len (pad (header (0xdeadbeef, (0xdeadbeef, 0xdeadbeef, 0xdeadbeef), 0xdeadbeef)))
  tail = '\0\0\0\0'
  # first encoding round, lengths unknown
  e = Encoder()
  first_idx = e.list_index (nodes)
  # measure lengths
  lengths = e.segment_lengths ()
  # second encoding round, lengths known
  e = Encoder (hl, hl + align (lengths[0]), hl + align (lengths[0]) + align (lengths[1]))
  nodelist_idx = e.list_index (nodes)
  # ensure stable encoding
  assert nodelist_idx >= first_idx
  assert lengths == e.segment_lengths()
  # merge parts
  l = hl + align (lengths[0]) + align (lengths[1]) + align (lengths[2])
  strings = e.segment_strings()
  s = pad (header (l, e.segment_offsets(), nodelist_idx))
  s += pad (strings[0]) + pad (strings[1]) + pad (strings[2]) + tail
  assert len (s) == l + len (tail)
  return s

class Generator:
  def __init__ (self, config = {}):
    self.config = {}
    self.config.update (config)
  def aux_strings (self, auxdata):
    result = []
    for ad in auxdata.items():
      if isinstance (ad[1], str) and ad[1][0] in '"\'':
        import rfc822
        astr = rfc822.unquote (ad[1])
      else:
        astr = str (ad[1])
      result += [ str (ad[0] + '=' + astr) ]
    return result
  def type_key (self, type_info):
    if type_info == 'REFERENCE':
      return 0 + Decls.TYPE_REFERENCE
    else:
      return 0 + type_info.storage
  reference_types = (Decls.ENUM, Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE)
  def generate_field (self, field_name, type_info):
    tp = type_info
    fields = []
    if tp.storage in self.reference_types:
      fields += [ int (self.type_key ('REFERENCE')) ]
    else:
      fields += [ int (self.type_key (tp)) ]
    fields += [ str (field_name) ]
    fields += [ self.aux_strings (type_info.auxdata) ]
    if tp.storage in self.reference_types:
      fields += [ str (tp.full_name()) ]
    else:
      fields += [ int (0) ]
    return tuple (fields)
  def generate_type (self, type_info):
    tp = type_info
    fields = []
    fields += [ int (self.type_key (type_info)) ]
    fields += [ str (type_info.full_name()) ]
    fields += [ self.aux_strings (type_info.auxdata) ]
    if tp.storage == Decls.SEQUENCE:
      fields += [ self.generate_field (tp.elements[0], tp.elements[1]) ]
    elif tp.storage == Decls.RECORD:
      members = []
      for fl in tp.fields:
        members += [ self.generate_field (fl[0], fl[1]) ]
      fields += [ members ]
    elif tp.storage == Decls.ENUM:
      s = []
      for opt in tp.options:
        (ident, label, blurb, number) = opt
        number = number if number >= 0 else 0x10000000000000000 + number # turn into uint64
        low, high = number & 0xffffffff, number >> 32
        for num in (low, high):
          if not (num >= 0 and num <= 0xffffffff):
            raise Exception ("Invalid enum value: %d" % num)
        s += [ low, high, ident, label, blurb ] # low, high, ident, label, blurb
      fields += [ s ]
    elif tp.storage == Decls.INTERFACE:
      s = []
      for pr in tp.prerequisites:
        s += [ str (pr.full_name()) ]
      fields += [ s ]
    else:
      fields += [ 0 ]
    return tuple (fields)
  def namespace_types (self, namespace):
    # sort namespace types for binary lookups
    types = namespace.types()[:]        # list copy
    types.sort (lambda o1, o2: cmp (o1.name, o2.name))
    # FIXME: filter types for isimpl
    return types
  def generate_namespace_type_map (self, namespace_list):
    # sort namespaces for binary lookups
    namespace_list = namespace_list[:]  # list copy
    namespace_list.sort (lambda o1, o2: cmp (o1.name, o2.name))
    # collect namespaced types
    types = []
    for ns in namespace_list:
      types += self.namespace_types (ns)
    return self.generate_type_map (types)
  def generate_type_map (self, type_list):
    types = type_list[:] # list copy
    # sort namespaced types for binary lookups
    types.sort (lambda o1, o2: cmp (o1.name, o2.name))
    # serialize types
    tsl = []
    for tp in types:
      t = self.generate_type (tp)
      tsl += [ t ]
    # strip builtin typedefs
    if self.config.get ('system-typedefs', 0):
      import re
      otsl,tsl = tsl,[]
      for tp in otsl:
        tid,fqn,aux,memb = tp
        fqn = re.sub (r'^Aida::__system_typedefs__::__builtin__', '', fqn)
        tsl += [ (tid, fqn, aux, memb) ]
    # encode type map from serialized types
    return encode_type_map (tsl)

def error (msg):
  import sys
  print >>sys.stderr, sys.argv[0] + ":", msg
  sys.exit (127)

def cquote (text):
  s, lastoctal = '"', False
  for c in text:
    oldnl = len (s) / 70
    shortoctal = False
    if   c == '\\':                             s += r'\\'
    elif c == '"':                              s += '\\"'
    elif c >= '0' and c <= '9' and lastoctal:   s += '""' + c
    elif c >= ' ' and c <= '~':                 s += c
    else:                                       s += '\%o' % ord (c) ; shortoctal = ord (c) < 64;
    lastoctal = shortoctal
    if len (s) / 70 != oldnl:                   s += '"\n"'; lastoctal = False
  s += '"'
  return s

def generate_type_map (type_list, **args):
  gg = Generator (args)
  binary_type_map = gg.generate_type_map (type_list)
  return binary_type_map

def generate_file (namespace_list, **args):
  import sys, tempfile, os
  config = { 'output' : 'idltypes.map' }
  config.update (args)
  gg = Generator (config)
  packdata = gg.generate_namespace_type_map (namespace_list)
  outputname = config['output']
  # print strcquote (packdata)
  # write data into a temporary file in the same dir as outputname
  odir = os.path.dirname (outputname) or '.'
  try:
    (fdout, tmpname) = tempfile.mkstemp ('-' + os.path.basename (outputname), '.tmp', odir)
  except Exception, ex:
    error ('Failed to create temporary file in %s: %s' % (odir, ex))
  os.write (fdout, packdata)
  os.close (fdout)
  # fix up permissions for mmap-able read-only file
  umask = os.umask (0777); os.umask (umask)     # fetch umask
  mode = 0444 & ~umask                          # mode = a+r
  os.chmod (tmpname, mode)
  # atomically replace mmap-able output file
  rex = None
  try: os.rename (tmpname, outputname)
  except Exception, rex: pass   # relay error after cleanup
  # cleanup
  try: os.remove (tmpname)
  except: pass
  if rex:
    error ('Failed to atomically replace "%s": %s' % (outputname, rex))

# register extension hooks
__Aida__.add_backend (__file__, generate_file, __doc__)
