// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsestartup.hh"
#include "bsemain.hh"
#include "bse/internal.hh"
#include "bse/bseserver.hh"
#include <bse/bse.hh>

namespace Bse {

// == BSE Initialization ==

/** Initialize and start BSE.
 * Initialize the BSE library and start the main BSE thread. Arguments specific to BSE are removed
 * from @a argc / @a argv.
 */
void
init_async (int *argc, char **argv, const char *app_name, const StringVector &args)
{
  _bse_init_async (argc, argv, app_name, args);
}

/// Check wether init_async() still needs to be called.
bool
init_needed ()
{
  return _bse_initialized() == false;
}

/// Extract BSE specific arguments from `argc, argv` and set `args` accordingly.
void
init_args (StringVector &args, int *argc, char **argv)
{
  const uint cargc = *argc;
  /* this function is called before the main BSE thread is started,
   * so we can't use any BSE functions yet.
   */
  uint i;
  for (i = 1; i < cargc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
        {
          args.push_back ("fatal-warnings=1");
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-pcm-driver", argv[i]) == 0 && i + 1 < cargc)
	{
          argv[i++] = NULL;
          args.push_back ("pcm-driver=" + std::string (argv[i]));
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-midi-driver", argv[i]) == 0 && i + 1 < cargc)
	{
          argv[i++] = NULL;
          args.push_back ("midi-driver=" + std::string (argv[i]));
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-override-plugin-globs", argv[i]) == 0 && i + 1 < cargc)
	{
          argv[i++] = NULL;
          args.push_back ("override-plugin-globs=" + std::string (argv[i]));
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-override-sample-path", argv[i]) == 0 && i + 1 < cargc)
	{
	  argv[i++] = NULL;
          args.push_back ("override-sample-path=" + std::string (argv[i]));
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-rcfile", argv[i]) == 0 && i + 1 < cargc)
	{
          argv[i++] = NULL;
          args.push_back ("rcfile=" + std::string (argv[i]));
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-disable-randomization", argv[i]) == 0)
	{
          args.push_back ("allow-randomization=0");
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-enable-randomization", argv[i]) == 0)
	{
          args.push_back ("allow-randomization=1");
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-jobs", argv[i]) == 0)
	{
          argv[i++] = NULL;
          args.push_back ("jobs=" + std::string (argv[i]));
	  argv[i] = NULL;
	}
    }

  if (*argc > 1)
    {
      uint e = 1;
      for (i = 1; i < cargc; i++)
        if (argv[i])
          {
            argv[e++] = argv[i];
            if (i >= e)
              argv[i] = NULL;
          }
      *argc = e;
    }
}

} // Bse
