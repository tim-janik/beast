/* BSE - Bedevilled Sound Engine
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
#include "bsecxxplugin.hh"
#include "bseplugin.h"
#include "bsemain.h"
#include <new>

namespace Bse {

BsePlugin*
ExportTypeKeeper::plugin_export_node (const ::BseExportIdentity *plugin_identity,
                                      ::BseExportNode           *enode)
{
  if (plugin_identity == &bse_builtin_export_identity)
    {
      /* handle builtin types */
      enode->next = bse_builtin_export_identity.export_chain;
      bse_builtin_export_identity.export_chain = enode;
      return NULL;
    }
  else
    return bse_exports__add_node (plugin_identity, enode);
}

void
ExportTypeKeeper::plugin_cleanup (BsePlugin                 *plugin,
                                  ::BseExportNode           *enode)
{
  bse_exports__del_node (plugin, enode);
}

} // Bse
