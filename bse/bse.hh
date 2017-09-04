// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_H__
#define __BSE_H__

#include <bse/bsestartup.hh>
#include <bse/bseclientapi.hh>

namespace Bse { // clientapi glue code, see bsestartup.cc
ServerHandle            init_server_instance   ();
Aida::ClientConnectionP init_server_connection ();
} // Bse

#endif /* __BSE_H__ */
