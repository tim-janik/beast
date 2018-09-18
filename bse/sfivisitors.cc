// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfivisitors.hh"
#include <unordered_map>
#include <typeindex>

namespace Bse { // BseCore

typedef std::unordered_map<std::type_index, SfiRecFields> VisitableTypeRecFieldMap;

static VisitableTypeRecFieldMap visitable_type_rec_fields;

bool
sfi_pspecs_rec_fields_cache (const std::type_info &type_info, SfiRecFields *rf, bool assign)
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

typedef std::unordered_map<std::type_index, GParamSpec*> VisitableTypeSeqFieldMap;

static VisitableTypeSeqFieldMap visitable_type_seq_field_map;

bool
sfi_pspecs_seq_field_cache (const std::type_info &type_info, GParamSpec  **pp, bool assign)
{
  if (assign)
    {
      visitable_type_seq_field_map[type_info] = *pp;
      return true;
    }
  auto it = visitable_type_seq_field_map.find (type_info);
  if (it == visitable_type_seq_field_map.end())
    return false;
  *pp = it->second;
  return true;
}

typedef std::unordered_map<std::type_index, std::vector<GParamSpec*> > VisitableTypeAcsFieldMap;

static VisitableTypeAcsFieldMap visitable_type_acs_fields;

bool
sfi_pspecs_acs_fields_cache (const std::type_info &type_info, std::vector<GParamSpec*> **pspecspp, bool assign)
{
  if (assign)
    {
      visitable_type_acs_fields[type_info] = **pspecspp;
      return true;
    }
  auto it = visitable_type_acs_fields.find (type_info);
  if (it == visitable_type_acs_fields.end())
    return false;
  *pspecspp = &it->second;
  return true;
}

} // Bse
