/* SfiWrapper - Birnet C wrapper
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "sfiwrapper.h"
#undef BIRNET__RUNTIME_PROBLEM
#include <birnet/birnet.hh>

/* --- initialization --- */
void
sfi_init (int            *argcp,
          char         ***argvp,
          const char     *app_name,
          SfiInitValue    sivalues[])
{
  BIRNET_STATIC_ASSERT (sizeof (SfiInitValue) == sizeof (BirnetInitValue));
  BIRNET_STATIC_ASSERT (offsetof (SfiInitValue, value_name) == offsetof (BirnetInitValue, value_name));
  BIRNET_STATIC_ASSERT (offsetof (SfiInitValue, value_string) == offsetof (BirnetInitValue, value_string));
  BIRNET_STATIC_ASSERT (offsetof (SfiInitValue, value_num) == offsetof (BirnetInitValue, value_num));
  Birnet::birnet_init (argcp, argvp, app_name, (BirnetInitValue*) sivalues);
}

bool
sfi_init_value_bool (SfiInitValue *value)
{
  return Birnet::init_value_bool ((BirnetInitValue*) value);
}

double
sfi_init_value_double (SfiInitValue *value)
{
  return Birnet::init_value_double ((BirnetInitValue*) value);
}

gint64
sfi_init_value_int (SfiInitValue *value)
{
  return Birnet::init_value_int ((BirnetInitValue*) value);
}

SfiInitSettings
sfi_init_settings (void)
{
  return ::Birnet::init_settings();
}

/* --- CPU Info --- */
SfiCPUInfo
sfi_cpu_info (void)
{
  return ::Birnet::cpu_info();
}

gchar*
sfi_cpu_info_string (const SfiCPUInfo *cpu_info)
{
  return g_strdup (::Birnet::cpu_info_string (*cpu_info).c_str());
}

/* --- file testing --- */
bool
birnet_file_check (const char *file,
                   const char *mode)
{
  return Birnet::Path::check (file ? file : "", mode ? mode : "");
}

bool
birnet_file_equals (const char *file1,
                    const char *file2)
{
  return Birnet::Path::equals (file1 ? file1 : "", file2 ? file2 : "");
}

/* --- url handling --- */
void
sfi_url_show (const char *url)
{
  return Birnet::url_show (url);
}

void
sfi_url_show_with_cookie (const char *url,
                          const char *url_title,
                          const char *cookie)
{
  return Birnet::url_show_with_cookie (url, url_title, cookie);
}

bool
sfi_url_test_show (const char *url)
{
  return Birnet::url_test_show (url);
}

bool
sfi_url_test_show_with_cookie (const char *url,
                               const char *url_title,
                               const char *cookie)
{
  return Birnet::url_test_show_with_cookie (url, url_title, cookie);
}

/* --- cleanup handlers --- */
void
birnet_cleanup_force_handlers (void)
{
  return Birnet::cleanup_force_handlers();
}

/* --- threading API --- */
SfiThread*
sfi_thread_run (const gchar  *name,
                SfiThreadFunc func,
                gpointer      user_data)
{
  g_return_val_if_fail (name && name[0], NULL);
  
  SfiThread *thread = sfi_thread_new (name);
  sfi_thread_ref_sink (thread);
  if (sfi_thread_start (thread, func, user_data))
    return thread;
  else
    {
      sfi_thread_unref (thread);
      return NULL;
    }
}

/* for the sfi_thread_table initialization to work, Birnet::ThreadTable must not be a reference */
extern "C" const BirnetThreadTable *sfi_thread_table = &::Birnet::ThreadTable;

void
sfi_runtime_problem (char        ewran_tag,
                     const char *domain,
                     const char *file,
                     int         line,
                     const char *funcname,
                     const char *msgformat,
                     ...)
{
  va_list args;
  va_start (args, msgformat);
  ::Birnet::birnet_runtime_problemv (ewran_tag, domain, file, line, funcname, msgformat, args);
  va_end (args);
}

/* vim:set ts=8 sts=2 sw=2: */
