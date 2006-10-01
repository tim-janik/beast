#!/usr/bin/env python2.4
#
# Doxer - Software documentation system
# Copyright (C) 2005-2006 Tim Janik
#
# qdoxygen.py - subshell to doxygen and generate XML descriptions
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
import os, sys, re, Config
def debug (*args): Config.debug_print (*args)

def create_doxygen_input (inputfile, destfile):
  debug ("convert to doxygen format:", inputfile, '->', destfile)
  fin = open (inputfile, 'r')
  fout = open (destfile, 'w')
  # put /** @file ... */ around the input file so doxygen
  # recognizes this as a documentation file
  fout.write ('/** @file\n')
  for line in fin:
    # we need to escape slash sequences to work around doxygen comment parsing
    # (since doxygen matches '*/' and strips '//'
    line = re.sub (r'([*/])/', r'\1<!---->/', line)
    fout.write (line)
  fout.write ('**/')
  fout.close()
  fin.close()

def doxer2xml (inputfile):
  # sanitize path names
  if not os.path.isabs (inputfile):
    inputfile = os.path.normpath (Config.srcdir + '/' + inputfile)
  # figure doxygen input filename
  tfile = os.path.basename (inputfile)
  tfile, tmext = os.path.splitext (tfile)
  tfile = os.path.join (Config.tmpdir, tfile)   # make temporary pathname
  tdoxyfile = tfile + ".Doxyfile"
  if tmext and tmext != '.doxi':                # strip '.doxi' file extension
    tfile = tfile + tmext
  # create intermediate
  create_doxygen_input (inputfile, tfile)
  # configure doxygen
  alias_list = ''
  #alias_list += ' "center=\\ref ::xml:center "'
  alias_list += r' "center=@ref http:///extlink"'
  write_config (tdoxyfile, {
    'OUTPUT_DIRECTORY'  : '.',
    'INPUT'             : tfile,
    'ALIASES'           : alias_list,
  })
  # invoke doxygen
  command = 'doxygen %s' % tdoxyfile
  debug ('subshell:', command)
  xcode = os.system (command)
  if xcode:
    raise RuntimeError ('Failed to execute doxygen: %d' % xcode)
  # cleanup
  try:
    os.remove (tfile)
    os.remove (tdoxyfile)
  except: pass
  xmlresult = os.path.join (Config.tmpdir, 'xml/')
  debug ('Asserting results in:', xmlresult)
  assert os.stat (xmlresult)
  return xmlresult

def sources2doxygenxml (srclist):
  allfiles = ''
  # sanitize path names
  for inputfile in srclist:
    if not os.path.isabs (inputfile):
      inputfile = os.path.normpath (Config.srcdir + '/' + inputfile)
    allfiles += ' ' + inputfile
  xmlresult = os.path.join (Config.tmpdir, 'xml/')
  # figure doxygen input filename
  tdoxyfile = os.path.join (Config.tmpdir, "sources2xml.Doxyfile")
  # configure comment filter
  commentfilter = os.path.join (Config.installdir, 'qcomment.py')
  os.environ['DOXER_QCOMMENT_CONFIG'] = Config.debug and ':debug:' or ''
  os.environ['DOXER_QCOMMENT_DUMP'] = os.path.join (xmlresult, '.qcomment.dump')
  # configure doxygen
  alias_list = ''
  write_config (tdoxyfile, {
    'OUTPUT_DIRECTORY'          : '.',
    'INPUT'                     : allfiles,
    'INPUT_FILTER'              : os.path.join (Config.installdir, 'qcomment.py'),
    'FILTER_SOURCE_FILES'       : 'YES',
    'PREDEFINED'                : 'DOXER',
  })
  # invoke doxygen
  command = 'doxygen %s' % tdoxyfile
  debug ('subshell:', command)
  xcode = os.system (command)
  if xcode:
    raise RuntimeError ('Failed to execute doxygen: %d' % xcode)
  # cleanup
  del os.environ['DOXER_QCOMMENT_DUMP']
  del os.environ['DOXER_QCOMMENT_CONFIG']
  try:
    if not Config.debug:
      os.remove (tdoxyfile)
  except: pass
  debug ('Asserting result location:', xmlresult)
  assert os.stat (xmlresult)
  return xmlresult

def cat_file (inputfile):
  debug ("cat", inputfile)
  fin = open (inputfile, 'r')
  for line in fin:
    sys.stdout.write (line)
  sys.stdout.flush()
  fin.close()

doxyfile_config = [     # Doxyfile from doxygen-1.4.4
  # Project related configuration options
  ( 'PROJECT_NAME',           '' ),
  ( 'PROJECT_NUMBER',         '' ),             # project version
  ( 'OUTPUT_DIRECTORY',       '' ),
  ( 'CREATE_SUBDIRS',           'NO' ),
  ( 'OUTPUT_LANGUAGE',          'English' ),
  ( 'USE_WINDOWS_ENCODING',     'NO' ),
  ( 'BRIEF_MEMBER_DESC',        'YES' ),        # whether to put out brief descriptions
  ( 'ABBREVIATE_BRIEF',         '' ),
  ( 'REPEAT_BRIEF',             'YES' ),        # repeat brief as detailed description
  ( 'ALWAYS_DETAILED_SEC',      'NO' ),         # turn brief into detailed description
  ( 'INLINE_INHERITED_MEMB',    'NO' ),
  ( 'FULL_PATH_NAMES',          'YES' ),
  ( 'STRIP_FROM_PATH',          '' ),
  ( 'STRIP_FROM_INC_PATH',      '' ),
  ( 'SHORT_NAMES',              'NO' ),
  ( 'JAVADOC_AUTOBRIEF',        'NO' ),         # whether to parse first-line brief descriptions
  ( 'MULTILINE_CPP_IS_BRIEF',   'NO' ),
  ( 'DETAILS_AT_TOP',           'NO' ),
  ( 'INHERIT_DOCS',             'NO' ),
  ( 'DISTRIBUTE_GROUP_DOC',     'NO' ),
  ( 'SEPARATE_MEMBER_PAGES',    'NO' ),
  ( 'TAB_SIZE',                 '8' ),
  ( 'ALIASES',                  '"br=<br>" returns=\return "docu=\file"' ),
  ( 'OPTIMIZE_OUTPUT_FOR_C',    'NO' ),
  ( 'OPTIMIZE_OUTPUT_JAVA',     'NO' ),
  ( 'SUBGROUPING',              'NO' ),
  # Build related configuration options
  ( 'EXTRACT_ALL',              'YES' ),
  # ( 'EXTRACT_ALL_LOCAL',        'YES' ),
  ( 'EXTRACT_PRIVATE',          'YES' ),
  ( 'EXTRACT_STATIC',           'YES' ),
  ( 'EXTRACT_LOCAL_CLASSES',    'YES' ),
  ( 'EXTRACT_LOCAL_METHODS',    'YES' ),
  ( 'HIDE_UNDOC_MEMBERS',       'NO' ),
  ( 'HIDE_UNDOC_CLASSES',       'NO' ),
  ( 'HIDE_FRIEND_COMPOUNDS',    'NO' ),
  ( 'HIDE_IN_BODY_DOCS',        'NO' ),
  ( 'INTERNAL_DOCS',            'NO' ),         # FIXME: need toggle
  ( 'CASE_SENSE_NAMES',         'YES' ),
  ( 'HIDE_SCOPE_NAMES',         'NO' ),
  ( 'SHOW_INCLUDE_FILES',       'YES' ),
  ( 'INLINE_INFO',              'YES' ),
  ( 'SORT_MEMBER_DOCS',         'NO' ),
  ( 'SORT_BRIEF_DOCS',          'NO' ),
  ( 'SORT_BY_SCOPE_NAME',       'NO' ),
  ( 'GENERATE_TODOLIST',        'YES' ),
  ( 'GENERATE_TESTLIST',        'YES' ),
  ( 'GENERATE_BUGLIST',         'YES' ),
  ( 'GENERATE_DEPRECATEDLIST',  'YES' ),
  ( 'ENABLED_SECTIONS',         '' ),
  ( 'MAX_INITIALIZER_LINES',    '10000' ),
  ( 'SHOW_USED_FILES',          'YES' ),        # FIXME: does this change XML?
  ( 'SHOW_DIRECTORIES',         'NO' ),         # FIXME: changes XML?
  ( 'FILE_VERSION_FILTER',      '' ),
  # configuration options related to warning and progress messages
  ( 'QUIET',                    'YES' ),
  ( 'WARNINGS',                 'YES' ),
  ( 'WARN_IF_UNDOCUMENTED',     'YES' ),
  ( 'WARN_IF_DOC_ERROR',        'YES' ),
  ( 'WARN_NO_PARAMDOC',         'NO' ),
  ( 'WARN_FORMAT',              '$file:$line: $text' ),
  ( 'WARN_LOGFILE',             '' ),
  # configuration options related to the input files
  ( 'INPUT',                    '' ),           # input files or dirs
  ( 'FILE_PATTERNS',            '*.h *.hh *.hpp *.H *.c *.cc *.cpp *.C' ),
  ( 'EXCLUDE_PATTERNS',         '' ),           # exclude pattern matches against abspath
  ( 'EXCLUDE',                  '' ),           # exclude file/dirs
  ( 'RECURSIVE',                'NO' ),
  ( 'EXCLUDE_SYMLINKS',         'NO' ),
  ( 'EXAMPLE_PATH',             '' ),
  ( 'EXAMPLE_PATTERNS',         '' ),
  ( 'EXAMPLE_RECURSIVE',        'NO' ),
  ( 'IMAGE_PATH',               '' ),
  ( 'INPUT_FILTER',             '' ),
  ( 'FILTER_PATTERNS',          '' ),
  ( 'FILTER_SOURCE_FILES',      'NO' ),
  # configuration options related to source browsing
  ( 'SOURCE_BROWSER',           'NO' ),
  ( 'INLINE_SOURCES',           'NO' ),
  ( 'STRIP_CODE_COMMENTS',      'YES' ),
  ( 'REFERENCED_BY_RELATION',   'YES' ),
  ( 'REFERENCES_RELATION',      'YES' ),
  # ( 'USE_HTAGS',                'NO' ),       # only supported since doxgen 1.4.4
  ( 'VERBATIM_HEADERS',         'NO' ),
  # configuration options related to the alphabetical class index
  ( 'ALPHABETICAL_INDEX',       'NO' ),
  ( 'COLS_IN_ALPHA_INDEX',      '5' ),
  ( 'IGNORE_PREFIX',            '' ),
  # configuration options related to the HTML output
  ( 'GENERATE_HTML',            'NO' ),
  ( 'HTML_OUTPUT',              'html' ),
  ( 'HTML_FILE_EXTENSION',      '.html' ),
  ( 'HTML_HEADER',              '' ),
  ( 'HTML_FOOTER',              '' ),
  ( 'HTML_STYLESHEET',          '' ),
  ( 'HTML_ALIGN_MEMBERS',       'YES' ),
  ( 'GENERATE_HTMLHELP',        'NO' ),
  ( 'CHM_FILE',                 '' ),
  ( 'HHC_LOCATION',             '' ),
  ( 'GENERATE_CHI',             'NO' ),
  ( 'BINARY_TOC',               'NO' ),
  ( 'TOC_EXPAND',               'NO' ),
  ( 'DISABLE_INDEX',            'NO' ),
  ( 'ENUM_VALUES_PER_LINE',     '1' ),
  ( 'GENERATE_TREEVIEW',        'NO' ),
  ( 'TREEVIEW_WIDTH',           '250' ),
  # configuration options related to the LaTeX output
  ( 'GENERATE_LATEX',           'NO' ),
  ( 'LATEX_OUTPUT',             'latex' ),
  ( 'LATEX_CMD_NAME',           'latex' ),
  ( 'MAKEINDEX_CMD_NAME',       'makeindex' ),
  ( 'COMPACT_LATEX',            'NO' ),
  ( 'PAPER_TYPE',               'a4wide' ),
  ( 'EXTRA_PACKAGES',           '' ),
  ( 'LATEX_HEADER',             '' ),
  ( 'PDF_HYPERLINKS',           '' ),
  ( 'USE_PDFLATEX',             'NO' ),
  ( 'LATEX_BATCHMODE',          'NO' ),
  ( 'LATEX_HIDE_INDICES',       'NO' ),
  # configuration options related to the RTF output
  ( 'GENERATE_RTF',             'NO' ),
  ( 'RTF_OUTPUT',               'rtf' ),
  ( 'COMPACT_RTF',              'NO' ),
  ( 'RTF_HYPERLINKS',           'NO' ),
  ( 'RTF_STYLESHEET_FILE',      '' ),
  ( 'RTF_EXTENSIONS_FILE',      '' ),
  # configuration options related to the man page output
  ( 'GENERATE_MAN',             'NO' ),
  ( 'MAN_OUTPUT',               'man' ),
  ( 'MAN_EXTENSION',            '.3' ),
  ( 'MAN_LINKS',                'NO' ),
  # configuration options related to the XML output
  ( 'GENERATE_XML',             'YES' ),
  ( 'XML_OUTPUT',               'xml' ),
  ( 'XML_SCHEMA',               '' ),
  ( 'XML_DTD',                  '' ),
  ( 'XML_PROGRAMLISTING',       'YES' ),
  # configuration options for the AutoGen Definitions output
  ( 'GENERATE_AUTOGEN_DEF',     'NO' ),
  # configuration options related to the Perl module output
  ( 'GENERATE_PERLMOD',         'NO' ),
  ( 'PERLMOD_LATEX',            'NO' ),
  ( 'PERLMOD_PRETTY',           'YES' ),
  ( 'PERLMOD_MAKEVAR_PREFIX',   '' ),
  # Configuration options related to the preprocessor   
  ( 'ENABLE_PREPROCESSING',     'YES' ),
  ( 'MACRO_EXPANSION',          'NO' ),
  ( 'EXPAND_ONLY_PREDEF',       'NO' ),
  ( 'SEARCH_INCLUDES',          'NO' ),
  ( 'INCLUDE_PATH',             '' ),
  ( 'INCLUDE_FILE_PATTERNS',    '' ),
  ( 'PREDEFINED',               '' ),
  ( 'EXPAND_AS_DEFINED',        '' ),
  ( 'SKIP_FUNCTION_MACROS',     'YES' ),
  # Configuration::additions related to external references   
  ( 'TAGFILES',                 '' ),
  ( 'GENERATE_TAGFILE',         '' ),
  ( 'ALLEXTERNALS',             'NO' ),
  ( 'EXTERNAL_GROUPS',          'YES' ),
  ( 'PERL_PATH',                '/usr/bin/perl' ),
  # Configuration options related to the dot tool   
  ( 'CLASS_DIAGRAMS',           'YES' ),
  ( 'HIDE_UNDOC_RELATIONS',     'NO' ),
  ( 'HAVE_DOT',                 'NO' ),
  ( 'CLASS_GRAPH',              'YES' ),
  ( 'COLLABORATION_GRAPH',      'YES' ),
  ( 'GROUP_GRAPHS',             'YES' ),
  ( 'UML_LOOK',                 'NO' ),
  ( 'TEMPLATE_RELATIONS',       'YES' ),
  ( 'INCLUDE_GRAPH',            'YES' ),
  ( 'INCLUDED_BY_GRAPH',        'YES' ),
  ( 'CALL_GRAPH',               'NO' ),
  ( 'GRAPHICAL_HIERARCHY',      'YES' ),
  ( 'DIRECTORY_GRAPH',          'YES' ),
  ( 'DOT_IMAGE_FORMAT',         'png' ),
  ( 'DOT_PATH',                 '' ),
  ( 'DOTFILE_DIRS',             '' ),
  ( 'MAX_DOT_GRAPH_WIDTH',      '1024' ),
  ( 'MAX_DOT_GRAPH_HEIGHT',     '1024' ),
  ( 'MAX_DOT_GRAPH_DEPTH',      '0' ),
  ( 'DOT_TRANSPARENT',          'NO' ),
  ( 'DOT_MULTI_TARGETS',        'NO' ),
  ( 'GENERATE_LEGEND',          'YES' ),
  ( 'DOT_CLEANUP',              'YES' ),
  # Configuration::additions related to the search engine   
  ( 'SEARCHENGINE',             'NO' ),
]

def write_config (doxyfile, cdict = {}):
  debug ('Creating doxyfile:', doxyfile)
  fout = open (doxyfile, 'w')
  for i in doxyfile_config:
    v = i[1]
    try:
      v = cdict[i[0]]
    except: pass
    print >>fout, i[0], '=', v
  fout.close()
