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

/* --- file testing --- */
bool
birnet_file_check (const char *file,
                   const char *mode)
{
  return Birnet::Path::check (file, mode);
}

bool
birnet_file_equals (const char *file1,
                    const char *file2)
{
  return Birnet::Path::equals (file1, file2);
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

/* vim:set ts=8 sts=2 sw=2: */
