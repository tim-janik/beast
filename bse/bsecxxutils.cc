// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecxxutils.hh"
#include "bsecxxbase.hh"
#include "bsecategories.hh"
#include <list>
using namespace std;    // FIXME

namespace Bse {

struct TypeRegistry::TypeEntry {
  guint               instance_size;
  const gchar        *name;
  const gchar        *parent;
  const ClassInfo    *cinfo;
  GBaseInitFunc       binit;
  GClassInitFunc      cinit;
  GInstanceInitFunc   iinit;
  TypeRegistry::Flags flags;
  TypeRegistry       *reg;
  /*Con*/ TypeEntry (guint               instance_size,
                     const gchar        *name,
                     const gchar        *parent,
                     const ClassInfo    *cinfo,
                     GBaseInitFunc       binit,
                     GClassInitFunc      cinit,
                     GInstanceInitFunc   iinit,
                     TypeRegistry::Flags flags)
    : reg (NULL)
  {
    this->instance_size = instance_size;
    this->name = name;
    this->parent = parent;
    this->cinfo = cinfo;
    this->binit = binit;
    this->cinit = cinit;
    this->iinit = iinit;
    this->flags = flags;
  }
};

static list<TypeRegistry::TypeEntry> *type_entries = NULL;

TypeRegistry::TypeRegistry (guint             instance_size,
                            const gchar      *name,
                            const gchar      *parent,
                            const ClassInfo  *cinfo,
                            GBaseInitFunc     binit,
                            void            (*class_init) (CxxBaseClass*),
                            GInstanceInitFunc iinit,
                            Flags             flags)
  : gtype_id (0)
{
  TypeEntry entry (instance_size, name, parent, cinfo, binit,
                   (GClassInitFunc) class_init,
                   iinit, flags);
  entry.reg = this;
  if (!type_entries)
    type_entries = new list<TypeEntry>();
  list<TypeEntry>::iterator li;
  for (li = type_entries->begin(); li != type_entries->end(); li++)
    if (strcmp (li->name, parent) == 0)
      break;
  if (li != type_entries->end())
    type_entries->insert (++li, entry);
  else  // parent not found in list
    type_entries->push_front (entry);
}

void
TypeRegistry::init_types()
{
  for (list<TypeEntry>::iterator li = type_entries->begin (); li != type_entries->end (); li++)
    {
      TypeRegistry *self = li->reg;
      GTypeInfo info = { 0, };
      info.class_size = BSE_CXX_COMMON_CLASS_SIZE;
      info.base_init = li->binit;
      info.class_init = li->cinit;
      info.instance_size = BSE_CXX_INSTANCE_OFFSET + li->instance_size;
      info.instance_init = li->iinit;
      self->gtype_id = g_type_register_static (g_type_from_name (li->parent),
                                               li->name, &info, (GTypeFlags) li->flags);
      if (li->cinfo)
        {
          if (li->cinfo->category)
            bse_categories_register (li->cinfo->category, NULL, self->gtype_id, NULL);
          if (li->cinfo->blurb)
            bse_type_add_blurb (self->gtype_id, li->cinfo->blurb, li->cinfo->file, li->cinfo->line);
        }
    }
  delete type_entries;
  type_entries = NULL;
}

extern "C" void
bse_cxx_init (void)  // prototyped in bseutils.hh
{
  // FIXME: delete: init_exception_handler ();
  Bse::TypeRegistry::init_types();
}

} // Bse
