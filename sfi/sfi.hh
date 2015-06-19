// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_H__
#define __SFI_H__


// == Rapicorn Imports ==
#include <rapicorn-core.hh>     // We move to Rapicorn core for low level stuff
using Rapicorn::printerr;
using Rapicorn::printout;
using Rapicorn::string_format;
using Rapicorn::Any;
typedef std::string String;

namespace Sfi {
using namespace Rapicorn;
} // Sfi
namespace Bse {
using namespace Rapicorn;
} // Bse

/* no bin-compat: #include <sfi/sficomwire.hh> */
#include <sfi/sficomport.hh>
#include <sfi/sfifilecrawler.hh>
#include <sfi/sfiglue.hh>
#include <sfi/sfigluecodec.hh>
#include <sfi/sfiglueproxy.hh>
#include <sfi/sfimemory.hh>
#include <sfi/sfinote.hh>
#include <sfi/sfiparams.hh>
#include <sfi/sfiprimitives.hh>
#include <sfi/sfiring.hh>
/* #include <sfi/sfisclock.hh> */
#include <sfi/sfiserial.hh>
/* no bin-compat: #include <sfi/sfistore.hh> */
#include <sfi/sfitime.hh>
#include <sfi/sfitypes.hh>
#include <sfi/sfiustore.hh>
#include <sfi/sfivalues.hh>
#include <sfi/sfivisitors.hh>
#include <sfi/sfivmarshal.hh>
#include <sfi/sfiwrapper.hh>

#endif /* __SFI_H__ */

/* vim:set ts=8 sts=2 sw=2: */
