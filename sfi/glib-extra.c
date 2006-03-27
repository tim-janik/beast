/* GLib Extra - Tentative GLib code and GLib supplements
 * Copyright (C) 1997-2002 Tim Janik
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
// FIXME: #define	_GNU_SOURCE
#include <string.h>
#include "glib-extra.h"


void
g_object_disconnect_any (gpointer object,
                         gpointer function,
                         gpointer data)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (function != NULL);
  /* FIXME: the only reason we have this function is that
   * g_object_disconnect() throws a warning for an any-signal::
   * disconnection that does not exist (it may do so for all-signals
   * instead).
   */
  g_signal_handlers_disconnect_matched (object, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                        0, 0, 0,
                                        function, data);
}


/* --- string functions --- */
const gchar*
g_printf_find_localised_directive (const gchar *format)
{
  static const gchar *pass_flags = "#0- +0123456789*$hlLqjzt";
  static const gchar *locale_flags = "I'";
  static const gchar *pass_conversion = "%diouxXcCsSpn";
  static const gchar *locale_conversion = "eEfFgGaA";
  if (format)
    {
      const gchar *perc = strchr (format, '%');
      while (perc)
        {
          const gchar *p = perc + 1;
          while (strchr (pass_flags, *p))
            p++;
          if (strchr (locale_flags, *p) ||
              strchr (locale_conversion, *p))
            return perc;
          if (!strchr (pass_conversion, *p))
            return perc;        /* unable to determine directive specifier */
          perc = strchr (p + 1, '%');
        }
    }
  return NULL;
}

gchar**
g_straddv (gchar      **str_array,
	   const gchar *new_str)
{
  if (new_str)
    {
      if (!str_array)
	{
	  str_array = g_new (gchar*, 2);
	  str_array[0] = g_strdup (new_str);
	  str_array[1] = NULL;
	}
      else
	{
	  guint i = 0;

	  while (str_array[i])
	    i++;
	  str_array = g_renew (gchar*, str_array, i + 1 + 1);
	  str_array[i] = g_strdup (new_str);
	  i++;
	  str_array[i] = NULL;
	}
    }
  return str_array;
}

guint
g_strlenv (gchar **str_array)
{
  guint i = 0;

  if (str_array)
    while (str_array[i])
      i++;

  return i;
}

gchar**
g_strslistv (GSList *slist)
{
  gchar **str_array;
  guint i;

  if (!slist)
    return NULL;

  i = g_slist_length (slist);
  str_array = g_new (gchar*, i + 1);
  i = 0;
  while (slist)
    {
      str_array[i++] = g_strdup (slist->data);
      slist = slist->next;
    }
  str_array[i] = NULL;

  return str_array;
}

gchar*
g_strdup_stripped (const gchar *string)
{
  if (string)
    {
      const gchar *s = string;
      guint l;

      while (*s == ' ')
	s++;
      l = strlen (s);
      while (l && s[l - 1] == ' ')
	l--;
      return g_strndup (s, l);
    }
  return NULL;
}

gchar*
g_strdup_rstrip (const gchar *string)
{
  if (string)
    {
      guint l = strlen (string);
      while (l && string[l - 1] == ' ')
	l--;
      return g_strndup (string, l);
    }
  return NULL;
}

gchar*
g_strdup_lstrip (const gchar *string)
{
  if (string)
    {
      while (*string == ' ')
	string++;
      return g_strdup (string);
    }
  return NULL;
}

#if !GLIB_CHECK_VERSION (2, 9, 0)
const gchar*
g_intern_string (const gchar *string)
{
  return string ? g_quark_to_string (g_quark_from_string (string)) : NULL;
}

const gchar*
g_intern_static_string (const gchar   *string)
{
  return string ? g_quark_to_string (g_quark_from_static_string (string)) : NULL;
}
#endif

static gchar*
delim_concat_varargs (const gchar *first_string,
                      gchar        delim,
                      va_list      var_args)
{
  GString *gstring;
  gchar *s;

  if (!first_string)
    return NULL;

  gstring = g_string_new (first_string);
  s = va_arg (var_args, gchar*);
  while (s)
    {
      if (s[0])
        {
          if (gstring->len && gstring->str[gstring->len - 1] != delim &&
              s[0] != delim && delim)
            g_string_append_c (gstring, delim);
          g_string_append (gstring, s);
        }
      s = va_arg (var_args, gchar*);
    }

  return g_string_free (gstring, FALSE);
}

const gchar*
g_intern_strconcat (const gchar *first_string,
                    ...)
{
  const gchar *c = NULL;
  if (first_string)
    {
      gchar *s;
      va_list args;
      va_start (args, first_string);
      s = delim_concat_varargs (first_string, 0, args);
      va_end (args);
      c = g_intern_string (s);
      g_free (s);
    }
  return c;
}

const gchar*
g_intern_printf (const gchar   *format,
                 ...)
{
  if (format)
    {
      gchar *buffer;
      va_list args;
      va_start (args, format);
      buffer = g_strdup_vprintf (format, args);
      va_end (args);
      const gchar *istring = g_intern_string (buffer);
      g_free (buffer);
      return istring;
    }
  else
    return NULL;
}

gchar*
g_path_concat (const gchar *first_path,
               ...)
{
  va_list args;
  gchar *s;
  va_start (args, first_path);
  s = delim_concat_varargs (first_path, G_SEARCHPATH_SEPARATOR, args);
  va_end (args);
  return s;
}

GString*
g_string_prefix_lines (GString     *gstring,
                       const gchar *pstr)
{
  if (gstring->len && pstr)
    {
      guint l = strlen (pstr);
      gchar *p = gstring->str - 1;
      while (p)
        {
          guint pos = p - gstring->str + 1;
          g_string_insert (gstring, pos, pstr);
          pos += l;
          p = strchr (gstring->str + pos, '\n');
        }
    }
  return gstring;
}


/* --- string options --- */
gchar*
g_option_concat (const gchar *first_option,
                 ...)
{
  va_list args;
  gchar *s;
  va_start (args, first_option);
  s = delim_concat_varargs (first_option, ':', args);
  va_end (args);
  return s;
}

static const gchar*
g_option_find_value (const gchar *option_string,
                     const gchar *option)
{
  const gchar *p, *match = NULL;
  gint l = strlen (option);

  g_return_val_if_fail (l > 0, NULL);

  if (!option_string)
    return NULL;        /* option not found */

  /* try first match */
  p = strstr (option_string, option);
  if (p &&
      (p == option_string || p[-1] == ':') &&
      (p[l] == ':' || p[l] == 0 || p[l] == '=' ||
       ((p[l] == '-' || p[l] == '+') && (p[l + 1] == ':' || p[l + 1] == 0))))
    match = p;
  /* allow later matches to override */
  while (p)
    {
      p = strstr (p + l, option);
      if (p &&
          p[-1] == ':' &&
          (p[l] == ':' || p[l] == 0 || p[l] == '=' ||
           ((p[l] == '-' || p[l] == '+') && (p[l + 1] == ':' || p[l + 1] == 0))))
        match = p;
    }
  return match ? match + l : NULL;
}

gchar*
g_option_get (const gchar *option_string,
              const gchar *option)
{
  const gchar *value = NULL;

  if (option && option[0])
    value = g_option_find_value (option_string, option);

  if (!value)
    return NULL;                        /* option not present */
  else switch (value[0])
    {
      gchar *s;
    case ':':   return g_strdup ("1");  /* option was present, no modifier */
    case 0:     return g_strdup ("1");  /* option was present, no modifier */
    case '+':   return g_strdup ("1");  /* option was present, enable modifier */
    case '-':   return NULL;            /* option was present, disable modifier */
    case '=':                           /* option present with assignment */
      value++;
      s = strchr (value, ':');
      return s ? g_strndup (value, s - value) : g_strdup (value);
    default:    return NULL;            /* anything else, undefined */
    }
}

gboolean
g_option_check (const gchar *option_string,
                const gchar *option)
{
  const gchar *value = NULL;

  if (option && option[0])
    value = g_option_find_value (option_string, option);

  if (!value)
    return FALSE;                       /* option not present */
  else switch (value[0])
    {
      gchar *s;
    case ':':   return TRUE;            /* option was present, no modifier */
    case 0:     return TRUE;            /* option was present, no modifier */
    case '+':   return TRUE;            /* option was present, enable modifier */
    case '-':   return FALSE;           /* option was present, disable modifier */
    case '=':                           /* option present with assignment */
      value++;
      s = strchr (value, ':');
      if (!s || s == value)             /* empty assignment */
        return FALSE;
      else switch (s[0])
        {
        case '0': case 'f': case 'F':
        case 'n': case 'N':             /* false assigments */
          return FALSE;
        default: return TRUE;           /* anything else holds true */
        }
    default:    return FALSE;           /* anything else, undefined */
    }
}


/* --- GParamSpec options --- */
static GQuark quark_pspec_options = 0;
static guint
pspec_flags (const gchar *poptions)
{
  guint flags = 0;
  if (poptions)
    {
      if (g_option_check (poptions, "r"))
        flags |= G_PARAM_READABLE;
      if (g_option_check (poptions, "w"))
        flags |= G_PARAM_WRITABLE;
      if (g_option_check (poptions, "construct"))
        flags |= G_PARAM_CONSTRUCT;
      if (g_option_check (poptions, "construct-only"))
        flags |= G_PARAM_CONSTRUCT_ONLY;
      if (g_option_check (poptions, "lax-validation"))
        flags |= G_PARAM_LAX_VALIDATION;
    }
  return flags;
}

void
g_param_spec_set_options (GParamSpec  *pspec,
                          const gchar *options)
{
  if (!quark_pspec_options)
    quark_pspec_options = g_quark_from_static_string ("GParamSpec-options");
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  if (options)
    g_param_spec_set_qdata (pspec, quark_pspec_options, (gchar*) g_intern_string (options));
  /* pspec->flags &= ~G_PARAM_MASK; */
  pspec->flags |= pspec_flags (options);
}

gboolean
g_param_spec_check_option (GParamSpec  *pspec,
                           const gchar *option)
{
  const gchar *poptions;
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  poptions = g_param_spec_get_options (pspec);
  return g_option_check (poptions, option);
}

void
g_param_spec_add_option (GParamSpec  *pspec,
                         const gchar *option,
                         const gchar *value)
{
  const gchar *options;
  guint append = 0;
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (option != NULL && !strchr (option, ':'));
  g_return_if_fail (value == NULL || !strcmp (value, "-") || !strcmp (value, "+"));
  options = g_param_spec_get_options (pspec);
  if (!options)
    options = "";
  if (value && strcmp (value, "-") == 0 &&
      g_option_check (options, option))
    append = 2;
  else if ((!value || strcmp (value, "+") == 0) &&
           !g_option_check (options, option))
    append = 1;
  if (append)
    {
      guint l = strlen (options);
      gchar *s = g_strconcat (options,
                              options[l] == ':' ? "" : ":",
                              option, /* append >= 1 */
                              append >= 2 ? value : "",
                              NULL);
      g_param_spec_set_options (pspec, s);
      g_free (s);
    }
}

gboolean
g_param_spec_provides_options (GParamSpec  *pspec,
                               const gchar *options)
{
  const gchar *p;
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (options != NULL, FALSE);
 recurse:
  while (options[0] == ':')
    options++;
  if (!options[0])
    return TRUE;
  p = strchr (options, ':');
  if (p)
    {
      gchar *h = g_strndup (options, p - options);
      gboolean match = g_param_spec_check_option (pspec, h);
      g_free (h);
      if (!match)
        return FALSE;
      options = p + 1;
      goto recurse;
    }
  else
    return g_param_spec_check_option (pspec, options);
}

const gchar*
g_param_spec_get_options (GParamSpec *pspec)
{
  const gchar *options;
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  options = g_param_spec_get_qdata (pspec, quark_pspec_options);
  return options ? options : "";
}

static GQuark quark_pspec_istepping = 0;
static GQuark quark_pspec_istepping64 = 0;

void
g_param_spec_set_istepping (GParamSpec  *pspec,
                            guint64      stepping)
{
  if (!quark_pspec_istepping)
    {
      quark_pspec_istepping = g_quark_from_static_string ("GParamSpec-istepping");
      quark_pspec_istepping64 = g_quark_from_static_string ("GParamSpec-istepping64");
    }
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  if (stepping >> 32)
    {
      guint64 *istepping64 = g_new (guint64, 1);
      *istepping64 = stepping;
      g_param_spec_set_qdata_full (pspec, quark_pspec_istepping64, istepping64, g_free);
      g_param_spec_set_qdata (pspec, quark_pspec_istepping, 0);
    }
  else
    {
      g_param_spec_set_qdata (pspec, quark_pspec_istepping64, NULL);
      g_param_spec_set_qdata (pspec, quark_pspec_istepping, (void*) (guint32) stepping);
    }
}

guint64
g_param_spec_get_istepping (GParamSpec *pspec)
{
  guint64 stepping;
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), 0);
  stepping = (guint32) g_param_spec_get_qdata (pspec, quark_pspec_istepping);
  if (!stepping)
    {
      guint64 *istepping64 = g_param_spec_get_qdata (pspec, quark_pspec_istepping64);
      stepping = istepping64 ? *istepping64 : 0;
    }
  return stepping;
}

static GQuark quark_pspec_fstepping = 0;

void
g_param_spec_set_fstepping (GParamSpec  *pspec,
                            gdouble      stepping)
{
  if (!quark_pspec_fstepping)
    quark_pspec_fstepping = g_quark_from_static_string ("GParamSpec-fstepping");
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  if (stepping)
    {
      gdouble *fstepping = g_new (gdouble, 1);
      *fstepping = stepping;
      g_param_spec_set_qdata_full (pspec, quark_pspec_fstepping, fstepping, g_free);
    }
  else
    g_param_spec_set_qdata (pspec, quark_pspec_fstepping, NULL);
}

gdouble
g_param_spec_get_fstepping (GParamSpec *pspec)
{
  gdouble *fstepping;
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), 0);
  fstepping = g_param_spec_get_qdata (pspec, quark_pspec_fstepping);
  return fstepping ? *fstepping : 0;
}

typedef struct {
  gdouble center;
  gdouble base;
  gdouble n_steps;
} LogScale;

static GQuark quark_pspec_log_scale = 0;

void
g_param_spec_set_log_scale (GParamSpec  *pspec,
                            gdouble      center,
                            gdouble      base,
                            gdouble      n_steps)
{
  if (!quark_pspec_log_scale)
    quark_pspec_log_scale = g_quark_from_static_string ("GParamSpec-log-scale");
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  if (n_steps > 0 && base > 0)
    {
      LogScale *lscale = g_new0 (LogScale, 1);
      lscale->center = center;
      lscale->base = base;
      lscale->n_steps = n_steps;
      g_param_spec_set_qdata_full (pspec, quark_pspec_log_scale, lscale, g_free);
      g_param_spec_add_option (pspec, "log-scale", "+");
    }
  else
    g_param_spec_set_qdata (pspec, quark_pspec_log_scale, NULL);
}

gboolean
g_param_spec_get_log_scale (GParamSpec  *pspec,
                            gdouble     *center,
                            gdouble     *base,
                            gdouble     *n_steps)
{
  LogScale *lscale;
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  lscale = g_param_spec_get_qdata (pspec, quark_pspec_log_scale);
  if (lscale)
    {
      if (center)
        *center = lscale->center;
      if (base)
        *base = lscale->base;
      if (n_steps)
        *n_steps = lscale->n_steps;
      return TRUE;
    }
  return FALSE;
}


/* --- list extensions --- */
gpointer
g_slist_pop_head (GSList **slist_p)
{
  gpointer data;
  
  g_return_val_if_fail (slist_p != NULL, NULL);
  
  if (!*slist_p)
    return NULL;
  data = (*slist_p)->data;
  *slist_p = g_slist_delete_link (*slist_p, *slist_p);
  return data;
}

GSList*
g_slist_append_uniq (GSList  *slist,
		     gpointer data)
{
  GSList *tmp, *last = NULL;
  for (tmp = slist; tmp; last = tmp, tmp = last->next)
    if (tmp->data == data)
      return slist;
  if (!last)
    return g_slist_append (slist, data);
  last->next = g_slist_append (NULL, data);
  return slist;
}

gpointer
g_list_pop_head (GList **list_p)
{
  gpointer data;

  g_return_val_if_fail (list_p != NULL, NULL);

  if (!*list_p)
    return NULL;
  data = (*list_p)->data;
  *list_p = g_list_delete_link (*list_p, *list_p);
  return data;
}

void
g_slist_free_deep (GSList         *slist,
		   GDestroyNotify  data_destroy)
{
  while (slist)
    {
      gpointer data = g_slist_pop_head (&slist);
      data_destroy (data);
      data = g_slist_pop_head (&slist);
    }
}

void
g_list_free_deep (GList         *list,
		  GDestroyNotify data_destroy)
{
  while (list)
    {
      gpointer data = g_list_pop_head (&list);
      data_destroy (data);
      data = g_list_pop_head (&list);
    }
}


/* --- name conversions --- */
static inline gchar
check_lower (gchar c)
{
  return c >= 'a' && c <= 'z';
}

static inline gchar
check_upper (gchar c)
{
  return c >= 'A' && c <= 'Z';
}

static inline gchar
char_convert (gchar    c,
	      gchar    fallback,
	      gboolean want_upper)
{
  if (c >= '0' && c <= '9')
    return c;
  if (want_upper)
    {
      if (check_lower (c))
	return c - 'a' + 'A';
      else if (check_upper (c))
	return c;
    }
  else
    {
      if (check_upper (c))
	return c - 'A' + 'a';
      else if (check_lower (c))
	return c;
    }
  return fallback;
}

static gchar*
type_name_to_cname (const gchar *type_name,
		    const gchar *insert,
		    gchar        fallback,
		    gboolean     want_upper)
{
  const gchar *s;
  gchar *result, *p;
  guint was_upper, ilen;

  s = type_name;

  /* special casing for GLib types */
  if (strcmp (s, "GString") == 0)
    s = "GGString";			/* G_TYPE_GSTRING */
  else if (check_lower (s[0]))
    {
      static const struct {
	gchar *gname;
	gchar *xname;
      } glib_ftypes[] = {
	{ "gboolean",   "GBoolean" },
	{ "gchar",      "GChar" },
	{ "guchar",     "GUChar" },
	{ "gint",       "GInt" },
	{ "guint",      "GUInt" },
	{ "glong",      "GLong" },
	{ "gulong",     "GULong" },
	{ "gint64",     "GInt64" },
	{ "guint64",    "GUInt64" },
	{ "gfloat",     "GFloat" },
	{ "gdouble",    "GDouble" },
	{ "gpointer",   "GPointer" },
	{ "gchararray", "GString" },	/* G_TYPE_STRING */
      };
      guint i;

      for (i = 0; i < G_N_ELEMENTS (glib_ftypes); i++)
	if (strcmp (s, glib_ftypes[i].gname) == 0)
	  {
	    s = glib_ftypes[i].xname;
	    break;
	  }
    }

  ilen = strlen (insert);
  result = g_new (gchar, strlen (s) * 2 + ilen + 1);
  p = result;

  *p++ = char_convert (*s++, fallback, want_upper);
  while (*s && !check_upper (*s))
    *p++ = char_convert (*s++, fallback, want_upper);
  strcpy (p, insert);
  p += ilen;
  was_upper = 0;
  while (*s)
    {
      if (check_upper (*s))
	{
	  if (!was_upper || (s[1] && check_lower (s[1]) && was_upper >= 2))
	    *p++ = fallback;
	  was_upper++;
	}
      else
	was_upper = 0;
      *p++ = char_convert (*s, fallback, want_upper);
      s++;
    }
  *p++ = 0;

  return result;
}

gchar*
g_type_name_to_cname (const gchar *type_name)
{
  g_return_val_if_fail (type_name != NULL, NULL);

  return type_name_to_cname (type_name, "", '_', FALSE);
}

gchar*
g_type_name_to_sname (const gchar *type_name)
{
  g_return_val_if_fail (type_name != NULL, NULL);

  return type_name_to_cname (type_name, "", '-', FALSE);
}

gchar*
g_type_name_to_cupper (const gchar *type_name)
{
  g_return_val_if_fail (type_name != NULL, NULL);

  return type_name_to_cname (type_name, "", '_', TRUE);
}

gchar*
g_type_name_to_type_macro (const gchar *type_name)
{
  g_return_val_if_fail (type_name != NULL, NULL);

  return type_name_to_cname (type_name, "_TYPE", '_', TRUE);
}


/* --- simple main loop source --- */
typedef struct {
  GSource         source;
  GSourcePending  pending;
  GSourceDispatch dispatch;
  gboolean        last_pending;
  gpointer        data;
  GDestroyNotify  destroy;
} SimpleSource;

static gboolean
simple_source_prepare (GSource *source,
		       gint    *timeout_p)
{
  SimpleSource *ssource = (SimpleSource*) source;
  ssource->last_pending = ssource->pending (ssource->data, timeout_p);
  return ssource->last_pending;
}

static gboolean
simple_source_check (GSource *source)
{
  SimpleSource *ssource = (SimpleSource*) source;
  gint timeout = -1;
  if (!ssource->last_pending)
    ssource->last_pending = ssource->pending (ssource->data, &timeout);
  return ssource->last_pending;
}

static gboolean
simple_source_dispatch (GSource    *source,
			GSourceFunc callback,
			gpointer    user_data)
{
  SimpleSource *ssource = (SimpleSource*) source;
  ssource->dispatch (ssource->data);
  return TRUE;
}

static void
simple_source_finalize (GSource *source)
{
  SimpleSource *ssource = (SimpleSource*) source;

  /* this finalize handler may be run due to g_source_remove() called
   * from some dispatch() implementation, possibly causing reentrancy
   * problems (mutexes etc.). however, there's hardly anything we could
   * do about that to prevent it.
   */
  if (ssource->destroy)
    ssource->destroy (ssource->data);
}

GSource*
g_source_simple (gint            priority,
		 GSourcePending  pending,
		 GSourceDispatch dispatch,
		 gpointer        data,
		 GDestroyNotify  destroy,
		 GPollFD        *first_pfd,
		 ...)
{
  static GSourceFuncs simple_source_funcs = {
    simple_source_prepare,
    simple_source_check,
    simple_source_dispatch,
    simple_source_finalize,
  };
  SimpleSource *ssource;
  GSource *source;
  va_list var_args;
  GPollFD *pfd;

  g_return_val_if_fail (pending != NULL, NULL);
  g_return_val_if_fail (dispatch != NULL, NULL);

  source = g_source_new (&simple_source_funcs, sizeof (SimpleSource));
  g_source_set_priority (source, priority);
  ssource = (SimpleSource*) source;
  ssource->pending = pending;
  ssource->dispatch = dispatch;
  ssource->last_pending = FALSE;
  ssource->data = data;
  ssource->destroy = destroy;
  pfd = first_pfd;
  va_start (var_args, first_pfd);
  while (pfd)
    {
      g_source_add_poll (source, pfd);
      pfd = va_arg (var_args, GPollFD*);
    }
  va_end (var_args);
  return source;
}

/* --- predicate idle --- */
typedef struct {
  GSource     source;
  GSourceFunc predicate;
} PredicateIdle;

static gboolean
predicate_idle_prepare (GSource *source,
                        gint    *timeout)
{
  PredicateIdle *p = (PredicateIdle*) source;
  if (p->predicate)
    return p->predicate (source->callback_data);
  else
    return FALSE;
}

static gboolean
predicate_idle_check (GSource *source)
{
  PredicateIdle *p = (PredicateIdle*) source;
  if (p->predicate)
    return p->predicate (source->callback_data);
  else
    return FALSE;
}

static gboolean
predicate_idle_dispatch (GSource    *source,
                         GSourceFunc callback,
                         gpointer    user_data)
{
  if (callback)
    return callback (user_data);
  else
    return FALSE;
}

guint
g_predicate_idle_add_full (gint            priority,
                           GSourceFunc     predicate,
                           GSourceFunc     function,
                           gpointer        data,
                           GDestroyNotify  notify)
{
  static GSourceFuncs predicate_idle_funcs = { predicate_idle_prepare, predicate_idle_check, predicate_idle_dispatch, };
  g_return_val_if_fail (predicate && function, 0);
  GSource *source = g_source_new (&predicate_idle_funcs, sizeof (PredicateIdle));
  g_source_set_priority (source, priority);
  ((PredicateIdle*) source)->predicate = predicate;
  g_source_set_callback (source, function, data, notify);
  guint id = g_source_attach (source, NULL);
  g_source_unref (source);
  return id;
}

guint
g_predicate_idle_add (GSourceFunc     predicate,
                      GSourceFunc     function,
                      gpointer        data)
{
  return g_predicate_idle_add_full (G_PRIORITY_DEFAULT_IDLE, predicate, function, data, NULL);
}

/* --- GLib main loop reentrant signal queue --- */
#if 0
typedef struct _GUSignalData GUSignalData;
struct _GUSignalData
{
  guint8       index;
  guint8       shift;
  GUSignalFunc callback;
};

static gboolean g_usignal_prepare  (gpointer  source_data,
			 	    GTimeVal *current_time,
				    gint     *timeout,
				    gpointer  user_data);
static gboolean g_usignal_check    (gpointer  source_data,
				    GTimeVal *current_time,
				    gpointer  user_data);
static gboolean g_usignal_dispatch (gpointer  source_data,
				    GTimeVal *dispatch_time,
				    gpointer  user_data);

static GSourceFuncs usignal_funcs = {
  g_usignal_prepare,
  g_usignal_check,
  g_usignal_dispatch,
  g_free
};
static	guint32	usignals_notified[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

static gboolean
g_usignal_prepare (gpointer  source_data,
		   GTimeVal *current_time,
		   gint     *timeout,
		   gpointer  user_data)
{
  GUSignalData *usignal_data = source_data;
  
  return usignals_notified[usignal_data->index] & (1 << usignal_data->shift);
}

static gboolean
g_usignal_check (gpointer  source_data,
		 GTimeVal *current_time,
		 gpointer  user_data)
{
  GUSignalData *usignal_data = source_data;
  
  return usignals_notified[usignal_data->index] & (1 << usignal_data->shift);
}

static gboolean
g_usignal_dispatch (gpointer  source_data,
		    GTimeVal *dispatch_time,
		    gpointer  user_data)
{
  GUSignalData *usignal_data = source_data;
  
  usignals_notified[usignal_data->index] &= ~(1 << usignal_data->shift);
  
  return usignal_data->callback (-128 + usignal_data->index * 32 + usignal_data->shift, user_data);
}

guint
g_usignal_add (gint8	    usignal,
	       GUSignalFunc function,
	       gpointer     data)
{
  return g_usignal_add_full (G_PRIORITY_DEFAULT, usignal, function, data, NULL);
}

guint
g_usignal_add_full (gint           priority,
		    gint8          usignal,
		    GUSignalFunc   function,
		    gpointer       data,
		    GDestroyNotify destroy)
{
  GUSignalData *usignal_data;
  guint s = 128 + usignal;
  
  g_return_val_if_fail (function != NULL, 0);
  
  usignal_data = g_new (GUSignalData, 1);
  usignal_data->index = s / 32;
  usignal_data->shift = s % 32;
  usignal_data->callback = function;
  
  return g_source_add (priority, TRUE, &usignal_funcs, usignal_data, data, destroy);
}

void
g_usignal_notify (gint8 usignal)
{
  guint index, shift;
  guint s = 128 + usignal;
  
  index = s / 32;
  shift = s % 32;
  
  usignals_notified[index] |= 1 << shift;
}
#endif

GScanner*
g_scanner_new64 (const GScannerConfig *config_templ)
{
  if (!config_templ->store_int64)
    g_error ("%s(): attempt to create 64bit scanner with store_int64==FALSE", G_STRFUNC);
  return g_scanner_new (config_templ);
}
