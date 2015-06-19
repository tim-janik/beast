// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfivisitors.hh"
#include <unordered_map>
#include <typeindex>

namespace Bse { // BseCore

typedef std::unordered_map<std::type_index, SfiRecFields> VisitableTypeRecFieldMap;

static VisitableTypeRecFieldMap visitable_type_rec_fields;

bool
sfi_psecs_rec_fields_cache (const std::type_info &type_info, SfiRecFields *rf, bool assign)
{
  if (assign)
    {
      visitable_type_rec_fields[type_info] = *rf;
      return true;
    }
  auto it = visitable_type_rec_fields.find (type_info);
  if (it == visitable_type_rec_fields.end())
    return false;
  *rf = it->second;
  return true;
}

} // Bse
