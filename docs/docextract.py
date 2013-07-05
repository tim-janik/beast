# Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
import sys, re

rapicorn_debug_items, rapicorn_debug_items_set = '', set()
rapicorn_debug_keys, rapicorn_debug_keys_set = '', set()
rapicorn_flippers, rapicorn_flippers_set = '', set()
rapicorn_todo_lists = {}

def process_start ():
  pass

def process_comment (txt):
  if not txt.startswith (('///', '/**', '/*!')):
    return      # filter non-doxygen comments
  # @TODO @TODOS
  global rapicorn_todo_lists
  pattern = r'[@\\] TODO[Ss]? \s* ( : )'
  match = re.search (pattern, txt, re.MULTILINE | re.VERBOSE)
  if match:
    text = txt[match.end (1):].strip()                  # take todo text, whitespace-stripped
    text = text[:-2] if text.endswith ('*/') else text  # strip comment-closing
    text = re.sub (r'( ^ | \n) \s* \*+',                # match comment prefix at line start
                   r'\1', text, 0, re.X | re.M)         # strip comment prefix from all lines
    pattern = r'\s* ( [*+-] | [0-9]+ \. )'              # pattern for list bullet
    if not re.match (pattern, text, re.X):              # not a list
      text = ' - ' + text                               # insert list bullet
    blurb = rapicorn_todo_lists.get (filename, '')
    blurb += text.rstrip() + '\n'
    rapicorn_todo_lists[filename] = blurb

def process_code (txt):
  cstring = r' " ( (?: [^\\"] | \\ .) * ) " '
  # RAPICORN_DEBUG_OPTION (option, blurb)
  global rapicorn_debug_items, rapicorn_debug_items_set
  pattern = r'\b RAPICORN_DEBUG_OPTION \s* \( \s* ' + cstring + r' \s* , \s* ' + cstring + ' \s* \) \s* ;'
  matches = re.findall (pattern, txt, re.MULTILINE | re.VERBOSE)
  for m in matches:
    if not m[0] in rapicorn_debug_items_set:
      rapicorn_debug_items += '  * - @c %s - %s\n' % (m[0], m[1])
      rapicorn_debug_items_set.add (m[0])
  # RAPICORN_KEY_DEBUG (keys, ...)
  global rapicorn_debug_keys, rapicorn_debug_keys_set
  pattern = r'\b RAPICORN_KEY_DEBUG \s* \( \s* ' + cstring + r' \s* , \s* [^;] *? \)'
  matches = re.findall (pattern, txt, re.MULTILINE | re.VERBOSE)
  for m in matches:
    if not m in rapicorn_debug_keys_set:
      rapicorn_debug_keys += '  * - @c %s - Debugging message key, =1 enable, =0 disable.\n' % m
      rapicorn_debug_keys_set.add (m)
  # RAPICORN_FLIPPER (option, blurb)
  global rapicorn_flippers, rapicorn_flippers_set
  pattern = r'\b RAPICORN_FLIPPER \s* \( \s* ' + cstring + r' \s* , \s* ' + cstring + ' \s* \) \s* ;'
  matches = re.findall (pattern, txt, re.MULTILINE | re.VERBOSE)
  for m in matches:
    if not m[0] in rapicorn_flippers_set:
      rapicorn_flippers += '  * - @c %s - %s\n' % (m[0], m[1])
      rapicorn_flippers_set.add (m[0])

def sanitize_ident (txt):
  return re.sub (r'[^0-9_a-zA-Z]+', '_', txt).strip ('_')

def process_end ():
  if rapicorn_debug_items:
    print '/** @var $RAPICORN_DEBUG'
    print rapicorn_debug_items, '*/'
  if rapicorn_debug_keys:
    print '/** @var $RAPICORN_DEBUG'
    print rapicorn_debug_keys, '*/'
  if rapicorn_flippers:
    print '/** @var $RAPICORN_FLIPPER'
    print rapicorn_flippers, '*/'
  if rapicorn_todo_lists:
    # ('/** @page todo_lists Todo Lists', ' * @section %s %s' % (sanitize_ident (filename), filename), '*/')
    for filename, blurb in rapicorn_todo_lists.items():
      ident = sanitize_ident (filename)
      filename = filename[2:] if filename.startswith ('./') else filename
      print '/** @file %s' % filename
      print '@xrefitem todo "Todos" "Source Code Todo List"'
      print blurb.rstrip()
      print '*/'

def process_specific (filename, text):
  def is_comment (t):
    return t.startswith ('//') or t.startswith ('/*')
  cxx_splitter = r"""(
    "   (?: [^\\"] | \\  .   ) * "        # double quoted string
  | '   (?: [^\\'] | \\  .   ) * '        # single quoted string
  | /\* (?: [^*]   | \* [^/] ) * \*/      # C comment
  | //  (?: [^\n]            ) * \n       # C++ comment
  )"""
  parts = re.split (cxx_splitter, text, 0, re.MULTILINE | re.VERBOSE)
  # parts = filter (len, parts) # strip empty parts
  # concatenate code vs. comment bits
  i = 0
  while i < len (parts):
    s = parts[i]
    isc = is_comment (s)
    i += 1
    while i < len (parts) and isc == is_comment (parts[i]):
      s += parts[i]
      i += 1
    if isc:
      process_comment (s)
    else:
      process_code (s)

process_start()
# print '/// @file'
for iline in sys.stdin.readlines():
  filename = iline.rstrip() # removes trailing '\n'
  process_specific (filename, open (filename, 'r').read())
process_end()
