// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/gsldatahandle.hh>
#include <bse/gslwavechunk.hh>
#include "bseloopfuncs.hh"
#include "bwtwave.hh"
#include <unistd.h>
#include <typeinfo>
#include <string>
namespace BseWaveTool {
using namespace std;
/* --- command + registry --- */
class Command {
public:
  const string name;
  explicit      Command    (const char *command_name);
  virtual uint  parse_args (uint   argc,
                            char **argv)        { return 0; }
  virtual Wave* create     ()                   { return NULL; }
  virtual bool  exec       (Wave *wave) = 0;
  virtual void  blurb      (bool bshort);
  virtual      ~Command    ()                   {}
  static list<Command*> registry;
};
} // BseWaveTool
