#!/usr/bin/env python
#
# Doxer - Software documentation system
# Copyright (C) 2005-2006 Tim Janik
#
# qcomment.py - subshelled from Doxygen, this is used to rip comments out of source files
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
qcommentfilter_dir = os.path.dirname (os.path.abspath (os.path.realpath (sys.argv[0])))
sys.path.append (qcommentfilter_dir)    # allow loading of modules from installation dir

debugging_on = os.environ.get ('DOXER_QCOMMENT_CONFIG', '').find (':debug:') >= 0
def debug (*args):
  if not debugging_on:
    return
  import sys
  line = ""
  for s in args:
    if line:
      line += " "
    line += str (s)
  sys.stderr.write ("|> " + line + "\n")

class CommentRegistry:
  def __init__ (self, seed = None):
    self.count = 0
    self.seed = seed
    self.comments = {}
  def add (self, comment_text, fname, fline):
    key = '____doxer_RegisteredComment_s%s_%u___' % (str (self.seed), 1 + len (self.comments))
    self.comments[key] = (comment_text, fname, fline)
    return key
  def flush (self, file):
    import cPickle
    cPickle.dump (self.comments, file)

class CommentTransformer:
  def __init__ (self, fin, cregistry, fname):
    self.fin = fin
    self.fname = fname
    self.buffer = ''
    self.cbuffer = ''
    self.feed_specific = self.feed_text
    self.cregistry = cregistry
    self.fline = 1
    self.start_line = 0
  def feed_text (self, char):
    self.buffer += char
    if char == '*' and self.buffer[-3:] in ('/**', '/*!'):
      self.start_line = self.fline
      self.feed_specific = self.feed_ccomment
    elif char == '<' and self.buffer[-4:] in ('///<', '//!<'):
      self.start_line = self.fline
      self.feed_specific = self.feed_scomment
    #elif char == '<' and self.buffer[-3:] == '/*<':
    #  self.buffer[-1] = '*'
    #  self.buffer += '<'        # for doxygen, fake '/**<' from  '/*<'
    #  self.start_line = self.fline
    #  self.feed_specific = self.feed_ccomment
    #elif char == '<' and self.buffer[-3:] == '//<':
    #  self.buffer[-1] = '/'
    #  self.buffer += '<'        # for doxygen, fake '///<' from  '//<'
    #  self.start_line = self.fline
    #  self.feed_specific = self.feed_scomment
    elif char == '\n':
      sys.stdout.write (self.buffer)
      self.buffer = ''
  def feed_ccomment (self, char):
    if not self.cbuffer and char == '<':
      self.buffer += '<'        # leave '/**<' and  '/*!<' comments intact
      return
    self.cbuffer += char
    if char == '\n':
      self.buffer += '\n'
    elif char == '/' and self.cbuffer[-2:] == '*/':
      self.buffer += ' ' + self.cregistry.add (self.cbuffer[:-2], self.fname, self.start_line) + ' '
      self.buffer += '*/'
      self.feed_specific = self.feed_text
      self.cbuffer = ''
  def feed_scomment (self, char):
    self.cbuffer += char
    if char == '\n':
      self.buffer += ' ' + self.cregistry.add (self.cbuffer[:-1], self.fname, self.start_line) + ' '
      self.buffer += '\n'
      self.feed_specific = self.feed_text
      self.cbuffer = ''
  def feed (self, char):
    self.feed_specific (char)
  def feed_line (self, lstr):
    for c in lstr:
      self.feed (c)
    self.fline += 1

# --- open comment registry ---
try:
  comment_registry_base = os.environ['DOXER_QCOMMENT_DUMP']
except:
  comment_registry_base = None
comment_registry_file = None
comment_registry_count = 0
if comment_registry_base:
  while not comment_registry_file:
    try:
      comment_registry_count += 1
      fname = comment_registry_base + (comment_registry_count and str (comment_registry_count) or "")
      comment_registry_file = os.fdopen (os.open (fname, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0644), "w")
    except OSError, ose:
      import errno
      if ose.errno != errno.EEXIST:
        raise

# --- command line help ---
def print_help (with_help = True):
  print "qcomment.py - part of doxer.py"
  if not with_help:
    return
  print "Usage: %s [options] {file} " % os.path.basename (sys.argv[0])
  print "Options:"
  print "  --help, -h                print this help message"

# --- filter all input ---
comment_registry = None
for arg in sys.argv[1:]:
  if arg == "--help" or arg == "-h":
    print_help()
    sys.exit (0)
  else:
    if not comment_registry:
      comment_registry = CommentRegistry (comment_registry_count)
    debug ('QCommentFilter: %s' % arg)
    fin = open (arg, 'r')
    ct = CommentTransformer (fin, comment_registry, arg)
    for lstr in fin:
      ct.feed_line (lstr)
    fin.close()
# --- dump comment registry ---
if comment_registry_file:
  comment_registry.flush (comment_registry_file)
  comment_registry_file.close()
