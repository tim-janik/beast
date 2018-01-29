# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"""AidaExtractDocs - Aida IDL Documentation Generator

More details at https://rapicorn.testbit.org/
"""
import Decls

class Generator:
  def __init__ (self, idl_file):
    self.idl_file = idl_file
  def fullname (self, type_node):
    tnsl = type_node.list_namespaces() # type namespace list
    namespace_names = [d.name for d in tnsl if d.name]
    return '::'.join (namespace_names + [ type_node.name ])
  def strip_c_quotes (self, string):
    string = string.strip()
    if len (string) < 1:
      return ''
    if string[0] == '_':                        # strip first part of _("...")
      string = string[1:]
    string = string.strip()
    if string[0] == '(' and string[-1] == ')':  # remove parenthesis
      string = string[1:-1]
    if string[0] == '"' and string[-1] == '"':
      import rfc822
      string = rfc822.unquote (string)          # proper C string unquoting
    return string
  def generate_interface_docs (self, class_type):
    s, full_class_name = '', self.fullname (class_type)
    if hasattr (class_type, 'blurb'):
      s += '/** @class ' + full_class_name + '\n'
      s += class_type.blurb
      s += ' */\n'
    s += '/**  @class ' + full_class_name + 'Handle\n'
    s += ' * @extends ' + full_class_name + '\n'
    s += ' */\n'
    s += '/**  @class ' + full_class_name + 'Iface\n'
    s += ' * @extends ' + full_class_name + '\n'
    s += ' */\n'
    return s
  def generate_property_docs (self, class_type, ident, ftype, blurb):
    s = ''
    s += '/** @property ' + self.fullname (class_type) + '::' + ident + '\n'
    s += self.strip_c_quotes (blurb) + '\n'
    s += ' */\n'
    return s
  def generate_docs (self, implementation_types):
    s = '// --- Documentation from %s ---\n' % self.idl_file
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl and not tp.is_forward:
        types += [ tp ]
    # walk types and extract docs
    for tp in types:
      if tp.storage == Decls.INTERFACE:
        s += '\n'
        s += self.generate_interface_docs (tp)
      if hasattr (tp, 'fields') and tp.fields:
        s += '\n'
        for field in tp.fields:
          ident, ftype = field
          blurb = ftype.auxdata.get ('blurb', None)
          if blurb:
            s += self.generate_property_docs (tp, ident, ftype, blurb)
    s += '\n'
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
    raise RuntimeError ("AidaExtractDocs: exactly one IDL input file is required")
  gg = Generator (idlfiles[0])
  all = config['backend-options'] == []
  ns_rapicorn = Decls.Namespace ('Rapicorn', None, [])
  textstring = gg.generate_docs (config['implementation_types']) # namespace_list
  outname = config.get ('output', '-')
  if outname != '-':
    fout = open (outname, 'w')
    fout.write (textstring)
    fout.close()
  else:
    print textstring,

# register extension hooks
__Aida__.add_backend (__file__, generate, __doc__)
