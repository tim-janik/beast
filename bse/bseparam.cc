/* BSE - Better Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bseparam.hh"
#include "bseobject.hh"


#define NULL_CHECKED(x)         ((x) && (x)[0] ? x : NULL)

/* --- prototypes --- */
extern void	bse_param_types_init	(void);	/* sync with btype.cc */


/* --- type initialization --- */
void
bse_param_types_init (void)	/* sync with btype.cc */
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
  
  pspec = g_param_spec_object (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), object_type, GParamFlags (0));
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
                     SfiReal      min_freq,
                     SfiReal      max_freq,
		     const gchar *hints)
{
#if 0
  if (!(default_freq >= min_freq && default_freq <= max_freq &&
        max_freq - min_freq >= 10 &&
        max_freq >= 15053 &&
        min_freq <= 51.9))
    g_printerr ("bse_param_spec_freq(\"%s\",\"%s\",\"%s\") assertion:\n", name, nick, blurb);
#endif
  g_return_val_if_fail (default_freq >= min_freq && default_freq <= max_freq, NULL);
  g_return_val_if_fail (max_freq - min_freq >= 10, NULL); /* check stepping */
  gdouble center = 2 * BSE_KAMMER_FREQUENCY, base = 2, n_steps = 4;
  g_return_val_if_fail (max_freq >= 15053, NULL); /* Ais+6 with A+1=444Hz */
  g_return_val_if_fail (min_freq <= 51.9, NULL);  /* As-1 with A+1=440Hz */

  GParamSpec *pspec = sfi_pspec_log_scale (name, nick, blurb,
                                           default_freq, min_freq, max_freq, 10.0,
                                           center, base, n_steps, hints);
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
  
  if (sfi_boxed_type_get_rec_fields (boxed_type).n_fields ||
      sfi_boxed_type_get_seq_element (boxed_type))
    {
      pspec = g_param_spec_boxed (name, nick, blurb, boxed_type, GParamFlags (0));
      sfi_pspec_set_options (pspec, hints);
    }
  else
    g_warning ("boxed parameter \"%s\" of type `%s' can't be converted to record or sequence",
	       name, g_type_name (boxed_type));
  return pspec;
}
