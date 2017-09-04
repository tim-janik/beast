// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseparam.hh"
#include "bseobject.hh"


#define NULL_CHECKED(x)         ((x) && (x)[0] ? x : NULL)

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

  assert_return (g_type_is_a (object_type, BSE_TYPE_OBJECT), NULL);

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
    printerr ("bse_param_spec_freq(\"%s\",\"%s\",\"%s\") assertion:\n", name, nick, blurb);
#endif
  assert_return (default_freq >= min_freq && default_freq <= max_freq, NULL);
  assert_return (max_freq - min_freq >= 10, NULL); /* check stepping */
  gdouble center = 2 * BSE_KAMMER_FREQUENCY, base = 2, n_steps = 4;
  assert_return (max_freq >= 15053, NULL); /* Ais+6 with A+1=444Hz */
  assert_return (min_freq <= 51.9, NULL);  /* As-1 with A+1=440Hz */

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

  assert_return (G_TYPE_IS_BOXED (boxed_type), NULL);

  if (sfi_boxed_type_get_rec_fields (boxed_type).n_fields ||
      sfi_boxed_type_get_seq_element (boxed_type))
    {
      pspec = g_param_spec_boxed (name, nick, blurb, boxed_type, GParamFlags (0));
      sfi_pspec_set_options (pspec, hints);
    }
  else
    Bse::warning ("boxed parameter \"%s\" of type `%s' can't be converted to record or sequence",
                  name, g_type_name (boxed_type));
  return pspec;
}
