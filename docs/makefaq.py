#!/usr/bin/env python
#
# makefaq.py
# Revision:  0.3
# Rev Date:  22 January 2000
#
# This program is designed to take a text file and generate
# a single-page formatted Frequently-Asked-Question file.
#
# It reads in FAQ categories, questions and answers from
# a file called "faq.dat", adds an HTML header and footer, and
# writes the information to "faq.html".  As of this version
# there is also the ability to write out a text version.
#
# See the "Notes" section below for more details.
#
# If you have any comments about this script, of if you make
# an improved version, please contact Dan York at 
# dyork@lodestar2.com
#
# Copyright (c) 1999-2000 Dan York, dyork@Lodestar2.com
# http://www.Lodestar2.com/software/makefaq/
#
# The author acknowledges significant contributions to the
# code by Dave Seidel (dave@superluminal.com) and he can
# definitely be considered as the co-author of this code.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or any later version.
#
# This program is distributed in the hope that it will be useful
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
# GNU General Public License for more details.
# http://www.gnu.org/copyleft/gpl.html
#
# -------------------------------------------------------------
#
# If you use this script to generate a FAQ page for a public
# web site, please do send the URL to dyork@lodestar2.com so
# that your FAQ can be listed on the home page for makefaq as
# another example.
#
# -------------------------------------------------------------
# NOTES
#
#
# This script is made available purely so that others might 
# be saved the frustration of building FAQ pages by hand.  
# Examples of its use can be found at:
#
#   http://www.lpi.org/faq.html
#   http://beast.gtk.org/docs/faq.html
#
# The input file must be called "faq.dat". It uses the pipe character
# as the delimiter and must have the format:
#
#  Category|Question|Answer
#
# Each entry is on a *single* line (so don't use an editor that
# wraps text. The Question and Answer sections can use HTML.
#
# In the directory where the script is run, there must be three
# files (in the default configuration):
#
# - faq.dat         - the text file with the questions and answers
# - faqheader.html  - an HTML file with the top of the file
# - faqfooter.html  - an HTML file with the bottom of the file
#
# Sample files should have been provided with this code.
#
# Please read the accompanying "ChangeLog" file to understand
# the substantial changes that have been made to the code base.
#
# Also please look at the README file before using this script.
#
# -------------------------------------------------------------
# BUGS
#
# Note that currently the Categories may not appear in the correct
# order (as they appear in the faq.dat file). I'm still working on it.
#
# -------------------------------------------------------------
# REVISION HISTORY
#
# 22 Jan 2000 - 0.3 uploaded after substantantial modifications
#               by Dave Seidel (dave@superluminal.com)
# 14 Jan 2000 - (dave) added command line processing, error handling,
#               comments
# 13 Jan 2000 - (dave) submitted to BEAST project
#  3 Jan 2000 - 0.2 uploaded in beta state to web site
#
# -------------------------------------------------------------
# QUICK SUMMARY OF CHANGES TO VERSION 0.3
#
# See the accompanying "ChangeLog" file for full details.
#
# New features:
# - Moved all formatting strings into classes or "configurations".
#   The base class, DefaultConfig, corresponds to Dan's original
#   settings.  Dave added two additional subclasses, BEASTConfig
#   and TextConfig, that respectively define a fancy HTML format
#   (designed for http://beast.gtk.org, where Dave is the webmaster)
#   and a plain text output.
# - Added command line processing.  It is possible to select
#   a configuration, and to override a configuration's settings
#   for input, output, header, and footer files.  Use the "-h"
#   switch for help on the options.
# - Added some simple error handling.
#
# -------------------------------------------------------------

import sys
import getopt
import time
import string
import re

TRUE  = 1
FALSE = 0

def TellTruth(thing):
   if thing:
      return 'TRUE'
   return 'FALSE'

# -------------------------------------------------------------
#
# base configuration class, with all filename and formatting
# defaults; note that all the data defined in this class
# is inherited by its subclasses, unless you override it
#
# -------------------------------------------------------------
class DefaultConfig:
   def show(self):
      print '  Configuration: ' + self.name     + '\n' + \
            '     Input file: ' + self.infile   + '\n' + \
            '    Header file: ' + self.headfile + '\n' + \
            '    Footer file: ' + self.footfile + '\n' + \
            '    Output file: ' + self.outfile  + '\n' + \
            '       hasLinks: ' + TellTruth(self.hasLinks)

   def __repr__(self):
      return self.name;

   def __init__(self):
      # name
      self.name = "default"

      # if this is FALSE, two things happen:
      # - <a></a> tags get stripped out (but not their content)
      # - <br> tags are converted to '\n'
      self.hasLinks = TRUE

      # default filenames
      self.headfile = 'faqheader.html'
      self.footfile = 'faqfooter.html'
      self.infile   = 'faq.dat'

      #If you want the default behaviour of makefaq 0.2, 
      #which was to dump the output to stdout, change the outfile
      #line to:
      #
      #self.outfile  = 'STDOUT'
      #
      self.outfile  = 'faq.html'

      # timestamp
      self.TS = {
         'Pre'   : '<p><i>',
         'Post'  : '</i></p>',
         'Pre+'  : '',
         'Post+' : ''
         }

      # headings
      self.Head = {
         'Pre'   : '<hr><h2>',
         'Post'  : '</h2>',
         'Pre+'  : '',
         'Post+' : ''
         }

      # sections
      self.Sec = {
         'Pre'   : '<dl>',
         'Post'  : '</dl>',
         'Pre+'  : '',
         'Post+' : ''
         }

      # table of contents
      self.TOC = {
         'Pre'       : '<dl>',
         'Post'      : '</dl>',
         'Pre+'      : '',
         'Post+'     : '',
         'CatPre'    : '<dt><b>',
         'CatPost'   : '</b></dt>',
         'ListPre'   : '<dd><ul>',
         'ListPost'  : '</ul></dd>',
         'EntryPre'  : '<li><a href="#',
         'EntryIn'   : '">',
         'EntryPost' : '</a></li>'
         }

      # questions
      self.Q = {
         'Pre'  : '<dt><b><a name="',
         'In'   : '\">',
         'Post' : '</a></b></dt>'
         }

      # answers
      self.A = {
         'Pre'  : '<dd>',
         'Post' : '<br><br></dd>'
         }


# -------------------------------------------------------------
#
# text output configuration
#
# -------------------------------------------------------------
class TextConfig(DefaultConfig):
   def __init__(self):
      DefaultConfig.__init__(self)

      # ID
      self.name = 'text'

      # flags
      self.hasLinks = FALSE

      # filenames
      self.headfile = 'faqheader.txt'
      self.footfile = 'faqfooter.txt'
      self.infile   = 'faq.dat'
      self.outfile  = 'faq.txt'

      # timestamp
      self.TS['Pre']   = '\n'
      self.TS['Post']  = '\n\n'
      self.TS['Pre+']  = ''
      self.TS['Post+'] = ''

      # heading
      self.Head['Pre']  = ''
      self.Head['Post'] = '\n'
      self.Head['Pre+']  = ''
      self.Head['Post+'] = ''

      # sections
      self.Sec['Pre']  = ''
      self.Sec['Post'] = ''
      self.Sec['Pre+']  = ''
      self.Sec['Post+'] = ''

      # TOC
      self.TOC['Pre']       = ''
      self.TOC['Post']      = '\n'
      self.TOC['Pre+']      = ''
      self.TOC['Post+']     = ''
      self.TOC['CatPre']    = ''
      self.TOC['CatPost']   = ''
      self.TOC['ListPre']   = ''
      self.TOC['ListPost']  = '\n'
      self.TOC['EntryPre']  = ''
      self.TOC['EntryIn']   = ''
      self.TOC['EntryPost'] = ''

      # questions
      self.Q['Pre']  = ''
      self.Q['In']   = ''
      self.Q['Post'] = '\n'

      # answers
      self.A['Pre']  = ''
      self.A['Post'] = '\n'


# -------------------------------------------------------------
#
# screen (text) output configuration
#
# -------------------------------------------------------------
class ScreenConfig(DefaultConfig):
   def __init__(self):
      DefaultConfig.__init__(self)

      # ID
      self.name = 'screen'

      # flags
      self.hasLinks = FALSE

      # filenames
      self.headfile = 'faqheader.txt'
      self.footfile = 'faqfooter.txt'
      self.infile   = 'faq.dat'
      self.outfile  = 'STDOUT'

      # timestamp
      self.TS['Pre']   = '\n'
      self.TS['Post']  = '\n\n'
      self.TS['Pre+']  = ''
      self.TS['Post+'] = ''

      # heading
      self.Head['Pre']  = ''
      self.Head['Post'] = '\n'
      self.Head['Pre+']  = ''
      self.Head['Post+'] = ''

      # sections
      self.Sec['Pre']  = ''
      self.Sec['Post'] = ''
      self.Sec['Pre+']  = ''
      self.Sec['Post+'] = ''

      # TOC
      self.TOC['Pre']       = ''
      self.TOC['Post']      = '\n'
      self.TOC['Pre+']      = ''
      self.TOC['Post+']     = ''
      self.TOC['CatPre']    = ''
      self.TOC['CatPost']   = ''
      self.TOC['ListPre']   = ''
      self.TOC['ListPost']  = '\n'
      self.TOC['EntryPre']  = ''
      self.TOC['EntryIn']   = ''
      self.TOC['EntryPost'] = ''

      # questions
      self.Q['Pre']  = ''
      self.Q['In']   = ''
      self.Q['Post'] = '\n'

      # answers
      self.A['Pre']  = ''
      self.A['Post'] = '\n'


# -------------------------------------------------------------
#
# BEAST configuration
# 
# Sample configuration provided by Dave Seidel
# (dave@superluminal.com)
#
# -------------------------------------------------------------
class BEASTConfig(DefaultConfig):
   def __init__(self):
      DefaultConfig.__init__(self)

      # ID
      self.name = 'BEAST'
      
      # filenames
      self.headfile = 'html.1.faq'
      self.footfile = 'html.2.faq'
      self.infile   = 'faq.dat'
      self.outfile  = 'faq.html'

      # timestamp
      self.TS['Pre+']  = '<tr><td>'
      self.TS['Post+'] = '</td></tr>'

      # headings
      self.Head['Pre']   = ''
      self.Head['Post']  = ''
      self.Head['Pre+']  = '<tr><td bgcolor="#005D5D"><font face="Lucida, Verdana, Arial, sans-serif" color="#D0E4D0" size="+2">'
      self.Head['Post+'] = '</font></td></tr>'

      # sections
      self.Sec['Pre+']  = '<tr><td><font face="Lucida, Verdana, Arial, sans-serif">'
      self.Sec['Post+'] = '</font></td></tr>'

      # table of contents
      self.TOC['Pre+']  = '<tr><td><font face="Lucida, Verdana, Arial, sans-serif">'
      self.TOC['Post+'] = '</font></td></tr>'


# -------------------------------------------------------------
#
# table of available configurations; if you add a new
# configuration, please add its class name to this list
#
# -------------------------------------------------------------

configTab = [
   DefaultConfig,
   TextConfig,
   ScreenConfig,
   BEASTConfig
   ]

#
# tells each member of configTab to print itself
#
def PrintConfigs():
   print 'Available configurations:'
   for i in configTab:
      cfg = i()
      cfg.show()

#
# given a config name, attempts to find a matching entry
# in configTab; if found returns an *instance* of the
# matching class
#
def FindConfig(name):
   for i in configTab:
      cfg = i()
      if name == str(cfg):
         return cfg


class FaqEntry:
   def __init__(self,content):
      self.question = content[0]
      self.answer = content[1]
      
   def __repr__(self):
      return "\n" +  self.question +  "\n" +  self.answer


def IncludeFile(out, inputfile):
   try:
      input = open(inputfile, 'r')
   except:
      sys.stderr.write('Error opening file ' + inputfile + ' for inclusion.\n')
      sys.exit(1)
   text = input.read()
   out.write(text)


def FixSpecialText(text):
   fixed = re.sub('<br>', '\n', text)
   fixed = re.sub('<a href=".+">', '', fixed)
   fixed = re.sub('</a>', '', fixed)
   return fixed


def ReadSource(cfg):
   try:
      input = open(cfg.infile, 'r')
   except:
      sys.stderr.write('Error opening file ' + inputfile + ' as input.\n')
      sys.exit(1)

   faq1 = {}

   i = 1
   for line in input.readlines():
      string.rstrip(line)
      x = string.split(line, '|')
      if len(x) < 3:
         print 'Error: ' + cfg.infile + ', line ' + str(i) + ': bad format'
         return
      i = i + 1
      category = x[0]

      # clean up answer text
      if not cfg.hasLinks:
         x[2] = FixSpecialText(x[2])

      # if the category exists, append the entry
      # otherwise add a new category, *then* append the entry.
      if faq1.has_key(category):
         faq1[category].append(FaqEntry(x[1:]))
      else:
         faq1[category] = []
         faq1[category].append(FaqEntry(x[1:]))
   input.close()
   return faq1


def PrintTimeStamp(cfg, out):
   out.write("%s%sFAQ Revised: %s%s%s\n" % (cfg.TS['Pre+'], cfg.TS['Pre'],
                                              time.asctime(time.localtime(time.time())),
                                              cfg.TS['Post'], cfg.TS['Post+']))


def PrintTOC(cfg, out, faq1):
   catlist = faq1.keys()

   # In early testing I had to reverse the keys
   #  catlist.reverse()

   out.write("%s%sTable of Contents%s%s\n" % (cfg.Head['Pre+'], cfg.Head['Pre'], \
                                              cfg.Head['Post'], cfg.Head['Post+']))

   out.write("%s%s\n" % (cfg.Sec['Pre+'], cfg.Sec['Pre']))
   i = 1
   for x in catlist:
      out.write("%s%s. %s%s\n" % (cfg.TOC['CatPre'],
                                  str(i), x,
                                  cfg.TOC['CatPost']))
      out.write("%s\n" % cfg.TOC['ListPre'])
      if cfg.hasLinks:
         for y in range(len(faq1[x])):
            out.write("%s%s%s%s%s.%s. %s%s\n" % (cfg.TOC['EntryPre'],
                                                 x, str(y),
                                                 cfg.TOC['EntryIn'],
                                                 str(i), str(y + 1),
                                                 faq1[x][y].question,
                                                 cfg.TOC['EntryPost']))
      else:
         for y in range(len(faq1[x])):
            out.write("%s%s%s.%s. %s%s\n" % (cfg.TOC['EntryPre'],
                                             cfg.TOC['EntryIn'],
                                             str(i), str(y + 1),
                                             faq1[x][y].question,
                                             cfg.TOC['EntryPost']))
            
      out.write("%s\n" % cfg.TOC['ListPost'])
      i = i + 1
   out.write("%s%s\n" % (cfg.Sec['Post'], cfg.Sec['Post+']))


def PrintQA(cfg, out, faq1, feedback):
   catlist = faq1.keys()

   i = 1
   for x in catlist:
      out.write("%s%s%s. %s%s%s\n" % (cfg.Head['Pre+'], cfg.Head['Pre'],
                                      str(i), x,
                                      cfg.Head['Post'], cfg.Head['Post+']))
      out.write("%s%s\n" % (cfg.Sec['Pre+'],
                            cfg.Sec['Pre']))

      for y in range(len(faq1[x])):
         if cfg.hasLinks:          
            out.write("%s%s%s%s%s.%s. %s%s\n" % (cfg.Q['Pre'],
                                                 x, str(y),
                                                 cfg.Q['In'],
                                                 str(i), str(y + 1),
                                                 faq1[x][y].question,
                                                 cfg.Q['Post']))
         else:
            out.write("%s%s%s.%s. %s%s\n" % (cfg.Q['Pre'],
                                             cfg.Q['In'],
                                             str(i), str(y + 1),
                                             faq1[x][y].question,
                                             cfg.Q['Post']))
         out.write("%s%s%s\n" % (cfg.A['Pre'],
                                 faq1[x][y].answer,
                                 cfg.A['Post']))
         if feedback == TRUE:
             sys.stdout.write('.')
      out.write("%s%s\n" % (cfg.Sec['Post'], cfg.Sec['Post+']))
      i = i + 1


def BuildFAQ(cfg, feedback):
   if cfg.outfile == "STDOUT":
      out = sys.stdout
      feedback = FALSE
   else:
      try:
         out = open(cfg.outfile, 'w')
      except:
         sys.stderr.write('Error opening file ' + cfg.outfile + ' for output, exiting.\n')
         sys.exit(1)

   faq = ReadSource(cfg)
   if not faq:
      return

   IncludeFile(out, cfg.headfile)
   PrintTimeStamp(cfg, out)
   PrintTOC(cfg, out, faq)

   if feedback == TRUE:
      print "\nWriting FAQ info to "+ cfg.outfile,

   PrintQA(cfg, out, faq, feedback)
   IncludeFile(out, cfg.footfile)

   if feedback == TRUE:
      print "done.\n"

def main():
   # flags
   do_nothing = FALSE
   verbose = FALSE
   feedback = TRUE

   # storage for user choices on command line
   user = {
      'cfg' : None,  # configuration instance
      'i'   : None,  # input filename
      'o'   : None,  # output filename
      't'   : None,  # header filename
      'b'   : None   # footer filename
      }

   # process the command line
   try:
      opts, args = getopt.getopt(sys.argv[1:], "hvnlc:i:o:t:b:")
   except getopt.error, msg:
      print 'Error: ' + msg
      sys.exit(2)
   for i in opts:
      if i[0] == '-h':
         print 'Usage: makefaq [-h] [-v] [-n] [-l] [-c config-name] [-i input-file] [-o output-file] [-t header-file] [-b footer-file]\n' + \
               '\n' + \
               'Hints:\n' + \
               '  - If you say "-v" (verbose), the default config settings will be displayed.\n' + \
               '  - Use -l to list all the configurations.\n' + \
               '  - Use -i, -o, -t, and -b to override config settings.\n' + \
               '  - You can say: "-o STDOUT".\n' + \
               '  - Use "-n" to test your config settings without doing anything.\n'
         sys.exit(0)
      elif i[0] =='-l':
         PrintConfigs()
         sys.exit(0)
      elif i[0] == '-v':
         verbose = TRUE
      elif i[0] == '-n':
         do_nothing = TRUE
      elif i[0] == '-c':
         user['cfg'] = FindConfig(i[1])
         if user['cfg'] == None:
            print 'Sorry, there is no configuration called ' + i[1]
            sys.exit(2)
      elif i[0] == '-i':
         user['i'] = i[1]
      elif i[0] == '-o':
         user['o'] = i[1]
      elif i[0] == '-t':
         user['t'] = i[1]
      elif i[0] == '-b':
         user['b'] = i[1]

   # commit the user choices; we do it this way to ensure that cfg
   # gets set before we try to set its attributes
   if user['cfg']:
      cfg = user['cfg']
   else:
      cfg = DefaultConfig()
   if user['i']:
      cfg.infile = user['i']
   if user['o']:
      cfg.outfile = user['o']
   if user['t']:
      cfg.headfile = user['t']
   if user['b']:
      cfg.footfile = user['b']

   if verbose:
      cfg.show()
   if do_nothing:
      print 'Skipping file processing.'
   else:
      BuildFAQ(cfg, feedback)


# Start main loop

if __name__ == '__main__':

   main()
