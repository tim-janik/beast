// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsetype.hh"        /* import all required types first */
#include "bsepart.hh"
#include "bsemain.hh"
#include "bseengine.hh"
#include "bsesequencer.hh"
#include "bsecxxplugin.hh" /* includes bsecore.genidl.hh for us */

namespace Bse {

namespace Procedure {
ThreadTotalsHandle
collect_thread_totals::exec ()
{
  struct Sub {
    static ThreadState convert (BirnetThreadState ts)
    {
      switch (ts)
        {
        default:
        case BIRNET_THREAD_UNKNOWN:     return THREAD_STATE_UNKNOWN;
        case BIRNET_THREAD_RUNNING:     return THREAD_STATE_RUNNING;
        case BIRNET_THREAD_SLEEPING:    return THREAD_STATE_SLEEPING;
        case BIRNET_THREAD_DISKWAIT:    return THREAD_STATE_DISKWAIT;
        case BIRNET_THREAD_TRACED:      return THREAD_STATE_TRACED;
        case BIRNET_THREAD_PAGING:      return THREAD_STATE_PAGING;
        case BIRNET_THREAD_ZOMBIE:      return THREAD_STATE_ZOMBIE;
        case BIRNET_THREAD_DEAD:        return THREAD_STATE_DEAD;
        }
    }
    static void assign (ThreadInfoHandle &th,
                        BirnetThreadInfo    *ti)
    {
      th->name = ti->name;
      th->thread_id = ti->thread_id;
      th->state = convert (ti->state);
      th->priority = ti->priority;
      th->processor = ti->processor;
      th->utime = ti->utime;
      th->stime = ti->stime;
      th->cutime = ti->cutime;
      th->cstime = ti->cstime;
    }
  };
  ThreadTotalsHandle tth (Sfi::INIT_DEFAULT);
  BirnetThreadInfo *ti;
  ti = sfi_thread_info_collect (bse_main_thread);
  tth->main = ThreadInfoHandle (Sfi::INIT_DEFAULT);
  Sub::assign (tth->main, ti);
  sfi_thread_info_free (ti);
  if (bse_sequencer_thread)
    {
      ti = sfi_thread_info_collect (bse_sequencer_thread);
      tth->sequencer = ThreadInfoHandle (Sfi::INIT_DEFAULT);
      Sub::assign (tth->sequencer, ti);
      sfi_thread_info_free (ti);
    }
  guint n;
  BirnetThread **t;
  t = bse_engine_get_threads (&n);
  for (guint i = 0; i < n; i++)
    {
      ti = sfi_thread_info_collect (t[i]);
      tth->synthesis.resize (i + 1);
      tth->synthesis[i] = ThreadInfoHandle (Sfi::INIT_DEFAULT);
      Sub::assign (tth->synthesis[i], ti);
      sfi_thread_info_free (ti);
    }
  g_free (t);
  return tth;
}

} // Procedure

/* export definitions follow */
BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_ALL_TYPES_FROM_BSECORE_IDL();

} // Bse

/* compile and initialize generated C stubs */
#include "bsegencore.cc"
void
_bse_init_c_wrappers (void)
{
  sfidl_types_init ();
}
