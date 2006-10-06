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
#ifndef __SFI_WRAPPER_H__
#define __SFI_WRAPPER_H__

#include <sfi/sfitypes.h>

/* sfiwrapper.h is a thin C language wrapper around C++ features
 * provided by libbirnet.
 */

BIRNET_EXTERN_C_BEGIN();

/* --- initialization --- */
typedef struct
{
  const char *value_name;       /* value list ends with value_name == NULL */
  const char *value_string;
  long double value_num;        /* valid if value_string == NULL */
} SfiInitValue;
void    sfi_init        (int            *argcp,
			 char         ***argvp,
			 const char     *app_name,
			 SfiInitValue    sivalues[]);
bool    sfi_init_value_bool   (SfiInitValue *value);
double  sfi_init_value_double (SfiInitValue *value);
gint64  sfi_init_value_int    (SfiInitValue *value);

/* --- file tests --- */
bool	birnet_file_check (const char *file,
			   const char *mode);
bool	birnet_file_equals (const char *file1,
			    const char *file2);

/* --- url handling --- */
void sfi_url_show                   	(const char           *url);
void sfi_url_show_with_cookie       	(const char           *url,
					 const char           *url_title,
					 const char           *cookie);
bool sfi_url_test_show              	(const char           *url);
bool sfi_url_test_show_with_cookie	(const char           *url,
					 const char           *url_title,
					 const char           *cookie);

/* --- cleanup handlers --- */
void birnet_cleanup_force_handlers     (void); // FIXME: remove 

BIRNET_EXTERN_C_END();

#endif /* __SFI_WRAPPER_H__ */

/* vim:set ts=8 sts=2 sw=2: */
