// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_STARTUP_HH__
#define __BSE_STARTUP_HH__

#include <bse/sfi.hh>

/// The Bse namespace contains all functions of the synthesis engine.
namespace Bse {

// == BSE Initialization ==
StringVector    init_args           (int *argc, char **argv);
void		init_async	    (const char *app_name, const StringVector &args = StringVector());
bool		init_needed	    ();
void		objects_debug_leaks ();
int             init_and_test       (const Bse::StringVector &args, const std::function<int()> &bsetester);

} // Bse

#endif // __BSE_STARTUP_HH__
