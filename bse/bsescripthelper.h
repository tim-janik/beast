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
  gchar	 *script_file;
  gchar	 *name;
  gchar	 *blurb;
  gchar	 *help;
  gchar	 *author;
  gchar	 *copyright;
  gchar	 *date;
  GSList *params;
} BseScriptData;
typedef struct
{
  BseProcedureClass parent_class;
  BseScriptData    *sdata;
} BseScriptProcedureClass;


/* --- API --- */
GType		bse_script_proc_register	(const gchar	*script_file,
						 const gchar	*name,
						 const gchar	*category,
						 const gchar	*blurb,
						 const gchar	*help,
						 const gchar	*author,
						 const gchar	*copyright,
						 const gchar	*date,
						 GSList		*params);
GSList*		bse_script_dir_list_files	(const gchar	*dir_list);
const gchar*    bse_script_file_register	(const gchar	*file_name);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SCRIPT_HELPER_H__ */
