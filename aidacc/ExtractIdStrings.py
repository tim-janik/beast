import sys, re

print 'import collections'
print 'id_dict = collections.OrderedDict (('
for line in open (sys.argv[1]).readlines():
  if line.find ('//: AIDAID') >= 0:
    t = re.sub ('[ \t]+', ' ', line.strip())
    if line.find ('//: AIDAID ') >= 0:
      ident = re.sub ('.*//: AIDAID (\w+)$', r'\1', t)
    else:
      ident = re.sub ('.* (\w+) \(.*', r'\1', t)
    if ident:
      tescaped = re.sub ("'", r'\'', t)
      print "  ('%s'," % ident, ' ' * (24 - len (ident)), "'%s')," % tescaped
print '))'
