/* BSE - Better Sound Engine
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bsecxxplugin.hh"
#include "bseplugin.hh"
#include "bsemain.hh"
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
