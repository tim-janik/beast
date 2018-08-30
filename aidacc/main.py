#!/usr/bin/env python2
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
import sys, os, re
pkginstall_configvars = {
  'AIDA_VERSION' : '0.0-uninstalled',
  'aidaccpydir'  : '.',
  #@PKGINSTALL_CONFIGVARS_IN24LINES@ # configvars are substituted upon script installation
}
modpath = pkginstall_configvars["aidaccpydir"]
modpath = modpath if os.path.isabs (modpath) else os.path.normpath (os.path.join (os.path.dirname (os.path.abspath (__file__)), modpath))
sys.path.insert (0, os.environ.get ('AIDACC_DESTDIR','') + modpath)
import yapps2runtime as runtime
import Parser, Decls, GenUtils # pre-import modules for Generator modules
true, false, length = (True, False, len)


class AidaMain:
  def __init__ (self):
    self.backends = {}  # { 'TypeMap' : (TypeMap.generate, __doc__) }
    self.default_backend = None
    self.auxillary_initializers = {}
  def strip_name (self, name):
    name = os.path.basename (name)   # strip all directories
    name = re.sub ('\..*$', '', name) # strip all extensions
    return name
  def add_auxillary_initializers (self, newdict):
    self.auxillary_initializers.update (newdict)
  def add_backend (self, bname, bgenerate, doc):
    assert callable (bgenerate)
    name = self.strip_name (bname)
    assert len (name)
    self.backends[name] = (bgenerate, doc)
  def set_default_backend (self, bname):
    name = self.strip_name (bname)
    assert self.backends[name]
    self.default_backend = name
  def list_backends (self):
    return self.backends.keys()
  def get_backend (self):
    if self.default_backend:
      return self.backends[self.default_backend]
    elif len (self.backends) == 1:
      return self.backends.values()[0]
    else:
      return None
__builtins__.__Aida__ = AidaMain() # used by extensions

def parse_main (config, filepairs):
  impltypes, error, caret, inclist = Parser.parse_files (config, filepairs)
  nsdict = {}
  nslist = []
  if impltypes:
    for type in impltypes:
      if not nsdict.get (type.namespace, None):
        nsdict[type.namespace] = 1
        nslist += [ type.namespace ]
  return (nslist, impltypes, error, caret, inclist)

def module_import (module_or_file):
  apath = os.path.abspath (module_or_file)
  module_dir, module_file = os.path.split (apath)
  module_name, module_ext = os.path.splitext (module_file)
  savedpath = sys.path
  sys.path = [ module_dir ] + savedpath
  try:
    module_obj = __import__ (module_name)
  except:
    sys.path = savedpath
    raise
  sys.path = savedpath
  return module_obj

def main():
  # parse args and file names
  config = parse_files_and_args()
  # parse IDL files
  files = config['files']
  if len (files) == 0: # interactive
    try:
      input_string = raw_input ('IDL> ')
    except EOFError:
      input_string = ""
    filename = '<stdin>'
    print
    nslist, impltypes, error, caret, inclist = parse_main (config, [ (filename, input_string) ])
  else: # file IO
    error = None
    filepairs = []
    for fname in files:
      f = open (fname, 'r')
      filepairs += [ (fname, f.read()) ]
    nslist, impltypes, error, caret, inclist = parse_main (config, filepairs)
  # display parsing errors
  if error:
    print >>sys.stderr, error
    if caret:
      print >>sys.stderr, caret
    for ix in inclist:
      print >>sys.stderr, ix
    sys.exit (7)
  # call backend generation
  backend = __Aida__.get_backend()
  if backend:
    generate = backend[0]
    generate (nslist, implementation_types = impltypes, **config) # generate (namespace_list, implementation_types, configs...)

def print_help (with_help = True):
  print "aidacc version", pkginstall_configvars["AIDA_VERSION"]
  if not with_help:
    return
  print "Usage: %s [options] idlfiles..." % os.path.basename (sys.argv[0])
  print "       %s [solitary-option]" % os.path.basename (sys.argv[0])
  print "Options:"
  print "  --help, -h                print this help message"
  print "  --version, -v             print version info"
  print "  -I <directory>            add include directory"
  print "  -o <outputfile>           output filename"
  print "  -x <MODULE>               load extension MODULE"
  print "  -G <generator-option>     set generator backend option"
  print "  --insertions=<insfile>    file for insertion points"
  print '  --inclusions=<"include">  include statements'
  print '  --skip-skels=<symfile>    symbols to skip skeletons for'
  print "Solitary Options:"
  print "  --list-formats            list output formats"

def parse_files_and_args():
  import re, getopt
  config = { 'files' : [], 'backend' : 'PrettyDump', 'backend-options' : [], 'includedirs' : [], 'color': True,
             'insertions' : [], 'inclusions' : [], 'skip-skels' : [], 'system-typedefs' : False, 'bse-extensions' : False }
  sop = 'vhG:g:o:I:x:'
  lop = ['help', 'version', 'list-formats',
         'aida-debug', 'cc-intern-file=', 'bse-extensions',
         'insertions=', 'inclusions=', 'skip-skels=']
  if pkginstall_configvars.get ('INTERN', 0):
    lop += [ 'system-typedefs' ]
  try:
    options,args = getopt.gnu_getopt (sys.argv[1:], sop, lop)
  except Exception, ex:
    print >>sys.stderr, sys.argv[0] + ":", str (ex)
    print_help(); sys.exit (1)
  for arg,val in options:
    if arg == '-h' or arg == '--help': print_help(); sys.exit (0)
    if arg == '-v' or arg == '--version': print_help (false); sys.exit (0)
    if arg == '--aida-debug': config['pass-exceptions'] = 1
    if arg == '-o': config['output'] = val
    if arg == '-I': config['includedirs'] += [ val ]
    if arg == '--insertions': config['insertions'] += [ val ]
    if arg == '--inclusions': config['inclusions'] += [ val ]
    if arg == '--skip-skels': config['skip-skels'] += [ val ]
    if arg == '-G': config['backend-options'] += [ val ]
    if arg == '-x': module_import (val)
    if arg == '--bse-extensions': config['bse-extensions'] = True
    if arg == '--cc-intern-file':
      import TypeMap
      data = open (val).read()
      s = TypeMap.cquote (data)
      print '// This file is generated by aidacc. DO NOT EDIT.'
      print 'static const char intern_%s[] =' % re.sub (r'[^a-zA-Z0-9_]', '_', val)
      print '  ' + re.sub ('\n', '\n  ', s) + ";"
      sys.exit (0)
    if arg == '--list-formats':
      print "\nAvailable Output Formats:"
      b = __Aida__.list_backends()
      for bname in b:
        bedoc = __Aida__.backends[bname][1]
        bedoc = re.sub ('\n\s*\n', '\n', bedoc)                         # remove empty lines
        bedoc = re.compile (r'^', re.MULTILINE).sub ('    ', bedoc)     # indent
        print "  %s" % bname
        if bedoc:
          print bedoc.rstrip()
      print
      sys.exit (0)
    if arg == '--system-typedefs':
      config['system-typedefs'] = True
  config['files'] += list (args)
  return config

failtestoption = '--aida-fail-file-test'
if len (sys.argv) > 2 and failtestoption in sys.argv:
  import tempfile, os
  sys.argv.remove (failtestoption) # remove --aida-fail-file-test
  config = parse_files_and_args()
  config['anonymize-filepaths'] = true # anonymize paths for varying builddirs (../a/b/c.idl -> .../c.idl)
  config['color'] = false
  files = config['files']
  if len (files) != 1:
    raise Exception (failtestoption + ': single input file required')
  infile = open (files[0], 'r')
  n = 0
  for line in infile:
    n += 1
    ls = line.strip()
    if ls and not ls.startswith ('//'):
      filename = infile.name
      Parser.yy.reset()
      if line.startswith ("include"):
        code = '\n' * (n-1) + line
      else:
        code = '\n' * (n-2) + 'namespace AidaFailTest {\n' + line + '\n}'
      nslist, impltypes, error, caret, inclist = parse_main (config, [ (filename, code) ])
      if error:
        import re
        error = re.sub (r'^[^:]*/([^/:]+):([0-9]+):', r'.../\1:\2:', error)
        print error
        if caret:
          print caret
        for ix in inclist:
          print ix
        # expected a failing tests
      else:
        raise Exception (filename + ': uncaught test:', line)
  infile.close()
elif __name__ == '__main__':
  main()
