# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"""AidaCxxStub - Aida C++ Code Generator

More details at https://beast.testbit.org/
"""
import Decls, GenUtils, TmplFiles, re, os, collections

def reindent (prefix, lines):
  return re.compile (r'^', re.M).sub (prefix, lines.rstrip())
def backslash_quote (string):
  return re.sub (r'(\\|")', r'\\\1', string)
def cquote (string):
  string = backslash_quote (string)
  return '"' + string + '"'
def cunquote (string):
  assert len (string) >= 2 and string.startswith ('"') and string.endswith ('"')
  string = string[1:-1]
  string = re.sub (r'\\(\\|")', r'\1', string)
  return string
def cunquote_chain (stringchain): # handle: "foo""bar""zonk" -> "foobarzonk"
  s, dq, sin = '', False, stringchain
  while sin:
    c, sin = sin[0], sin[1:]
    if   c == '"':
      dq = not dq
    elif c == '\\':
      assert len (sin) > 0
      c, sin = sin[0], sin[1:]
      s += c
    else:
      s += c
  assert not dq
  return s

def aux_data_value_string (cvalue):
  if isinstance (cvalue, str) and cvalue.startswith ('_(') and cvalue.endswith (')'):
    cvalue = cvalue[2:-1]
  if isinstance (cvalue, str):
    return cunquote_chain (cvalue)
  return str (cvalue)

def hasancestor (child, parent):
  for p in child.prerequisites:
    if p == parent or hasancestor (p, parent):
      return True
def inherit_reduce (type_list):
  # find the type(s) we *directly* derive from
  reduced = []
  while type_list:
    p = type_list.pop()
    skip = 0
    for c in type_list + reduced:
      if c == p or hasancestor (c, p):
        skip = 1
        break
    if not skip:
      reduced = [ p ] + reduced
  return reduced
def bases (tp):
  ancestors = [pr for pr in tp.prerequisites]
  return inherit_reduce (ancestors)
def type_name_parts (type_node, include_empty = False):
  parts = [ns.name for ns in type_node.list_namespaces()] + [ type_node.name ]
  if not include_empty:
    parts = [p for p in parts if p]
  return parts

I_prefix_postfix = ('', 'Iface')

class G4STUB: pass    # generate stub classes (remote handles)
class G4SERVANT: pass    # generate servants classes (interfaces)

class Generator:
  def __init__ (self, idl_file):
    assert len (idl_file) > 0
    self.ntab = 26
    self.gen_docs = False
    self.namespaces = []
    self.insertions = {}
    self.idl_file = idl_file
    self.cppmacro = None
    self.ns_aida = None
    self.gen_inclusions = []
    self.iface_base = 'Aida::ImplicitBase'
    self.property_list = 'Aida::PropertyList'
    self.gen_mode = None
    self.strip_path = ''
    self.declared_pointerdefs = set() # types for which pointerdefs have been generated
    self.reset()
  def warning (self, message, input_file = '', input_line = -1, input_col = -1):
    import sys
    if input_file:
      if input_line > 0 and input_col > 0:
        loc = '%s:%u:%u' % (input_file, input_line, input_col)
      elif input_line > 0:
        loc = '%s:%u' % (input_file, input_line)
      else:
        loc = input_file
    else:
      loc = self.idl_path()
    input_file = input_file if input_file else self.idl_path()
    print >>sys.stderr, '%s: WARNING: %s' % (loc, message)
  @staticmethod
  def prefix_namespaced_identifier (prefix, ident, postfix = ''):     # Foo::bar -> Foo::PREFIX_bar_POSTFIX
    cc = ident.rfind ('::')
    if cc >= 0:
      ns, base = ident[:cc+2], ident[cc+2:]
    else:
      ns, base = '', ident
    return ns + prefix + base + postfix
  def Iwrap (self, name):
    return self.prefix_namespaced_identifier (I_prefix_postfix[0], name, I_prefix_postfix[1])
  def type2cpp_absolute (self, type_node):
    tstorage = type_node.storage
    if tstorage == Decls.VOID:          return 'void'
    if tstorage == Decls.BOOL:          return 'bool'
    if tstorage == Decls.INT32:         return 'int'
    if tstorage == Decls.INT64:         return 'int64_t'
    if tstorage == Decls.FLOAT64:       return 'double'
    if tstorage == Decls.STRING:        return 'std::string'
    if tstorage == Decls.ANY:           return 'Aida::Any'
    tnsl = type_node.list_namespaces() # type namespace list
    absolute_namespaces = [d.name for d in tnsl if d.name]
    fullnsname = '::'.join (absolute_namespaces + [ type_node.name ])
    return fullnsname
  def type2cpp_relative (self, type_node):
    tstorage = type_node.storage
    if tstorage == Decls.VOID:          return 'void'
    if tstorage == Decls.BOOL:          return 'bool'
    if tstorage == Decls.INT32:         return 'int'
    if tstorage == Decls.INT64:         return 'int64_t'
    if tstorage == Decls.FLOAT64:       return 'double'
    if tstorage == Decls.STRING:        return 'std::string'
    if tstorage == Decls.ANY:           return 'Aida::Any'
    fullnsname = '::'.join (self.type_relative_namespaces (type_node) + [ type_node.name ])
    return fullnsname
  def type2cpp (self, type_node):
    return self.type2cpp_relative (type_node)
  def C4server (self, type_node):
    tname = self.type2cpp (type_node)
    if type_node.storage == Decls.INTERFACE:
      return self.Iwrap (tname)                         # construct servant class interface name
    elif type_node.storage in (Decls.SEQUENCE, Decls.RECORD):
      return self.prefix_namespaced_identifier ('', tname)
    return tname
  def C4client (self, type_node):
    tname = self.type2cpp (type_node)
    if type_node.storage == Decls.INTERFACE:
      return tname + 'Handle'                           # construct client class RemoteHandle
    elif type_node.storage in (Decls.SEQUENCE, Decls.RECORD):
      return self.prefix_namespaced_identifier ('', tname)
    return tname
  def C (self, type_node, mode = None):                 # construct Class name
    mode = mode or self.gen_mode
    if mode == G4SERVANT:
      return self.C4server (type_node)
    else: # G4STUB
      return self.C4client (type_node)
  def R (self, type_node):                              # construct Return type
    tname = self.C (type_node)
    if self.gen_mode == G4SERVANT and type_node.storage == Decls.INTERFACE:
      tname += 'P'
    return tname
  def M (self, type_node):                              # construct Member type
    if self.gen_mode == G4STUB and type_node.storage == Decls.INTERFACE:
      classH = self.C4client (type_node) # remote handle class name
      classC = self.C4server (type_node) # servant class name
      return 'Aida::RemoteMember<%s>' % classH # classC
    else:
      return self.R (type_node)
  def V (self, ident, type_node, f_delta = -999999):    # construct Variable
    s = ''
    s += self.C (type_node)
    s = self.F (s, f_delta)
    if self.gen_mode == G4SERVANT and type_node.storage == Decls.INTERFACE:
      s += '*'
    else:
      s += ' '
    return s + ident
  def A (self, ident, type_node, defaultinit = None):   # construct call Argument
    constref = type_node.storage in (Decls.STRING, Decls.SEQUENCE, Decls.RECORD, Decls.ANY)
    needsref = constref or type_node.storage == Decls.INTERFACE
    s = self.C (type_node)                      # const {Obj} &foo = 3
    s += ' ' if ident else ''                   # const Obj{ }&foo = 3
    if constref:
      s = 'const ' + s                          # {const }Obj &foo = 3
    if needsref:
      s += '&'                                  # const Obj {&}foo = 3
    s += ident                                  # const Obj &{foo} = 3
    if defaultinit != None:
      if type_node.storage == Decls.ENUM:
        s += ' = %s (%s)' % (self.C (type_node), defaultinit)  # { = 3}
      elif type_node.storage in (Decls.SEQUENCE, Decls.RECORD):
        s += ' = %s()' % self.C (type_node)                    # { = 3}
      elif type_node.storage == Decls.INTERFACE:
        s += ' = *(%s*) NULL' % self.C (type_node)             # { = 3}
      else:
        s += ' = %s' % defaultinit                             # { = 3}
    return s
  def Args (self, ftype, prefix, argindent = 2):        # construct list of call Arguments
    l = []
    for a in ftype.args:
      l += [ self.A (prefix + a[0], a[1]) ]
    return (',\n' + argindent * ' ').join (l)
  def U (self, ident, type_node):                       # construct function call argument Use
    s = '*' if type_node.storage == Decls.INTERFACE and self.gen_mode == G4SERVANT else ''
    return s + ident
  def F (self, string, delta = 0):                      # Format string to tab stop
    return string + ' ' * max (1, self.ntab + delta - len (string))
  def tab_stop (self, n):
    self.ntab = n
  def close_inner_namespace (self):
    current_namespace = self.namespaces.pop()
    return '} // %s\n' % current_namespace.name if current_namespace.name else ''
  def open_inner_namespace (self, namespace):
    self.namespaces += [ namespace ]
    current_namespace = self.namespaces[-1]
    return '\nnamespace %s {\n' % current_namespace.name if current_namespace.name else ''
  def open_namespace (self, typeinfo):
    s = ''
    newspaces = []
    if type (typeinfo) == tuple:
      newspaces = list (typeinfo)
    elif typeinfo:
      newspaces = typeinfo.list_namespaces()
      # s += '// ' + str ([n.name for n in newspaces]) + '\n'
    while len (self.namespaces) > len (newspaces):
      s += self.close_inner_namespace()
    while self.namespaces and newspaces[0:len (self.namespaces)] != self.namespaces:
      s += self.close_inner_namespace()
    for n in newspaces[len (self.namespaces):]:
      s += self.open_inner_namespace (n)
    return s
  def type_relative_namespaces (self, type_node):
    tnsl = type_node.list_namespaces() # type namespace list
    # remove common prefix with global namespace list
    for n in self.namespaces:
      if tnsl and tnsl[0] == n:
        tnsl = tnsl[1:]
      else:
        break
    namespace_names = [d.name for d in tnsl if d.name]
    return namespace_names
  def namespaced_identifier (self, ident):
    names = [d.name for d in self.namespaces if d.name]
    if ident:
      names += [ ident ]
    return '::'.join (names)
  def mkzero (self, type):
    if type.storage == Decls.STRING:
      return '""'
    if type.storage == Decls.ENUM:
      return self.C (type) + ' (0)'
    if type.storage in (Decls.RECORD, Decls.SEQUENCE, Decls.ANY):
      return self.C (type) + '()'
    return '0'
  def generate_recseq_visit (self, type_info):
    s = ''
    s += 'template<class Visitor> void\n%s::__visit__ (Visitor &&_visitor_)\n{\n' % self.C (type_info)
    if type_info.storage == Decls.RECORD:
      for fname, ftype in type_info.fields:
        s += '  std::forward<Visitor> (_visitor_) (%s, "%s");\n' % (fname, fname)
    s += '}\n'
    return s
  def generate_recseq_accept (self, type_info):
    s = ''
    s += '  template<class Visitor> void  __accept__  (Visitor &_visitor_)\n'
    s += '  {\n'
    if type_info.storage == Decls.RECORD:
      for fname, ftype in type_info.fields:
        s += '    _visitor_ (%s, "%s");\n' % (fname, fname)
    s += '  }\n'
    return s
  def generate_recseq_decl (self, type_info):
    s = '\n'
    classC, classFull = self.C (type_info), self.namespaced_identifier (type_info.name)
    # s += self.generate_shortdoc (type_info)   # doxygen IDL snippet
    if type_info.storage == Decls.SEQUENCE:
      fl = type_info.elements
      #s += '/// @cond GeneratedRecords\n'
      s += 'class ' + classC + ' : public std::vector<' + self.M (fl[1]) + '>\n'
      s += '{\n'
    else:
      #s += '/// @cond GeneratedSequences\n'
      s += 'class %s\n' % classC
      s += '{\n'
    s += 'public:\n'
    if type_info.storage == Decls.RECORD:
      s += '  /// @cond GeneratedFields\n'
      fieldlist = type_info.fields
      for fl in fieldlist:
        s += '  ' + self.F (self.M (fl[1])) + fl[0] + ';\n'
      s += '  /// @endcond\n'
    elif type_info.storage == Decls.SEQUENCE:
      s += '  typedef std::vector<' + self.M (fl[1]) + '> Sequence;\n'
      s += '  reference append_back() ///< Append data at the end, returns write reference to data.\n'
      s += '  { resize (size() + 1); return back(); }\n'
    if type_info.storage == Decls.SEQUENCE:
      s += '  ' + self.F ('explicit') + '%s (std::initializer_list<value_type> il) : Sequence (il) {};\n' % classC
      s += '  ' + self.F ('inline') + '%s () = default;\n' % classC # ctor
      s += '  ' + self.F ('inline') + '%s (const Aida::AnyList &al) : %s() { __aida_from_any__ (Aida::Any (al)); }\n' % (classC, classC) # ctor
    elif type_info.storage == Decls.RECORD:
      s += '  ' + self.F ('inline') + '%s () {' % classC # ctor
      for fl in fieldlist:
        if fl[1].storage in (Decls.BOOL, Decls.INT32, Decls.INT64, Decls.FLOAT64, Decls.ENUM):
          s += " %s = %s;" % (fl[0], self.mkzero (fl[1]))
      s += ' }\n'
      s += '  ' + self.F ('inline') + '%s (const Aida::AnyDict &ad) : %s() { __aida_from_any__ (Aida::Any (ad)); }\n' % (classC, classC) # ctor
    s += '  ' + self.F ('std::string') + '__typename__      () const\t{ return "%s"; }\n' % classFull
    s += '  ' + self.F ('const Aida::StringVector&') + '__aida_aux_data__ () const;\n'
    if type_info.storage == Decls.SEQUENCE:
      s += '  ' + self.F ('Aida::Any') + '__aida_to_any__   () { return Aida::any_from_sequence (*this); }\n'
      s += '  ' + self.F ('void') + '__aida_from_any__ (const Aida::Any &any) { return Aida::any_to_sequence (any, *this); }\n'
      s += '  ' + self.F ('operator') + 'Aida::AnyList     () const '
      s += '{ return const_cast<%s*> (this)->__aida_to_any__().get<Aida::AnyList>(); }\n' % classC
    if type_info.storage == Decls.RECORD:
      s += '  ' + self.F ('Aida::Any') + '__aida_to_any__   () { return Aida::any_from_visitable (*this); }\n'
      s += '  ' + self.F ('void') + '__aida_from_any__ (const Aida::Any &any) { return Aida::any_to_visitable (any, *this); }\n'
      s += '  ' + self.F ('operator') + 'Aida::AnyDict     () const '
      s += '{ return const_cast<%s*> (this)->__aida_to_any__().get<Aida::AnyDict>(); }\n' % classC
    if type_info.storage == Decls.RECORD:
      s += '  ' + self.F ('bool') + 'operator==   (const %s &other) const;\n' % classC
      s += '  ' + self.F ('bool') + 'operator!=   (const %s &other) const { return !operator== (other); }\n' % classC
      s += '  ' + self.F ('operator') + 'Aida::AnyRec () const { Aida::AnyRec r; const_cast<%s*> (this)->__visit__ ([&r] (const auto &v, const char *n) { r[n] = v; }); return r; }\n' % classC
      s += '  ' + self.F ('template<class Visitor> void') + '__visit__    (Visitor &&_visitor_);\n'
      s += self.generate_recseq_accept (type_info)
    if self.gen_mode == G4STUB:
      s += self.insertion_text ('handle_scope:' + type_info.name)
    if self.gen_mode == G4SERVANT:
      s += self.insertion_text ('interface_scope:' + type_info.name)
    s += '};\n'
    if type_info.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += 'void operator<<= (Aida::ProtoMsg&, const %s&);\n' % classC
      s += 'void operator>>= (Aida::ProtoReader&, %s&);\n' % classC
    #s += '/// @endcond\n'
    return s
  def generate_proto_add_args (self, fb, type_info, aprefix, arg_info_list, apostfix):
    s = ''
    for arg_it in arg_info_list:
      ident, type_node = arg_it
      ident = aprefix + ident + apostfix
      s += '  %s <<= %s;\n' % (fb, ident)
    return s
  def generate_proto_pop_args (self, fbr, type_info, aprefix, arg_info_list, apostfix = ''):
    s = ''
    for arg_it in arg_info_list:
      ident, type_node = arg_it
      ident = aprefix + ident + apostfix
      s += '  %s >>= %s;\n' % (fbr, ident)
    return s
  def generate_aux_data_string (self, tp, name = ''):
    s, prefix = '', (name + '.' if name else '')
    if name:
      s += '  "%stype=%s\\0"\n' % (prefix, Decls.storage_name (tp.storage))
      if tp.storage in (Decls.ENUM, Decls.RECORD, Decls.SEQUENCE, Decls.INTERFACE):
        fullnsname = self.type2cpp_absolute (tp)
        s += '  "%stypename=%s\\0"\n' % (prefix, fullnsname)
    for k,v in tp.auxdata.items():
      qvalue = backslash_quote (aux_data_value_string (v))
      if qvalue:
        s += '  "%s%s=%s\\0"\n' % (prefix, k, qvalue)
    if not name and tp.storage == Decls.ENUM:
      for ev in tp.options:
        (ident, label, blurb, number) = ev
        s += '  "%s.value=%d\\0"\n' % (ident, number)
        if label:
          #label = label[-1] == ')' and re.sub ('[A-Z]*_\(', '', label[:-1]) or label # strip i18n function wrapper
          s += '  "%s.label=" %s "\\0"\n' % (ident, label)
        if blurb:
          #blurb = blurb[-1] == ')' and re.sub ('[A-Z]*_\(', '', blurb[:-1]) or blurb # strip i18n function wrapper
          s += '  "%s.blurb=" %s "\\0"\n' % (ident, blurb)
    if not name and tp.storage == Decls.SEQUENCE:
      s += self.generate_aux_data_string (tp.elements[1], prefix + tp.elements[0])
    if not name and tp.storage in (Decls.RECORD, Decls.INTERFACE):
      for fid, ftp in tp.fields:
        s += self.generate_aux_data_string (ftp, prefix + fid)
    return s
  def generate_recseq_aux_method_impls (self, tp): # FIXME: rename
    absolute_typename = self.type2cpp_absolute (tp)
    s = ''
    # __aida_aux_data__
    s += 'const Aida::StringVector&\n%s::__aida_aux_data__() const\n{\n' % self.C (tp)
    s += '  static const Aida::StringVector sv = Aida::IntrospectionRegistry::lookup ("%s");\n' % absolute_typename
    s += '  return sv;\n'
    s += '}\n'
    return s
  def generate_record_impl (self, type_info):
    s = ''
    s += self.generate_type_aux_data_registration (type_info)
    s += self.generate_recseq_aux_method_impls (type_info)
    s += 'bool\n'
    s += '%s::operator== (const %s &other) const\n{\n' % (self.C (type_info), self.C (type_info))
    for field in type_info.fields:
      ident, type_node = field
      s += '  if (this->%s != other.%s) return false;\n' % (ident, ident)
    s += '  return true;\n'
    s += '}\n'
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator<<= (Aida::ProtoMsg &dst, const %s &self)\n{\n' % self.C (type_info)
    s += '  Aida::ProtoMsg &__p_ = dst.add_rec (%u);\n' % len (type_info.fields)
    s += self.generate_proto_add_args ('__p_', type_info, 'self.', type_info.fields, '')
    s += '}\n'
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator>>= (Aida::ProtoReader &src, %s &self)\n{\n' % self.C (type_info)
    s += '  Aida::ProtoReader fbr (src.pop_rec());\n'
    s += '  if (fbr.remaining() < %u) return;\n' % len (type_info.fields)
    s += self.generate_proto_pop_args ('fbr', type_info, 'self.', type_info.fields)
    s += '}\n'
    return s
  def generate_sequence_impl (self, type_info):
    s = ''
    s += self.generate_type_aux_data_registration (type_info)
    s += self.generate_recseq_aux_method_impls (type_info)
    el = type_info.elements
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator<<= (Aida::ProtoMsg &dst, const %s &self)\n{\n' % self.C (type_info)
    s += '  const size_t len = self.size();\n'
    s += '  Aida::ProtoMsg &__p_ = dst.add_seq (len);\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += reindent ('  ', self.generate_proto_add_args ('__p_', type_info, '',
                                                       [('self', type_info.elements[1])],
                                                       '[k]')) + '\n'
    s += '  }\n'
    s += '}\n'
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator>>= (Aida::ProtoReader &src, %s &self)\n{\n' % self.C (type_info)
    s += '  Aida::ProtoReader fbr (src.pop_seq());\n'
    s += '  const size_t len = fbr.remaining();\n'
    s += '  self.resize (len);\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += '    fbr >>= self[k];\n'
    s += '  }\n'
    s += '}\n'
    return s
  def generate_enum_info_impl (self, type_info):
    u_typename, c_typename = '__'.join (type_name_parts (type_info)), '::'.join (type_name_parts (type_info))
    s, varray = '\n', '_aida_enumvalues_%u' % self.idcounter
    s += self.generate_type_aux_data_registration (type_info)
    self.idcounter += 1
    enum_ns, enum_class = '::'.join (self.type_relative_namespaces (type_info)), type_info.name
    s += 'template<> const EnumInfo&\n'
    s += 'enum_info<%s> ()\n{\n' % c_typename
    s += '  static const EnumValue %s[] = {\n' % varray
    for opt in type_info.options:
      (ident, label, blurb, number) = opt
      # number = self.c_long_postfix (number)
      number = enum_ns + '::' + enum_class + '::' + ident
      ident = cquote (ident)
      label = label if label else "NULL"
      blurb = blurb if blurb else "NULL"
      s += '    { int64_t (%s), %s, %s, %s },\n' % (number, ident, label, blurb)
    s += '  };\n'
    s += '  return ::Aida::EnumInfo::cached_enum_info ("%s", %s, %s);\n' % (c_typename, int (type_info.combinable), varray)
    s += '} // specialization\n'
    # explicit instantiation occuring after an explicit specialization has no effect
    # s += 'template const EnumInfo& enum_info<%s> (); // instantiation\n' % c_typename
    return s
  def digest2cbytes (self, digest):
    return '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL' % digest
  def method_digest (self, method_info):
    return self.digest2cbytes (method_info.type_hash())
  def class_digest (self, class_info):
    return self.digest2cbytes (class_info.type_hash())
  def internal_digest (self, class_info, tag):
    return self.digest2cbytes (class_info.tag_hash (tag + ' # Aida:::internal'))
  def any_method_digest (self, class_info, methodname):
    return self.digest2cbytes (class_info.twoway_hash ('%s # internal-method' % methodname))
  def setter_digest (self, class_info, fident, ftype):
    setter_hash = class_info.property_hash ((fident, ftype), True)
    return self.digest2cbytes (setter_hash)
  def getter_digest (self, class_info, fident, ftype):
    getter_hash = class_info.property_hash ((fident, ftype), False)
    return self.digest2cbytes (getter_hash)
  def class_ancestry (self, type_info):
    def deep_ancestors (type_info):
      l = [ type_info ]
      for a in type_info.prerequisites:
        l += deep_ancestors (a)
      return l
    def make_list_uniq (lst):
      r, q = [], set()
      for e in lst:
        if not e in q:
          q.add (e)
          r += [ e ]
      return r
    return make_list_uniq (deep_ancestors (type_info))
  def interface_class_ancestors (self, type_info):
    l = []
    for pr in type_info.prerequisites:
      l += [ pr ]
    l = inherit_reduce (l)
    return l
  def interface_class_inheritance (self, type_info):
    aida_remotehandle, ddc = 'Aida::RemoteHandle', False
    l = self.interface_class_ancestors (type_info)
    l = [self.C (pr) for pr in l] # types -> names
    if self.gen_mode == G4SERVANT:
      if l:
        heritage = 'public virtual'
      else:
        l, ddc = [self.iface_base], True
        heritage = 'public virtual'
    else:
      if l:
        heritage = 'public virtual'
      else:
        l, ddc = [aida_remotehandle], True
        heritage = 'public virtual'
    if self.gen_mode == G4SERVANT:
      cl = []
    else:
      cl = l if l == [aida_remotehandle] else [aida_remotehandle] + l
    return (l, heritage, cl, ddc) # prerequisites, heritage type, constructor args, direct-descendant (of ancestry root)
  def generate_interface_pointerdefs (self, type_info):
    s, classC = '', self.C (type_info)
    if classC in self.declared_pointerdefs:
      return s
    assert self.gen_mode == G4SERVANT
    s += 'class %s;\n' % classC
    s += 'typedef std::shared_ptr<%s> %sP;\n' % (classC, classC)
    s += 'typedef std::weak_ptr  <%s> %sW;\n' % (classC, classC)
    s += '\n'
    self.declared_pointerdefs.add (classC)
    return s
  def generate_interface_class (self, type_info, class_name_list):
    s, classC, classH, classFull = '\n', self.C (type_info), self.C4client (type_info), self.namespaced_identifier (type_info.name)
    class_name_list += [ classFull ]
    if self.gen_mode == G4SERVANT:
      s += self.generate_interface_pointerdefs (type_info)
    # declare
    if self.gen_docs:
      s += self.generate_shortdoc (type_info)     # doxygen IDL snippet
    s += 'class %s' % classC
    # inherit
    precls, heritage, cl, ddc = self.interface_class_inheritance (type_info)
    s += ' : ' + heritage + ' %s' % (', ' + heritage + ' ').join (precls) + '\n'
    s += '{\n'
    if self.gen_mode == G4STUB:
      s += '  ' + self.F ('friend') + 'class ' + self.C4server (type_info) + ';\n'
      s += '  ' + self.F ('static const Aida::TypeHash&') + '__aida_typeid__();\n'
    # constructors
    s += 'protected:\n'
    if self.gen_mode == G4SERVANT:
      s += '  explicit' + self.F (' ') + '%s ();\n' % self.C (type_info) # ctor
      s += '  virtual ' + self.F (' /*dtor*/ ', -1) + '~%s () override = 0;\n' % self.C (type_info)
    s += 'public:\n'
    if self.gen_mode == G4STUB:
      s += '  ' + self.F ('virtual /*dtor*/ ', -1) + '~%s () override;\n' % classC
      s += '  ' + self.F ('/*copy*/') + '%s (const %s&) = default;\n' % (classC, classC)
      s += '  ' + self.F (classC + '&') + 'operator= (const %s&) = default;\n' % classC
    if self.gen_mode == G4SERVANT:
      s += '  ' + self.F ('%s' % classH) + '        __handle__         ();\n'
      s += '  virtual ' + self.F ('Aida::TypeHashList') + '__aida_typelist__  () const override;\n'
      s += '  virtual ' + self.F ('std::string') + '__typename__       () const override\t{ return "%s"; }\n' % classFull
      s += self.generate_class_aux_method_decls (type_info)
      s += self.generate_class_any_method_decls (type_info)
    else: # G4STUB
      s += '  ' + self.F ('static %s' % classH) + 'down_cast (const RemoteHandle &smh);\n'
      s += '  ' + self.F ('explicit') + '%s ();\n' % classH # ctor
      #s += '  ' + self.F ('inline') + '%s (const %s &src)' % (classH, classH) # copy ctor
      #s += ' : ' + ' (src), '.join (cl) + ' (src) {}\n'
    # properties
    il = 0
    if type_info.fields:
      il = max (len (fl[0]) for fl in type_info.fields)
    for fl in type_info.fields:
      s += self.generate_property_prototype (type_info, fl[0], fl[1], il)
    # methods
    il = 0
    if type_info.methods:
      il = max (len (m.name) for m in type_info.methods)
      il = max (il, len (self.C (type_info)))
    for m in type_info.methods:
      s += self.generate_method_decl (type_info, m, il)
    if self.gen_mode == G4STUB:
      s += '  __%s_ifx__ ( %s*  __iface__ () const );\n' % (self.cppmacro, self.C4server (type_info))
      # s += '  __%s_ifx__ ( %s  operator= (%s*) );\n' % (self.cppmacro, classC, self.C4server (type_info))
      s += '  __%s_ifx__ ( /*conv*/    %s (const std::shared_ptr<%s>&) );\n' % (self.cppmacro, classC, self.C4server (type_info))
      s += self.insertion_text ('handle_scope:' + type_info.name)
    if self.gen_mode == G4SERVANT:
      s += self.insertion_text ('interface_scope:' + type_info.name)
    # accept
    s += self.generate_class_accept_accessor (type_info)
    s += '};\n'
    if self.gen_mode == G4SERVANT:
      s += 'void operator<<= (Aida::ProtoMsg&, const %sP&);\n' % self.C (type_info)
      s += 'void operator>>= (Aida::ProtoReader&, %s*&);\n' % self.C (type_info)
      s += 'void operator>>= (Aida::ProtoReader&, %sP&);\n' % self.C (type_info)
    # typedef alias
    if self.gen_mode == G4STUB:
      s += self.generate_handle_typedefs (type_info)
    return s
  def generate_shortdoc (self, type_info):      # doxygen snippets
    classC = self.C (type_info) # class name
    xkind = 'servant' if self.gen_mode == G4SERVANT else 'stub'
    s  = '/** @interface %s\n' % type_info.name
    s += ' * See also the corresponding C++ %s class %s. */\n' % (xkind, classC)
    s += '/// See also the corresponding IDL class %s.\n' % type_info.name
    return s
  def generate_handle_typedefs (self, type_info):
    assert self.gen_mode == G4STUB
    s = ''
    cxxtypename = self.type2cpp (type_info)
    s += 'typedef %s %sH;' % (self.C (type_info), cxxtypename)
    s += ' ///< Convenience alias for the IDL type %s.\n' % type_info.name
    s += 'typedef ::Aida::ScopedHandle<%sH> %sS;\n' % (cxxtypename, cxxtypename)
    return s
  def generate_method_decl (self, class_info, functype, pad):
    s = '  '
    if self.gen_mode == G4SERVANT:
      s += 'virtual '
    s += self.F (self.R (functype.rtype))
    s += functype.name
    s += ' ' * max (0, pad - len (functype.name))
    s += ' ('
    argindent = len (s)
    l = []
    for a in functype.args:
      l += [ self.A (a[0], a[1], a[2]) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ')'
    if self.gen_mode == G4SERVANT and functype.pure:
      s += ' = 0'
    s += ';'
    if self.gen_docs:
      copydoc = 'See ' + self.type2cpp (class_info) + '::' + functype.name + '()'
      s += ' \t///< %s' % copydoc
    s += '\n'
    return s
  def generate_class_accept_accessor (self, tp):
    s, classH = '', self.C (tp)
    reduced_immediate_ancestors = self.interface_class_ancestors (tp)
    s += '  template<class Visitor> void  __accept_accessor__ (Visitor &__visitor_)\n'
    if not tp.fields and not reduced_immediate_ancestors:
      return s + '  {}\n'
    s += '  {\n'
    for fname, ftype in tp.fields:
      ctype = self.C (ftype)
      reftype = ctype
      if ftype.storage in (Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE, Decls.ANY):
        pass # reftype = 'const %s&' % ctype
      s += '    __visitor_ (*this, "%s", &%s::%s, &%s::%s);\n' % (fname, classH, fname, classH, fname)
    for atp in reduced_immediate_ancestors:
      s += '    this->%s::__accept_accessor__ (__visitor_);\n' % self.C (atp)
    s += '  }\n'
    return s
  def generate_type_aux_data_registration (self, tp):
    s = ''
    aux_data_string = self.generate_aux_data_string (tp)
    aux_data_string = aux_data_string if aux_data_string else '  ""'
    fileprefix = 'srvt' if self.gen_servercc else 'clnt'
    absolute_typename = self.type2cpp_absolute (tp)
    unique_type_identifier = re.sub ('::', '_', absolute_typename)
    s += 'static const Aida::IntrospectionRegistry __aida_aux_data_%s__%s_ = {\n' % (fileprefix, unique_type_identifier)
    s += '  "%s\\0"\n' % absolute_typename
    s += '  "%s\\0"\n' % Decls.storage_name (tp.storage)
    s += aux_data_string
    s += '};\n'
    return s
  def generate_class_aux_method_decls (self, tp):
    assert self.gen_mode == G4SERVANT
    s = ''
    virtual = 'virtual '
    s += '  %-37s __aida_aux_data__  () const override;\n' % (virtual + 'const Aida::StringVector&')
    return s
  def generate_server_class_aux_method_impls (self, tp):
    assert self.gen_mode == G4SERVANT
    s, classH = '', self.C (tp)
    absolute_typename = self.type2cpp_absolute (tp)
    reduced_immediate_ancestors = self.interface_class_ancestors (tp)
    # __aida_aux_data__
    s += self.generate_type_aux_data_registration (tp)
    s += 'const Aida::StringVector&\n%s::__aida_aux_data__() const\n{\n' % classH
    s += '  static const Aida::StringVector __d_ =\n    ::Aida::aux_vectors_combine ('
    s += 'Aida::IntrospectionRegistry::lookup ("%s")' % absolute_typename
    for atp in reduced_immediate_ancestors:
      s += ',\n                                 this->%s::__aida_aux_data__()' % self.C (atp)
    s += ');\n'
    s += '  return __d_;\n'
    s += '}\n'
    return s
  def generate_class_any_method_decls (self, tp):
    assert self.gen_mode == G4SERVANT
    s = ''
    virtual = 'virtual '
    s += '  %-37s __access__         (const std::string &propertyname, const PropertyAccessor&) override;\n' % (virtual + 'bool')
    s += '  %-37s __aida_dir__       () const override;\n' % (virtual + 'std::vector<std::string>')
    s += '  %-37s __aida_get__       (const std::string &name) const override;\n' % (virtual + 'Aida::Any')
    s += '  %-37s __aida_set__       (const std::string &name, const Aida::Any &any) override;\n' % (virtual + 'bool')
    return s
  def generate_server_class_any_method_impls (self, tp):
    assert self.gen_mode == G4SERVANT
    s, classH = '', self.C (tp)
    reduced_immediate_ancestors = self.interface_class_ancestors (tp)
    # __access__
    s += 'bool\n%s::__access__ (const std::string &__n, const PropertyAccessor &__p)\n{\n' % classH
    a  = '  const bool __all = __n.empty();\n'
    for fname, ftype in tp.fields:
      if ftype.storage in (Decls.RECORD, Decls.SEQUENCE):
        continue
      if a:
        s, a = s + a, ''
      ctype = self.C (ftype) # self.type2cpp_relative (ftype)
      s += '  if (__all || __n == "%s") return __p (' % fname
      s += 'Aida::PropertyDesc<%s,%s> ("%s", *this, &%s::%s, &%s::%s, NULL)' % (classH, ctype, fname, classH, fname, classH, fname)
      s += '), true;\n'
    for atp in reduced_immediate_ancestors:
      s += '  if (this->%s::__access__ (__n, __p)) return true;\n' % self.C (atp)
    s += '  return false;\n'
    s += '}\n'
    # __aida_dir__
    s += 'std::vector<std::string>\n%s::__aida_dir__ () const\n{\n' % classH
    s += '  std::vector<std::string> __d_;\n'
    for fname, ftype in tp.fields:
      ctype = self.C (ftype)
      s += '  __d_.push_back ("%s");\n' % fname
    for atp in reduced_immediate_ancestors:
      s += '  { const Aida::StringVector &__t_ = this->%s::__aida_dir__();\n' % self.C (atp)
      s += '    __d_.insert (__d_.end(), __t_.begin(), __t_.end()); }\n'
    s += '  return __d_;\n'
    s += '}\n'
    # __aida_get__
    s += 'Aida::Any\n%s::__aida_get__ (const std::string &__n_) const\n{\n' % classH
    s += '  Aida::Any __a_;\n'
    for fname, ftype in tp.fields:
      ctype = self.C (ftype)
      cond = 'if (__n_ == "%s")' % fname
      s += '  %-30s { %s (%s()); return __a_; }\n' % (cond, '__a_.set', fname)
    for atp in reduced_immediate_ancestors:
      s += '  __a_ = this->%s::__aida_get__ (__n_); if (__a_.kind()) return __a_;\n' % self.C (atp)
    s += '  return __a_;\n'
    s += '}\n'
    # __aida_set__
    s += 'bool\n%s::__aida_set__ (const std::string &__n_, const Aida::Any &__a_)\n{\n' % classH
    for fname, ftype in tp.fields:
      ctype = self.C (ftype)
      cond = 'if (__n_ == "%s")' % fname
      if ftype.storage == Decls.INTERFACE: # need to deref shared_ptr
        s += '  %-30s { %s (__a_.get<%sP>().get()); return true; }\n' % (cond, fname, ctype)
      else:
        s += '  %-30s { %s (__a_.get<%s>()); return true; }\n' % (cond, fname, ctype)
    for atp in reduced_immediate_ancestors:
      s += '  if (this->%s::__aida_set__ (__n_, __a_)) return true;\n' % self.C (atp)
    s += '  return false;\n'
    s += '}\n'
    return s
  def generate_client_class_methods (self, class_info):
    s, classH, classC = '', self.C4client (class_info), self.C4server (class_info)
    classH2 = (classH, classH)
    precls, heritage, cl, ddc = self.interface_class_inheritance (class_info)
    s += '%s::%s ()' % classH2 # ctor
    s += '\n{}\n'
    s += '%s::~%s ()\n{} // define empty dtor to emit vtable\n' % classH2 # dtor
    s += 'const Aida::TypeHash&\n'
    s += '%s::__aida_typeid__()\n{\n' % classH
    s += '  static const Aida::TypeHash type_hash = Aida::TypeHash (%s);\n' % self.class_digest (class_info)
    s += '  return type_hash;\n'
    s += '}\n'
    s += '%s\n%s::down_cast (const Aida::RemoteHandle &other)\n{\n' % classH2 # similar to ctor
    s += '  Aida::ImplicitBaseP &ifacep = const_cast<Aida::RemoteHandle&> (other).__iface_ptr__();\n'
    s += '  return std::dynamic_pointer_cast<%s> (ifacep);\n' % classC
    s += '}\n'
    return s
  def generate_server_class_methods (self, tp):
    assert self.gen_mode == G4SERVANT
    s, classC, classH = '\n', self.C (tp), self.C4client (tp)
    s += '%s::%s ()' % (classC, classC) # ctor
    s += '\n{}\n'
    s += '%s::~%s ()\n{} // define empty dtor to emit vtable\n' % (classC, classC) # dtor
    s += '%s\n%s::__handle__()\n{\n' % (classH, classC)
    s += '  Aida::ExecutionContext &ec = this->__execution_context_mt__();\n'
    s += '  %s handle;\n' % classH
    s += '  handle.__iface_ptr__() = std::dynamic_pointer_cast<%s> (ec.adopt_deleter_mt (this->shared_from_this()));\n' % classC
    s += '  return handle;\n'
    s += '}\n'
    # s += '  __%s_ifx__ ( /*conv*/    %s (%s*) );\n' % (self.cppmacro, classC, self.C4server (type_info))
    s += '%s::%s (const std::shared_ptr<%s> &ifacep)\n{\n' % (classH, classH, classC)
    s += '  if (!ifacep)\n    return;\n'
    s += '  Aida::ExecutionContext &ec = ifacep->__execution_context_mt__();\n'
    s += '  __iface_ptr__() = std::dynamic_pointer_cast<%s> (ec.adopt_deleter_mt (ifacep));\n' % classC
    s += '}\n'
    # s_ifx__ ( __iface__() )
    s += '%s*\n%s::__iface__() const\n{\n' % (classC, classH)
    s += '  return dynamic_cast<%s*> (const_cast<%s*> (this)->__iface_ptr__().get());\n' % (classC, classH)
    s += '}\n'
    # s += '  __%s_ifx__ ( %s  operator= (%s*) );\n' % (self.cppmacro, classC, self.C4server (type_info))
    s += 'void\n'
    s += 'operator<<= (Aida::ProtoMsg &__p_, const %sP &ptr)\n{\n' % classC
    s += '  __p_ <<= ptr.get();\n'
    s += '}\n'
    s += 'void\n'
    s += 'operator>>= (Aida::ProtoReader &__f_, %sP &obj)\n{\n' % classC
    s += '  obj = __f_.pop_instance<%s>();\n' % classC
    s += '}\n'
    s += 'void\n'
    s += 'operator>>= (Aida::ProtoReader &__f_, %s* &obj)\n{\n' % classC
    s += '  obj = __f_.pop_instance<%s>().get();\n' % classC
    s += '}\n'
    s += 'Aida::TypeHashList\n'
    s += '%s::__aida_typelist__ () const\n{\n' % classC
    s += '  Aida::TypeHashList thl;\n'
    ancestors = self.class_ancestry (tp)
    for an in ancestors:
      s += '  thl.push_back (Aida::TypeHash (%s)); // %s\n' % (self.class_digest (an), an.name)
    s += '  return thl;\n'
    s += '}\n'
    return s
  def generate_remote_call (self, classname, methodname, rstorage, specialcase = None, args = ()):
    s = ''
    asconst, argprefix, deref = False, '', '*'
    if specialcase == 'c':      # property getters use const this
      asconst = True
    elif specialcase == '&':    # property setters may set NULL pointers, regular method args may not be NULL (FIXME?)
      deref = ''
    elif isinstance (specialcase, basestring):
      argprefix = specialcase
    variant = 'remote_callv' if rstorage == Decls.VOID else ('remote_callc' if asconst else 'remote_callr')
    s += '  return __AIDA_Local__::%s (*this, &%s::%s' % (variant, classname, methodname)
    for a in args:
      if a[1].storage == Decls.INTERFACE:
        s += ', ' + deref + argprefix + a[0] + '.__iface__()'
      else:
        s += ', ' + argprefix + a[0]
    s += ');\n'
    return s
  def generate_client_method_stub (self, class_info, mtype):
    s,copydoc = '', ''
    hasret = mtype.rtype.storage != Decls.VOID
    if self.gen_docs:
      copydoc = ' /// See ' + self.type2cpp (class_info) + '::' + mtype.name + '()'
    # prototype
    s += self.C (mtype.rtype) + '\n'
    q = '%s::%s (' % (self.C (class_info), mtype.name)
    s += q + self.Args (mtype, 'arg_', len (q)) + ')%s\n{\n' % copydoc
    # generate wrapped lambda call
    s += self.generate_remote_call (self.C4server (class_info), mtype.name, mtype.rtype.storage, 'arg_', mtype.args)
    return s + '}\n'
  def generate_server_method_stub (self, class_info, mtype, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__aida_call__%s__%s' % (class_info.name, mtype.name)
    reglines += [ (self.method_digest (mtype), self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Aida::ProtoMsg*\n'
    s += dispatcher_name + ' (Aida::ProtoReader &fbr)\n'
    s += '{\n'
    s += '  AIDA_ASSERT_RETURN (fbr.remaining() == 3 + 1 + %u, NULL);\n' % len (mtype.args)
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_ASSERT_RETURN (self != NULL, NULL);\n'
    # fetch args
    for a in mtype.args:
      s += '  ' + self.V ('arg_' + a[0], a[1]) + ';\n'
      s += self.generate_proto_pop_args ('fbr', class_info, 'arg_', [(a[0], a[1])])
    # return var
    s += '  '
    hasret = mtype.rtype.storage != Decls.VOID
    if hasret:
      s += self.R (mtype.rtype) + ' rval = '
    # call out
    s += 'self->' + mtype.name + ' ('
    s += ', '.join (self.U ('arg_' + a[0], a[1]) for a in mtype.args)
    s += ');\n'
    # store return value
    if hasret:
      s += '  Aida::ProtoMsg &rb = *__AIDA_Local__::new_call_result (fbr, %s);\n' % self.method_digest (mtype) # invalidates fbr
      rval = 'rval'
      s += self.generate_proto_add_args ('rb', class_info, '', [(rval, mtype.rtype)], '')
      s += '  return &rb;\n'
    else:
      s += '  return NULL;\n'
    # done
    s += '}\n'
    return s
  def generate_property_prototype (self, class_info, fident, ftype, pad = 0):
    s, v, v0, rptr, ptr = '', '', '', '', ''
    if self.gen_docs:
      copydoc = ' \t///< See ' + self.type2cpp (class_info) + '::' + fident
    else:
      copydoc = ''
    if self.gen_mode == G4SERVANT:
      v, v0, rptr, ptr = 'virtual ', ' = 0', 'P', '*'
    tname = self.C (ftype)
    pid = fident + ' ' * max (0, pad - len (fident))
    if ftype.storage in (Decls.BOOL, Decls.INT32, Decls.INT64, Decls.FLOAT64, Decls.ENUM):
      s += '  ' + v + self.F (tname)  + pid + ' () const%s;%s\n' % (v0, copydoc)
      s += '  ' + v + self.F ('void') + pid + ' (' + tname + ')%s;%s\n' % (v0, copydoc)
    elif ftype.storage in (Decls.STRING, Decls.RECORD, Decls.SEQUENCE, Decls.ANY):
      s += '  ' + v + self.F (tname)  + pid + ' () const%s;%s\n' % (v0, copydoc)
      s += '  ' + v + self.F ('void') + pid + ' (const ' + tname + '&)%s;%s\n' % (v0, copydoc)
    elif ftype.storage == Decls.INTERFACE:
      s += '  ' + v + self.F (tname + rptr)  + pid + ' () const%s;%s\n' % (v0, copydoc)
      s += '  ' + v + self.F ('void') + pid + ' (' + tname + ptr + ')%s;%s\n' % (v0, copydoc)
    return s
  def generate_client_property_stub (self, class_info, fident, ftype):
    s, tname, copydoc = '', self.C (ftype), ''
    if self.gen_docs:
      copydoc = ' /// See ' + self.type2cpp (class_info) + '::' + fident
    # getter prototype
    s += tname + '\n'
    q = '%s::%s (' % (self.C (class_info), fident)
    s += q + ') const%s\n{\n' % copydoc
    s += self.generate_remote_call (self.C4server (class_info), fident, ftype.storage, 'c')
    s += '}\n'
    # setter prototype
    s += 'void\n'
    if ftype.storage in (Decls.STRING, Decls.RECORD, Decls.SEQUENCE, Decls.ANY):
      s += q + 'const ' + tname + ' &value)%s\n{\n' % copydoc
    else:
      s += q + tname + ' value)%s\n{\n' % copydoc
    s += self.generate_remote_call (self.C4server (class_info), fident, Decls.VOID, '&', [ ('value', ftype) ])
    s += '}\n'
    return s
  def generate_server_property_setter (self, class_info, fident, ftype, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__aida_set__%s__%s' % (class_info.name, fident)
    setter_hash = self.setter_digest (class_info, fident, ftype)
    reglines += [ (setter_hash, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Aida::ProtoMsg*\n'
    s += dispatcher_name + ' (Aida::ProtoReader &fbr)\n'
    s += '{\n'
    s += '  AIDA_ASSERT_RETURN (fbr.remaining() == 3 + 1 + 1, NULL);\n'
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_ASSERT_RETURN (self != NULL, NULL);\n'
    # fetch property
    s += '  ' + self.V ('arg_' + fident, ftype) + ';\n'
    s += self.generate_proto_pop_args ('fbr', class_info, 'arg_', [(fident, ftype)])
    ref = '&' if ftype.storage == Decls.INTERFACE else ''
    # call out
    s += '  self->' + fident + ' (' + ref + self.U ('arg_' + fident, ftype) + ');\n'
    s += '  return NULL;\n'
    s += '}\n'
    return s
  def generate_server_property_getter (self, class_info, fident, ftype, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__aida_get__%s__%s' % (class_info.name, fident)
    getter_hash = self.getter_digest (class_info, fident, ftype)
    reglines += [ (getter_hash, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Aida::ProtoMsg*\n'
    s += dispatcher_name + ' (Aida::ProtoReader &fbr)\n'
    s += '{\n'
    s += '  AIDA_ASSERT_RETURN (fbr.remaining() == 3 + 1, NULL);\n'
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_ASSERT_RETURN (self != NULL, NULL);\n'
    # return var
    s += '  '
    s += self.R (ftype) + ' rval = '
    # call out
    s += 'self->' + fident + ' ();\n'
    # store return value
    s += '  Aida::ProtoMsg &rb = *__AIDA_Local__::new_call_result (fbr, %s);\n' % getter_hash # invalidates fbr
    rval = 'rval'
    s += self.generate_proto_add_args ('rb', class_info, '', [(rval, ftype)], '')
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def generate_server_method_registry (self, reglines):
    s = '\n'
    s += 'namespace { namespace AIDA_CPP_PASTE (__aida_stubs, __COUNTER__) {\n'
    if len (reglines) == 0:
      return '// Skipping empty MethodRegistry\n'
    s += 'static const __AIDA_Local__::MethodEntry entries[] = {\n'
    for dispatcher in reglines:
      cdigest, dispatcher_name = dispatcher
      s += '  { ' + cdigest + ', '
      s += dispatcher_name + ', },\n'
    s += '};\n'
    s += 'static __AIDA_Local__::MethodRegistry registry (entries);\n'
    s += '} } // anon::__aida_stubs##__COUNTER__\n'
    return s
  def c_long_postfix (self, number):
    if number >= 9223372036854775808:
      return str (number) + 'u'
    if number == -9223372036854775808:    # in C++ parsed as two tokens '-' '9223372036854775808'
      return '(-9223372036854775807 - 1)' # avoid "integer constant is so large that it is unsigned"
    return str (number)
  def generate_enum_decl (self, type_info):
    s = '\n'
    nm, c_typename = type_info.name, self.namespaced_identifier (type_info.name)
    l = []
    s += '/// @cond GeneratedEnums\n'
    s += 'enum class %s : int64_t {\n' % type_info.name
    for opt in type_info.options:
      (ident, label, blurb, number) = opt
      s += '  %s = %s,' % (ident, self.c_long_postfix (number))
      if blurb:
        s += ' // %s' % re.sub ('\n', ' ', blurb)
      s += '\n'
    s += '};\n'
    s += 'inline const char* operator->* (::Aida::IntrospectionTypename, %s) { return "%s"; }\n' % (nm, c_typename)
    s += 'inline void operator<<= (Aida::ProtoMsg &__p_,  %s  e) ' % nm
    s += '{ __p_ <<= Aida::EnumValue (e); }\n'
    s += 'inline void operator>>= (Aida::ProtoReader &__f_, %s &e) ' % nm
    s += '{ e = %s (__f_.pop_evalue()); }\n' % nm
    s += 'AIDA_ENUM_DEFINE_ARITHMETIC_EQ (%s);\n' % nm
    if type_info.combinable: # enum as flags
      s += 'AIDA_FLAGS_DEFINE_ARITHMETIC_OPS (%s);\n' % nm
    s += '/// @endcond\n'
    return s
  def generate_enum_info_specialization (self, type_info):
    s = ''
    classFull = '::'.join (self.type_relative_namespaces (type_info) + [ type_info.name ])
    s += 'template<> const EnumInfo& enum_info<%s> ();\n' % classFull
    return s
  def insertion_text (self, key):
    text = self.insertions.get (key, '')
    lstrip = re.compile ('^\n*')
    rstrip = re.compile ('\n*$')
    text = lstrip.sub ('', text)
    text = rstrip.sub ('', text)
    if text:
      ind = '  ' if key.startswith (('class_scope:', 'handle_scope:', 'interface_scope:')) else '' # inner class indent
      return ind + '// ' + key + ':\n' + text + '\n'
    else:
      return ''
  def insertion_file (self, filename):
    f = open (filename, 'r')
    key = None
    for line in f:
      m = (re.match ('(includes):\s*(//.*)?$', line) or
           re.match ('(class_scope:\w+):\s*(//.*)?$', line) or
           re.match ('(handle_scope:\w+):\s*(//.*)?$', line) or
           re.match ('(interface_scope:\w+):\s*(//.*)?$', line))
      if not m:
        m = re.match ('(IGNORE):\s*(//.*)?$', line)
      if m:
        key = m.group (1)
        continue
      if key:
        block = self.insertions.get (key, '')
        block += line
        self.insertions[key] = block
  def idl_path (self):
    apath = os.path.abspath (self.idl_file)
    if self.strip_path:
      for prefix in (self.strip_path, os.path.abspath (self.strip_path)):
        if apath.startswith (prefix):
          apath = apath[len (prefix):]
          if apath[0] == '/':
            apath = apath[1:]
          break
    return apath
  def reset (self):
    self.idcounter = 1001
  def generate_impl_types (self, implementation_types):
    self.reset()
    def text_expand (txt):
      txt = txt.replace ('$AIDA_iface_base$', self.iface_base)
      return txt
    self.gen_mode = G4SERVANT if self.gen_serverhh or self.gen_servercc else G4STUB
    if self.cppmacro == None:
      self.cppmacro = re.sub (r'(^[^A-Z_a-z])|([^A-Z_a-z0-9])', '_', self.idl_path())
    s = '// --- Generated by AidaCxxStub ---\n'
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # CPP guards
    sc_macro_prefix, sc_other_prefix = '__CLNT__', '__SRVT__' # for G4STUB
    clntsrvt_id = 2 if self.gen_serverhh or self.gen_servercc else 1
    if self.gen_mode == G4SERVANT:
      sc_macro_prefix, sc_other_prefix = sc_other_prefix, sc_macro_prefix
    if self.gen_serverhh or self.gen_clienthh:
      s += '#ifndef %s\n#define %s\n\n' % (sc_macro_prefix + self.cppmacro, sc_macro_prefix + self.cppmacro)
    # interface hooks
    if self.gen_clienthh:
      s += '#ifndef __%s_ifx__\n' % self.cppmacro
      s += '#define __%s_ifx__(...) /**/\n' % self.cppmacro
      s += '#endif\n\n'
    # inclusions
    if self.gen_serverhh:
      for tp in types:  # we need to pre-declare classes for Handle<->Iface conversions
        if not tp.is_forward and tp.storage == Decls.INTERFACE:
          s += self.open_namespace (tp)
          s += 'class %s;\n' % self.C (tp)
      s += self.open_namespace (None)
      s += '#define __%s_ifx__(interfacecodeextension)\tinterfacecodeextension\n\n' % self.cppmacro
      s += '#include "%s"\n' % os.path.basename (self.filename_clienthh)
    if self.gen_clientcc and not self.gen_clienthh:
      s += '#include "%s"\n' % os.path.basename (self.filename_serverhh)
    if self.gen_servercc and not self.gen_serverhh:
      s += '#include "%s"\n' % os.path.basename (self.filename_serverhh)
    if self.gen_inclusions:
      s += '\n// --- Custom Includes ---\n'
    if self.gen_inclusions and (self.gen_clientcc or self.gen_servercc):
      s += '#ifndef __AIDA_UTILITIES_HH__\n'
    for i in self.gen_inclusions:
      s += '#include %s\n' % i
    if self.gen_inclusions and (self.gen_clientcc or self.gen_servercc):
      s += '#endif\n'
    s += self.insertion_text ('includes')
    if self.gen_clienthh:
      s += '#include <aidacc/aida.hh>\n'
    if self.gen_servercc:
      s += text_expand (TmplFiles.CxxStub_server_cc) + '\n'
    if self.gen_clientcc:
      s += text_expand (TmplFiles.CxxStub_client_cc) + '\n'
    self.tab_stop (30)
    s += self.open_namespace (None)
    # Generate Enum Declarations
    if self.gen_clienthh:
      s += self.open_namespace (None)
      spc_enums = []
      for tp in types:
        if tp.is_forward:
          continue
        elif tp.storage == Decls.ENUM:
          s += self.open_namespace (tp)
          s += self.generate_enum_decl (tp)
          spc_enums += [ tp ]
      if spc_enums:
        s += self.open_namespace (self.ns_aida)
        for tp in spc_enums:
          s += self.generate_enum_info_specialization (tp)
        s += self.open_namespace (None)
    # generate client/server decls
    if self.gen_clienthh or self.gen_serverhh:
      self.gen_mode = G4SERVANT if self.gen_serverhh else G4STUB
      class_name_list = []
      for tp in types:
        if tp.is_forward:
          s += self.open_namespace (tp) + '\n'
          if tp.storage == Decls.INTERFACE and self.gen_mode == G4SERVANT:
            s += self.generate_interface_pointerdefs (tp)
          else:
            s += 'class %s;\n' % self.C (tp)
        elif self.gen_clienthh and tp.storage in (Decls.RECORD, Decls.SEQUENCE):
          s += self.open_namespace (tp)
          s += self.generate_recseq_decl (tp)
        elif tp.storage == Decls.INTERFACE:
          s += self.open_namespace (tp)
          s += self.generate_interface_class (tp, class_name_list)     # Class remote handle
      # template impls after *all* types are defined
      for tp in types:
        if self.gen_clienthh and not tp.is_forward and tp.storage in (Decls.RECORD,): # Decls.SEQUENCE):
          s += self.open_namespace (tp)
          s += self.generate_recseq_visit (tp)
      s += self.open_namespace (None)
      if self.gen_serverhh and class_name_list:
        s += '\n#define %s_INTERFACE_LIST' % self.cppmacro
        for i in class_name_list:
          s += ' \\\n\t  %s_INTERFACE_NAME (%s)' % (self.cppmacro, i)
        s += '\n'
    # generate client/server impls
    if self.gen_clientcc or self.gen_servercc:
      self.gen_mode = G4SERVANT if self.gen_servercc else G4STUB
      s += '\n// --- Implementations ---\n'
      for tp in types:
        if tp.is_forward:
          continue
        if tp.storage == Decls.RECORD and self.gen_clientcc:
          s += self.open_namespace (tp)
          s += self.generate_record_impl (tp)
        elif tp.storage == Decls.SEQUENCE and self.gen_clientcc:
          s += self.open_namespace (tp)
          s += self.generate_sequence_impl (tp)
        elif tp.storage == Decls.INTERFACE:
          if self.gen_servercc:
            s += self.open_namespace (tp)
            s += self.generate_server_class_methods (tp)
            s += self.generate_server_class_aux_method_impls (tp)
            s += self.generate_server_class_any_method_impls (tp)
          if self.gen_clientcc:
            s += self.open_namespace (tp)
            s += self.generate_client_class_methods (tp)
            for fl in tp.fields:
              s += self.generate_client_property_stub (tp, fl[0], fl[1])
            for m in tp.methods:
              s += self.generate_client_method_stub (tp, m)
    # Generate Enum Implementations
    if self.gen_clientcc or self.gen_servercc:
      spc_enums = []
      for tp in types:
        if tp.is_forward:
          continue
        elif tp.storage == Decls.ENUM:
          spc_enums += [ tp ]
      if spc_enums:
        s += self.open_namespace (None)
        s += '\n'
        if self.gen_servercc:
          s += '#ifndef __ENUMCC__%s__\n' % self.cppmacro # pick default implementation
          s += '#define __ENUMCC__%s__    %d\n' % (self.cppmacro, clntsrvt_id)
          s += '#endif\n'
        s += '#if     __ENUMCC__%s__ == %d\n' % (self.cppmacro, clntsrvt_id) # pick either client or server aliases
        s += self.open_namespace (self.ns_aida)
        for tp in spc_enums:
          s += self.generate_enum_info_impl (tp)
        s += self.open_namespace (None)
        s += '\n#endif // __ENUMCC__%s__\n\n' % self.cppmacro
    # generate unmarshalling server calls
    if self.gen_servercc:
      self.gen_mode = G4SERVANT
      s += '\n// --- Method Dispatchers & Registry ---\n'
      reglines = []
      for tp in types:
        if tp.is_forward:
          continue
        s += self.open_namespace (tp)
        if tp.storage == Decls.INTERFACE:
          for fl in tp.fields:
            s += self.generate_server_property_getter (tp, fl[0], fl[1], reglines)
            s += self.generate_server_property_setter (tp, fl[0], fl[1], reglines)
          for m in tp.methods:
            s += self.generate_server_method_stub (tp, m, reglines)
          s += '\n'
      s += self.open_namespace (None)
      s += self.generate_server_method_registry (reglines) + '\n'
    s += self.open_namespace (None) # close all namespaces
    # Aida IDs
    if self.gen_aidaids and (self.gen_servercc or self.gen_clientcc):
      s += self.generate_aida_ids ()
    # CPP guard
    if self.gen_serverhh or self.gen_clienthh:
      s += '\n#endif /* %s */\n' % (sc_macro_prefix + self.cppmacro)
    return s
  def generate_aida_ids (self):
    s, nslist = '', []
    nslist += [ Decls.Namespace ('Aida', None, []) ]
    iface = Decls.TypeInfo ('ImplicitBase', Decls.INTERFACE, False)
    nslist[-1].add_type (iface) # iface.full_name() == Aida::ImplicitBase
    import IdStrings
    identifiers = IdStrings.id_dict
    for k,v in identifiers.items():
      IDENT, digest = k.upper(), self.internal_digest (iface, v)
      s += 'static_assert (Aida::TypeHash { AIDA_HASH_%s } ==\n' % IDENT
      s += '               Aida::TypeHash { %s },\n' % digest
      s += '               "Expecting hash defined as:\\n#define AIDA_HASH_%s \t%s");\n' % (IDENT, digest)
    return s

def error (msg):
  import sys
  print >>sys.stderr, sys.argv[0] + ":", msg
  sys.exit (127)

def generate (namespace_list, **args):
  import sys, tempfile, os
  global I_prefix_postfix
  config = {}
  config.update (args)
  idlfiles = config['files']
  if len (idlfiles) != 1:
    raise RuntimeError ("CxxStub: exactly one IDL input file is required")
  outdir = config.get ('output', '')
  gg = Generator (idlfiles[0])
  gg.gen_aidaids = True
  gg.gen_inclusions = config['inclusions']
  for opt in config['backend-options']:
    if opt == 'docs':
      gg.gen_docs = True
    if opt.startswith ('macro='):
      gg.cppmacro = opt[6:]
    if opt.startswith ('strip-path='):
      gg.strip_path += opt[11:]
    if opt.startswith ('iface-postfix='):
      I_prefix_postfix = (I_prefix_postfix[0], opt[14:])
    if opt.startswith ('iface-prefix='):
      I_prefix_postfix = (opt[13:], I_prefix_postfix[1])
    if opt.startswith ('property-list=') and opt[14:].lower() in ('0', 'no', 'none', 'false'):
      gg.property_list = ""
  for ifile in config['insertions']:
    gg.insertion_file (ifile)
  gg.ns_aida = ( Decls.Namespace ('Aida', None, []), )  # Aida namespace tuple for open_namespace()
  base_filename = gg.idl_file[:gg.idl_file.rfind ('.')] # strip extension
  if outdir:
    base_filename = os.path.join (outdir, os.path.basename (base_filename))
  gg.filename_serverhh = base_filename + '_interfaces.hh'
  gg.filename_servercc = base_filename + '_interfaces.cc'
  gg.filename_clienthh = base_filename + '_handles.hh'
  gg.filename_clientcc = base_filename + '_handles.cc'
  for i in range (1, 5):
    gg.gen_serverhh = i == 1
    gg.gen_servercc = i == 2
    gg.gen_clienthh = i == 3
    gg.gen_clientcc = i == 4
    destfilename = [ gg.filename_serverhh, gg.filename_servercc, gg.filename_clienthh, gg.filename_clientcc ][i-1]
    textstring = gg.generate_impl_types (config['implementation_types']) # namespace_list
    fout = open (destfilename, 'w')
    fout.write (textstring)
    fout.close()

# register extension hooks
__Aida__.add_backend (__file__, generate, __doc__)
__Aida__.set_default_backend (__file__)
