#!/usr/bin/env python2.4
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
import os, sys, re, datetime, Config, Data
def debug (*args): Config.debug_print (*args)

# --- doxer commands ---
command_dict = {
  'doxer_newline':              0,
  'doxer_line':                 Data.SPAN_LINE,
  'doxer_alias':                Data.SPAN_LINE,
  'include':                    Data.SPAN_LINE,
  'doxer_dnl':                  Data.SPAN_LINE,
  'doxer_args':                 0,
  'doxer_set':                  Data.SPAN_LINE,
  'doxer_add':                  Data.SPAN_LINE,
  'doxer_get':                  0,
  'doxer_msg':                  Data.NESTING,
  'doxer_done':                 Data.SPAN_LINE,
  'doxer_command':              Data.SCOPED | Data.SPAN_LINE,
  'doxer_visible':              Data.SCOPED,
  'doxer_hidden':               Data.SCOPED,
  'doxer_table':                Data.SCOPED | Data.SPAN_LINE,
  'doxer_row':                  Data.SCOPED,
  'doxer_cell':                 0,
  'doxer_list':                 Data.SCOPED,
  'doxer_deflist':              Data.SCOPED,
  'doxer_item':                 Data.SPAN_LINE,
  'doxer_div':       	        Data.SCOPED,
  'doxer_start_section':        Data.SECTIONED,
  'doxer_sub':                  Data.NESTING,
  'doxer_cmatch':               Data.NESTING,
  'doxer_anchor':               0,
  'doxer_template_hook':        0,
# layout commands
  'doxer_definition':           Data.SPAN_WORD | Data.CLAIM_PARA,
  'doxer_parameter':            Data.SPAN_WORD | Data.CLAIM_PARA,
  'doxer_flush_parameters':     0,
  'doxer_hseparator':           Data.SPAN_LINE,
  'doxer_title_tail':           Data.SPAN_LINE,
  'doxer_center':       	Data.NESTING,
  'doxer_bold':         	Data.NESTING,
  'doxer_italic':       	Data.NESTING,
  'doxer_underline':       	Data.NESTING,
  'doxer_monospace':    	Data.NESTING,
  'doxer_strikethrough':	Data.NESTING,
  'doxer_underline':    	Data.NESTING,
  'doxer_subscript':    	Data.NESTING,
  'doxer_superscript':  	Data.NESTING,
  'doxer_fontsize':     	Data.NESTING,
  'doxer_span':       	        Data.NESTING,
  'doxer_uri':       	        0,
  'doxer_longuri':     	        0,
  'doxer_image':       	        0,
  'doxer_raw':       	        Data.NESTING | Data.QUOTABLE,
  'doxer_preformatted':	        Data.NESTING | Data.QUOTABLE,
}

# --- identifier definition constants ---
IDENTIFIER_CHARS = set ("-_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
TERM_EOS, TERM_BEFORE_NEWLINE, TERM_AT_NEWLINE, TERM_AFTER_NEWLINE, TERM_ARG_SEP, TERM_DOXER_DONE = range (6)
def term_name (term_type):
  return {
    TERM_EOS:                   'Stream End',
    TERM_BEFORE_NEWLINE:        'Newline',
    TERM_AT_NEWLINE:            'Newline',
    TERM_AFTER_NEWLINE:         'Newline',
    TERM_ARG_SEP:               'Closing brace (terminating arguments)',
    TERM_DOXER_DONE:            '@doxer_done directive',
    }[term_type]

# --- Parsing Errors ---
class MiscError (RuntimeError):
  def __init__ (self, fname, fline, msg):
    RuntimeError.__init__ (self, '%s:%d: %s' % (fname, fline, msg))
class ParseError (MiscError):
  def __init__ (self, fname, fline, msg):
    MiscError.__init__ (self, fname, fline, msg)
class MacroError (ParseError):
  def __init__ (self, macro, msg):
    assert isinstance (macro, Data.TextNode)
    ParseError.__init__ (self, macro.fname, macro.fline, msg)

# --- simple date parsing and formatting ---
rfc2822_months = ( 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec' )
def datetime_parse (string, fname = '?', fline = 0):
  if re.match (r'^\D*\d{4}[-/]\d{1,2}[-/]\d{1,2}\D+\d+:\d+:\d+(\D\d*)*$', string): # "2005/09/16 19:28:29", "2005-09-16T19:28:29.215369"
    numstrings = re.findall (r'\d+', string)
    nums = [int (n) for n in numstrings]
    return datetime.datetime (year = nums[0], month = nums[1], day = nums[2], hour = nums[3], minute = nums[4], second = nums[5])
  if re.match (r'^\D*\d+:\d+:\d+\D+\d{4}[-/]\d{1,2}[-/]\d{1,2}\D*$', string): # "19:28:29 2005/09/16", "19:28:29 2005-09-16"
    numstrings = re.findall (r'\d+', string)
    nums = [int (n) for n in numstrings]
    return datetime.datetime (year = nums[3], month = nums[4], day = nums[5], hour = nums[0], minute = nums[1], second = nums[2])
  mpat = '|'.join (rfc2822_months)
  mo = re.match (r'^\D*(\d+)\s+(' + mpat + r')\s+(\d+)\s+(\d+):(\d+):(\d+)\s+(?:([+-])(\d\d)(\d\d))?$', string) # "Sat, 13 Jan 2007 19:16:08 +0200"
  if mo: # rfc2822 date
    g = list (mo.groups())
    g[1] = 1 + list (rfc2822_months).index (g[1])
    g[6] = g[6] == '-' and -1 or +1     # tz sign
    g[7] = g[7] or 0                    # default tz hours
    g[8] = g[8] or 0                    # default tz minutes
    g = [int (n) for n in g]
    tzd = datetime.timedelta (hours = g[7], minutes = g[8]) * g[6]
    return datetime.datetime (year = g[2], month = g[1], day = g[0], hour = g[3], minute = g[4], second = g[5]) - tzd
  raise ParseError (fname, fline, "Failed to parse date: %s" % string)

def datetime_format (datetime_obj):
  # Sun Jul 31 13:35:47 2005
  day = ( 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun' )
  string = day[datetime_obj.weekday()] + ' ' + rfc2822_months[datetime_obj.month - 1] + ' '
  return string + datetime_obj.strftime ("%02d %02H:%02M:%02S %04Y")
  return datetime_obj.strftime ("%04Y-%02m-%02d %02H:%02M:%02S")

def datetime_isoformat (datetime_obj):
  return datetime_obj.isoformat ()

# --- convert tokens back to text ---
def reescape_quotable_string (string):
  escaped, squotes, dquotes = False, False, False
  result = ""
  for ch in list (string):
    if escaped:
      result += ch
    elif ch == '"' and not squotes:
      result += ch
      dquotes = not dquotes
    elif ch == "'" and not dquotes:
      result += ch
      squotes = not squotes
    elif ch == '\\' and (squotes or dquotes):
      result += ch
      escaped = True
    elif ch == '@' and not (squotes or dquotes):
      result += '@@'
      escaped = True
    else:
      result += ch
  return result
def internal_text_from_token_list (token_list, reconstruct_escapes_and_macros, command_dict, quotable):
  result = ''
  for tok in token_list:
    if isinstance (tok, str):
      if reconstruct_escapes_and_macros and not quotable:
        result += re.sub ('@', '@@', tok)
      elif reconstruct_escapes_and_macros and quotable:
        result += reescape_quotable_string (tok)
      else:
        result += tok
    else: # handle macros
      macro_flags = 0
      if reconstruct_escapes_and_macros:
        result += '@' + tok.name
        macro_flags = command_dict.get (tok.name, 0)
      if reconstruct_escapes_and_macros and tok.arg:
        result += '{' + internal_text_from_token_list (tok.arg, reconstruct_escapes_and_macros, command_dict, macro_flags & Data.QUOTABLE) + '}'
      elif tok.arg and not reconstruct_escapes_and_macros:
        result += internal_text_from_token_list (tok.arg, reconstruct_escapes_and_macros, command_dict, False)
      result += internal_text_from_token_list (tok.title, reconstruct_escapes_and_macros, command_dict, False)
      result += internal_text_from_token_list (tok.contents, reconstruct_escapes_and_macros, command_dict, False)
  return result
def plain_text_from_token_list (token_list):
  return internal_text_from_token_list (token_list, False, {}, False)
def parsable_text_from_token_list (token_list, command_dict):
  return internal_text_from_token_list (token_list, True, command_dict, False)

def split_arg_from_token_list (token_list):     # returns ( "arg", [ rest_tokens ] )
  def arg_split_string (string):
    cpos = string.find (',')
    if cpos >= 0:
      return string[:cpos], string[cpos+1:]
    return None
  tstack = token_list[:]
  arg, rest, unsplit = [], [], True
  # read up first arg
  for tok in token_list:
    if unsplit and isinstance (tok, str):
      pair = arg_split_string (tok)
      if pair:
        unsplit = False
        if pair[0]:
          arg += [ pair[0] ]
        if pair[1]:
          rest += [ pair[1] ]
      else:
        arg += [ tok ]
    elif unsplit:
      arg += [ tok ]
    else:
      rest += [ tok ]
  return arg, rest

def token_list_join_strings (token_list):
  # join consecutive strings
  joined_list = []
  for tok in token_list:
    if not isinstance (tok, str):
      if tok.name != 'doxer_dnl':
        joined_list += [ tok ]
    elif joined_list and isinstance (joined_list[-1], str):
      joined_list[-1] += tok
    else:
      joined_list += [ tok ]
  return joined_list

def split_arg_tokens (token_list):
  token_stack = token_list_join_strings (token_list)
  array = [ [] ]
  while token_stack:
    tok = token_stack.pop (0)
    if not isinstance (tok, str):
      array[-1] += [ tok ]
      continue
    cpos = tok.find (',')
    if cpos < 0:
      array[-1] += [ tok ]
      continue
    astr = tok[:cpos]
    rest = tok[cpos + 1:]
    array[-1] += [ astr ]
    if rest or not token_stack:
      token_stack = [ rest ] + token_stack
    array += [ [] ]
  if array[-1] == []:
    array.pop()
  return array # array of token lists

def parse_range_string (range_spec):
  isrange, rfirst, rlast = False, None, None
  range_spec = range_spec.strip()
  if range_spec == '*':         # special [1:] range support
    return (True, 1, None)
  if range_spec and range_spec[0] in "-0123456789":
    rfirst = 0
    negate = False
    while range_spec and range_spec[0] in "-0123456789":
      if range_spec[0] == '-':
        negate = not negate
      else:
        rfirst = rfirst * 10 + int (range_spec[0])
      range_spec = range_spec[1:]
    if negate:
      rfirst = -rfirst
  if range_spec and range_spec[0] == ':':
    isrange = True
    range_spec = range_spec[1:]
  if range_spec and range_spec[0] in "-0123456789":
    rlast = 0
    negate = False
    while range_spec and range_spec[0] in "-0123456789":
      if range_spec[0] == '-':
        negate = not negate
      else:
        rlast = rlast * 10 + int (range_spec[0])
      range_spec = range_spec[1:]
    if negate:
      rlast = -rlast
  if not isrange:
    if rfirst != None:
      rlast = rfirst + 1
      isrange = True
    elif rlast != None:
      rfirst = rlast - 1
      isrange = True
  return (isrange, rfirst, rlast)

def expand_doxer_args (args_kind, macro):
  args_kind = args_kind.strip()
  isrange, rfirst, rlast = parse_range_string (args_kind)
  token_list = []
  # substitute arg
  if isrange:
    array = split_arg_tokens (macro.arg)        # 0..n are args
    array = [ [ macro.name ] ] + array          # 0 is name, 1..n+1 are args
    if rfirst == None and rlast == None:
      aslice = array[:]
    elif rfirst == None:
      aslice = array[:rlast]
    elif rlast == None:
      aslice = array[rfirst:]
    else:
      aslice = array[rfirst:rlast]
    tokens = []
    for alist in aslice:
      if not alist:
        continue
      if tokens:
        tokens += [ ',' ]
      tokens += alist
    token_list += tokens
  # '@doxer_args{all}' for all args, line and contents
  # '@doxer_args{text}' for line and contents
  if args_kind == 'all':
    token_list += macro.arg
  if args_kind == 'line' or args_kind in ('all', 'text'):
    token_list += macro.title
  if args_kind == 'contents' or args_kind in ('all', 'text'):
    token_list += macro.contents
  return token_list

def list_expand_args (token_list, macro):
  result = []
  for tok in token_list:
    if isinstance (tok, str):
      result += [ tok ]
    elif tok.name == 'doxer_args':
      if tok.arg:
        result += expand_doxer_args (plain_text_from_token_list (tok.arg), macro)
    else:
      node = tok.clone_macro()
      node.arg = list_expand_args (node.arg, macro)
      node.title = list_expand_args (node.title, macro)
      node.contents = list_expand_args (node.contents, macro)
      if node.name in ('doxer_add', 'doxer_set', 'doxer_get'):
        result += doxer_setget (node)
      else:
        result += [ node ]
  return result

def doxer_setget (macro, env_variables = None):
  fname, fline = macro.fname, macro.fline
  if not env_variables:
    env_variables = macro.root.variables
  if not macro.arg:
    raise MiscError (fname, fline, '%s: missing variable name' % macro.name)
  atext = plain_text_from_token_list (macro.arg)
  cpos = atext.find (',')
  if cpos >= 0:
    name = atext[:cpos].strip()
    atext = atext[cpos + 1:]
  else:
    name = atext.strip()
    atext = ''
  # FIXME: filter non-identifier chars?
  if not name:
    raise MiscError (fname, fline, '%s: missing variable name' % macro.name)
  if macro.name in ('doxer_add', 'doxer_set'):
    if (env_variables.has_key (name) and
        (macro.name == 'doxer_set' or
         (macro.name == 'doxer_add' and
          not isinstance (env_variables[name], tuple)))):
      raise MacroError (macro, '%s: variable already assigned: %s' % (macro.name, name))
    val_unstripped = plain_text_from_token_list (macro.title)
    val = val_unstripped.strip()
    for word in re.split (',', atext):
      flags = plain_text_from_token_list (word).strip()
      if flags.find ('parse_date') >= 0:
        date = datetime_parse (val.strip(), macro.fname, macro.fline)
        val = datetime_format (date)
      elif flags.find ('unstripped') >= 0:
        val = val_unstripped
      elif flags.find ('git_author_date') >= 0:
        import os, subprocess
        sfile = env_variables['source-file']    # absolute
        sfile = os.path.split (sfile)           # (dir, file)
        p1 = subprocess.Popen (['git-log', '-n1', '--pretty=format:%aD', sfile[1]], stdout = subprocess.PIPE, cwd = sfile[0])
        val_unstripped = p1.stdout.readline().strip()
        val = val_unstripped.strip()
    if macro.name == 'doxer_add':
      head = env_variables.get (name)
      env_variables[name] = (head and head or ()) + (val,)
    else:
      env_variables[name] = val
  elif macro.name == 'doxer_get':
    text = env_variables.get (name)
    if text:
      return [ text ]
  return []

# --- input stream file handle ---
class IStreamFile:
  fhandle = None
  fiter = None
  def __init__ (self, fname, istring = ''):
    self.fname = fname
    self.fline = 0
    if istring:
      self.fiter = filter (None, re.split('([^\n]*\n)', istring)).__iter__()
    elif fname:
      self.fhandle = open (fname, 'r')
      self.fiter = self.fhandle.__iter__()
    self.buffer = ''
    self.eos_reached = False
  def get_line (self):
    if self.buffer:
      b = self.buffer
      self.buffer = ''
      return b
    try:
      buffer = self.fiter.next()
      self.fline += 1
    except:
      buffer = ''
      self.eos_reached = True
    return buffer
  def clone_with_buffer (self, buffer):
    ifile = IStreamFile ('')
    ifile.fname = self.fname
    ifile.fline = self.fline
    ifile.buffer = buffer
    ifile.eos_reched = not buffer
    return ifile
  def mtime (self):
    try:
      return datetime.datetime.fromtimestamp (os.path.getmtime (self.fname))
    except:
      return datetime.datetime.now()
  def close (self):
    if self.fhandle:
      self.fhandle.close()

# --- input character stream ---
class IStream:
  last_fname = '?'
  last_fline = 0
  def __init__ (self):
    self.fstack = []
    self.buffer = ''
  def save_buffer (self):
    if self.buffer:
      if not self.fstack:
        raise self.Error ('Missing input stack for put-back')
      ifile = self.fstack[-1].clone_with_buffer (self.buffer)
      self.buffer = ''
      self.fstack += [ ifile ]
  def push_input (self, fname):
    self.save_buffer()
    ifile = IStreamFile (fname)
    self.fstack += [ ifile ]
  def push_input_string (self, istring):
    self.save_buffer()
    ifile = IStreamFile ('<inline>', istring)
    self.fstack += [ ifile ]
  def put_back (self, text):
    # catch "recursion" errors
    if self.put_backs > 999:
      raise self.Error ('Too many alias substitutions')
    self.buffer = text + self.buffer
    self.put_backs += 1
  def eos (self):
    return not self.buffer and not self.fstack
  def pop_fstack (self):
    self.last_fname = self.fname()
    self.last_fline = self.fline()
    ifile = self.fstack.pop()
    ifile.close()
  def next_line (self):
    self.put_backs = 0
    while self.fstack:
      self.buffer = self.fstack[-1].get_line()
      if self.buffer:
        return
      self.pop_fstack() # closes eof files
  def peek_char (self):
    if not self.buffer:
      self.next_line()
    if self.buffer:
      return self.buffer[0]
    return ''
  def pop_char (self):
    if not self.buffer:
      self.next_line()
    if self.buffer:
      char = self.buffer[0]
      self.buffer = self.buffer[1:]
      return char
    return ''
  def mtime (self):
    return self.fstack and self.fstack[-1].mtime() or datetime.datetime.now()
  def fname (self):
    return self.fstack and self.fstack[-1].fname or '?'
  def fline (self):
    return self.fstack and self.fstack[-1].fline or 0
  def set_file_line (self, fname, fline):
    if self.fstack:
      self.fstack[-1].fname = fname
      self.fstack[-1].fline = fline - 1 # next line read will fline
  def Error (self, msg):
    if self.fstack:
      return ParseError (self.fname(), self.fline(), msg)
    else:
      return ParseError (self.last_fname, self.last_fline, msg)
  def MiscError (self, msg):
    if self.fstack:
      return MiscError (self.fname(), self.fline(), msg)
    else:
      return MiscError (self.last_fname, self.last_fline, msg)

# --- basic text & macro parser ---
class DoxiParser:
  atescapes = '\n\r'
  atatescapes = atescapes + '@'
  anyatatescapes = atatescapes + '{}'
  def __init__ (self, fname, output_file, init_variables = {}, template_text = ''):
    self.alias_dict = {}
    self.command_protection = 0
    self.template_text = template_text
    nowstring = datetime_isoformat (datetime.datetime.now())
    self.variables = {
      'source-file'             : fname,
      'source-basename'         : os.path.basename (fname),
      'destination-file'        : output_file,
      'destination-basename'    : os.path.basename (output_file),
      'today'                   : nowstring,
      'mtime'                   : nowstring,
    }
    self.variables.update (init_variables)
    self.command_dict = command_dict.copy()
  def read_literal (self, istream, endmarks):
    text = ''
    while not istream.peek_char() in endmarks:
      ch = istream.pop_char()
      if ch == '@' and istream.peek_char() in ' ' + self.atescapes:
        text += istream.pop_char()
      else:
        text += ch
    return text
  def parse_stream (self, istream, term, quotation_and_nesting = 0):
    self.variables['mtime'] = datetime_isoformat (istream.mtime())
    def add_text (toklist, text):
      if not len (toklist) or not isinstance (toklist[-1], str):
        toklist += [ '' ]
      toklist[-1] += text
    # parse until termination mark
    ignore_braces = 0
    token_list = []
    while not istream.eos():
      c = istream.peek_char()
      if quotation_and_nesting & Data.QUOTABLE and c in '"\'':
        quot = istream.pop_char()
        add_text (token_list, quot)
        quotescaped = False
        c = istream.peek_char()
        while c != quot or quotescaped:
          c = istream.pop_char()
          add_text (token_list, c)
          # quotescaped = c == '\\'
          c = istream.peek_char()
        c = istream.pop_char() # quot
        add_text (token_list, c)
        continue
      if term == TERM_ARG_SEP and c == '}':
        if ignore_braces:
          ignore_braces -= 1
        else:
          return token_list
      if term == TERM_ARG_SEP and c == '{' and quotation_and_nesting & Data.NESTING:
        ignore_braces += 1
      if c == '\n' and term == TERM_BEFORE_NEWLINE:     # leave \n in input queue
        return token_list
      c = istream.pop_char()
      if c == '@' and istream.peek_char() in self.anyatatescapes:
        c = istream.pop_char()
        add_text (token_list, c)
      elif c == '@':
        tnode = Data.TextNode (istream.fname(), istream.fline())
        result_list = self.parse_node (tnode, istream)
        token_list += result_list
        if result_list and term in (TERM_BEFORE_NEWLINE, TERM_AT_NEWLINE, TERM_AFTER_NEWLINE):
          last = result_list[-1]
          if isinstance (last, Data.TextNode) and last.flags & Data.SPAN_LINE_MASK:
            return token_list
        if result_list and term == TERM_DOXER_DONE:
          last = result_list[-1]
          if isinstance (last, Data.TextNode) and last.name == 'doxer_done':
            return token_list[:-1]
      else:
        if c == '\n' and term == TERM_AT_NEWLINE:       # forget the terminating \n
          return token_list
        add_text (token_list, c)
        if c == '\n' and term == TERM_AFTER_NEWLINE:
          return token_list                             # include \n in result
    if term != TERM_EOS:
      raise istream.Error ('Premature end of input; expecting: %s' % term_name)
    return token_list
  def parse_node (self, macro, istream):
    # special case newline and space shorthands
    if istream.peek_char() == '*':
      istream.pop_char()
      macro.name = 'doxer_newline'
      return [ macro ]
    if istream.peek_char() == ' ':
      istream.pop_char()
      return [ ' ' ]
      macro.name = 'doxer_monospace'
      macro.arg = [ ' ' ]
      return [ macro ]
    # parse name
    while istream.peek_char() in IDENTIFIER_CHARS:
      macro.name += istream.pop_char()
    # check macro name, assign flags
    if not macro.name:
      raise istream.Error ("Missing macro name around '@'")
    if not self.alias_dict.has_key (macro.name):
      if self.command_dict.has_key (macro.name):
        macro.flags = self.command_dict[macro.name]
      else:
        raise istream.Error ("Unknown macro: %s" % macro.name)
    # handle special commands
    if macro.name == 'doxer_alias':
      return self.substitute (macro, istream)
    elif macro.name == 'doxer_dnl':
      return self.dnl (macro, istream)
    elif macro.name == 'doxer_line':
      return self.parse_doxer_line (macro, istream)
    elif macro.name == 'include':
      return self.parse_include (macro, istream)
    # parse args
    if istream.peek_char() == '{':
      lastsep = istream.pop_char()
      assert lastsep == '{'
      while lastsep != '}':
        argtree = self.parse_stream (istream, TERM_ARG_SEP, macro.flags)
        macro.arg += argtree
        lastsep = istream.pop_char()
    # handle aliases
    if self.alias_dict.has_key (macro.name):
      return self.trigger_alias (macro, istream)
    # read span_line and span_word tokens
    if macro.flags & Data.SPAN_LINE_MASK:
      macro.title = self.parse_stream (istream, TERM_AT_NEWLINE)
    elif macro.flags & Data.SPAN_WORD:
      macro.title = self.parse_word (istream, '@' + macro.name)
    # handle command definitions
    if macro.name == 'doxer_command':
      if self.command_protection:
        raise istream.Error ("Invalid recursive command definition: %s" % plain_text_from_token_list ([ macro ]).strip())
      return self.parse_command (macro, istream)
    # call builtins
    handler = self.builtin_macros.get (macro.name)
    if handler:
      method = handler.__get__ (self)
      return method (macro, istream)
    # parse scoped macros
    if macro.flags & Data.SCOPED:
      return self.parse_scope_rest (macro, istream)
    # special template hook
    if macro.name == 'doxer_template_hook' and self.template_text:
      istream.push_input_string (self.template_text)
    # add macro definition to token tree
    return [ macro ]
  def parse_word (self, istream, directive):
    # eat up whitespace
    while istream.peek_char() in "\t ":
      istream.pop_char()
    word = ''
    letters = set ('0123456789abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ-+!$%&*/<=>?^~.:@') # for scheme: + '@'
    while istream.peek_char() in letters:
      word += istream.pop_char()
    if not word:
      raise istream.Error ("Missing word after directive: %s" % directive)
    return word
  def parse_doxer_line (self, tnode, istream):
    # eat up whitespace
    while istream.peek_char() in "\t ":
      istream.pop_char()
    # read up digits
    numstr = ''
    while istream.peek_char() in '0123456789':
      numstr += istream.pop_char()
    if not numstr:
      raise istream.Error ('Missing line number')
    # eat up whitespace
    while istream.peek_char() in "\t ":
      istream.pop_char()
    # read up to newline
    itext = ''
    while istream.peek_char() != '\n':
      itext += istream.pop_char()
    # eat newline
    istream.pop_char()
    # figure filename
    fname = itext.strip()
    if not fname or len (fname) < 2:
      raise istream.Error ('Missing input file name')
    if not (fname[0] == '"' and fname[-1] == '"'):
      raise istream.Error ('Invalid input file name')
    fname = fname[1:-1]
    if fname.find ('"') != -1:
      raise istream.Error ('Invalid input file name')
    istream.set_file_line (fname, int (numstr))
    return []
  def parse_include (self, tnode, istream):
    # parse @include directive up to line end
    tokens = self.parse_stream (istream, TERM_AT_NEWLINE)
    itext = plain_text_from_token_list (tokens)
    # figure filename
    fname = itext.strip()
    #if not fname or len (fname) < 2:
    #  raise istream.Error ('Missing include file name')
    if (not fname or not
        ((fname[0] == '"' and fname[-1] == '"') or
         (fname[0] == '<' and fname[-1] == '>'))):
      raise istream.Error ('Invalid include directive: @include%s' % itext)
    stdinc = fname[0] == '<'
    fname = fname[1:-1]
    if fname.find ('"') + fname.find ('>') + fname.find ('>') != -3:
      raise istream.Error ('Invalid include directive')
    if os.path.isabs (fname):
      fullname = fname
      access_ok = os.access (fullname, os.R_OK)
    else:
      if stdinc:
        plist = [Config.installdir,]
      else:
        plist = [Config.srcdir,]
      for d in Config.includes:
        if d[0:1] == '/':
          plist += [ d ]
        else:
          plist += [ os.path.join (Config.srcdir, d) ]
      access_ok = False
      for p in plist:
        fullname = os.path.join (p, fname)
        if os.access (fullname, os.R_OK):
          access_ok = True
          break
    if not access_ok:
      raise istream.MiscError ('No such include file: %s' % fname)
    istream.push_input (fullname)
    return []
  def parse_new_command_name (macro):
    cmdname = macro.title and macro.title[0].strip() or ''
    if not cmdname:
      raise MacroError (macro, "Missing command name")
    for c in cmdname:
      if not c in IDENTIFIER_CHARS:
        raise MacroError (macro, "Invalid command name: %s" % cmdname)
    return cmdname
  parse_new_command_name = staticmethod (parse_new_command_name)
  def parse_command (self, macro, istream):
    cflags = 0
    astring = plain_text_from_token_list (macro.arg).strip()
    f = Data.custom_command_flags_dict.get (astring)
    if f:
      cflags |= f
    elif astring:
      raise istream.Error ('Invalid command flag: %s' % astring)
    cmdname = self.parse_new_command_name (macro)
    if self.command_dict.has_key (cmdname):
      raise istream.Error ("Command already defined: %s" % cmdname)
    self.command_dict[cmdname] = cflags
    self.command_protection += 1
    toklist = self.parse_scope_rest (macro, istream)
    self.command_protection -= 1
    return toklist
  def parse_scope_rest (self, macro, istream):
    assert not macro.contents
    assert macro.flags & Data.SCOPED
    macro.contents = self.parse_stream (istream, TERM_DOXER_DONE)
    # add macro definition to token tree
    return [ macro ]
  def substitute (self, macro, istream):
    alias_name = ''
    # parsed '@doxer_alias' so far
    if istream.pop_char() != '{':
      raise istream.Error ('Invalid alias definition')
    while istream.peek_char() in IDENTIFIER_CHARS:
      alias_name += istream.pop_char()
    if istream.pop_char() != '}':
      raise istream.Error ('Invalid alias definition')
    # check proper name
    if not alias_name or not istream.peek_char() in "\t \n":
      raise istream.Error ('Invalid alias definition')
    # eat up whitespace
    while istream.peek_char() in "\t ":
      istream.pop_char()
    # read up to newline
    alias_text = self.read_literal (istream, '\n')
    # eat newline
    ch = istream.pop_char()
    assert ch == '\n'
    self.alias_dict[alias_name] = alias_text
    return []
  def dnl (self, macro, istream):
    # read up to newline
    title_text = self.read_literal (istream, '\n')
    macro.title = [ title_text ]
    # eat newline
    ch = istream.pop_char()
    assert ch == '\n'
    return [ macro ]
  def doxer_setget (self, macro, istream):
    if self.command_protection:
      return [ macro ]          # defer execution
    return doxer_setget (macro, self.variables)
  def text_expand_args_reescape (self, text, macro):
    assert isinstance (text, str)
    result = ''
    # perform argument substitution
    ltext = list (text)
    while ltext:
      if ltext[:12] != list ('@doxer_args{'):
        ch = ltext.pop (0)
        result += ch
        if ch == '@' and ltext:
          ch = ltext.pop (0)
          result += ch
        continue
      ltext = ltext[12:]
      # got '@doxer_args{', now parse '123}'
      anum = ''
      ch = ltext.pop (0)
      while ch != '}':
        anum += ch
        ch = ltext.pop (0)
      # substitute args
      tok_list = expand_doxer_args (anum, macro)
      result += parsable_text_from_token_list (tok_list, self.command_dict)
    return result
  def trigger_alias (self, macro, istream):
    atext = self.alias_dict[macro.name]
    # perform argument substitution
    result = self.text_expand_args_reescape (atext, macro)
    # reparse text
    istream.put_back (result)
    return []
  builtin_macros = {
    'doxer_add'         : doxer_setget,
    'doxer_set'         : doxer_setget,
    'doxer_get'         : doxer_setget,
  }

# --- mark paragraph breaks ---
class LineBreaker:
  def __init__ (self, fname):
    self.fname = fname
    self.fline = 1
  def process_list (self, token_list, following_macro = False, inside_content = 0):
    after_title_break = inside_content
    following_done_statement = 0
    # join consecutive strings
    joined_list = token_list_join_strings (token_list)
    # inside sections and scopes, we ignore trailing and
    # non-explicit newlines and whitespaces
    if inside_content and joined_list and isinstance (joined_list[-1], str):
      joined_list[-1] = re.sub (r'\s*$', '', joined_list[-1])
    token_list = joined_list
    # convert/generate line breaks
    result = []
    for tok in token_list:
      if isinstance (tok, str):
        # in order to consider scoped macros part of the normal text flow,
        # we ignore macro special casing conditions after scoped macros
        if following_done_statement:
          after_title_break = False             # do not ignore newlines after '@done'
          tok = '\n' + tok
          following_done_statement = 0
        lines = re.split (r'(\n\s*\n)', tok)    # split up empty lines
        for l in lines:
          if not l:
            continue
          ncount = l.count ('\n')
          self.fline += ncount
          if ncount >= 2 and not l.strip() and not after_title_break:
            tnode = Data.TextNode (self.fname, self.fline, 'doxer_newline')
            tnode.arg = [ 'automatic_newline' ]
            tnode.contents = [ l ]
            result += [ tnode ]
            if not following_macro or ncount >= 3:
              tnode = Data.TextNode (self.fname, self.fline, 'doxer_newline')
              # preserve explicit empty lines in text flow
              result += [ tnode ]
            following_macro = True
          else:
            result += [ l ]
            if l.strip():       # preserve flag across spaces
              following_macro = False
              after_title_break = False
      else:
        if not tok.flags & Data.QUOTABLE:
          tok.arg = self.process_list (tok.arg, True)
        tok.title = self.process_list (tok.title, True)
        tok.contents = self.process_list (tok.contents, False, True)
        result += [ tok ]
        self.fline = tok.fline
        after_title_break = self.test_title_break (tok)
        following_macro = True
        following_done_statement = bool (tok.flags & Data.SCOPED)
    return result
  def test_title_break (self, macro):
    if macro.flags & Data.SECTIONED:
      return 1
    if macro.name in ('doxer_done', 'doxer_newline', 'doxer_dnl'):
      return 1
    if macro.flags & Data.SPAN_LINE_MASK:
      return 1
    return 0

# --- section identification ---
class CreateSections:
  def __init__ (self):
    self.token_list = []
    self.section_list = []
  def section_new (self, node):
    block = node.clone_macro()
    assert len (node.contents) == 0
    block.contents = []
    self.section_list += [ block ]
    return block
  def open_section (self, node):
    self.section_new (node)
  def section_add (self, text_or_node):
    if self.section_list:
      sect = self.section_list[-1]
      assert isinstance (sect, Data.TextBlock)
      sect.contents += [ text_or_node ]
    else:
      self.token_list += [ text_or_node ]
  def process_tokens (self, tokens):
    for tok in tokens:
      if isinstance (tok, str):
        self.section_add (tok)
      else:
        assert isinstance (tok, Data.TextNode)
        if tok.contents:
          CreateSections ().process_node (tok)
        if tok.flags & Data.SECTIONED:
          self.open_section (tok)
        else:
          self.section_add (tok)
  def process_node (self, node):
    self.process_tokens (node.contents)
    node.contents = self.token_list + self.section_list
  def process_list (self, tokens, fname):
    self.process_tokens (tokens)
    if self.token_list:
      sect = self.section_new (Data.TextNode (fname, 0, 'doxer_start_section', command_dict['doxer_start_section']))
      sect.hidden = True # intro section
      sect.contents += self.token_list
      return [ sect ] + self.section_list
    else:
      return self.section_list

# --- scope identification ---
class ScopeParser:
  def add_content (self, token):
    # handle scope closing
    if isinstance (token, Data.TextNode) and token.name == 'doxer_done':
      if self.assimilators:
        # close innermost scope
        self.assimilators.pop()
        return
      else:
        raise MacroError (token, 'No scope can be closed by: %s' % token.name)
    # append as usual
    if self.assimilators:
      self.assimilators[-1].contents += [ token ]
    else:
      self.content_list += [ token ]
  def process_list (self, token_list):
    self.content_list = []
    self.assimilators = []
    for entry in token_list:
      self.add_content (entry)
      if isinstance (entry, Data.TextNode):
        # apply scope parsing recursively
        entry.arg = ScopeParser().process_list (entry.arg)
        entry.title = ScopeParser().process_list (entry.title)
        entry.contents = ScopeParser().process_list (entry.contents)
        # recognize opened scopes
        if entry.flags & Data.SCOPED:
          self.assimilators += [ entry ]
    # sanity checks
    if self.assimilators:
      raise MacroError (self.assimilators[-1], 'Unclosed scope opened by: %s' % self.assimilators[-1].name)
    return self.content_list

# --- apply commands ---
class ApplyCommands:
  def __init__ (self):
    self.commands = {}
  def add_command (self, macro):
    cmdname = DoxiParser.parse_new_command_name (macro)
    self.commands[cmdname] = macro
  def process_list (self, token_list):
    result = []
    # parse and process commands
    for tok in token_list:
      if isinstance (tok, str):
        result += [ tok ]
      elif tok.name == 'doxer_command':
        self.add_command (tok)
        # skip command definitions: result += [ tok ]
      elif self.commands.has_key (tok.name):
        tokens = self.apply_command (tok)
        result += tokens
      else: # ordinary node
        self.traverse_node (tok)
        result += [ tok ]
    return result
  def traverse_node (self, node):
    node.arg = self.process_list (node.arg)
    node.title = self.process_list (node.title)
    node.contents = self.process_list (node.contents)
  def apply_command (self, macro):
    cmd = self.commands[macro.name]
    body = cmd.contents[:]
    # strip trailing newline
    if body:
      rest, last = body[:-1], body[-1]
      if isinstance (last, str) and last and last[-1] == '\n':
        last = [ last[:-1] ]
      else:
        if (isinstance (last, Data.TextNode) and
            last.name == 'doxer_newline' and
            last.arg == [ 'automatic_newline' ]):
          last = []
        else:
          last = [ last ]
      body = rest + last
    token_list = list_expand_args (body, macro)
    # recursive macro expansion
    return self.process_list (token_list)

# --- check nesting and containment ---
class TreeChecker:
  # FIXME: this should catch dangling @doxer_done macros
  def traverse_children (self, token_list, parent, part):
    ccheck = self.child_check.get (parent.name)
    for tok in token_list:
      if not isinstance (tok, Data.TextNode):
        continue
      if ccheck:
        ccheck (self, parent, tok, part)
      pcheck = self.parent_check.get (tok.name)
      if pcheck:
        pcheck (self, tok, parent, part)
      self.traverse_children (tok.arg, tok, 'ARG')
      self.traverse_children (tok.title, tok, 'TITLE')
      self.traverse_children (tok.contents, tok, 'CONTENTS')
      if tok.flags & Data.SECTIONED:
        self.check_section (tok, parent, part)
      if tok.flags & Data.SPAN_LINE_MASK:
        self.check_span_line (tok, parent, part)
      if tok.flags & Data.SCOPED:
        self.check_scoped (tok, parent, part)
  def traverse (self, root):
    self.traverse_children ([ root ], root, 'SELF')
  def check_section (self, child, parent, parent_part):
    if parent_part != 'CONTENTS':
      raise MacroError (child, 'Invalid section nesting: %s' % child.name)
  def check_span_line (self, child, parent, parent_part):
    pass
  def check_scoped (self, child, parent, parent_part):
    if not parent_part in ('TITLE', 'CONTENTS'):
      raise MacroError (child, 'Invalid scope nesting: %s' % child.name)
  def table_ccheck (self, parent, child, parent_part):
    if child.name != 'doxer_row' and child.name != 'doxer_newline':
      raise MacroError (child, 'Invalid table child: %s' % child.name)
  child_check = {
    'doxer_table' : table_ccheck,
  }
  def item_pcheck (self, child, parent, parent_part):
    if not parent.name in ('doxer_list', 'doxer_deflist'):
      raise MacroError (child, 'Invalid item parent: %s' % parent.name)
  def row_pcheck (self, child, parent, parent_part):
    if parent.name != 'doxer_table':
      raise MacroError (child, 'Invalid row parent: %s' % parent.name)
  def cell_pcheck (self, child, parent, parent_part):
    if parent.name != 'doxer_row':
      raise MacroError (child, 'Invalid cell parent: %s' % parent.name)
  parent_check = {
    'doxer_item' : item_pcheck,
    'doxer_row'  : row_pcheck,
    'doxer_cell' : cell_pcheck,
  }

# --- process sections ---
class EnumerateSections:
  def __init__ (self):
    self.level_counters = []
    self.section_list = []
  def ensure_levels (self, n = 1):
    while n > len (self.level_counters):
      self.level_counters += [ 0 ]
  def next_counter (self, level = 1):
    # ensure ancestry levels >= 1
    self.ensure_levels (level)
    for i in range (0, level - 1):
      self.level_counters[i] = max (self.level_counters[i], 1)
    # increment level
    if level > 0:
      self.level_counters[level - 1] += 1
    # reset descendant levels
    for i in range (level, len (self.level_counters)):
      self.level_counters[i] = 0
    # construct numbering
    nstr = ''
    for i in self.level_counters:
      if i == 0:
        break
      if nstr:
        nstr += '.'
      nstr += str (i)
    return nstr
  def count_section (self, node):
    # process args
    fetch_number, level = None, 1
    tokens = node.arg
    while tokens:
      arg, tokens = split_arg_from_token_list (tokens)
      arg = plain_text_from_token_list (arg).strip()
      if arg == 'numbered':
        fetch_number = True
      elif arg == 'hidden':
        node.hidden = True
      elif arg == 'generate-toc':
        node.generate_toc = True
      elif re.match ('level=[0-9]$', arg):
        level = int (arg[6:])
      elif re.match ('id=[-0-9a-zA-Z_]+$', arg):
        node._id = arg[3:]
      elif re.match ('title-span=[-0-9a-zA-Z_]+$', arg):
        node.title_span = arg[11:]
      elif re.match ('body-span=[-0-9a-zA-Z_]+$', arg):
        node.body_span = arg[10:]
      elif re.match ('span=[-0-9a-zA-Z_]+$', arg):
        node.span = arg[5:]
      elif re.match ('group=[-0-9a-zA-Z_]+$', arg):
        node.group = arg[6:]
      elif re.match ('align=(left|center|right|justify)$', arg):
        node.align = arg[6:]
      else:
        raise MacroError (node, 'Invalid section arg: %s' % arg)
    node.level = level
    if fetch_number:
      node.numbering = self.next_counter (node.level)
  def traverse_list (self, token_list):
    for entry in token_list:
      if isinstance (entry, Data.TextNode):
        self.traverse_node (entry)
  def traverse_node (self, node):
    if node.flags & Data.SECTIONED:
      self.section_list += [ node ]
      self.count_section (node)
      if node.generate_toc:
        node.section_list = self.section_list
      if node.title and isinstance (node.title[0], str):
        while node.title[0] and node.title[0][0].isspace():
          node.title[0] = node.title[0][1:]
    self.traverse_list (node.arg)
    self.traverse_list (node.title)
    self.traverse_list (node.contents)

# --- section grouping ---
class ClaimParagraphs:
  # here, Data.CLAIM_PARA behaviour is implemented.
  # text-nodes that have Data.CLAIM_PARA, will claim the
  # paragraph (text until the next empty line) immediately
  # following the node, which then is appended
  # to node.contents.
  # e.g.
  # @doxer_parameter foo foo text.\n
  # @doxer_parameter bar bar text.\n
  # ends up as:
  # [ doxer_parameter { .title = 'foo', .contents = 'foo text.\n' },
  #   doxer_parameter { .title = 'bar', .contents = 'bar text.\n' } ]
  def __init__ (self):
    self.groups = {}
  def process_list (self, token_list):
    para = None
    result = []
    skip_next_newline = False
    for tok in token_list:
      skip_newline = skip_next_newline
      skip_next_newline = False
      if isinstance (tok, Data.TextNode):
        if tok.flags & Data.CLAIM_PARA:
          if para:
            para.has_para_sibling = True
          para = tok
          result += [ tok ]
        elif tok.name == 'doxer_newline':
          if para:
            # para.contents += [ tok ]  # skipping finishing newline of CLAIM_PARA section
            para.has_para_sibling = False
            skip_next_newline = True    # allow one extra newline after CLAIM_PARA nodes
            para = None
          elif skip_newline:            # skipping a newline after CLAIM_PARA node
            skip_newline = False
          else:
            result += [ tok ]
        elif para:
          para.contents += [ self.process_node (tok) ]
        else:
          result += [ self.process_node (tok) ]
      elif para:
        para.contents += [ tok ]
      else:
        result += [ tok ]
    if para:
      para.has_para_sibling = False
    return result
  def process_node (self, node):
    # const: node.arg
    # const: node.title
    node.contents = self.process_list (node.contents)
    return node

# --- anchor creation ---
class CreateAnchors:
  def __init__ (self):
    self.anchor_hash = {}
  def create_anchor (self, macro):
    explicit_anchor, needs_anchor, needs_toc_anchor = False, False, False
    # determine anchor name origin
    if macro.name == 'doxer_anchor':
      title = plain_text_from_token_list (macro.arg)
      explicit_anchor = True
      needs_anchor = True
    else:
      needs_toc_anchor = True           # sections must have an anchor for TOC generation
      title = "section-" + plain_text_from_token_list (macro.title)
      for tok in macro.title:
        if not isinstance (tok, str) and tok.name == 'doxer_anchor':
          explicit_anchor = True        # have explicit anchor already
          title = plain_text_from_token_list (tok.arg)
          break
    # sanity checks
    title = title.strip()
    if not title:
      return
    anchor = ''
    if explicit_anchor:
      # leave alone allowed anchor chars according to RFC-1808:
      for c in list (title):
        if c in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" + "0123456789" + "$-_.+" + "!*'()," + ";/?:@&=": # + ("%" hex hex)
          anchor += c
        else:
          anchor += '%%%02x' % ord (c)
    else:
      ignored_boundary_chars = set ('!"$/()?\\*+,;.:' + "`" + "'")
      delimiter_chars = set ('-!"?\\,;.: \t' + "`" + "'")
      special_chars = set ('$&/')
      allowed_chars = set ("_+*()ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")
      while title[-1:] in ignored_boundary_chars:
        title = title[:-1]
      while title[0:1] in ignored_boundary_chars:
        title = title[1:]
      for c in list (title):
        if c in delimiter_chars:
          if anchor[-1:] != '-':
            anchor += '-'               # delimiting place holder
        elif c in special_chars:
          if c == '&':
            anchor += '+'               # special chars
          else:
            anchor += '*'               # special chars
        elif c in allowed_chars:
          anchor += c.lower()           # allowed chars
        else:
          if anchor[-1:] != '-':
            anchor += '-'               # other chars
    dedup = 1
    base_anchor = anchor
    while anchor in self.anchor_hash:
      anchor = base_anchor + ('.%u' % dedup)
      dedup += 1
    if (needs_anchor or         # covers @doxer_anchor{} cases
        not explicit_anchor or  # generated section anchors
        base_anchor != anchor): # deduped (generated) section anchors
      self.anchor_hash[anchor] = True
      macro.anchor = anchor
    if needs_toc_anchor:        # seciton anchor to use in toc
      macro.toc_anchor = anchor
  def traverse_list (self, token_list):
    for entry in token_list:
      if isinstance (entry, Data.TextNode):
        self.traverse_node (entry)
  def traverse_node (self, node):
    if node.flags & Data.SECTIONED or node.name == 'doxer_anchor':
      self.create_anchor (node)
    self.traverse_list (node.arg)
    self.traverse_list (node.title)
    self.traverse_list (node.contents)

# --- apply customizations (user commands) ---
class ApplyCustomizations:
  def __init__ (self):
    self.anchor_hash = {}
  def process_list (self, token_list):
    tlist = []
    for tok in token_list:
      if isinstance (tok, Data.TextNode):
        node = self.process_node (tok)
        if node:
          tlist += [ node ]
      else:
        tlist += [ tok ]
    return tlist
  def process_node (self, node):
    node.arg = self.process_list (node.arg)
    node.title = self.process_list (node.title)
    node.contents = self.process_list (node.contents)
    if node.name == 'doxer_sub':
      return self.doxer_sub (node)
    if node.name == 'doxer_cmatch':
      return self.doxer_cmatch (node)
    if node.name == 'doxer_msg':
      print plain_text_from_token_list (node.arg)
      return None
    else:
      return node
  def parse_quoted_args (self, node, argstring, maxsplit = -1):
    args = []
    dqs = re.compile (r'\s*' + r'"((?:[^"\\]|\\.)*)"' + r'\s*,?')  # double quoted string pattern
    sqs = re.compile (r'\s*' + r"'((?:[^'\\]|\\.)*)'" + r'\s*,?')  # single quoted string pattern
    while argstring.split() and (len (args) < maxsplit or maxsplit < 0):
      match = dqs.match (argstring)
      if not match:
        match = sqs.match (argstring)
      if not match:
        raise MacroError (node, 'Invalid string argument: %s' % argstring)
      arg = match.groups()[0]
      argstring = argstring[match.end():]
      args += [ arg ]
    if argstring:
      args += [ argstring ]
    return args
  def doxer_sub (self, node):
    subok = False
    argstring = plain_text_from_token_list (node.arg)
    if isinstance (argstring, str):
      args = self.parse_quoted_args (node, argstring, 2)
      if len (args) == 3:
        try:
          pat = re.compile (args[0])
        except Exception, ex:
          raise MacroError (node, 'Invalid substitution (%s): %s' % (str (ex), argstring))
        return pat.sub (args[1], args[2])
    raise MacroError (node, 'Invalid substitution: %s' % argstring)
    return None
  def doxer_cmatch (self, node):
    atext = plain_text_from_token_list (node.arg)
    alist = re.split (',', atext)
    if len (alist) < 1:
      return '0'
    pattern = alist.pop (0)
    try:
      pat = re.compile (pattern)
    except Exception, ex:
      raise MacroError (node, 'Invalid pattern (%s): %s' % (str (ex), pattern))
    for i in alist:
      if pat.match (i):
        return '1'
    return '0'

# --- parser API ---
def parse_file (fname, output_file, init_variables = {}, template_text = ''):
  # sanitize path names
  if not os.path.isabs (fname):
    fname = os.path.normpath (Config.srcdir + '/' + fname)
  # parse input stream
  root = Data.TextBlock (fname, 0, 'doxi-document')
  istream = IStream ()
  istream.push_input (fname)
  try:
    dp = DoxiParser (fname, output_file, init_variables, template_text)
    debug ("Parsing: %s..." % fname)
    token_list = dp.parse_stream (istream, TERM_EOS)
    # post-process token stream
    debug ("Create sections: %s..." % fname)
    token_list = CreateSections ().process_list (token_list, fname)
    debug ("Break lines: %s..." % fname)
    token_list = LineBreaker (fname).process_list (token_list)
    # scopes are parsed inline now: token_list = ScopeParser().process_list (token_list)
    # setup parse tree
    root.variables = dp.variables
    root.contents += token_list
    root.root = root
    for n in root.descendants():
      n.root = root
    debug ("Apply commands: %s..." % fname)
    ApplyCommands().traverse_node (root)
    debug ("Check parse tree: %s..." % fname)
    TreeChecker().traverse (root)
    debug ("Enumerate sections: %s..." % fname)
    sproc = EnumerateSections()
    sproc.traverse_node (root)
    debug ("Claim paragraphs: %s..." % fname)
    root = ClaimParagraphs().process_node (root)
    debug ("Create anchors: %s..." % fname)
    CreateAnchors().traverse_node (root)
    debug ("Regexp substitutions: %s..." % fname)
    root = ApplyCustomizations().process_node (root)
    debug ("Check parse tree: %s..." % fname)
    TreeChecker().traverse (root)
  except Exception, ex:
    parser_exception = 0
    if isinstance (ex, MiscError):
      print >>sys.stderr, "%s" % str (ex)
      parser_exception = 1
    elif isinstance (ex, ParseError):
      print >>sys.stderr, "%s (%s)" % (str (ex), ex.__class__.__name__)
      parser_exception = 1
    else:
      print >>sys.stderr, "%s:%u: %s (%s)" % (istream.fname(), istream.fline(), str (ex), ex.__class__.__name__)
    if Config.debug or not parser_exception:
      raise
    # exit silently if not debugging
    os._exit (1)
  return root
