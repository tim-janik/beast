/* BseFreeVerbCpp - Free Verb C++ Wrapper for BSE
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_FREE_VERB_CPP_H__
#define __BSE_FREE_VERB_CPP_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct
{
  /* runtime parameters */
  gfloat room_size;
  gfloat damp;
  gfloat wet;
  gfloat dry;
  gfloat width;
} BseFreeVerbConfig;
typedef struct
{
  /* constants */
  gfloat room_offset;
  gfloat room_scale;
  gfloat damp_scale;
  gfloat wet_scale;
  gfloat dry_scale;
  gfloat width_scale;
} BseFreeVerbConstants;
typedef struct
{
  gpointer obj;
  BseFreeVerbConfig saved_config;
} BseFreeVerbCpp;

void   bse_free_verb_cpp_create		(BseFreeVerbCpp		*cpp);
void   bse_free_verb_cpp_configure	(BseFreeVerbCpp		*cpp,
					 BseFreeVerbConfig	*config);
void   bse_free_verb_cpp_process	(BseFreeVerbCpp		*cpp,
					 guint			 n_values,
					 const gfloat		*ileft,
					 const gfloat		*iright,
					 gfloat			*oleft,
					 gfloat			*oright);
void   bse_free_verb_cpp_destroy	(BseFreeVerbCpp		*cpp);
void   bse_free_verb_cpp_defaults	(BseFreeVerbConfig	*config,
					 BseFreeVerbConstants	*constants);
void   bse_free_verb_cpp_save_config	(BseFreeVerbCpp		*cpp,
					 BseFreeVerbConfig	*config);
void   bse_free_verb_cpp_restore_config	(BseFreeVerbCpp		*cpp,
					 BseFreeVerbConfig	*config);


G_END_DECLS


#endif /* __BSE_FREE_VERB_CPP_H__ */
