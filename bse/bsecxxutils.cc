/* BSE - Bedevilled Sound Engine                        -*-mode: c++;-*-
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsecxxutils.h"

#include "bsecxxbase.h"
#include "bsecategories.h"
#include <list>
using namespace std;

namespace {
using namespace Bse;

/* --- functions --- */
struct TypeEntry {
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

static list<TypeEntry> *type_entries = NULL;

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
          if (li->cinfo->category != "")
            bse_categories_register (li->cinfo->category, self->gtype_id, NULL);
          if (li->cinfo->blurb != "")
            bse_type_set_blurb (self->gtype_id, li->cinfo->blurb);
        }
    }
  delete type_entries;
  type_entries = NULL;
}

static void
bse_terminate_handler ()
{
  try {
    throw;      // rethrow
  }
  catch (Exception &e) {
    sfi_error ("aborting due to exception: %s [in %s]", e.what(), e.where());
    abort ();
  }
  catch (std::exception &e) {
    sfi_error ("aborting due to exception: %s", e.what());
    abort ();
  }
  catch (...) {
    sfi_error ("aborting due to unknown exception");
    abort ();
  }
}

static void
init_exception_handler ()
{
#if 0
  unexpected_handler former = set_unexpected (bse_unexpected_handler);
  if (former != std::unexpected)
    set_unexpected (former);
#else
  set_terminate (bse_terminate_handler);
#endif
}

extern "C" void
bse_cxx_init (void)  // prototyped in bseutils.h
{
  init_exception_handler ();
  Bse::TypeRegistry::init_types();
}

} // namespace
