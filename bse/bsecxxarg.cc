// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecxxarg.hh"
#include "bsecxxbase.hh"
namespace {
using namespace Bse;
} // namespace
namespace Bse {
const String
tokenize_gtype (GType t)
{
  switch (G_TYPE_FUNDAMENTAL (t))
    {
    case G_TYPE_BOOLEAN:        return "b";
    case G_TYPE_INT:            return "i";
    case SFI_TYPE_NUM:          return "n";
    case SFI_TYPE_REAL:         return "r";
    case G_TYPE_POINTER:        return "*";
    case G_TYPE_STRING:         return "s";
    case G_TYPE_PARAM:          return "P";
    case G_TYPE_OBJECT:
      if (g_type_is_a (t, BSE_TYPE_CXX_BASE))
        return "X";
      else
        return "O";
    default:
      throw InvalidArgument (G_STRLOC);
    }
}
} // Bse
