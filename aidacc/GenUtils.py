# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
import Decls
true, false, length = (True, False, len)

def static_vars (*varname_value_list):
  def decorate (func):
    for tup in varname_value_list:
      varname, value = tup
      setattr (func, varname, value)
    return func
  return decorate

@static_vars (("iddict", {}), ("idcounter", 0))
def type_id (type):
  self = type_id
  otype = type
  types = []
  while type:
    types += [ type ]
    if hasattr (type, 'ownertype'):
      type = type.ownertype
    elif hasattr (type, 'namespace'):
      type = type.namespace
    else:
      type = None
  types = tuple (types)
  if not self.iddict.has_key (types):
    self.idcounter += 1
    if otype.storage == Decls.FUNC:
      if otype.rtype == None or otype.rtype.storage == Decls.VOID:
        self.iddict[types] = self.idcounter + 0x01000000
      else:
        self.iddict[types] = self.idcounter + 0x02000000
    else:
      self.iddict[types] = self.idcounter + 0x03000000
  return self.iddict[types]
