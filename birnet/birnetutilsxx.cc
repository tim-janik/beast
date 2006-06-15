/* Birnet
 * Copyright (C) 2005-2006 Tim Janik
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
#include <sys/time.h>
#include <vector>
#include <algorithm>

extern "C" {

/* --- initialization for C --- */
static void (*birnet_init_cplusplus_func) (void) = NULL;
static BirnetInitSettings default_init_settings = {
  false,        /* stand_alone */
};
BirnetInitSettings *birnet_init_settings = NULL;

static void
birnet_init_settings_values (BirnetInitValue bivalues[])
{
  BirnetInitValue *value = bivalues;
  while (value->value_name)
    {
      if (strcmp (value->value_name, "stand-alone") == 0)
        birnet_init_settings->stand_alone = birnet_init_value_bool (value);
      value++;
    }
}

void
birnet_init_extended (int            *argcp,    /* declared in birnetcore.h */
                      char         ***argvp,
                      const char     *app_name,
                      BirnetInitValue bivalues[])
{
  /* mandatory initial initialization */
  if (!g_threads_got_initialized)
    g_thread_init (NULL);

  /* update program/application name upon repeated initilization */
  char *prg_name = argcp && *argcp ? g_path_get_basename ((*argvp)[0]) : NULL;
  if (birnet_init_settings != NULL)
    {
      if (prg_name && !g_get_prgname ())
        g_set_prgname (prg_name);
      g_free (prg_name);
      if (app_name && !g_get_application_name())
        g_set_application_name (app_name);
      return;   /* ignore repeated initializations */
    }

  /* normal initialization */
  birnet_init_settings = &default_init_settings;
  if (bivalues)
    birnet_init_settings_values (bivalues);
  if (prg_name)
    g_set_prgname (prg_name);
  g_free (prg_name);
  if (app_name && (!g_get_application_name() || g_get_application_name() == g_get_prgname()))
    g_set_application_name (app_name);

  /* initialize random numbers */
  {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    srand48 (tv.tv_usec + (tv.tv_sec << 16));
    srand (lrand48());
  }

  /* initialize sub systems */
  _birnet_init_cpuinfo();
  _birnet_init_threads();
  _birnet_init_logging ();
  if (birnet_init_cplusplus_func)
    birnet_init_cplusplus_func();
}

} // "C"

namespace Birnet {

/* --- InitHooks --- */
static InitHook *init_hooks = NULL;

InitHook::InitHook (InitHookFunc _func,
                    int          _priority) :
  next (NULL), priority (_priority), hook (_func)
{
  BIRNET_ASSERT (birnet_init_settings == NULL);
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

/* --- file utils --- */
const String
dirname (const String &path)
{
  const char *filename = path.c_str();
  const char *base = strrchr (filename, BIRNET_DIR_SEPARATOR);
  if (!base)
    return ".";
  while (*base == BIRNET_DIR_SEPARATOR && base > filename)
    base--;
  return String (filename, base - filename + 1);
}

const String
basename (const String &path)
{
  const char *filename = path.c_str();
  const char *base = strrchr (filename, BIRNET_DIR_SEPARATOR);
  if (!base)
    return filename;
  return String (base + 1);
}

/* --- DataList --- */
void
DataList::set_data (NodeBase *node)
{
  /* delete old node */
  NodeBase *it = rip_data (node->key);
  if (it)
    delete it;
  /* prepend node */
  node->next = nodes;
  nodes = node;
}

DataList::NodeBase*
DataList::get_data (DataKey<void> *key) const
{
  NodeBase *it;
  for (it = nodes; it; it = it->next)
    if (it->key == key)
      return it;
  return NULL;
}

DataList::NodeBase*
DataList::rip_data (DataKey<void> *key)
{
  NodeBase *last = NULL, *it;
  for (it = nodes; it; it = it->next)
    if (it->key == key)
      {
        /* unlink existing node */
        if (last)
          last->next = it->next;
        else
          nodes = it->next;
        it->next = NULL;
        return it;
      }
  return NULL;
}

void
DataList::clear_like_destructor()
{
  while (nodes)
    {
      NodeBase *it = nodes;
      nodes = it->next;
      it->next = NULL;
      delete it;
    }
}

DataList::~DataList()
{
  clear_like_destructor();
}

} // Birnet
