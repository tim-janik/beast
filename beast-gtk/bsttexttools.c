/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsttexttools.h"
#include "bstutils.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


/* --- prototypes --- */
static void	text_buffer_add_error	(GtkTextBuffer	*tbuffer,
					 const gchar	*format,
					 ...) G_GNUC_PRINTF (2, 3);


/* --- functions --- */
static void
text_buffer_add_error (GtkTextBuffer *tbuffer,
		       const gchar   *format,
		       ...)
{
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table (tbuffer);
  GtkTextTag *tag = gtk_text_tag_table_lookup (table, "bst-text-tools-error");
  GtkTextIter iter;
  gchar *string, *text;
  va_list args;
  if (!tag)
    {
      tag = g_object_new (GTK_TYPE_TEXT_TAG,
			  "name", "bst-text-tools-error",
			  "foreground", "#ff0000",
			  "background", "#000000",
			  "wrap_mode", GTK_WRAP_WORD,
			  NULL);
      g_object_set_int (tag, "bst-text-tools-owned", 1);
      gtk_text_tag_table_add (table, tag);
      g_object_unref (tag);
    }
  gtk_text_buffer_get_end_iter (tbuffer, &iter);
  va_start (args, format);
  text = g_strdup_vprintf (format, args);
  va_end (args);
  string = g_strdup_printf ("\n%s\n", text);
  g_free (text);
  gtk_text_buffer_insert_with_tags (tbuffer, &iter, string, strlen (string), tag, NULL);
  g_free (string);
}

static GSList *image_paths = NULL;
void
bst_text_add_tsm_path (const gchar *path)
{
  if (path)
    image_paths = g_slist_append (image_paths, g_strdup (path));
}

static GdkPixbuf*
pixbuf_new_from_path (const gchar *file_name,
		      GError     **error)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (file_name, error);
  if (*error && !g_path_is_absolute (file_name))
    {
      GSList *slist;
      for (slist = image_paths; slist && !pixbuf; slist = slist->next)
	{
	  gchar *loc = g_strconcat (slist->data, G_DIR_SEPARATOR_S, file_name, NULL);
	  pixbuf = gdk_pixbuf_new_from_file (loc, NULL);
	  g_free (loc);
	}
      if (pixbuf)
	g_clear_error (error);
    }
  return pixbuf;
}


/* --- TextTag property setup --- */
static inline gchar
char2eval (gchar c)
{
  if (c >= '0' && c <= '9')
    return c;
  else if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  else if (c >= 'a' && c <= 'z')
    return c;
  else
    return '-';
}

static inline gboolean
enum_match (const gchar *str1,
	    const gchar *str2)
{
  while (*str1 && *str2)
    {
      guchar s1 = char2eval (*str1++);
      guchar s2 = char2eval (*str2++);
      if (s1 != s2)
	return FALSE;
    }
  return *str1 == 0 && *str2 == 0;
}

static gint
enum_match_value (GEnumClass  *eclass,
		  const gchar *name,
		  gint         fallback)
{
  guint i, length = strlen (name);
  
  for (i = 0; i < eclass->n_values; i++)
    {
      gchar *vname = eclass->values[i].value_name;
      guint n = strlen (vname);
      if (n >= length && enum_match (vname + n - length, name))
	return eclass->values[i].value;
    }
  for (i = 0; i < eclass->n_values; i++)
    {
      gchar *vname = eclass->values[i].value_nick;
      guint n = strlen (vname);
      if (n >= length && enum_match (vname + n - length, name))
	return eclass->values[i].value;
    }
  return fallback;
}

static inline gboolean
check_boolean (const gchar *value)
{
  return !(strlen (value) < 1 || value[0] == '0' ||
	   value[0] == 'f' || value[0] == 'F' ||
	   value[0] == 'n' || value[0] == 'N');
}

static gboolean
text_buffer_tagdef (GtkTextBuffer *tbuffer,
		    const gchar   *tag_name,
		    const gchar   *property,
		    const gchar   *tag_value)
{
  GtkTextTagTable *ttable = gtk_text_buffer_get_tag_table (tbuffer);
  GtkTextTag *tag = gtk_text_tag_table_lookup (ttable, tag_name);
  GValue value = { 0, };
  GParamSpec *pspec = NULL;
  if (!tag)
    {
      tag = g_object_new (GTK_TYPE_TEXT_TAG,
			  "name", tag_name,
			  NULL);
      g_object_set_int (tag, "bst-text-tools-owned", 1);
      gtk_text_tag_table_add (ttable, tag);
      g_object_unref (tag);
    }
  if (property)
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tag), property);
  switch (pspec ? G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)) : 0)
    {
      gdouble v_float;
    case G_TYPE_BOOLEAN:
      g_value_init (&value, G_TYPE_BOOLEAN);
      g_value_set_boolean (&value, check_boolean (tag_value));
      break;
    case G_TYPE_STRING:
      g_value_init (&value, G_TYPE_STRING);
      g_value_set_string (&value, tag_value);
      break;
    case G_TYPE_INT:
      g_value_init (&value, G_TYPE_INT);
      v_float = g_strtod (tag_value, NULL);
      g_value_set_int (&value, v_float > 0 ? v_float + 0.5 : v_float - 0.5);
      break;
    case G_TYPE_FLOAT:
    case G_TYPE_DOUBLE:
      g_value_init (&value, G_TYPE_DOUBLE);
      g_value_set_double (&value, g_strtod (tag_value, NULL));
      break;
    case G_TYPE_ENUM:
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_value_set_enum (&value,
			enum_match_value (G_PARAM_SPEC_ENUM (pspec)->enum_class,
					  tag_value,
					  G_PARAM_SPEC_ENUM (pspec)->default_value));
      break;
    default:
      return FALSE;
    }
  if (G_VALUE_TYPE (&value))
    {
      g_object_set_property (G_OBJECT (tag), property, &value);
      g_value_unset (&value);
    }
  return TRUE;
}


/* --- TagSpanMarkup parser --- */
typedef struct _TsmLevel TsmLevel;
typedef struct {
  GtkTextBuffer *tbuffer;
  guint		 tagns;
  gint           lc;
  guint          opos;
  TsmLevel      *lstack;
  guint          after_whitespace : 1;
  guint          after_newline : 1;
  GtkStyle      *style;
} TsmData;

static GtkStyle*
tsm_data_ensure_style (TsmData *md)
{
  if (!md->style)
    md->style = g_object_new (GTK_TYPE_STYLE, NULL);
  return md->style;
}

static gboolean
data_tag_event (GtkTextTag        *tag,
		GObject           *event_object, /* widget, canvas item, whatever */
		GdkEvent          *event,        /* the event itself */
		const GtkTextIter *iter,         /* location of event in buffer */
		TsmData           *md)
{
  if (event->type == GDK_BUTTON_PRESS)
    {
      const gchar *data = g_object_get_data (G_OBJECT (tag), "data");
      g_message ("text activation with data: %s\n", data);
    }
  else if (0)
    {
      GEnumClass *eclass = g_type_class_ref (GDK_TYPE_EVENT_TYPE);
      GEnumValue *ev = g_enum_get_value (eclass, event->type);
      g_message ("TextTagEvent: %s\n", ev->value_name);
      g_type_class_unref (eclass);
    }
  return FALSE;
}

struct _TsmLevel
{
  guint        strip_space : 1;
  gchar       *tag_name;
  GtkTextTag  *tag;
  GtkTextMark *mark;
  TsmLevel    *next;
};

static const TsmLevel*
tsm_peek_level (TsmData *md)
{
  static const TsmLevel template = {
    TRUE,	/* strip_space */
    NULL,	/* tag_name */
    NULL,	/* mark */
    NULL,	/* next */
  };
  return md->lstack ? md->lstack : &template;
}

static TsmLevel*
tsm_push_level (TsmData *md)
{
  const TsmLevel *src = tsm_peek_level (md);
  TsmLevel *ml = g_memdup (src, sizeof (TsmLevel));
  GtkTextIter iter;
  ml->tag_name = NULL;
  ml->tag = NULL;
  gtk_text_buffer_get_iter_at_mark (md->tbuffer, &iter, gtk_text_buffer_get_insert (md->tbuffer));
  ml->mark = gtk_text_buffer_create_mark (md->tbuffer, NULL, &iter, TRUE);
  ml->next = md->lstack;
  md->lstack = ml;
  return ml;
}

static void
tsm_pop_level (TsmData *md)
{
  TsmLevel *ml = md->lstack;
  g_return_if_fail (ml != NULL);
  md->lstack = ml->next;
  if (ml->mark)
    gtk_text_buffer_delete_mark (md->tbuffer, ml->mark);
  g_free (ml->tag_name);
  g_free (ml);
}

/* Called for open tags <foo bar="baz"> */
static void
tsm_start_element  (GMarkupParseContext *context,
		    const gchar         *element_name,
		    const gchar        **attribute_names,
		    const gchar        **attribute_values,
		    gpointer             user_data,
		    GError             **error)
{
  TsmData *md = user_data;
  
  if (strcmp (element_name, "tagdef") == 0)
    {
      guint i;
      for (i = 0; attribute_names[i]; i++)
	if (strcmp (attribute_names[i], "name") == 0)
	  break;
      if (attribute_names[i])
	{
	  const gchar *errtname = attribute_values[i];
	  gchar *tname = g_strdup_printf ("%u-%s", md->tagns, attribute_values[i]);
	  text_buffer_tagdef (md->tbuffer, tname, NULL, NULL);
	  for (i = 0; attribute_names[i]; i++)
	    if (strcmp (attribute_names[i], "name") != 0 &&
		strcmp (attribute_names[i], "comment") != 0)
	      {
		if (!text_buffer_tagdef (md->tbuffer, tname, attribute_names[i], attribute_values[i]))
		  {
		    gint c = -1, l = -1;
		    g_markup_parse_context_get_position (context, &c, &l);
		    text_buffer_add_error (md->tbuffer, "Invalid property within tagdef \"%s\" on line %d char %d: %s",
					   errtname, c, l, attribute_names[i]);
		  }
	      }
	  g_free (tname);
	}
    }
  else if (strcmp (element_name, "span") == 0)
    {
      TsmLevel *ml;
      guint i;
      ml = tsm_push_level (md);
      for (i = 0; attribute_names[i]; i++)
	if (strcmp (attribute_names[i], "tag") == 0)
	  {
	    g_free (ml->tag_name);
	    ml->tag_name = g_strdup (attribute_values[i]);
	  }
    }
  else if (strcmp (element_name, "strip-space") == 0)
    {
      TsmLevel *ml = tsm_push_level (md);
      ml->strip_space = TRUE;
    }
  else if (strcmp (element_name, "keep-space") == 0)
    {
      TsmLevel *ml = tsm_push_level (md);
      ml->strip_space = FALSE;
    }
  else if (strcmp (element_name, "activatable") == 0)
    {
      TsmLevel *ml = tsm_push_level (md);
      guint i;
      for (i = 0; attribute_names[i]; i++)
	if (strcmp (attribute_names[i], "data") == 0)
	  {
	    GtkTextTagTable *ttable = gtk_text_buffer_get_tag_table (md->tbuffer);
	    ml->tag = g_object_new (GTK_TYPE_TEXT_TAG, NULL);
	    g_object_set_int (ml->tag, "bst-text-tools-owned", 1);
	    g_object_set_data_full (G_OBJECT (ml->tag), "data", g_strdup (attribute_values[i]), g_free);
	    g_object_connect (ml->tag, "signal::event", data_tag_event, md, NULL);
	    gtk_text_tag_table_add (ttable, ml->tag);
	    g_object_unref (ml->tag);
	    break;
	  }
    }
  else if (strcmp (element_name, "image") == 0)
    {
      GdkPixbuf *pixbuf = NULL;
      GtkIconSet *iset = NULL;
      GtkIconSize isize = GTK_ICON_SIZE_MENU;
      gint i, c, l;
      for (i = 0; attribute_names[i]; i++)
	if (!pixbuf && strcmp (attribute_names[i], "file") == 0)
	  {
	    GError *pixerror = NULL;
	    pixbuf = pixbuf_new_from_path (attribute_values[i], &pixerror);
	    if (!pixbuf)
	      {
		g_markup_parse_context_get_position (context, &c, &l);
		text_buffer_add_error (md->tbuffer, "Failed to load pixbuf \"%s\" for image tag on line %d char %d: %s",
				       attribute_values[i], c, l, pixerror->message);
	      }
	    g_clear_error (&pixerror);
	  }
	else if (!iset && strcmp (attribute_names[i], "stock") == 0)
	  {
	    iset = gtk_icon_factory_lookup_default (attribute_values[i]);
	    if (!iset)
	      {
		g_markup_parse_context_get_position (context, &c, &l);
		text_buffer_add_error (md->tbuffer, "Unknown stock id \"%s\" for image tag on line %d char %d",
				       attribute_values[i], c, l);
	      }
	  }
	else if (strcmp (attribute_names[i], "size") == 0)
	  {
	    GEnumClass *eclass = g_type_class_ref (GTK_TYPE_ICON_SIZE);
	    gint c, l = enum_match_value (eclass, attribute_values[i], GTK_ICON_SIZE_INVALID);
	    g_type_class_unref (eclass);
	    if (l == GTK_ICON_SIZE_INVALID)
	      l = gtk_icon_size_from_name (attribute_values[i]);
	    if (l != GTK_ICON_SIZE_INVALID)
	      isize = l;
	    else
	      {
		g_markup_parse_context_get_position (context, &c, &l);
		text_buffer_add_error (md->tbuffer, "Unknown icon size \"%s\" for image tag on line %d char %d",
				       attribute_values[i], c, l);
	      }
	  }
      if (!pixbuf && iset)
	pixbuf = g_object_ref (gtk_icon_set_render_icon (iset, tsm_data_ensure_style (md),
							 GTK_TEXT_DIR_NONE, GTK_STATE_NORMAL,
							 isize, NULL, NULL));
      if (pixbuf)
	{
	  GtkTextIter iter;
	  gtk_text_buffer_get_iter_at_mark (md->tbuffer, &iter, gtk_text_buffer_get_insert (md->tbuffer));
	  gtk_text_buffer_insert_pixbuf (md->tbuffer, &iter, pixbuf);
	  g_object_unref (pixbuf);
	}
    }
  else if (strcmp (element_name, "breakline") == 0)
    {
      if (!md->after_newline)
	{
	  gtk_text_buffer_insert_at_cursor (md->tbuffer, "\n", 1);
	  md->after_whitespace = md->after_newline = TRUE;
	}
    }
  else if (strcmp (element_name, "newline") == 0)
    {
      gtk_text_buffer_insert_at_cursor (md->tbuffer, "\n", 1);
      md->after_whitespace = md->after_newline = TRUE;
    }
  else
    text_buffer_add_error (md->tbuffer, "<%s>", element_name);
}

/* Called for close tags </foo> */
static void
tsm_end_element (GMarkupParseContext *context,
		 const gchar         *element_name,
		 gpointer             user_data,
		 GError             **error)
{
  TsmData *md = user_data;
  
  if (strcmp (element_name, "tagdef") == 0)
    {
    }
  else if (strcmp (element_name, "span") == 0)
    {
      const TsmLevel *ml = tsm_peek_level (md);
      if (ml->tag_name)
	{
	  GtkTextTagTable *ttable = gtk_text_buffer_get_tag_table (md->tbuffer);
	  gchar *tname = g_strdup_printf ("%u-%s", md->tagns, ml->tag_name);
	  GtkTextTag *tag = gtk_text_tag_table_lookup (ttable, tname);
	  g_free (tname);
	  if (tag)
	    {
	      GtkTextIter iter1, iter2;
	      gtk_text_buffer_get_iter_at_mark (md->tbuffer, &iter1, ml->mark);
	      gtk_text_buffer_get_iter_at_mark (md->tbuffer, &iter2, gtk_text_buffer_get_insert (md->tbuffer));
	      gtk_text_buffer_apply_tag (md->tbuffer, tag, &iter1, &iter2);
	    }
	  else
	    {
	      gint c = -1, l = -1;
	      g_markup_parse_context_get_position (context, &c, &l);
	      text_buffer_add_error (md->tbuffer, "Span refers to invalid tag: Error on line %d char %d: tag \"%s\" unknown",
				     c, l, ml->tag_name);
	    }
	}
      tsm_pop_level (md);
    }
  else if (strcmp (element_name, "strip-space") == 0)
    tsm_pop_level (md);
  else if (strcmp (element_name, "keep-space") == 0)
    tsm_pop_level (md);
  else if (strcmp (element_name, "activatable") == 0)
    {
      const TsmLevel *ml = tsm_peek_level (md);
      if (ml->tag)
	{
	  GtkTextIter iter1, iter2;
	  gtk_text_buffer_get_iter_at_mark (md->tbuffer, &iter1, ml->mark);
	  gtk_text_buffer_get_iter_at_mark (md->tbuffer, &iter2, gtk_text_buffer_get_insert (md->tbuffer));
	  gtk_text_buffer_apply_tag (md->tbuffer, ml->tag, &iter1, &iter2);
	}
      tsm_pop_level (md);
    }
  else if (strcmp (element_name, "image") == 0)
    {
    }
  else if (strcmp (element_name, "newline") == 0)
    {
    }
  else if (strcmp (element_name, "breakline") == 0)
    {
    }
  else
    text_buffer_add_error (md->tbuffer, "</%s>", element_name);
}

/* Called for character data */
static void
tsm_text (GMarkupParseContext *context,
	  const gchar         *text,	/* text is not nul-terminated */
	  gsize                text_len,
	  gpointer             user_data,
	  GError             **error)
{
  TsmData *md = user_data;
  gchar *string = g_new (gchar, text_len + 1), *dest = string;
  const TsmLevel *ml = tsm_peek_level (md);
  guint i;
  
  if (0)
    gtk_text_buffer_insert_at_cursor (md->tbuffer, text, text_len);
  
  for (i = 0; i < text_len; i++)
    switch (text[i])
      {
      case ' ': case '\t': case '\v': case '\f': case '\r': case '\n':
	if (!ml->strip_space)
	  {
	    *dest++ = text[i];
	    md->after_newline = text[i] == '\n';
	  }
	else if (!md->after_whitespace)
	  {
	    *dest++ = ' ';
	    md->after_newline = FALSE;
	  }
	md->after_whitespace = TRUE;
	break;
      default:
	*dest++ = text[i];
	md->after_whitespace = FALSE;
	md->after_newline = text[i] == '\n';
	break;
      }
  *dest = 0;
  gtk_text_buffer_insert_at_cursor (md->tbuffer, string, dest - string);
  g_free (string);
}

/* this includes comments and processing instructions */
static void
tsm_passthrough (GMarkupParseContext *context,
		 const gchar         *passthrough_text,	/* text is not nul-terminated. */
		 gsize                text_len,
		 gpointer             user_data,
		 GError             **error)
{
  // TsmData *md = user_data;
}

/* Called on error, including one set by other methods in the vtable */
static void
tsm_error (GMarkupParseContext *context,
	   GError              *error,	/* the GError should not be freed */
	   gpointer             user_data)
{
  // TsmData *md = user_data;
}

static void
text_buffer_insert (GtkTextBuffer *tbuffer,
		    gboolean       parse_tsm,
		    guint          indent,
		    guint        (*read_callback) (gpointer data,
						   guint8  *buffer,
						   GError **error),
		    gpointer       callback_data,
		    guint8	  *data_buffer)
{
  GtkTextTagTable *ttable = gtk_text_buffer_get_tag_table (tbuffer);
  GtkTextIter start_iter, end_iter;
  GtkTextMark *start_mark;
  GtkTextTag *tag;
  gchar *name;
  
  gtk_text_buffer_get_iter_at_mark (tbuffer, &start_iter, gtk_text_buffer_get_insert (tbuffer));
  start_mark = gtk_text_buffer_create_mark (tbuffer, NULL, &start_iter, TRUE);
  
  if (!parse_tsm)
    {
      GError *error = NULL;
      guint n;
      n = read_callback (callback_data, data_buffer, &error);
      while (n)
	{
	  gtk_text_buffer_insert_at_cursor (tbuffer, data_buffer, n);
	  n = read_callback (callback_data, data_buffer, &error);
	}
      if (error)
	{
	  text_buffer_add_error (tbuffer, "Failed to retrive text: %s", error->message);
	  g_clear_error (&error);
	}
    }
  else
    {
      static GMarkupParser tsm_parser = {
	tsm_start_element,
	tsm_end_element,
	tsm_text,
	tsm_passthrough,
	tsm_error,
      };
      TsmData md = { 0, };
      GMarkupParseContext *context;
      GError *error = NULL;
      guint icon_square_sizes[] = { 8, 10, 12, 14, 16, 18, 20, 24, 32, 48, 56, 64, 96 };
      static guint n_icon_sizes = G_N_ELEMENTS (icon_square_sizes);
      guint n;
      
      /* ensure certain icon sizes */
      for (n = 0; n < n_icon_sizes; n++)
	{
	  name = g_strdup_printf ("%ux%u", icon_square_sizes[n], icon_square_sizes[n]);
	  
	  if (gtk_icon_size_from_name (name) == GTK_ICON_SIZE_INVALID)
	    gtk_icon_size_register (name, icon_square_sizes[n], icon_square_sizes[n]);
	  g_free (name);
	}
      n_icon_sizes = 0;
      
      md.tbuffer = tbuffer;
      md.tagns = gtk_text_tag_table_get_size (ttable); /* unique tag namespace */
      md.lc = ' ';
      md.opos = 0;
      md.lstack = NULL;
      md.after_whitespace = TRUE;
      md.after_newline = TRUE;
      md.style = NULL;
      context = g_markup_parse_context_new (&tsm_parser, 0, &md, NULL);
      
      if (!g_markup_parse_context_parse (context, "<strip-space>", 13, &error))
	goto ABORT;
      n = read_callback (callback_data, data_buffer, &error);
      while (n)
	{
	  if (!g_markup_parse_context_parse (context, data_buffer, n, &error))
	    goto ABORT;
	  n = read_callback (callback_data, data_buffer, &error);
	}
      if (error || !g_markup_parse_context_parse (context, "</strip-space>", 14, &error))
	goto ABORT;
      g_markup_parse_context_end_parse (context, &error);
    ABORT:
      if (error)
	{
	  text_buffer_add_error (tbuffer, "Failed to parse markup: %s", error->message);
	  g_clear_error (&error);
	}
      while (md.lstack)
	tsm_pop_level (&md);
      g_markup_parse_context_free (context);
      if (md.style)
	g_object_unref (md.style);
    }
  
  name = g_strdup_printf ("bst-text-buffer-indent-tag-%u", indent);
  tag = gtk_text_tag_table_lookup (ttable, name);
  if (!tag)
    {
      const guint left_margin = 3, right_margin = 3;
      tag = g_object_new (GTK_TYPE_TEXT_TAG,
			  "name", name,
			  "left_margin", left_margin + indent * 8,
			  "right_margin", right_margin,
			  "family", g_object_get_data (G_OBJECT (tbuffer), "family"),
			  NULL);
      g_object_set_int (tag, "bst-text-tools-owned", 1);
      gtk_text_tag_table_add (ttable, tag);
      gtk_text_tag_set_priority (tag, 0);
      g_object_unref (tag);
    }
  g_free (name);
  
  gtk_text_buffer_get_iter_at_mark (tbuffer, &start_iter, start_mark);
  gtk_text_buffer_delete_mark (tbuffer, start_mark);
  gtk_text_buffer_get_iter_at_mark (tbuffer, &end_iter, gtk_text_buffer_get_insert (tbuffer));
  gtk_text_buffer_apply_tag (tbuffer, tag, &start_iter, &end_iter);
}

static guint
static_read_callback (gpointer data,
		      guint8  *buffer,
		      GError **error)
{
  guint i, *ip = data;
  
  i = *ip;
  *ip = 0;
  return i;
}

void
bst_text_buffer_append_from_string (GtkTextBuffer *tbuffer,
				    gboolean       parse_tsm,
				    guint          indent_margin,
				    guint          text_length,
				    const gchar   *text)
{
  g_return_if_fail (GTK_IS_TEXT_BUFFER (tbuffer));
  if (text_length)
    g_return_if_fail (text != NULL);
  
  bst_text_buffer_cursor_to_end (tbuffer);
  if (!text_length)
    return;
  text_buffer_insert (tbuffer, parse_tsm, indent_margin, static_read_callback, &text_length, (guint8*) text);
}

#define FILE_READ_BUFFER_SIZE	(8192)

static guint
fd_read_callback (gpointer data,
		  guint8  *buffer,
		  GError **error)
{
  gint l, *fdp = data;
  
  do
    l = read (*fdp, buffer, FILE_READ_BUFFER_SIZE);
  while (l < 0 && errno == EINTR);
  if (l < 0)
    g_set_error (error, 0, errno, "file read failed: %s", g_strerror (errno));
  return MAX (0, l);
}

void
bst_text_buffer_append_from_file (GtkTextBuffer *tbuffer,
				  gboolean       parse_tsm,
				  guint          indent_margin,
				  const gchar   *file_name)
{
  gint fd;
  
  g_return_if_fail (GTK_IS_TEXT_BUFFER (tbuffer));
  g_return_if_fail (file_name != NULL);
  
  bst_text_buffer_cursor_to_end (tbuffer);
  
  fd = open (file_name, O_RDONLY);
  if (fd >= 0)
    {
      guint8 data_buffer[FILE_READ_BUFFER_SIZE];
      text_buffer_insert (tbuffer, parse_tsm, indent_margin, fd_read_callback, &fd, data_buffer);
      close (fd);
    }
  else
    text_buffer_add_error (tbuffer, "Failed to open \"%s\": %s", file_name, g_strerror (errno));
}

void
bst_text_buffer_cursor_to_start (GtkTextBuffer *tbuffer)
{
  GtkTextIter iter;
  
  g_return_if_fail (GTK_IS_TEXT_BUFFER (tbuffer));
  
  gtk_text_buffer_get_start_iter (tbuffer, &iter);
  gtk_text_buffer_place_cursor (tbuffer, &iter);
}

void
bst_text_buffer_cursor_to_end (GtkTextBuffer *tbuffer)
{
  GtkTextIter iter;
  
  g_return_if_fail (GTK_IS_TEXT_BUFFER (tbuffer));
  
  gtk_text_buffer_get_end_iter (tbuffer, &iter);
  gtk_text_buffer_place_cursor (tbuffer, &iter);
}


/* --- TextView browse mode --- */
static inline void
adjustment_set_clamped_value (GtkAdjustment *adjustment,
			      gdouble        value)
{
  gtk_adjustment_set_value (adjustment, MIN (value, adjustment->upper - adjustment->page_size));
}

static gboolean
text_view_key_event (GtkTextView *tview,
		     GdkEventKey *event)
{
  GtkAdjustment *hadjustment = tview->hadjustment;
  GtkAdjustment *vadjustment = tview->vadjustment;
  gboolean handled = TRUE;
  if (event->type != GDK_KEY_PRESS)
    return TRUE;
  switch (event->keyval)
    {
    case GDK_Left: case GDK_KP_Left:
      if (event->state & GDK_CONTROL_MASK)
	adjustment_set_clamped_value (hadjustment, hadjustment->value - hadjustment->page_increment);
      else
	adjustment_set_clamped_value (hadjustment, hadjustment->value - hadjustment->step_increment);
      break;
    case GDK_Right: case GDK_KP_Right:
      if (event->state & GDK_CONTROL_MASK)
	adjustment_set_clamped_value (hadjustment, hadjustment->value + hadjustment->page_increment);
      else
	adjustment_set_clamped_value (hadjustment, hadjustment->value + hadjustment->step_increment);
      break;
    case GDK_Up: case GDK_KP_Up:
      if (event->state & GDK_CONTROL_MASK)
	adjustment_set_clamped_value (vadjustment, vadjustment->value - vadjustment->page_increment);
      else
	adjustment_set_clamped_value (vadjustment, vadjustment->value - vadjustment->step_increment);
      break;
    case GDK_Down: case GDK_KP_Down:
      if (event->state & GDK_CONTROL_MASK)
	adjustment_set_clamped_value (vadjustment, vadjustment->value + vadjustment->page_increment);
      else
	adjustment_set_clamped_value (vadjustment, vadjustment->value + vadjustment->step_increment);
      break;
    case GDK_Page_Up: case GDK_KP_Page_Up:
      adjustment_set_clamped_value (vadjustment, vadjustment->value - vadjustment->page_increment);
      break;
    case GDK_Page_Down: case GDK_KP_Page_Down:
      adjustment_set_clamped_value (vadjustment, vadjustment->value + vadjustment->page_increment);
      break;
    case GDK_Home: case GDK_KP_Home:
      adjustment_set_clamped_value (hadjustment, hadjustment->lower);
      adjustment_set_clamped_value (vadjustment, vadjustment->lower);
      break;
    case GDK_End: case GDK_KP_End:
      adjustment_set_clamped_value (hadjustment, hadjustment->lower);
      adjustment_set_clamped_value (vadjustment, vadjustment->upper - vadjustment->page_size);
      break;
    case GDK_Tab: case GDK_KP_Tab: case GDK_ISO_Left_Tab:
    case GDK_space: case GDK_KP_Space:
    case GDK_Return: case GDK_KP_Enter:
      handled = FALSE;
      break;
    }
  return handled;
}

void
bst_text_view_enter_browse_mode (GtkTextView *tview)
{
  g_return_if_fail (GTK_IS_TEXT_VIEW (tview));

  if (!g_signal_handler_find (tview, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, text_view_key_event, tview))
    {
      g_signal_connect (tview, "key_press_event", G_CALLBACK (text_view_key_event), tview);
      g_signal_connect (tview, "key_release_event", G_CALLBACK (text_view_key_event), tview);
    }
}

void
bst_text_view_leave_browse_mode (GtkTextView *tview)
{
  g_return_if_fail (GTK_IS_TEXT_VIEW (tview));

  if (g_signal_handler_find (tview, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, text_view_key_event, tview))
    g_signal_handlers_disconnect_by_func (tview, text_view_key_event, tview);
}

void
bst_text_view_cursor_to_start (GtkTextView *tview)
{
  GtkTextBuffer *tbuffer;

  g_return_if_fail (GTK_IS_TEXT_VIEW (tview));

  tbuffer = gtk_text_view_get_buffer (tview);
  bst_text_buffer_cursor_to_start (tbuffer);
  gtk_text_view_scroll_mark_onscreen (tview, gtk_text_buffer_get_insert (tbuffer));
}

void
bst_text_view_cursor_to_end (GtkTextView *tview)
{
  GtkTextBuffer *tbuffer;

  g_return_if_fail (GTK_IS_TEXT_VIEW (tview));

  tbuffer = gtk_text_view_get_buffer (tview);
  bst_text_buffer_cursor_to_end (tbuffer);
  gtk_text_view_scroll_mark_onscreen (tview, gtk_text_buffer_get_insert (tbuffer));
}

static void
scroll_text_patchup_size_request (GtkWidget      *sctext,
				  GtkRequisition *requisition,
				  GtkTextView    *tview)
{
  if (!GTK_WIDGET_MAPPED (sctext))
    {
      /* provide initial size */
      requisition->width += 220;
      requisition->height += 100;
    }
}

GtkWidget*
bst_scroll_text_create (BstTextViewFlags flags,
			const gchar     *string)
{
  GtkWidget *widget, *sctext;

  sctext = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
			 "visible", TRUE,
			 "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
			 "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
			 NULL);
  widget = g_object_new (GTK_TYPE_TEXT_VIEW,
			 "visible", TRUE,
			 "cursor_visible", FALSE,
			 "parent", sctext,
			 NULL);
  g_signal_connect_after (sctext, "size_request", G_CALLBACK (scroll_text_patchup_size_request), widget);
  if (TRUE)
    {
      g_object_set (widget, "editable", FALSE, NULL);
      bst_text_view_enter_browse_mode (GTK_TEXT_VIEW (widget));
    }
  if (flags & BST_TEXT_VIEW_NO_WRAP)
    g_object_set (widget, "wrap_mode", GTK_WRAP_NONE, NULL);
  else
    g_object_set (widget, "wrap_mode", GTK_WRAP_WORD, NULL);
  if (flags & BST_TEXT_VIEW_CENTER)
    g_object_set (widget, "justification", GTK_JUSTIFY_CENTER, NULL);
  if (flags & BST_TEXT_VIEW_MONO_SPACED)
    g_object_set_data ((GObject*) gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)), "family", "mono");
  if (!(flags & BST_TEXT_VIEW_SHEET_BG))
    bst_widget_modify_base_as_bg (widget);

  bst_scroll_text_append (sctext, string);
  
  return sctext;
}

GtkWidget*
bst_scroll_text_from_file (BstTextViewFlags flags,
			   const gchar     *file_name)
{
  GtkWidget *sctext;

  g_return_val_if_fail (file_name != NULL, NULL);

  sctext = bst_scroll_text_create (flags, "");
  if (flags & BST_TEXT_VIEW_PARSE_TSM)
    bst_scroll_text_append_file_tsm (sctext, file_name);
  else
    bst_scroll_text_append_file (sctext, file_name);
  return sctext;
}

static void
text_tag_remove (GtkTextTag *tag,
		 gpointer    data)
{
  GSList **slist_p = data;
  if (g_object_get_int (tag, "bst-text-tools-owned"))
    *slist_p = g_slist_prepend (*slist_p, tag);
}

void
bst_scroll_text_clear (GtkWidget *sctext)
{
  GtkTextView *tview;
  GtkTextBuffer *tbuffer;
  GtkTextTagTable *ttable;
  GtkTextIter iter1, iter2;
  GSList *node, *slist = NULL;

  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (sctext));

  tview = GTK_TEXT_VIEW (GTK_BIN (sctext)->child);
  tbuffer = gtk_text_view_get_buffer (tview);
  ttable = gtk_text_buffer_get_tag_table (tbuffer);

  gtk_text_buffer_get_start_iter (tbuffer, &iter1);
  gtk_text_buffer_get_end_iter (tbuffer, &iter2);
  gtk_text_buffer_delete (tbuffer, &iter1, &iter2);
  gtk_text_tag_table_foreach (ttable, text_tag_remove, &slist);
  for (node = slist; node; node = node->next)
    gtk_text_tag_table_remove (ttable, node->data);
  g_slist_free (slist);
  g_object_set_int (tview, "indent", 0);
}

void
bst_scroll_text_set (GtkWidget   *sctext,
		     const gchar *string)
{
  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (sctext));

  bst_scroll_text_clear (sctext);
  bst_scroll_text_append (sctext, string);
}

void
bst_scroll_text_set_tsm (GtkWidget   *sctext,
			 const gchar *string)
{
  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (sctext));

  bst_scroll_text_clear (sctext);
  bst_scroll_text_append_tsm (sctext, string);
}

void
bst_scroll_text_append (GtkWidget   *sctext,
			const gchar *string)
{
  GtkTextView *tview;
  GtkTextBuffer *tbuffer;
  
  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (sctext));

  tview = GTK_TEXT_VIEW (GTK_BIN (sctext)->child);
  tbuffer = gtk_text_view_get_buffer (tview);
  if (string)
    bst_text_buffer_append_from_string (tbuffer, FALSE, g_object_get_int (tview, "indent"), strlen (string), string);
  bst_text_view_cursor_to_start (tview);
}

void
bst_scroll_text_append_tsm (GtkWidget   *sctext,
			    const gchar *string)
{
  GtkTextView *tview;
  GtkTextBuffer *tbuffer;
  
  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (sctext));

  tview = GTK_TEXT_VIEW (GTK_BIN (sctext)->child);
  tbuffer = gtk_text_view_get_buffer (tview);
  if (string)
    bst_text_buffer_append_from_string (tbuffer, TRUE, g_object_get_int (tview, "indent"), strlen (string), string);
  bst_text_view_cursor_to_start (tview);
}

void
bst_scroll_text_append_file (GtkWidget   *sctext,
			     const gchar *file_name)
{
  GtkTextView *tview;
  GtkTextBuffer *tbuffer;

  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (sctext));
  g_return_if_fail (file_name != NULL);

  tview = GTK_TEXT_VIEW (GTK_BIN (sctext)->child);
  tbuffer = gtk_text_view_get_buffer (tview);
  bst_text_buffer_append_from_file (tbuffer, FALSE, g_object_get_int (tview, "indent"), file_name);
  bst_text_view_cursor_to_start (tview);
}

void
bst_scroll_text_append_file_tsm (GtkWidget   *sctext,
				 const gchar *file_name)
{
  GtkTextView *tview;
  GtkTextBuffer *tbuffer;

  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (sctext));
  g_return_if_fail (file_name != NULL);

  tview = GTK_TEXT_VIEW (GTK_BIN (sctext)->child);
  tbuffer = gtk_text_view_get_buffer (tview);
  bst_text_buffer_append_from_file (tbuffer, TRUE, g_object_get_int (tview, "indent"), file_name);
  bst_text_view_cursor_to_start (tview);
}

void
bst_scroll_text_aprintf (GtkWidget   *sctext,
			 const gchar *text_fmt,
			 ...)
{
  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (sctext));

  if (text_fmt)
    {
      va_list args;
      gchar *buffer;
      
      va_start (args, text_fmt);
      buffer = g_strdup_vprintf (text_fmt, args);
      va_end (args);

      bst_scroll_text_append (sctext, buffer);
      g_free (buffer);
    }
}

void
bst_scroll_text_aprintf_tsm (GtkWidget   *sctext,
			     const gchar *text_fmt,
			     ...)
{
  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (sctext));

  if (text_fmt)
    {
      va_list args;
      gchar *buffer;
      
      va_start (args, text_fmt);
      buffer = g_strdup_vprintf (text_fmt, args);
      va_end (args);

      bst_scroll_text_append_tsm (sctext, buffer);
      g_free (buffer);
    }
}

GtkTextView*
bst_scroll_text_get_text_view (GtkWidget *sctext)
{
  GtkTextView *tview;

  g_return_val_if_fail (GTK_IS_SCROLLED_WINDOW (sctext), NULL);

  tview = GTK_TEXT_VIEW (GTK_BIN (sctext)->child);

  return tview;
}

void
bst_scroll_text_push_indent (GtkWidget   *sctext,
			     const gchar *spaces)
{
  GtkTextView *tview;
  guint indent;

  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (sctext));

  tview = GTK_TEXT_VIEW (GTK_BIN (sctext)->child);
  indent = g_object_get_int (tview, "indent");
  g_object_set_int (tview, "indent", indent + 2);
}

void
bst_scroll_text_pop_indent (GtkWidget *sctext)
{
  GtkTextView *tview;
  guint indent;

  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (sctext));

  tview = GTK_TEXT_VIEW (GTK_BIN (sctext)->child);
  indent = g_object_get_int (tview, "indent");
  if (indent)
    g_object_set_int (tview, "indent", indent - 2);
}
