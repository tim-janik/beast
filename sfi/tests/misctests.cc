// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#undef G_LOG_DOMAIN
#define  G_LOG_DOMAIN __FILE__
// #define TEST_VERBOSE
#include <sfi/testing.hh>
#include <sfi/path.hh>
#include <unistd.h>
#include <string.h>
#include <signal.h>	/* G_BREAKPOINT() */
#include <math.h>

using namespace Bse;

/* provide IDL type initializers */
#define sfidl_pspec_Real(group, name, nick, blurb, dflt, min, max, step, hints)  \
  sfi_pspec_real (name, nick, blurb, dflt, min, max, step, hints)
#define sfidl_pspec_Record(group, name, nick, blurb, hints, fields)            \
  sfi_pspec_rec (name, nick, blurb, fields, hints)
#define sfidl_pspec_Choice(group, name, nick, blurb, default_value, hints, choices) \
  sfi_pspec_choice (name, nick, blurb, default_value, choices, hints)

/* FIXME: small hackery */
#define sfidl_pspec_Rec(group, name, nick, blurb, hints)            \
  sfi_pspec_int (name, nick, blurb, 0, 0, 0, 0, hints)
#define sfidl_pspec_PSpec(group, name, nick, blurb, hints)            \
  sfi_pspec_int (name, nick, blurb, 0, 0, 0, 0, hints)






#include "../private.hh"

int
main (int   argc,
      char *argv[])
{
  Bse::Test::init (&argc, argv);

  if (0 != Bse::Test::run())
    return -1;

  return 0;
}
