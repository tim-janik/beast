/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "GLibExtra"
#include	"glib-extra.h"


#include	<string.h>


/* string functions
 */
gchar*
g_strcanon (gchar       *string,
	    const gchar *extra_valid_chars,
	    gchar        subsitutor)
{
  register gchar *c;
  
  g_return_val_if_fail (string != NULL, NULL);
  
  if (!extra_valid_chars)
    extra_valid_chars = "";
  
  for (c = string; *c; c++)
    {
      if ((*c < 'a' || *c > 'z') &&
	  (*c < 'A' || *c > 'Z') &&
	  (*c < '0' || *c > '9') &&
	  !strchr (extra_valid_chars, *c))
	*c = subsitutor;
    }
  
  return string;
}

gchar*
g_strdup_quoted (const gchar *string)
{
  GString *gstring;
  gchar *retval;
  
  g_return_val_if_fail (string != NULL, NULL);
  
  gstring = g_string_new ("\"");
  while (*string)
    {
      switch (*string)
	{
	case '\\':
	  g_string_append (gstring, "\\\\");
	  break;
	case '\t':
	  g_string_append (gstring, "\\t");
	  break;
	case '\n':
	  g_string_append (gstring, "\\n");
	  break;
	case '\r':
	  g_string_append (gstring, "\\r");
	  break;
	case '\b':
	  g_string_append (gstring, "\\b");
	  break;
	case '\f':
	  g_string_append (gstring, "\\f");
	  break;
	default:
	  if (*string > 126 || *string < 32)
	    g_string_sprintfa (gstring, "\\%03o", (guchar) *string);
	  else
	    g_string_append_c (gstring, *string);
	  break;
	}
      string++;
    }
  g_string_append_c (gstring, '"');
  
  retval = gstring->str;
  g_string_free (gstring, FALSE);
  
  return retval;
}


/* GLib list stuff
 */
GSList*
g_slist_insert_before (GSList  *slist,
		       GSList  *sibling,
		       gpointer data)
{
  if (!slist)
    {
      slist = g_slist_alloc ();
      slist->data = data;
      g_return_val_if_fail (sibling == NULL, slist);
      return slist;
    }
  else
    {
      GSList *node, *last = NULL;

      for (node = slist; node; last = node, node = last->next)
	if (node == sibling)
	  break;
      if (!last)
	{
	  node = g_slist_alloc ();
	  node->data = data;
	  node->next = slist;

	  return node;
	}
      else
	{
	  node = g_slist_alloc ();
	  node->data = data;
	  node->next = last->next;
	  last->next = node;

	  return slist;
	}
    }
}

GList*
g_list_insert_before (GList   *list,
		      GList   *sibling,
		      gpointer data)
{
  if (!list)
    {
      list = g_list_alloc ();
      list->data = data;
      g_return_val_if_fail (sibling == NULL, list);
      return list;
    }
  else if (sibling)
    {
      GList *node;

      node = g_list_alloc ();
      node->data = data;
      if (sibling->prev)
	{
	  node->prev = sibling->prev;
	  node->prev->next = node;
	  node->next = sibling;
	  sibling->prev = node;
	  return list;
	}
      else
	{
	  node->next = sibling;
	  sibling->prev = node;
	  g_return_val_if_fail (sibling == list, node);
	  return node;
	}
    }
  else
    {
      GList *last;

      last = list;
      while (last->next)
	last = last->next;

      last->next = g_list_alloc ();
      last->next->data = data;
      last->next->prev = last;

      return list;
    }
}


/* GLib main loop reentrant signal queue
 */
typedef struct _GSignalData GSignalData;
struct _GSignalData
{
  guint8      index;
  guint8      shift;
  GSignalFunc callback;
};

static gboolean g_signal_prepare  (gpointer  source_data,
				   GTimeVal *current_time,
				   gint     *timeout,
				   gpointer  user_data);
static gboolean g_signal_check    (gpointer  source_data,
				   GTimeVal *current_time,
				   gpointer  user_data);
static gboolean g_signal_dispatch (gpointer  source_data,
				   GTimeVal *current_time,
				   gpointer  user_data);

static GSourceFuncs signal_funcs = {
  g_signal_prepare,
  g_signal_check,
  g_signal_dispatch,
  g_free
};
static	guint32	signals_notified[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

static gboolean
g_signal_prepare (gpointer  source_data,
		  GTimeVal *current_time,
		  gint     *timeout,
		  gpointer  user_data)
{
  GSignalData *signal_data = source_data;
  
  return signals_notified[signal_data->index] & (1 << signal_data->shift);
}

static gboolean
g_signal_check (gpointer  source_data,
		GTimeVal *current_time,
		gpointer  user_data)
{
  GSignalData *signal_data = source_data;
  
  return signals_notified[signal_data->index] & (1 << signal_data->shift);
}

static gboolean
g_signal_dispatch (gpointer  source_data,
		   GTimeVal *current_time,
		   gpointer  user_data)
{
  GSignalData *signal_data = source_data;
  
  signals_notified[signal_data->index] &= ~(1 << signal_data->shift);
  
  return signal_data->callback (-128 + signal_data->index * 32 + signal_data->shift, user_data);
}

guint
g_signal_add (gint8	  signal,
	      GSignalFunc function,
	      gpointer    data)
{
  return g_signal_add_full (G_PRIORITY_DEFAULT, signal, function, data, NULL);
}

guint
g_signal_add_full (gint           priority,
		   gint8          signal,
		   GSignalFunc    function,
		   gpointer       data,
		   GDestroyNotify destroy)
{
  GSignalData *signal_data;
  guint s = 128 + signal;
  
  g_return_val_if_fail (function != NULL, 0);
  
  signal_data = g_new (GSignalData, 1);
  signal_data->index = s / 32;
  signal_data->shift = s % 32;
  signal_data->callback = function;
  
  return g_source_add (priority, TRUE, &signal_funcs, signal_data, data, destroy);
}

void
g_signal_notify (gint8 signal)
{
  guint index, shift;
  guint s = 128 + signal;
  
  index = s / 32;
  shift = s % 32;
  
  signals_notified[index] |= 1 << shift;
}

/* Glib pattern matching, featuring "?" and "*" wildcards
 */
static inline gboolean
g_pattern_ph_match (const gchar *match_pattern,
                    const gchar *match_string)
{
  register const gchar *pattern, *string;
  register gchar ch;
  
  pattern = match_pattern;
  string = match_string;
  
  ch = *pattern;
  pattern++;
  while (ch)
    {
      switch (ch)
        {
        case '?':
          if (!*string)
            return FALSE;
          string++;
          break;
          
        case '*':
          do
            {
              ch = *pattern;
              pattern++;
              if (ch == '?')
                {
                  if (!*string)
                    return FALSE;
                  string++;
                }
            }
          while (ch == '*' || ch == '?');
          if (!ch)
            return TRUE;
          do
            {
              while (ch != *string)
                {
                  if (!*string)
                    return FALSE;
                  string++;
                }
              string++;
              if (g_pattern_ph_match (pattern, string))
                return TRUE;
            }
          while (*string);
          break;
          
        default:
          if (ch == *string)
            string++;
          else
            return FALSE;
          break;
        }
      
      ch = *pattern;
      pattern++;
    }
  
  return *string == 0;
}

gboolean
g_pattern_match (GPatternSpec *pspec,
                 guint         string_length,
                 const gchar  *string,
                 const gchar  *string_reversed)
{
  g_return_val_if_fail (pspec != NULL, FALSE);
  g_return_val_if_fail (string != NULL, FALSE);
  g_return_val_if_fail (string_reversed != NULL, FALSE);
  
  switch (pspec->match_type)
    {
    case G_MATCH_ALL:
      return g_pattern_ph_match (pspec->pattern, string);
      
    case G_MATCH_ALL_TAIL:
      return g_pattern_ph_match (pspec->pattern_reversed, string_reversed);
      
    case G_MATCH_HEAD:
      if (pspec->pattern_length > string_length)
        return FALSE;
      else if (pspec->pattern_length == string_length)
        return strcmp (pspec->pattern, string) == 0;
      else if (pspec->pattern_length)
        return strncmp (pspec->pattern, string, pspec->pattern_length) == 0;
      else
        return TRUE;
      
    case G_MATCH_TAIL:
      if (pspec->pattern_length > string_length)
        return FALSE;
      else if (pspec->pattern_length == string_length)
        return strcmp (pspec->pattern_reversed, string_reversed) == 0;
      else if (pspec->pattern_length)
        return strncmp (pspec->pattern_reversed,
                        string_reversed,
                        pspec->pattern_length) == 0;
      else
        return TRUE;
      
    case G_MATCH_EXACT:
      if (pspec->pattern_length != string_length)
        return FALSE;
      else
        return strcmp (pspec->pattern_reversed, string_reversed) == 0;
      
    default:
      g_return_val_if_fail (pspec->match_type < G_MATCH_LAST, FALSE);
      return FALSE;
    }
}

GPatternSpec*
g_pattern_spec_new (const gchar *pattern)
{
  GPatternSpec *pspec;
  gchar *p, *t;
  const gchar *h;
  guint hw = 0, tw = 0, hj = 0, tj = 0;
  
  g_return_val_if_fail (pattern != NULL, NULL);

  pspec = g_new (GPatternSpec, 1);
  pspec->pattern_length = strlen (pattern);
  pspec->pattern = strcpy (g_new (gchar, pspec->pattern_length + 1), pattern);
  pspec->pattern_reversed = g_new (gchar, pspec->pattern_length + 1);
  t = pspec->pattern_reversed + pspec->pattern_length;
  *(t--) = 0;
  h = pattern;
  while (t >= pspec->pattern_reversed)
    {
      register gchar c = *(h++);

      if (c == '*')
	{
	  if (t < h)
	    hw++;
	  else
	    tw++;
	}
      else if (c == '?')
	{
	  if (t < h)
	    hj++;
	  else
	    tj++;
	}

      *(t--) = c;
    }
  pspec->match_type = hw > tw || (hw == tw && hj > tj) ? G_MATCH_ALL_TAIL : G_MATCH_ALL;
  
  if (hj || tj)
    return pspec;
  
  if (hw == 0 && tw == 0)
    {
      pspec->match_type = G_MATCH_EXACT;
      return pspec;
    }

  if (hw)
    {
      p = pspec->pattern;
      while (*p == '*')
	p++;
      if (p > pspec->pattern && !strchr (p, '*'))
	{
	  gchar *tmp;
	  
	  pspec->match_type = G_MATCH_TAIL;
	  pspec->pattern_length = strlen (p);
	  tmp = pspec->pattern;
	  pspec->pattern = strcpy (g_new (gchar, pspec->pattern_length + 1), p);
	  g_free (tmp);
	  g_free (pspec->pattern_reversed);
	  pspec->pattern_reversed = g_new (gchar, pspec->pattern_length + 1);
	  t = pspec->pattern_reversed + pspec->pattern_length;
	  *(t--) = 0;
	  h = pspec->pattern;
	  while (t >= pspec->pattern_reversed)
	    *(t--) = *(h++);
	  return pspec;
	}
    }

  if (tw)
    {
      p = pspec->pattern_reversed;
      while (*p == '*')
	p++;
      if (p > pspec->pattern_reversed && !strchr (p, '*'))
	{
	  gchar *tmp;
	  
	  pspec->match_type = G_MATCH_HEAD;
	  pspec->pattern_length = strlen (p);
	  tmp = pspec->pattern_reversed;
	  pspec->pattern_reversed = strcpy (g_new (gchar, pspec->pattern_length + 1), p);
	  g_free (tmp);
	  g_free (pspec->pattern);
	  pspec->pattern = g_new (gchar, pspec->pattern_length + 1);
	  t = pspec->pattern + pspec->pattern_length;
	  *(t--) = 0;
	  h = pspec->pattern_reversed;
	  while (t >= pspec->pattern)
	    *(t--) = *(h++);
	}
    }

  return pspec;
}

gboolean
g_pattern_match_string (GPatternSpec *pspec,
                        const gchar  *string)
{
  gchar *string_reversed, *t;
  const gchar *h;
  guint length;
  gboolean ergo;
  
  g_return_val_if_fail (pspec != NULL, FALSE);
  g_return_val_if_fail (string != NULL, FALSE);
  
  length = strlen (string);
  string_reversed = g_new (gchar, length + 1);
  t = string_reversed + length;
  *(t--) = 0;
  h = string;
  while (t >= string_reversed)
    *(t--) = *(h++);
  
  ergo = g_pattern_match (pspec, length, string, string_reversed);
  g_free (string_reversed);
  
  return ergo;
}

gboolean
g_pattern_match_simple (const gchar *pattern,
                        const gchar *string)
{
  GPatternSpec *pspec;
  gboolean ergo;
  
  g_return_val_if_fail (pattern != NULL, FALSE);
  g_return_val_if_fail (string != NULL, FALSE);
  
  pspec = g_pattern_spec_new (pattern);
  ergo = g_pattern_match_string (pspec, string);
  g_pattern_spec_free (pspec);
  
  return ergo;
}

void
g_pattern_spec_free (GPatternSpec *pspec)
{
  g_return_if_fail (pspec != NULL);
  
  g_free (pspec->pattern);
  pspec->pattern = NULL;
  g_free (pspec->pattern_reversed);
  pspec->pattern_reversed = NULL;
  g_free (pspec);
}
