# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
import os, sys, re, shutil, hashlib, collections
true, false, length = (True, False, len)

# --- types ---
VOID, BOOL, INT32, INT64, FLOAT64, STRING, ENUM, SEQUENCE, RECORD, INTERFACE, FUNC, TYPE_REFERENCE, STREAM, ANY = [ord (x) for x in 'vbildsEQRCFTMY']
def storage_name (storage):
  name = {
    VOID      : 'VOID',
    BOOL      : 'BOOL',
    INT32     : 'INT32',
    INT64     : 'INT64',
    FLOAT64   : 'FLOAT64',
    STRING    : 'STRING',
    ENUM      : 'ENUM',
    RECORD    : 'RECORD',
    SEQUENCE  : 'SEQUENCE',
    FUNC      : 'FUNC',
    INTERFACE : 'INTERFACE',
    STREAM    : 'STREAM',
    ANY       : 'ANY',
  }.get (storage, None)
  if not name:
    raise RuntimeError ("Invalid storage type: " + storage)
  return name

# --- declarations ---
class BaseDecl (object):
  def __init__ (self):
    self.name = None
    self.namespace = None
    self.loc = ()
    self.hint = ''
    self.docu = ()
  def list_namespaces (self):
    nslist = []
    ns = self.namespace
    while ns:
      nslist = [ ns ] + nslist
      ns = ns.namespace
    return nslist

class Namespace (BaseDecl):
  def __init__ (self, name, outer, impl_list):
    super (Namespace, self).__init__()
    self.name = name
    self.namespace = outer
    if outer:
      outer.ns_nested[self.name] = self
    self.ns_nested = {}
    self.ns_using = []
    self.cmembers = [] # holds: (name, content)
    self.tmembers = [] # holds: (name, content)
    self.type_dict = {}
    self.const_dict = {}
    self.impl_set = set()
    self.global_impl_list = impl_list
  def full_name (self):
    return "::". join ([x.name for x in self.list_namespaces() if x.name] + [self.name])
  def add_const (self, name, content, isimpl):
    self.cmembers += [ (name, content) ]
    self.const_dict[name] = self.cmembers[-1]
    if isimpl:
      self.impl_set.add (name)
  def add_type (self, type):
    assert isinstance (type, TypeInfo)
    type.namespace = self
    self.tmembers += [ (type.name, type) ]
    self.type_dict[type.name] = type
    self.global_impl_list += [ type ]
  def types (self):
    return [mb[1] for mb in self.tmembers]
  def consts (self):
    return self.cmembers
  def unknown (self, name):
    return not (self.const_dict.has_key (name) or self.type_dict.has_key (name))
  def find_const (self, name):
    nc = self.const_dict.get (name, None)
    return nc and (nc[1],) or ()
  def find_type (self, name, fallback = None):
    return self.type_dict.get (name, fallback)

class TypeInfo (BaseDecl):
  collector = 'void'
  def __init__ (self, name, storage, isimpl):
    super (TypeInfo, self).__init__()
    assert storage in (VOID, BOOL, INT32, INT64, FLOAT64, STRING, ENUM, RECORD, SEQUENCE, FUNC, INTERFACE, STREAM, ANY)
    self.name = name
    self.storage = storage
    self.isimpl = isimpl
    # clonable fields:
    self.is_forward = False
    self.options = []           # holds: (ident, label, blurb, number)
    self.combinable = False
    self.location = ('', '??', '??') # holds: (input_file, input_line, input_col)
    if (self.storage == RECORD or
        self.storage == INTERFACE):
      self.fields = []          # holds: (ident, TypeInfo)
    if self.storage == SEQUENCE:
      self.elements = None      # holds: ident, TypeInfo
    if self.storage == FUNC:
      self.args = []            # holds: (ident, TypeInfo, defaultinit)
      self.rtype = None         # holds: TypeInfo
      self.ownertype = None    # TypeInfo
      self.pure = False
      self.issignal = False
    if self.storage == INTERFACE:
      self.prerequisites = []
      self.methods = []         # holds: TypeInfo
      self.signals = []         # holds: TypeInfo
    self.auxdata = collections.OrderedDict()
    if self.storage == STREAM:
      self.ioj_stream = ''      # one of: 'I', 'O', 'J'
  @staticmethod
  def builtin_type (ident, bseextensions = False):
    def mkstream (ioj):
      ti = TypeInfo (ioj + 'Stream', STREAM, False)
      ti.set_stream_type (ioj)
      return ti
    builtins = {
      'void'    : TypeInfo ('void',     VOID,    False),
      'bool'    : TypeInfo ('bool',     BOOL,    False),
      'int32'   : TypeInfo ('int32',    INT32,   False),
      'int64'   : TypeInfo ('int64',    INT64,   False),
      'float64' : TypeInfo ('float64',  FLOAT64, False),
      'String'  : TypeInfo ('String',   STRING,  False),
      'Any'     : TypeInfo ('Any',      ANY,     False),
      'IStream' : mkstream ('I'),
      'OStream' : mkstream ('O'),
      'JStream' : mkstream ('J'),
    }
    if bseextensions:
      builtins['Bool'] = builtins['bool']
      builtins['Int']  = builtins['int32']
      builtins['Num']  = builtins['int64']
      builtins['Real'] = builtins['float64']
      builtins['SfiString'] = builtins['String']
    return builtins.get (ident, None)
  def string_digest (self):
    typelist, arglist = [], [] # owner, self, rtype, arg*type, ...
    if self.__dict__.get ('ownertype', None): typelist += [self.ownertype]
    typelist += [self]
    if self.__dict__.get ('rtype', None): arglist += [self.rtype]
    if self.__dict__.get ('args', []): arglist += [a[1] for a in self.args]
    typelist = '::'.join ([tp.full_name() for tp in typelist]) # MethodObject method_name
    arglist  =  '+'.join ([tp.full_name() for tp in arglist])  # void bool int float string
    return typelist + (' ' if arglist else '') + arglist
  def ident_digest (self):
    digest = self.string_digest()
    digest = re.sub ('[^a-zA-Z0-9]', '_', digest)
    return digest
  def hash128digest (self, prefix, postfix = ''):
    sha224 = hashlib.sha224()
    hash_feed = '5e8398ff-75eb-466f-b256-213fc9de5b6e | ' + prefix + ' | ' + self.string_digest() + postfix
    # print >>sys.stderr, "HASH:", hash_feed
    sha224.update (hash_feed)
    hash128 = sha224.digest()[3:12] + sha224.digest()[16:23]
    return hash128
  def tag_hash (self, tag):
    hbytes = self.hash128digest (tag)
    return tuple ([ord (c) for c in hbytes])
  def type_hash (self):
    if self.storage == FUNC:
      if self.issignal:
        tag = "sigcon"
      elif self.rtype.storage == VOID:
        tag = "oneway"
      else:
        tag = "twoway"
    else:
      tag = "type"
    bytes = self.hash128digest (tag)
    t = tuple ([ord (c) for c in bytes])
    return t
  def twoway_hash (self, special = ''):
    tag = "twoway"
    tag = '%s/%s' % (tag, special) if special else tag
    return self.tag_hash (tag)
  def property_hash (self, field, setter):
    if setter:
      tag = "setter" # oneway
    else:
      tag = "getter" # twoway
    postfix = '::' + field[0] + ' ' + field[1].full_name()
    bytes = self.hash128digest (tag, postfix)
    t = tuple ([ord (c) for c in bytes])
    return t
  def link (self, isimpl):
    origin = self
    while hasattr (origin, '__origin__'):
      origin = origin.__origin__ # resolve links to links
    ti = TypeLink (self)
    ti.isimpl = self.isimpl
    assert id (ti.__origin__) == id (self)
    assert id (ti.location) != id (self.location)
    assert id (ti.auxdata) != id (self.auxdata)
    return ti
  def clone (self, newname, isimpl):
    if newname == None: newname = self.name
    ti = TypeInfo (newname, self.storage, isimpl)
    ti.is_forward = self.is_forward
    ti.options += self.options
    ti.combinable = self.combinable
    ti.location = ('', '??', '??')              # clones are *not* defined in the same location
    if hasattr (self, 'ioj_stream'):
      ti.ioj_stream = self.ioj_stream
    if hasattr (self, 'namespace'):
      ti.namespace = self.namespace
    if hasattr (self, 'ns_nested'):
      ti.ns_nested = {}
      ti.ns_nested.update (self.ns_nested)
    if hasattr (self, 'ns_using'):
      ti.ns_using += self.ns_using
    if hasattr (self, 'fields'):
      ti.fields += self.fields
    if hasattr (self, 'args'):
      ti.args += self.args
    if hasattr (self, 'rtype'):
      ti.rtype = self.rtype
    if hasattr (self, 'pure'):
      ti.pure = self.pure
    if hasattr (self, 'issignal'):
      ti.issignal = self.issignal
    if hasattr (self, 'elements'):
      ti.elements = self.elements
    if hasattr (self, 'prerequisites'):
      ti.prerequisites += self.prerequisites
    if hasattr (self, 'methods'):
      ti.methods += self.methods
    if hasattr (self, 'ownertype'):
      ti.ownertype = self.ownertype
    if hasattr (self, 'signals'):
      ti.signals += self.signals
    ti.auxdata.update (self.auxdata)
    return ti
  def update_auxdata (self, auxdict):
    self.auxdata.update (auxdict)
  def set_combinable (self, as_flags):
    assert self.storage == ENUM
    self.combinable = as_flags
    self.update_auxdata ({ 'enum_combinable' : int (bool (self.combinable)) })
  def set_location (self, input_file, input_line, input_col):
    self.location = (input_file, input_line, input_col)
  def add_option (self, ident, label, blurb, number):
    assert self.storage == ENUM
    assert isinstance (ident, str)
    assert isinstance (label, str)
    assert isinstance (blurb, str)
    assert isinstance (number, (int, long))
    self.options += [ (ident, label, blurb, number) ]
  def has_option (self, ident = None, label = None, blurb = None, number = None):
    assert self.storage == ENUM
    if (ident, label, blurb, number) == (None, None, None, None):
      return len (self.options) > 0
    for o in self.options:
      if ((ident  == None or o[0] == ident) and
          (label  == None or o[1] == label) and
          (blurb  == None or o[2] == blurb) and
          (number == None or o[3] == number)):
        return true
    return false
  def set_stream_type (self, ioj):
    assert self.storage == STREAM
    assert ioj in ('I', 'O', 'J')
    self.ioj_stream = ioj
  def add_field (self, ident, type):
    assert self.storage == RECORD or self.storage == INTERFACE
    assert isinstance (ident, str)
    assert isinstance (type, TypeInfo)
    self.fields += [ (ident, type) ]
  def add_arg (self, ident, type, defaultinit):
    assert self.storage == FUNC
    assert isinstance (ident, str)
    assert isinstance (type, TypeInfo)
    self.args += [ (ident, type, defaultinit) ]
  def set_rtype (self, type):
    assert self.storage == FUNC
    assert isinstance (type, TypeInfo)
    assert self.rtype == None
    self.rtype = type
  def set_pure (self, vbool):
    assert self.storage == FUNC
    self.pure = bool (vbool)
  def set_collector (self, collkind):
    self.collector = collkind
  def add_method (self, ftype, issignal = False):
    assert self.storage == INTERFACE
    assert isinstance (ftype, TypeInfo)
    assert ftype.storage == FUNC
    assert isinstance (ftype.rtype, TypeInfo)
    ftype.ownertype = self
    ftype.issignal = issignal
    if issignal:
      self.signals += [ ftype ]
    else:
      self.methods += [ ftype ]
  def add_prerequisite (self, type):
    assert self.storage == INTERFACE
    assert isinstance (type, TypeInfo)
    assert type.storage == INTERFACE
    self.prerequisites += [ type ]
  def set_elements (self, ident, type):
    assert self.storage == SEQUENCE
    assert isinstance (ident, str)
    assert isinstance (type, TypeInfo)
    self.elements = (ident, type)
  def full_name (self):
    # record fields are not namespaced
    prefix = self.namespace.full_name() if self.namespace else ''
    if prefix: prefix += '::'
    return prefix + self.name

class TypeLink (TypeInfo):
  def __init__ (self, type_info):
    assert issubclass (type_info.__class__, TypeInfo)
    self.__origin__ = type_info
    self.location = ('', '??', '??')            # links are *not* defined in the same location
    self.auxdata = collections.OrderedDict()    # links can carry own auxillary data
  def __getattr__ (self, name):
    return getattr (self.__origin__, name)
