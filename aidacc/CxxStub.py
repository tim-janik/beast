# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"""AidaCxxStub - Aida C++ Code Generator

More details at https://beast.testbit.org/
"""
import Decls, TmplFiles, re, os

def backslash_quote (string):
  return re.sub (r'(\\|")', r'\\\1', string)
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
    self.foreach_seq = []
    self.gen_mode = None
    self.strip_path = ''
    self.declared_pointerdefs = set() # types for which pointerdefs have been generated
  @staticmethod
  def prefix_namespaced_identifier (prefix, ident, postfix = ''):     # Foo::bar -> Foo::PREFIX_bar_POSTFIX
    cc = ident.rfind ('::')
    if cc >= 0:
      ns, base = ident[:cc+2], ident[cc+2:]
    else:
      ns, base = '', ident
    return ns + prefix + base + postfix
  def Iwrap (self, name):
    return self.prefix_namespaced_identifier ('', name, 'Iface')
  def type_identifier (self, tp):
    # compound types
    if tp.storage in (Decls.ENUM, Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE):
      tnsl = tp.list_namespaces() # type namespace list
      absolute_namespaces = [d.name for d in tnsl if d.name]
      fullnsname = '.'.join (absolute_namespaces + [ tp.name ])
      return fullnsname
    return Decls.storage_name (tp.storage)
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
  def C (self, type_node, mode = None):                 # construct Class name
    mode = mode or self.gen_mode
    return self.C4server (type_node)
  def R (self, type_node):                              # construct Return type
    tname = self.C (type_node)
    if self.gen_mode == G4SERVANT and type_node.storage == Decls.INTERFACE:
      tname += 'P'
    return tname
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
  def generate_recseq_visitors (self, type_info):
    s, classC = '', self.C (type_info)
    if type_info.storage == Decls.RECORD:
      s += 'template<class Visitor> void\n%s::__visit__ (Visitor &&_visitor_)\n{\n' % classC
      for fname, ftype in type_info.fields:
        s += '  std::forward<Visitor> (_visitor_) (%s, "%s");\n' % (fname, fname)
      s += '}\n'
    if type_info.storage == Decls.SEQUENCE:
      s += 'inline\n'
      s += '%s::%s (const Aida::AnySeq &s)\n{\n' % (classC, classC) # ctor
      s += '  for (const auto &any : s)\n'
      s += '    push_back (any.get< typename std::decay<%s::value_type>::type >());\n' % classC
      s += '}\n'
    if type_info.storage == Decls.SEQUENCE and type_info.elements[1].storage != Decls.ANY:
      s += 'inline\n'
      s += '%s::operator Aida::AnySeq () const\n{\n' % classC
      s += '  Aida::AnySeq s;\n'
      s += '  for (const auto &v : *this)\n'
      s += '    s.push_back (Aida::Any (v));\n'
      s += '  return s;\n'
      s += '}\n'
    return s
  def generate_recseq_decl (self, type_info):
    s = '\n'
    classC = self.C (type_info)
    type_identifier = self.type_identifier (type_info)
    # s += self.generate_shortdoc (type_info)   # doxygen IDL snippet
    if type_info.storage == Decls.SEQUENCE:
      self.foreach_seq.append (type_info)
      fl = type_info.elements
      s += 'class ' + classC + ' : public std::vector<' + self.R (fl[1]) + '>\n'
      s += '{\n'
    else:
      s += 'class %s\n' % classC
      s += '{\n'
    s += 'public:\n'
    if type_info.storage == Decls.RECORD:
      fieldlist = type_info.fields
      for fl in fieldlist:
        initializer = ''
        if fl[1].storage in (Decls.BOOL, Decls.INT32, Decls.INT64, Decls.FLOAT64, Decls.ENUM):
          initializer = " = %s" % self.mkzero (fl[1])
        s += '  ' + self.F (self.R (fl[1])) + fl[0] + '%s;\n' % initializer
    elif type_info.storage == Decls.SEQUENCE:
      s += '  typedef std::vector<' + self.R (fl[1]) + '> Sequence;\n'
    if type_info.storage == Decls.SEQUENCE:
      s += '  ' + self.F ('inline') + '%s () = default;\n' % classC # ctor
      s += '  ' + self.F ('inline') + '%s (const Aida::AnySeq &s);\n' % classC # ctor
      s += '  ' + self.F ('explicit') + '%s (std::initializer_list<value_type> il) : Sequence (il) {};\n' % classC
    elif type_info.storage == Decls.RECORD:
      s += '  ' + self.F ('inline') + '%s () = default;\n' % classC # ctor
      s += '  ' + self.F ('inline') + '%s (const Aida::AnyRec &r) { __visit__ ([&r] (auto &v, const char *n) { ' % classC # ctor
      s += 'v = r[n].get< typename std::decay<decltype (v)>::type >(); }); }\n'
    s += '  ' + self.F ('static const Aida::StringVector&') + '__typedata__ ();\n'
    if type_info.storage == Decls.SEQUENCE and type_info.elements[1].storage != Decls.ANY:
      s += '  ' + self.F ('inline operator') + 'Aida::AnySeq      () const;\n'
    if type_info.storage == Decls.RECORD:
      s += '  ' + self.F ('bool') + 'operator==   (const %s &other) const;\n' % classC
      s += '  ' + self.F ('bool') + 'operator!=   (const %s &other) const { return !operator== (other); }\n' % classC
      s += '  ' + self.F ('operator') + 'Aida::AnyRec () const { Aida::AnyRec r; const_cast<%s*> (this)->__visit__ ([&r] (const auto &v, const char *n) { r[n] = v; }); return r; }\n' % classC
      s += '  ' + self.F ('template<class Visitor> void') + '__visit__    (Visitor &&_visitor_);\n'
    if self.gen_mode == G4SERVANT:
      s += self.insertion_text ('interface_scope:' + type_info.name)
    s += '};\n'
    return s
  def generate_aux_data_string (self, tp, fieldname = '', prefix = ''):
    s = ''
    # types
    if not fieldname and not prefix and tp.storage in (Decls.ENUM, Decls.RECORD, Decls.SEQUENCE, Decls.INTERFACE):
      s += '  "typename=%s\\0"\n' % self.type_identifier (tp)
      s += '  "%stype=%s\\0"\n' % (prefix, Decls.storage_name (tp.storage))
    # fields
    if fieldname:
      # s += '  "%sname=%s\\0"\n' % (prefix, fieldname)
      s += '  "%stype=%s\\0"\n' % (prefix, self.type_identifier (tp))
    if not fieldname and tp.storage == Decls.SEQUENCE:
      s += '  "%sfields=%s\\0"\n' % (prefix, "0")
    if not fieldname and tp.storage == Decls.RECORD:
      s += '  "%sfields=' % prefix
      s += ';'.join (fid for (fid, ftp) in tp.fields)
      s += '\\0"\n'
    if not fieldname and tp.storage == Decls.ENUM:
      s += '  "%senumerators=' % prefix
      s += ';'.join (ev[0] for ev in tp.options)
      s += '\\0"\n'
    for k,v in tp.auxdata.items():
      qvalue = backslash_quote (aux_data_value_string (v))
      if qvalue:
        s += '  "%s%s=%s\\0"\n' % (prefix, k, qvalue)
    if not fieldname and tp.storage == Decls.ENUM:
      for ev in tp.options:
        (ident, label, blurb, number) = ev
        s += '  "%s.value=%d\\0"\n' % (ident, number)
        if label:
          #label = label[-1] == ')' and re.sub ('[A-Z]*_\(', '', label[:-1]) or label # strip i18n function wrapper
          s += '  "%s.label=" %s "\\0"\n' % (ident, label)
        if blurb:
          #blurb = blurb[-1] == ')' and re.sub ('[A-Z]*_\(', '', blurb[:-1]) or blurb # strip i18n function wrapper
          s += '  "%s.blurb=" %s "\\0"\n' % (ident, blurb)
    if not fieldname and tp.storage == Decls.SEQUENCE:
      s += self.generate_aux_data_string (tp.elements[1], tp.elements[0], prefix + "0" + '.')
    if not fieldname and tp.storage == Decls.RECORD:
      for fid, ftp in tp.fields:
        s += self.generate_aux_data_string (ftp, fid, prefix + fid + '.')
    return s
  def generate_recseq_aux_method_impls (self, tp): # FIXME: rename
    type_identifier = self.type_identifier (tp)
    s = ''
    # __typedata__
    s += 'const Aida::StringVector&\n%s::__typedata__()\n{\n' % self.C (tp)
    s += '  static const Aida::StringVector &sv = Aida::Introspection::find_type ("%s");\n' % type_identifier
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
    return s
  def generate_sequence_impl (self, type_info):
    s = ''
    s += self.generate_type_aux_data_registration (type_info)
    s += self.generate_recseq_aux_method_impls (type_info)
    return s
  def generate_enum_info_impl (self, type_info):
    type_identifier = self.type_identifier (type_info)
    s = '\n'
    s += self.generate_type_aux_data_registration (type_info)
    return s
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
    s, classC, classFull = '\n', self.C (type_info), self.namespaced_identifier (type_info.name)
    type_identifier = self.type_identifier (type_info)
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
    # constructors
    s += 'protected:\n'
    if self.gen_mode == G4SERVANT:
      s += '  explicit' + self.F (' ') + '%s ();\n' % self.C (type_info) # ctor
      s += '  virtual ' + self.F (' /*dtor*/ ', -1) + '~%s () override = 0;\n' % self.C (type_info)
    s += 'public:\n'
    if self.gen_mode == G4SERVANT:
      s += self.generate_class_any_method_decls (type_info)
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
    if self.gen_mode == G4SERVANT:
      s += self.insertion_text ('interface_scope:' + type_info.name)
    # done
    s += '};\n'
    return s
  def generate_shortdoc (self, type_info):      # doxygen snippets
    classC = self.C (type_info) # class name
    xkind = 'servant' if self.gen_mode == G4SERVANT else 'stub'
    s  = '/** @interface %s\n' % type_info.name
    s += ' * See also the corresponding C++ %s class %s. */\n' % (xkind, classC)
    s += '/// See also the corresponding IDL class %s.\n' % type_info.name
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
  def generate_type_aux_data_registration (self, tp):
    s = ''
    aux_data_string = self.generate_aux_data_string (tp)
    aux_data_string = aux_data_string if aux_data_string else '  ""'
    absolute_typename = self.type2cpp_absolute (tp)
    unique_type_identifier = re.sub ('::', '_', absolute_typename)
    s += 'static const Aida::IntrospectionRegistry __aida__aux__data__%s_ = {\n' % unique_type_identifier
    s += aux_data_string
    s += '};\n'
    return s
  def generate_class_any_method_decls (self, tp):
    assert self.gen_mode == G4SERVANT
    s = ''
    virtual = 'virtual '
    if len (tp.fields) > 0:
      s += '  %-37s __access__         (const std::string &propertyname, const PropertyAccessorPred&) override;\n' % (virtual + 'bool')
    return s
  def generate_server_class_any_method_impls (self, tp):
    assert self.gen_mode == G4SERVANT
    s, classH = '', self.C (tp)
    reduced_immediate_ancestors = self.interface_class_ancestors (tp)
    # __access__
    if len (tp.fields) > 0:
      s += 'bool\n%s::__access__ (const std::string &__n, const PropertyAccessorPred &__p)\n{\n' % classH
      a  = '  const bool __all = __n.empty();\n'
      for fname, ftype in tp.fields:
        if a:
          s, a = s + a, ''
        faux = self.generate_aux_data_string (ftype, fname)
        s += '  const char *const __aux__%s =\n    %s;\n' % (fname, faux.strip().replace ('\n', '\n  '))
        ctype = self.C (ftype) # self.type2cpp_relative (ftype)
        s += '  if ((__all || __n == "%s") &&\n      __p (' % fname
        s += 'Aida::PropertyAccessorImpl<%s,%s> ("%s", *this, &%s::%s, &%s::%s, __aux__%s)' % (classH, ctype, fname, classH, fname, classH, fname, fname)
        s += '))\n    return true;\n'
      for atp in reduced_immediate_ancestors:
        s += '  if (this->%s::__access__ (__n, __p)) return true;\n' % self.C (atp)
      s += '  return false;\n'
      s += '}\n'
    return s
  def generate_server_class_methods (self, tp):
    assert self.gen_mode == G4SERVANT
    s, classC = '\n', self.C (tp)
    s += '%s::%s ()' % (classC, classC) # ctor
    s += '\n{}\n'
    s += '%s::~%s ()\n{} // define empty dtor to emit vtable\n' % (classC, classC) # dtor
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
  def c_long_postfix (self, number):
    if number >= 9223372036854775808:
      return str (number) + 'u'
    if number == -9223372036854775808:    # in C++ parsed as two tokens '-' '9223372036854775808'
      return '(-9223372036854775807 - 1)' # avoid "integer constant is so large that it is unsigned"
    return str (number)
  def generate_enum_decl (self, type_info):
    s = '\n'
    nm, type_identifier = type_info.name, self.type_identifier (type_info)
    l = []
    s += 'enum class %s : int64_t {\n' % type_info.name
    for opt in type_info.options:
      (ident, label, blurb, number) = opt
      s += '  %s = %s,' % (ident, self.c_long_postfix (number))
      if blurb:
        s += ' // %s' % re.sub ('\n', ' ', blurb)
      s += '\n'
    s += '};\n'
    s += 'AIDA_DEFINE_ENUM_EQUALITY (%s);\n' % nm
    s += 'inline std::string to_string   (%s ev)                         { return Aida::enum_value_to_string (ev); }\n' % nm
    s += 'inline bool        from_string (const std::string &en, %s &ev) { ev = Aida::enum_value_from_string<%s> (en); return true; }\n' % (nm, nm)
    if type_info.combinable: # enum as flags
      s += 'AIDA_DEFINE_FLAGS_ARITHMETIC (%s);\n' % nm
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
  def generate_impl_types (self, implementation_types):
    def text_expand (txt):
      txt = txt.replace ('$AIDA_iface_base$', self.iface_base)
      return txt
    self.gen_mode = G4SERVANT # self.gen_serverhh or self.gen_servercc
    if self.cppmacro == None:
      self.cppmacro = re.sub (r'(^[^A-Z_a-z])|([^A-Z_a-z0-9])', '_', self.idl_path())
    s = '// --- Generated by AidaCxxStub ---\n'
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # CPP guards
    sc_macro_prefix = '__SRVT__'
    if self.gen_serverhh:
      s += '#ifndef %s\n#define %s\n\n' % (sc_macro_prefix + self.cppmacro, sc_macro_prefix + self.cppmacro)
    if self.gen_serverhh:
      s += '#ifndef DOXYGEN\n'
    # inclusions
    if self.gen_serverhh:
      for tp in types:  # we need to pre-declare classes for Handle<->Iface conversions
        if not tp.is_forward and tp.storage == Decls.INTERFACE:
          s += self.open_namespace (tp)
          s += 'class %s;\n' % self.C (tp)
      s += self.open_namespace (None)
      s += '#define __%s_ifx__(interfacecodeextension)\tinterfacecodeextension\n\n' % self.cppmacro
    if self.gen_servercc and not self.gen_serverhh:
      s += '#include "%s"\n' % os.path.basename (self.filename_serverhh)
    if self.gen_inclusions:
      s += '\n// --- Custom Includes ---\n'
    if self.gen_inclusions and self.gen_servercc:
      s += '#ifndef __AIDA_UTILITIES_HH__\n'
    for i in self.gen_inclusions:
      s += '#include %s\n' % i
    if self.gen_inclusions and self.gen_servercc:
      s += '#endif\n'
    s += self.insertion_text ('includes')
    if self.gen_serverhh:
      s += '#include <aidacc/aida.hh>\n'
    if self.gen_servercc:
      s += text_expand (TmplFiles.CxxStub_server_cc) + '\n'
    self.tab_stop (30)
    s += self.open_namespace (None)
    # Generate Enum Declarations
    if self.gen_serverhh:
      s += self.open_namespace (None)
      spc_enums = []
      for tp in types:
        if tp.is_forward:
          continue
        elif tp.storage == Decls.ENUM:
          s += self.open_namespace (tp)
          s += self.generate_enum_decl (tp)
          spc_enums += [ tp ]
    # generate server decls
    if self.gen_serverhh:
      self.gen_mode = G4SERVANT # self.gen_serverhh
      class_name_list = []
      for tp in types:
        if tp.is_forward:
          s += self.open_namespace (tp) + '\n'
          if tp.storage == Decls.INTERFACE and self.gen_mode == G4SERVANT:
            s += self.generate_interface_pointerdefs (tp)
          else:
            s += 'class %s;\n' % self.C (tp)
        elif self.gen_serverhh and tp.storage in (Decls.RECORD, Decls.SEQUENCE):
          s += self.open_namespace (tp)
          s += self.generate_recseq_decl (tp)
        elif tp.storage == Decls.INTERFACE:
          s += self.open_namespace (tp)
          s += self.generate_interface_class (tp, class_name_list)     # Class remote handle
      # template impls after *all* types are defined
      for tp in types:
        if self.gen_serverhh and not tp.is_forward and tp.storage in (Decls.RECORD, Decls.SEQUENCE):
          s += self.open_namespace (tp)
          s += self.generate_recseq_visitors (tp)
      s += self.open_namespace (None)
      if self.gen_serverhh and class_name_list:
        s += '\n#define %s_INTERFACE_LIST' % self.cppmacro
        for i in class_name_list:
          s += ' \\\n\t  %s_INTERFACE_NAME (%s)' % (self.cppmacro, i)
        s += '\n'
    # generate server impls
    if self.gen_servercc:
      self.gen_mode = G4SERVANT # self.gen_servercc
      s += '\n// --- Implementations ---\n'
      for tp in types:
        if tp.is_forward:
          continue
        if tp.storage == Decls.RECORD and self.gen_servercc:
          s += self.open_namespace (tp)
          s += self.generate_record_impl (tp)
        elif tp.storage == Decls.SEQUENCE and self.gen_servercc:
          s += self.open_namespace (tp)
          s += self.generate_sequence_impl (tp)
        elif tp.storage == Decls.INTERFACE:
          if self.gen_servercc:
            s += self.open_namespace (tp)
            s += self.generate_server_class_methods (tp)
            s += self.generate_server_class_any_method_impls (tp)
    # Generate Enum Implementations
    if self.gen_servercc:
      spc_enums = []
      for tp in types:
        if tp.is_forward:
          continue
        elif tp.storage == Decls.ENUM:
          spc_enums += [ tp ]
      if spc_enums:
        s += self.open_namespace (None)
        s += '\n'
        s += self.open_namespace (self.ns_aida)
        for tp in spc_enums:
          s += self.generate_enum_info_impl (tp)
    s += self.open_namespace (None) # close all namespaces
    # foreach sequence macro
    if self.gen_serverhh:
      s += '#define %s_FOREACH_IFACE_SEQ() \\\n' % self.cppmacro
      for tp in self.foreach_seq:
        if tp.elements[1].storage != Decls.INTERFACE:
          continue
        classC = self.C (tp)
        s += '  %s_FOREACH_STEP (%s) \\\n' % (self.cppmacro, classC)
      s += '\n'
    # DOXYGEN
    if self.gen_serverhh:
      s += '\n#else // DOXYGEN\n'
      for tp in types:
        if not tp.is_forward and tp.storage == Decls.INTERFACE:
          s += self.open_namespace (tp)
          s += '\n'
          s += '/// IDL interface class for %s\n' % self.type2cpp_absolute (tp)
          s += '/// @extends %s\n' % self.type2cpp_absolute (tp)
          s += 'class %s {};\n' % self.C (tp)
          s += '\n'
          s += '/// @class %sImpl\n' % self.type2cpp_absolute (tp)
          s += '/// @implements %s\n' % self.type2cpp_absolute (tp)
      s += '\n'
      s += self.open_namespace (None) # close all namespaces
      s += '#endif // DOXYGEN\n'
    # CPP guard
    if self.gen_serverhh:
      s += '\n#endif /* %s */\n' % (sc_macro_prefix + self.cppmacro)
    return s

def generate (namespace_list, **args):
  import os
  config = {}
  config.update (args)
  idlfiles = config['files']
  if len (idlfiles) != 1:
    raise RuntimeError ("CxxStub: exactly one IDL input file is required")
  outdir = config.get ('output', '')
  gg = Generator (idlfiles[0])
  gg.gen_inclusions = config['inclusions']
  for opt in config['backend-options']:
    if opt == 'docs':
      gg.gen_docs = True
    if opt.startswith ('macro='):
      gg.cppmacro = opt[6:]
    if opt.startswith ('strip-path='):
      gg.strip_path += opt[11:]
  for ifile in config['insertions']:
    gg.insertion_file (ifile)
  gg.ns_aida = ( Decls.Namespace ('Aida', None, []), )  # Aida namespace tuple for open_namespace()
  base_filename = gg.idl_file[:gg.idl_file.rfind ('.')] # strip extension
  if outdir:
    base_filename = os.path.join (outdir, os.path.basename (base_filename))
  gg.filename_serverhh = base_filename + '_interfaces.hh'
  gg.filename_servercc = base_filename + '_interfaces.cc'
  for i in range (1, 3):
    gg.gen_serverhh = i == 1
    gg.gen_servercc = i == 2
    destfilename = [ gg.filename_serverhh, gg.filename_servercc ][i-1]
    textstring = gg.generate_impl_types (config['implementation_types']) # namespace_list
    fout = open (destfilename, 'w')
    fout.write (textstring)
    fout.close()

# register extension hooks
__Aida__.add_backend (__file__, generate, __doc__)
__Aida__.set_default_backend (__file__)
