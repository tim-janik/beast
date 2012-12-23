// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
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
