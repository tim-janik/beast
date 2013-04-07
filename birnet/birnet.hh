// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BIRNET_H__
#define __BIRNET_H__
#include <birnet/birnetconfig.h>
#include <birnet/birnetmsg.hh>
#include <birnet/birnetutils.hh>

// We're migrating to Bse for everything and use Rapicorn core for the lower level stuff
#include <rapicorn-core.hh>
namespace Bse {
using namespace Rapicorn;
} // Bse

#endif /* __BIRNET_H__ */
