#!/usr/bin/env python
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

import os, sys, re, shutil
assert sys.hexversion >= 0x02040000
# determine installation directory
doxer_dir = os.path.dirname (os.path.abspath (os.path.realpath (sys.argv[0])))
sys.path.append (doxer_dir)     # allow loading of modules from installation dir
# imports
import Config, Utils, Data, DoxiParser, ScadParser, Code2Doxi, HtmlGenerator, ManGenerator, qdoxygen, qxmlparser
def debug (*args): Config.debug_print (*args)

# --- handle abortion ---
def abort (reason = '', status = -1):
  if reason:
    reason += ': '
  if sys.exc_type:
    if Config.print_traceback:
      import traceback
      # traceback.print_tb (sys.exc_traceback, file = sys.stderr)
      etb = list (traceback.extract_tb (sys.exc_traceback))
      etb = etb[-10:]
      # etb.reverse()
      fn = len (etb)
      print >>sys.stderr, "  %s(): %s" % etb[0][2:]
      for frame in etb:
        fn -= 1
        print >>sys.stderr, "  #%u" % fn, "%s:%u: %s()" % frame[0:3]
    extype = sys.exc_type.__name__
    exargs = sys.exc_value.__str__()
    if exargs:
      extype += ': '
    print >>sys.stderr, "%s: %s%s%s" % (sys.argv[0], reason, extype, exargs)
    sys.exit (status)
  print >>sys.stderr, "%s: %s" % (sys.argv[0], reason)
  sys.exit (status)

# --- progress indication ---
def progress (*args):
  line = ""
  for s in args:
    if line:
      line += " "
    line += str (s)
  if Config.debug:
    sys.stderr.write (line + "...\n")

def string_from_files (list, joiner = ' '):
  line = ''
  for i in list:
    if line:
      line += joiner
    line += os.path.basename (i)
  return line

def scad2sfiles (source_files, docu_files):
  progress ('Parsing sources: %s' % string_from_files (source_files + docu_files))
  # parse input files
  ifiles = source_files + docu_files
  sfiles = []
  for ifile in ifiles:
    if not os.path.isabs (ifile):
      ifile = os.path.normpath (os.path.join (Config.srcdir, ifile))
    new_sfiles = ScadParser.parse_file (ifile)
    for sf in new_sfiles:
      sf.preserve_extension = False
    sfiles += new_sfiles
  # return parsed source file list
  return sfiles

def sources2sfiles (source_files, docu_files):
  progress ('Parsing sources: %s' % string_from_files (source_files + docu_files))
  # canonify input files
  ifilesets = ((set(), source_files), (set(), docu_files))
  for ifs in ifilesets:
    iset = ifs[0]
    for ifile in ifs[1]:
      if not os.path.isabs (ifile):
        ifile = os.path.normpath (os.path.join (Config.srcdir, ifile))
      ifile = os.path.realpath (ifile)
      ifile = os.path.abspath (ifile)
      ifile = os.path.normpath (ifile)
      iset.add (ifile)
  ifileset1 = ifilesets[0][0]
  ifileset2 = ifilesets[1][0]
  # generate XML docs via doxygen
  tmpxmldir = qdoxygen.sources2doxygenxml (list (ifileset1) + list (ifileset2))
  # parse (XML) input files
  sfiles, doc_queue = qxmlparser.parse_tree (tmpxmldir)
  # filter extra sfiles
  all_srcfiles = sfiles
  sfiles = []
  for sfile in all_srcfiles:
    if sfile.name in ifileset1:
      sfiles += [ sfile ]
    elif sfile.name in ifileset2:
      pass
    else:
      raise RuntimeError ('Failed to identify source file: %s' % sfile.name)
  # remove XML dir
  if tmpxmldir and not Config.debug:
    shutil.rmtree (tmpxmldir)
  # return parsed source file list
  return sfiles

def sfiles2pickle (sfiles):
  import cPickle
  # check target
  if not Config.output_file:
    raise RuntimeError ('No output file specified for pickled sources')
  # create pickled source list
  ofile = Config.output_file
  if not os.path.isabs (ofile):
    ofile = os.path.join (Config.destdir, ofile)
  debug ('Generating %s...' % ofile)
  fout = open (ofile, 'w')
  cPickle.dump (sfiles, fout)
  fout.close()

def pickle2sfiles (pickle_file):
  import cPickle
  # read pickled sources
  ifile = pickle_file
  if not os.path.isabs (ifile):
    ifile = os.path.join (Config.srcdir, ifile)
  debug ('Loading %s...' % ifile)
  fin = open (ifile, 'r')
  pickled_sources = cPickle.load (fin)
  fin.close()
  # check pickled sources
  list_type = type ([])
  assert type (pickled_sources) == list_type
  for pickled_source_file in pickled_sources:
    assert isinstance (pickled_source_file, Data.SrcFile)
  # return validated sources
  return pickled_sources

def sfiles2doxi (sfiles):
  # create targets
  dwriter = Code2Doxi.DoxiWriter ()
  top_webdir = '.' # Config.destdir
  # build up index contents
  for sfile in sfiles:
    if not sfile.doxi_file:
      obase = os.path.basename (sfile.name)
      preserved_obase = obase
      extmatch = re.match (r'(.+)\.([0-9][A-Za-z0-9]*)\.scad$', obase)
      if extmatch:
        mansuffix = extmatch.expand (r'\2')
        trunk = extmatch.expand (r'\1')
        obase = trunk + '.' + mansuffix
        defs.update ({ 'man-section' : mansuffix })
      else:
        if Config.manpage:
          raise RuntimeError ('Failed to detect doxi manual page suffix: %s' % obase)
        extmatch = re.match (r'(.+)\.[^.]+$', obase)
        if extmatch:
          trunk = extmatch.expand (r'\1')
          obase = trunk
        else:
          trunk = obase
      if sfile.preserve_extension:
        obase = preserved_obase
      if Config.doxiext[0:1] == '.':
        obased = obase + Config.doxiext
      else:
        obased = obase + '.' + Config.doxiext
      sfile.doxi_file = obased
      if Config.htmlext[0:1] == '.':
        obaseh = obase + Config.htmlext
      else:
        obaseh = obase + '.' + Config.htmlext
      sfile.html_file = obaseh
    dwriter.index_add_file (sfile, sfile.html_file)
  # canonify template filename
  ifile = Config.template;
  if not os.path.isabs (ifile):
    ifile = os.path.normpath (os.path.join (Config.srcdir, ifile))
  # create .doxi file per source file
  for sfile in sfiles:
    def custom_lookup (keyword):
      ele = dwriter.index_lookup (keyword, top_webdir)
      if ele:
        return ele[1]
      return None
    ofile = os.path.join (Config.destdir, sfile.doxi_file)
    debug ('Generating:', ofile, "from:", ifile)
    fout = open (ofile, 'w')
    fin = open (ifile, 'r')
    def filler (fout, template_name, arg):
      dwriter.write_file (sfile, top_webdir, custom_lookup, fout)
    Utils.template_copy (fin, fout, 'doxer_template_hook', filler, Config.defines, 'doxer_template_get')
    fin.close()
    fout.close()
  # write index page
  if Config.with_index:
    # make sure this is the last page written, so it also serves as a stamp file
    doxi_file = 'index'
    if Config.doxiext[0:1] == '.':
      doxi_file += Config.doxiext
    else:
      doxi_file += '.' + Config.doxiext
    ofile = os.path.join (Config.destdir, doxi_file)
    debug ('Generating %s...' % ofile)
    fout = open (ofile, 'w')
    fin = open (ifile, 'r')
    def filler (fout, template_name, arg):
      dwriter.write_index_page (doxi_file, top_webdir, fout)
    Utils.template_copy (fin, fout, 'doxer_template_hook', filler, Config.defines, 'doxer_template_get')
    fin.close()
    fout.close()

def main():
  Config.VERSION = Config.CONFIG_VERSION
  # setup default config
  cwdir = os.getcwd();
  config_defaults = {
    "doc_root"          : "/ptmp/beast-scratch/docs/doxygen/doxml/",
    "srcdir"            : cwdir,
    "destdir"           : cwdir,
    "tmpdir"            : Config.TMP_DIR + '/doxer-%06x.tmp' % os.getpid(),
    "installdir"        : doxer_dir,
    "template"          : os.path.join (doxer_dir, "srctemplate.doxi"),
    "with_index"        : False,
    "htmlext"           : '.html',
    "doxiext"           : '.doxi',
    "with_doxer_styles" : True,
    "manpage"           : False,
    "includes"          : [],
    "defines"           : {},
  }
  Config.update (config_defaults)
  orig_template = Config.template
  # parse args
  try:
    new_config = parse_command_and_args ()
  except SystemExit, arg:
    sys.exit (arg)
  except Exception, ex:
    if Config.debug:
      raise
    abort ("Failed to parse arguments")
  oldincs = Config.includes
  newdefs = Config.defines
  Config.update (new_config)
  if oldincs != Config.includes:
    Config.update ({ 'includes' : oldincs + Config.includes })
  newdefs.update (Config.defines)
  Config.update ({ 'defines'           : newdefs,
                   'with_doxer_styles' : Config.template == orig_template })
  # create and cd into a dedicated temporary directory
  try:
    os.mkdir (Config.tmpdir, 0700)
    os.chdir (Config.tmpdir)
  except:
    abort ('Failed to create temporary directory: %s' % Config.tmpdir)
  debug ('Entering temporary directory: %s' % Config.tmpdir)
  def write_css_to_destdir ():
    import shutil
    cssfile = os.path.join (Config.destdir, 'doxer-style.css')
    srccss = os.path.join (Config.installdir, 'doxer-style.css')
    debug ('Generating:', cssfile, "from:", srccss)
    shutil.copy (srccss, cssfile)
  # execute commands
  try:
    if Config.command == 'doxi2html' and Config.inputs:
      progress ('Processing: %s' % string_from_files (Config.inputs))
      for ifile in Config.inputs:
        if not os.path.isabs (ifile):
          ifile = os.path.normpath (os.path.join (Config.srcdir, ifile))
        obase = os.path.basename (ifile)
        defs = {}
        extmatch = re.match (r'(.+)\.([0-9][A-Za-z0-9]*)\.doxi$', obase)
        if extmatch:
          mansuffix = extmatch.expand (r'\2')
          trunk = extmatch.expand (r'\1')
          obase = trunk + '.' + mansuffix
          defs.update ({ 'man-section' : mansuffix })
        else:
          if Config.manpage:
            raise RuntimeError ('Failed to detect doxi manual page suffix: %s' % obase)
          extmatch = re.match (r'(.+)\.[^.]+$', obase)
          if extmatch:
            trunk = extmatch.expand (r'\1')
            obase = trunk
          else:
            trunk = obase
        if Config.htmlext[0:1] == '.':
          obase = obase + Config.htmlext
        else:
          obase = obase + '.' + Config.htmlext
        ofile = os.path.join (Config.destdir, obase)
        defs.update (Config.defines)
        root = DoxiParser.parse_file (ifile, ofile, defs)
        debug ('Generating %s...' % ofile)
        fout = open (ofile, 'w')
        print >>fout, HtmlGenerator.process_root (root)
        fout.close()
    elif Config.command == 'doxi2man' and Config.inputs:
      progress ('Processing: %s' % string_from_files (Config.inputs))
      for ifile in Config.inputs:
        if not os.path.isabs (ifile):
          ifile = os.path.normpath (os.path.join (Config.srcdir, ifile))
        obase = os.path.basename (ifile)
        extmatch = re.match (r'(.+)\.([0-9][A-Za-z0-9]*)\.doxi$', obase)
        if not extmatch:
          raise RuntimeError ('Failed to detect doxi manual page suffix: %s' % obase)
        mansuffix = extmatch.expand (r'\2')
        trunk = extmatch.expand (r'\1')
        ofile = os.path.join (Config.destdir, trunk + '.' + mansuffix)
        defs = { 'man-section' : mansuffix }
        defs.update (Config.defines)
        root = DoxiParser.parse_file (ifile, ofile, defs)
        debug ('Generating %s...' % ofile)
        fout = open (ofile, 'w')
        print "file: ",ofile
        print >>fout, ManGenerator.process_root (root)
        fout.close()
    elif Config.command == 'writecss':
      write_css_to_destdir()
    elif Config.command == 'scad2doxi' and Config.inputs and Config.template:
      # parse input files
      sfiles = scad2sfiles (Config.inputs, Config.extra_inputs)
      # create doxi files
      sfiles2doxi (sfiles)
    elif Config.command == 'scad2pickle' and Config.inputs:
      # parse input files
      sfiles = scad2sfiles (Config.inputs, Config.extra_inputs)
      # create pickle files
      sfiles2pickle (sfiles)
    elif Config.command == 'src2pickle' and Config.inputs:
      # parse sources with doxygen
      sfiles = sources2sfiles (Config.inputs, Config.extra_inputs)
      # create pickle files
      sfiles2pickle (sfiles)
    elif Config.command == 'pickle2doxi' and Config.inputs and Config.template:
      # parse sources with doxygen
      sfiles = []
      for ifile in Config.inputs + Config.extra_inputs:
        sfiles += pickle2sfiles (ifile)
      # create doxi files
      sfiles2doxi (sfiles)
    elif Config.command == 'src2doxi' and Config.inputs and Config.template:
      # parse sources with doxygen
      sfiles = sources2sfiles (Config.inputs, Config.extra_inputs)
      # create doxi files
      sfiles2doxi (sfiles)
    elif Config.command == 'doxi2xml' and Config.inputs:
      progress ('Processing: %s' % string_from_files (Config.inputs))
      for ifile in Config.inputs:
        if not os.path.isabs (ifile):
          ifile = os.path.normpath (os.path.join (Config.srcdir, ifile))
        ofile = os.path.join (Config.destdir, os.path.basename (ifile) + '.xml')
        root = DoxiParser.parse_file (ifile, ofile, Config.defines)
        Data.XML.serialize_node (root, ofile)
    else:
      print "Nothing to be done."
  except KeyboardInterrupt, ki:
    sys.stderr.write ("Cancelled by KeyboardInterrupt...\n")
    if Config.debug:
      raise
    else:
      debug ('Removing temporary directory: %s' % Config.tmpdir)
      shutil.rmtree (Config.tmpdir) # should not fail
    sys.exit (1)
  except Exception, ex:
    estr = str (ex).strip()
    if not estr:
      raise     # usually an AssertionError
    print >>sys.stderr, estr
    if Config.debug:
      raise
    sys.exit (1)
  except:
    raise # abort()
  # cleanup
  try:
    os.chdir (Config.TMP_DIR)
  except: pass
  if not Config.debug:
    debug ('Removing temporary directory: %s' % Config.tmpdir)
    shutil.rmtree (Config.tmpdir) # should not fail
  sys.exit (0)

def parse_command_and_args():
  newconf = Config.MutableDict()
  newconf.inputs = []
  newconf.extra_inputs = []
  newconf.defines = {}
  includes = []
  command = None
  arg_iter = sys.argv[1:].__iter__()
  rest = len (sys.argv) - 1
  seen_extra_qualifier = False
  for arg in arg_iter:
    rest -= 1
    if arg == '--debug':
      Config.debug = True
      Config.print_traceback = True
    elif arg == '--help' or arg == '-h':
      print_help ()
      sys.exit (0)
    elif arg == '--version' or arg == '-v':
      print_help (False)
      sys.exit (0)
    elif rest and arg in ('-d',):
      rest -= 1
      newconf.destdir = os.path.abspath (arg_iter.next())
    elif rest and arg == '-I':
      includes += [ arg_iter.next() ]; rest -= 1
    elif len (arg) > 2 and arg[0:1] == '-I':
      includes += [ arg[2:] ]
    elif rest > 1 and arg == '-D':
      name = arg_iter.next(); rest -= 1
      value = arg_iter.next(); rest -= 1
      newconf.defines[name] = value
    elif arg == '-i' and command in ('scad2doxi', 'src2doxi', 'pickle2doxi'):
      newconf.with_index = True
    elif command in ('src2pickle', 'scad2pickle') and arg == '-o' and rest:
      newconf.output_file = arg_iter.next(); rest -= 1
    elif command == None and arg == 'doxi2xml':
      command = 'doxi2xml'
    elif command == None and arg == 'writecss':
      command = 'writecss'
    elif command in ('doxi2html', 'scad2doxi', 'src2doxi', 'pickle2doxi') and arg == '-j' and rest:
      newconf.htmlext = arg_iter.next(); rest -= 1
    elif command == None and arg == 'doxi2html':
      command = 'doxi2html'
    elif command == None and arg == 'doxi2htmlman':
      command = 'doxi2html'
      newconf.manpage = True
    elif command == None and arg == 'scad2doxi': # source code analysis data
      command = 'scad2doxi'
    elif command in ('scad2doxi', 'src2doxi', 'pickle2doxi') and arg == '-e' and rest:
      newconf.doxiext = arg_iter.next(); rest -= 1
    elif command == None and arg in ('src2doxi', 'src2pickle', 'scad2pickle', 'pickle2doxi'):
      command = arg
    elif command in ('src2doxi', 'scad2doxi', 'pickle2doxi') and arg == '-T' and rest:
      newconf.template = arg_iter.next(); rest -= 1
    elif command == None and arg == 'doxi2man':
      command = 'doxi2man'
      newconf.manpage = True
    elif command == None:
      raise RuntimeError ('Missing command for argument: %s' % arg)
    elif command in ('src2doxi', 'src2pickle') and arg == '-Y':
      seen_extra_qualifier = True
    elif command in ('doxi2xml', 'doxi2html', 'src2doxi', 'src2pickle', 'pickle2doxi', 'scad2doxi', 'scad2pickle', 'doxi2man'):
      if seen_extra_qualifier:
        newconf.extra_inputs += [ arg ]
      else:
        newconf.inputs += [ arg ]
    else:
      raise RuntimeError ('Unknown argument: %s' % arg)
  if not command:
    print_help ()
    sys.exit (1)
  else:
    newconf.command = command
  if includes:
    newconf.includes = includes
  return newconf

def print_help (with_help = True):
  print "doxer.py version", Config.VERSION
  if not with_help:
    return
  print "Usage: %s [options] command [command args] " % os.path.basename (sys.argv[0])
  print "Options:"
  print "  --help, -h                print this help message"
  print "  --version, -v             print version info"
  print "  --debug                   print debug information"
  print "  -d OUTPUTDIR              write output to OUTPUTDIR/ (defaults to ./)"
  print "  -D name value             define 'name' to 'value'"
  print "  -I DIRECTORY              add DIRECTORY to include path"
  #      12345678901234567890123456789012345678901234567890123456789012345678901234567890
  print "Commands:"
  print "  src2pickle [sources...]   generate pickled source code analysis dump"
  print "    -Y [xsources...]        add descriptions from xsources"
  print "    -o OUTPUTFILE           write pickled output to OUTPUTFILE"
  print "  pickle2doxi [pickles...]  generate Doxer documentation descriptions for"
  print "                            pickles (pickle files) in OUTPUTDIR/"
  print "    -T DOXITEMPLATE         generate doc files from DOXITEMPLATE"
  print "    -i                      generate index file from DOXITEMPLATE"
  print "    -e DoxiExtension        extension to use instead of .doxi"
  print "    -j HtmlExtension        extension to use instead of .html"
  print "  src2doxi [sources...]     generate Doxer documentation descriptions for"
  print "                            sources in OUTPUTDIR/"
  print "    -T DOXITEMPLATE         generate doc files from DOXITEMPLATE"
  print "    -i                      generate index file from DOXITEMPLATE"
  print "    -Y [xsources...]        add descriptions from xsources"
  print "    -e DoxiExtension        extension to use instead of .doxi"
  print "    -j HtmlExtension        extension to use instead of .html"
  print "  scad2doxi [dsources...]   generate Doxer documentation descriptions for"
  print "                            dsources in OUTPUTDIR/"
  print "    -T DOXITEMPLATE         generate doc files from DOXITEMPLATE"
  print "    -i                      generate index file from DOXITEMPLATE"
  print "    -e DoxiExtension        extension to use instead of .doxi"
  print "    -j HtmlExtension        extension to use instead of .html"
  print "  scad2pickle [dsources...] generate pickled source code analysis dump"
  print "    -o OUTPUTFILE           write pickled output to OUTPUTFILE"
  print "  writecss                  create the CSS file needed by doxer HTML pages"
  print "                            in OUTPUTDIR/"
  print "  doxi2html [files...]      process doxi files and generate HTML pages"
  print "                            in OUTPUTDIR/"
  print "    -j HtmlExtension        extension to use instead of .html"
  print "  doxi2htmlman [...]        variant of doxi2html for manual pages"
  print "  doxi2man [dmfiles...]     process doxi manual page files and generate groff"
  print "                            manual pages in OUTPUTDIR/"
  print "  doxi2xml [dfiles...]      process doxi files and generate XML descriptions"
  print "                            in OUTPUTDIR/"
  print "This program comes with ABSOLUTELY NO WARRANTY."
  print "You may redistribute copies of this program under the terms of"
  print "the GNU General Public License which can be found on the GNU"
  print "website, available at http://www.gnu.org/copyleft/gpl.html"

# --- run main() ---
main()
