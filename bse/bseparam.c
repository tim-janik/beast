/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
#include "bseparam.h"
#include "bseobject.h"


#define NULL_CHECKED(x)         ((x) && (x)[0] ? x : NULL)

/* --- prototypes --- */
extern void	bse_param_types_init	(void);	/* sync with btype.c */


/* --- type initialization --- */
void
bse_param_types_init (void)	/* sync with btype.c */
{
}


/* --- object param specs --- */
GParamSpec*
bse_param_spec_object (const gchar    *name,
		       const gchar    *nick,
		       const gchar    *blurb,
		       GType           object_type,
		       const gchar    *hints)
{
  GParamSpec *pspec;
  
  g_return_val_if_fail (g_type_is_a (object_type, BSE_TYPE_OBJECT), NULL);
  
  pspec = g_param_spec_object (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), object_type, 0);
  sfi_pspec_set_options (pspec, hints);
  sfi_pspec_add_option (pspec, "skip-default", "+");
  
  return pspec;
}

GValue*
bse_value_object (gpointer vobject)
{
  GValue *value = sfi_value_empty ();
  g_value_init (value, BSE_TYPE_OBJECT);
  bse_value_set_object (value, vobject);
  return value;
}


/* --- convenience pspec constructors --- */
GParamSpec*
bse_param_spec_freq (const gchar *name,
		     const gchar *nick,
		     const gchar *blurb,
		     SfiReal      default_freq,
		     const gchar *hints)
{
  GParamSpec *pspec;
  
  g_return_val_if_fail (default_freq >= BSE_MIN_OSC_FREQUENCY_f && default_freq <= BSE_MAX_OSC_FREQUENCY_f, NULL);
  
  pspec = sfi_pspec_log_scale (name, nick, blurb,
			       default_freq, BSE_MIN_OSC_FREQUENCY_f, BSE_MAX_OSC_FREQUENCY_f, 10.0,
			       2 * BSE_KAMMER_FREQUENCY_f, 2, 4,
			       hints);
  
  return pspec;
}

GParamSpec*
bse_param_spec_boxed (const gchar *name,
		      const gchar *nick,
		      const gchar *blurb,
		      GType        boxed_type,
		      const gchar *hints)
{
  GParamSpec *pspec = NULL;
  
  g_return_val_if_fail (G_TYPE_IS_BOXED (boxed_type), NULL);
  
  if (sfi_boxed_get_record_info (boxed_type) ||
      sfi_boxed_get_sequence_info (boxed_type) ||
      sfi_boxed_type_get_rec_fields (boxed_type).n_fields ||
      sfi_boxed_type_get_seq_element (boxed_type))
    {
      pspec = g_param_spec_boxed (name, nick, blurb, boxed_type, 0);
      sfi_pspec_set_options (pspec, hints);
    }
  else
    g_warning ("boxed parameter \"%s\" of type `%s' can't be converted to record or sequence",
	       name, g_type_name (boxed_type));
  return pspec;
}
