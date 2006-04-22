/* Birnet
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "birnetutilsxx.hh"
#include "birnetthread.h"
#include "birnetmsg.h"
#include "birnetcpu.h"
#include <vector>
#include <algorithm>

extern "C" {

static bool birnet_init_initialization_entered = false;
static void (*birnet_init_cplusplus_func) (void) = NULL;

void
birnet_init (int        *argcp,         /* declared in birnetcore.h */
             char     ***argvp,
             const char *app_name)
{
  char *prg_name = argcp && *argcp ? g_path_get_basename ((*argvp)[0]) : NULL;
  if (birnet_init_initialization_entered)
    {
      if (prg_name && !g_get_prgname ())
        g_set_prgname (prg_name);
      g_free (prg_name);
      if (app_name && !g_get_application_name())
        g_set_application_name (app_name);
      return;
    }
  birnet_init_initialization_entered = true;

  if (!g_threads_got_initialized)
    g_thread_init (NULL);

  if (prg_name)
    g_set_prgname (prg_name);
  g_free (prg_name);
  if (app_name && (!g_get_application_name() || g_get_application_name() == g_get_prgname()))
    g_set_application_name (app_name);

  _birnet_init_cpuinfo();

  _birnet_init_threads();

  _birnet_init_logging ();
  
  if (birnet_init_cplusplus_func)
    birnet_init_cplusplus_func();
}

} // "C"

namespace Birnet {

static InitHook *init_hooks = NULL;

InitHook::InitHook (InitHookFunc _func,
                    int          _priority) :
  next (NULL), priority (_priority), hook (_func)
{
  BIRNET_ASSERT (birnet_init_initialization_entered == false);
  /* the above assertion guarantees single-threadedness */
  next = init_hooks;
  init_hooks = this;
  birnet_init_cplusplus_func = invoke_hooks;
}

void
InitHook::invoke_hooks (void)
{
  std::vector<InitHook*> hv;
  struct Sub {
    static int
    init_hook_cmp (const InitHook *const &v1,
                   const InitHook *const &v2)
    {
      return v1->priority < v2->priority ? -1 : v1->priority > v2->priority;
    }
  };
  for (InitHook *ihook = init_hooks; ihook; ihook = ihook->next)
    hv.push_back (ihook);
  stable_sort (hv.begin(), hv.end(), Sub::init_hook_cmp);
  for (std::vector<InitHook*>::iterator it = hv.begin(); it != hv.end(); it++)
    (*it)->hook();
}

} // Birnet
