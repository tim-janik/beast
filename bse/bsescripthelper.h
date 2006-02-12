/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_SCRIPT_HELPER_H__
#define __BSE_SCRIPT_HELPER_H__

#include        <bse/bseprocedure.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- typedefs & structures --- */
typedef struct
{
  gchar	  *script_file;
  gchar	  *name;
  SfiRing *params;
} BseScriptData;
typedef struct
{
  BseProcedureClass parent_class;
  BseScriptData    *sdata;
} BseScriptProcedureClass;


/* --- API --- */
GType		bse_script_proc_register	(const gchar	*script_file,
						 const gchar	*name,
						 const gchar	*options,
						 const gchar	*category,
						 const gchar	*blurb,
						 const gchar    *file,
						 guint           line,
						 const gchar	*authors,
						 const gchar	*license,
						 SfiRing	*params);
SfiRing*	bse_script_path_list_files	(void);
BseErrorType    bse_script_file_register	(const gchar	*file_name,
						 BseJanitor    **janitor_p);
GValue*		bse_script_check_client_msg	(SfiGlueDecoder *decoder,
						 BseJanitor	*janitor,
						 const gchar    *message,
						 const GValue   *value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SCRIPT_HELPER_H__ */
