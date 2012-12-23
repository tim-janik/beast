/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2006 Tim Janik
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
#include "gxktexttools.h"
#include "gxkstock.h"
#include "gxkradget.h"
#include "sfi/sfiwrapper.hh" /* for sfi_url_show() */
#include <gdk/gdkkeysyms.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>


#define GXK_IS_SCROLL_TEXT      GTK_IS_VBOX

typedef struct {
  gchar  *url;
  gdouble vpos;
} HEntry;       /* History entry */

typedef struct {
  GtkWidget *sctext;
  GtkWidget *backb;
  GtkWidget *forwardb;
  GtkWidget *refe;
  gchar     *index;
  gchar     *proto;
  gchar     *path;
  gchar     *file;
  gchar     *anchor;
  GtkAdjustment *vadjustment;
  gdouble    vert_frac;
  HEntry    *current;
  GSList    *back_stack;        /* HEntry* */
  GSList    *fore_stack;        /* HEntry* */
} TextNavigation;


/* --- prototypes --- */
static TextNavigation*  navigation_from_sctext  (GtkWidget      *sctext);
static void             navigate_back           (GtkWidget      *sctext);
static void             navigate_forward        (GtkWidget      *sctext);
static void             navigate_reload         (GtkWidget      *sctext);
static void             navigate_index          (GtkWidget      *sctext);
static void             navigate_find           (GtkWidget      *sctext);
static void             navigate_goto           (GtkWidget      *sctext);
static void             navigate_link           (GtkWidget      *sctext,
                                                 const gchar    *uri);
static bool             navigate_urls           (GtkWidget      *sctext,
                                                 const gchar    *uri);
static void             text_buffer_add_error   (GtkTextBuffer  *tbuffer,
                                                 const gchar    *format,
                                                 ...) G_GNUC_PRINTF (2, 3);


/* --- functions --- */
static void
text_buffer_add_error (GtkTextBuffer *tbuffer,
                       const gchar   *format,
                       ...)
{
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table (tbuffer);
  GtkTextTag *tag = gtk_text_tag_table_lookup (table, "gxk-text-tools-error");
  GtkTextIter iter;
  gchar *string, *text;
  va_list args;
  if (!tag)
    {
      tag = (GtkTextTag*) g_object_new (GTK_TYPE_TEXT_TAG,
                                        "name", "gxk-text-tools-error",
                                        "foreground", "#000000",
                                        "background", "#ff0000",
                                        "wrap_mode", GTK_WRAP_WORD,
                                        NULL);
      g_object_set_int (tag, "gxk-text-tools-owned", 1);
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

static GSList *tsm_paths = NULL;
void
gxk_text_add_tsm_path (const gchar *path)
{
  guint l = strlen (path ? path : "");
  if (l)
    {
      gchar *str, *p, *s;
      gboolean was_slash = FALSE;
      /* make path absolute and ensure it ends in '/' */
      if (path[0] != '/')
        {
          gchar *tmp = g_get_current_dir ();
          str = g_strconcat (tmp, "/", path, "/", NULL);
          g_free (tmp);
        }
      else
        str = g_strconcat (path, "/", NULL);
      /* compress '/'s and "/./" */
      for (p = str, s = p; *p; p++)
        if (*p != '/')
          {
            *s++ = *p;
            was_slash = FALSE;
          }
        else
          {
            if (!was_slash)
              {
                *s++ = *p;
                was_slash = TRUE;
              }
            if (p[1] == '.' && p[2] == '/')
              p += 2;
          }
      *s = 0;
      tsm_paths = g_slist_append (tsm_paths, g_strdup (str));
      g_free (str);
    }
}

static GdkPixbuf*
pixbuf_new_from_path (const gchar *file_name,
                      GError     **error)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (file_name, error);
  if (*error && !g_path_is_absolute (file_name))
    {
      GSList *slist;
      for (slist = tsm_paths; slist && !pixbuf; slist = slist->next)
        {
          char *loc = g_strconcat ((const char*) slist->data, file_name, NULL);
          pixbuf = gdk_pixbuf_new_from_file (loc, NULL);
          g_free (loc);
        }
      if (pixbuf)
        g_clear_error (error);
    }
  return pixbuf;
}


/* --- Textget handling --- */
typedef struct {
  GtkTextChildAnchor *anchor;
  GSList             *widgets;
} Textget;

static void
free_textgets (gpointer data)
{
  GSList *slist = (GSList*) data;
  while (slist)
    {
      Textget *textget = (Textget*) g_slist_pop_head (&slist);
      while (textget->widgets)
        {
          GtkWidget *widget = (GtkWidget*) g_slist_pop_head (&textget->widgets);
          g_object_unref (widget);
        }
      g_free (textget);
    }
}

static Textget*
gxk_text_buffer_create_textget (GtkTextBuffer *tbuffer)
{
  Textget *textget = g_new (Textget, 1);
  GtkTextIter iter;
  GSList *slist;

  gtk_text_buffer_get_iter_at_mark (tbuffer, &iter, gtk_text_buffer_get_insert (tbuffer));
  textget->anchor = gtk_text_buffer_create_child_anchor (tbuffer, &iter);
  textget->widgets = NULL;
  slist = (GSList*) g_object_steal_data ((GObject*) tbuffer, "textgets");
  slist = g_slist_prepend (slist, textget);
  g_object_set_data_full ((GObject*) tbuffer, "textgets", slist, free_textgets);
  return textget;
}

typedef struct {
  const gchar          *element_name;
  GxkTextTextgetHandler handler;
  gpointer              user_data;
} TextgetHandler;
static GSList *textget_handlers = NULL;


void
gxk_text_register_textget_handler (const gchar          *element_name,
                                   GxkTextTextgetHandler handler,
                                   gpointer              user_data)
{
  TextgetHandler *th;

  g_return_if_fail (element_name != NULL);
  g_return_if_fail (handler != NULL);

  th = g_new (TextgetHandler, 1);
  th->element_name = g_strdup (element_name);
  th->handler = handler;
  th->user_data = user_data;

  textget_handlers = g_slist_append (textget_handlers, th);
}

static void
gxk_text_buffer_handle_textget (GtkTextBuffer *tbuffer,
                                const gchar   *element_name,
                                const gchar  **attribute_names,
                                const gchar  **attribute_values)
{
  Textget *textget = NULL;
  GSList *slist;
  for (slist = textget_handlers; slist; slist = slist->next)
    {
      TextgetHandler *th = (TextgetHandler*) slist->data;
      if (strcmp (element_name, th->element_name) == 0)
        {
          GtkWidget *widget = th->handler (th->user_data, element_name, attribute_names, attribute_values);
          if (widget)
            {
              if (!textget)
                textget = gxk_text_buffer_create_textget (tbuffer);
              textget->widgets = g_slist_append (textget->widgets, g_object_ref (widget));
              gtk_object_sink (GTK_OBJECT (widget));
            }
        }
    }
}

void
gxk_text_buffer_add_textgets_to_view (GtkTextBuffer *tbuffer,
                                      GtkTextView   *tview)
{
  g_return_if_fail (GTK_IS_TEXT_BUFFER (tbuffer));
  g_return_if_fail (GTK_IS_TEXT_VIEW (tview));

  GSList *slist = (GSList*) g_object_steal_data ((GObject*) tbuffer, "textgets");
  while (slist)
    {
      Textget *textget = (Textget*) g_slist_pop_head (&slist);
      while (textget->widgets)
        {
          GtkWidget *widget = (GtkWidget*) g_slist_pop_head (&textget->widgets);
          gtk_text_view_add_child_at_anchor (tview, widget, textget->anchor);
          g_object_unref (widget);
        }
    }
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
      const char *vname = eclass->values[i].value_name;
      guint n = strlen (vname);
      if (((n > length && char2eval (vname[n - 1 - length]) == '-')
           || n == length)
          && enum_match (vname + n - length, name))
        return eclass->values[i].value;
    }
  for (i = 0; i < eclass->n_values; i++)
    {
      const char *vname = eclass->values[i].value_nick;
      guint n = strlen (vname);
      if (((n > length && char2eval (vname[n - 1 - length]) == '-')
           || n == length)
          && enum_match (vname + n - length, name))
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
  GType vtype = 0;
  gint edefault = 0;
  if (!tag)
    {
      tag = (GtkTextTag*) g_object_new (GTK_TYPE_TEXT_TAG,
                                        "name", tag_name,
                                        NULL);
      g_object_set_int (tag, "gxk-text-tools-owned", 1);
      gtk_text_tag_table_add (ttable, tag);
      g_object_unref (tag);
    }
  if (property)
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tag), property);
  if (pspec)
    {
      vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
      /* special casing of weight which is int/enum */
      if (G_IS_PARAM_SPEC_ENUM (pspec))
        edefault = G_PARAM_SPEC_ENUM (pspec)->default_value;
      else if (strcmp (pspec->name, "weight") == 0)
        {
          vtype = PANGO_TYPE_WEIGHT;
          edefault = G_PARAM_SPEC_INT (pspec)->default_value;
        }
    }
  switch (G_TYPE_FUNDAMENTAL (vtype))
    {
      GEnumClass *eclass;
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
      g_value_init (&value, vtype);
      eclass = (GEnumClass*) g_type_class_ref (vtype);
      g_value_set_enum (&value, enum_match_value (eclass, tag_value, edefault));
      g_type_class_unref (eclass);
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
static guint signal_custom_activate = 0;

void
gxk_text_buffer_init_custom (void)
{
  if (!signal_custom_activate)
    signal_custom_activate = g_signal_new ("custom-activate",
                                           GTK_TYPE_TEXT_BUFFER,
                                           G_SIGNAL_RUN_LAST,
                                           0, NULL, NULL,
                                           g_cclosure_marshal_VOID__STRING,
                                           G_TYPE_NONE, 1, G_TYPE_STRING);
}

typedef struct _TsmLevel TsmLevel;
typedef struct {
  GtkTextBuffer *tbuffer;
  guint          tagns;
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
    md->style = (GtkStyle*) g_object_new (GTK_TYPE_STYLE, NULL);
  return md->style;
}

static gboolean
data_tag_event (GtkTextTag        *tag,
                GObject           *event_object, /* widget, canvas item, whatever */
                GdkEvent          *event,        /* the event itself */
                const GtkTextIter *iter,         /* location of event in buffer */
                GtkTextBuffer     *tbuffer)
{
  if (0)
    {
      GEnumClass *eclass = (GEnumClass*) g_type_class_ref (GDK_TYPE_EVENT_TYPE);
      GEnumValue *ev = g_enum_get_value (eclass, event->type);
      g_message ("TextTagEvent: %s\n", ev->value_name);
      g_type_class_unref (eclass);
    }
  if (event->type == GDK_BUTTON_PRESS)
    {
      const gchar *data = (const char*) g_object_get_data (G_OBJECT (tag), "data");
      g_signal_emit (tbuffer, signal_custom_activate, 0, data);
      return TRUE;
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
  static const TsmLevel ttemplate = {
    TRUE,       /* strip_space */
    NULL,       /* tag_name */
    NULL,       /* mark */
    NULL,       /* next */
  };
  return md->lstack ? md->lstack : &ttemplate;
}

static TsmLevel*
tsm_push_level (TsmData *md)
{
  const TsmLevel *src = tsm_peek_level (md);
  TsmLevel *ml = (TsmLevel*) g_memdup (src, sizeof (TsmLevel));
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
  TsmData *md = (TsmData*) user_data;
  
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
  else if (strcmp (element_name, "tag-span-markup") == 0)
    {
      TsmLevel *ml = tsm_push_level (md);
      ml->strip_space = TRUE;
    }
  else if (strcmp (element_name, "keep-space") == 0)
    {
      TsmLevel *ml = tsm_push_level (md);
      ml->strip_space = FALSE;
    }
  else if (strcmp (element_name, "anchor") == 0)
    {
      guint i;
      for (i = 0; attribute_names[i]; i++)
        if (strcmp (attribute_names[i], "name") == 0)
          {
            gchar *aname = g_strconcat ("#-", attribute_values[i], NULL);
            GtkTextMark *mark = gtk_text_buffer_get_mark (md->tbuffer, aname);
            GtkTextIter iter;
            gtk_text_buffer_get_iter_at_mark (md->tbuffer, &iter, gtk_text_buffer_get_insert (md->tbuffer));
            if (mark)
              gtk_text_buffer_move_mark (md->tbuffer, mark, &iter);
            else
              mark = gtk_text_buffer_create_mark (md->tbuffer, aname, &iter, TRUE);
            g_free (aname);
            break;
          }
    }
  else if (strcmp (element_name, "xlink") == 0)
    {
      TsmLevel *ml = tsm_push_level (md);
      guint i;
      for (i = 0; attribute_names[i]; i++)
        if (strcmp (attribute_names[i], "ref") == 0)
          {
            GtkTextTagTable *ttable = gtk_text_buffer_get_tag_table (md->tbuffer);
            ml->tag = (GtkTextTag*) g_object_new (GTK_TYPE_TEXT_TAG, NULL);
            g_object_set_int (ml->tag, "gxk-text-tools-owned", 1);
            g_object_set_data_full (G_OBJECT (ml->tag), "data", g_strdup (attribute_values[i]), g_free);
            g_object_connect (ml->tag, "signal::event", data_tag_event, md->tbuffer, NULL);
            gtk_text_tag_table_add (ttable, ml->tag);
            g_object_unref (ml->tag);
            break;
          }
    }
  else if (strcmp (element_name, "image") == 0)
    {
      GdkPixbuf *pixbuf = NULL;
      GtkIconSet *iset = NULL;
      GtkIconSize isize = GXK_ICON_SIZE_MENU;
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
            GEnumClass *eclass = (GEnumClass*) g_type_class_ref (GTK_TYPE_ICON_SIZE);
            gint c, l = enum_match_value (eclass, attribute_values[i], GTK_ICON_SIZE_INVALID);
            g_type_class_unref (eclass);
            if (l == GTK_ICON_SIZE_INVALID)
              l = gtk_icon_size_from_name (attribute_values[i]);
            if (l != GTK_ICON_SIZE_INVALID)
              isize = GtkIconSize (l);
            else
              {
                g_markup_parse_context_get_position (context, &c, &l);
                text_buffer_add_error (md->tbuffer, "Unknown icon size \"%s\" for image tag on line %d char %d",
                                       attribute_values[i], c, l);
              }
          }
      if (!pixbuf && iset)
        pixbuf = (GdkPixbuf*) g_object_ref (gtk_icon_set_render_icon (iset, tsm_data_ensure_style (md),
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
  else if (strncmp (element_name, "textget-", 8) == 0)
    {
      gxk_text_buffer_handle_textget (md->tbuffer,
                                      element_name,
                                      attribute_names,
                                      attribute_values);
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
  TsmData *md = (TsmData*) user_data;
  
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
  else if (strcmp (element_name, "tag-span-markup") == 0)
    tsm_pop_level (md);
  else if (strcmp (element_name, "keep-space") == 0)
    tsm_pop_level (md);
  else if (strcmp (element_name, "anchor") == 0)
    {
    }
  else if (strcmp (element_name, "xlink") == 0)
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
  else if (strncmp (element_name, "textget-", 8) == 0)
    {
    }
  else
    text_buffer_add_error (md->tbuffer, "</%s>", element_name);
}

/* Called for character data */
static void
tsm_text (GMarkupParseContext *context,
          const gchar         *text,    /* text is not nul-terminated */
          gsize                text_len,
          gpointer             user_data,
          GError             **error)
{
  TsmData *md = (TsmData*) user_data;
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
  i = dest - string;
  /* force-validate to UTF-8 */
  dest = string;
  while (!g_utf8_validate (dest, i - (dest - string), (const char**) &dest))
    *dest = '?';
  gtk_text_buffer_insert_at_cursor (md->tbuffer, string, i);
  g_free (string);
}

/* this includes comments and processing instructions */
static void
tsm_passthrough (GMarkupParseContext *context,
                 const gchar         *passthrough_text, /* text is not nul-terminated. */
                 gsize                text_len,
                 gpointer             user_data,
                 GError             **error)
{
  // TsmData *md = user_data;
}

/* Called on error, including one set by other methods in the vtable */
static void
tsm_error (GMarkupParseContext *context,
           GError              *error,  /* the GError should not be freed */
           gpointer             user_data)
{
  // TsmData *md = user_data;
}

static void
gxk_text_buffer_insert (GtkTextBuffer *tbuffer,
                        gboolean       parse_tsm,
                        guint          indent,
                        const gchar   *text_src_name,
                        guint        (*read_callback) (gpointer data,
                                                       guint8  *buffer,
                                                       GError **error),
                        gpointer       callback_data,
                        guint8        *data_buffer)
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
          /* force-validate to UTF-8 */
          char *dest = (char*) data_buffer;
          while (!g_utf8_validate (dest, n - (dest - (gchar*) data_buffer), (const gchar**) &dest))
            *dest = '?';
          gtk_text_buffer_insert_at_cursor (tbuffer, (char*) data_buffer, n);
          /* next chunk */
          n = read_callback (callback_data, data_buffer, &error);
        }
      if (error)
        {
          text_buffer_add_error (tbuffer, "Failed to read text from \"%s\": %s", text_src_name, error->message);
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
      context = g_markup_parse_context_new (&tsm_parser, GMarkupParseFlags (0), &md, NULL);
      
      n = read_callback (callback_data, data_buffer, &error);
      while (n)
        {
          if (!g_markup_parse_context_parse (context, (char*) data_buffer, n, &error))
            goto ABORT;
          n = read_callback (callback_data, data_buffer, &error);
        }
      if (error)
        goto ABORT;
      g_markup_parse_context_end_parse (context, &error);
    ABORT:
      while (md.lstack)
        tsm_pop_level (&md);
      if (error)
        {
          text_buffer_add_error (tbuffer, "Failed to parse markup: %s", error->message);
          g_clear_error (&error);
        }
      g_markup_parse_context_free (context);
      if (md.style)
        g_object_unref (md.style);
    }
  
  name = g_strdup_printf ("gxk-text-tools-indent-%u", indent);
  tag = gtk_text_tag_table_lookup (ttable, name);
  if (!tag)
    {
      const guint left_margin = 3, right_margin = 3;
      tag = (GtkTextTag*) g_object_new (GTK_TYPE_TEXT_TAG,
                                        "name", name,
                                        "left_margin", left_margin + indent * 8,
                                        "right_margin", right_margin,
                                        "family", g_object_get_data (G_OBJECT (tbuffer), "family"),
                                        NULL);
      g_object_set_int (tag, "gxk-text-tools-owned", 1);
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
  guint i, *ip = (guint*) data;
  
  i = *ip;
  *ip = 0;
  return i;
}

void
gxk_text_buffer_append_from_string (GtkTextBuffer *tbuffer,
                                    gboolean       parse_tsm,
                                    guint          indent_margin,
                                    guint          text_length,
                                    const gchar   *text)
{
  g_return_if_fail (GTK_IS_TEXT_BUFFER (tbuffer));
  if (text_length)
    g_return_if_fail (text != NULL);
  
  gxk_text_buffer_cursor_to_end (tbuffer);
  if (!text_length)
    return;
  gxk_text_buffer_insert (tbuffer, parse_tsm, indent_margin, "<Inlined String>", static_read_callback, &text_length, (guint8*) text);
}

#define FILE_READ_BUFFER_SIZE   (8192)

static guint
fd_read_callback (gpointer data,
                  guint8  *buffer,
                  GError **error)
{
  gint l, *fdp = (gint*) data;
  
  do
    l = read (*fdp, buffer, FILE_READ_BUFFER_SIZE);
  while (l < 0 && errno == EINTR);
  if (l < 0)
    g_set_error (error, 0, errno, "file read failed: %s", g_strerror (errno));
  return MAX (0, l);
}

void
gxk_text_buffer_append_from_file (GtkTextBuffer *tbuffer,
                                  gboolean       parse_tsm,
                                  guint          indent_margin,
                                  const gchar   *file_name)
{
  gint fd;
  
  g_return_if_fail (GTK_IS_TEXT_BUFFER (tbuffer));
  g_return_if_fail (file_name != NULL);
  
  gxk_text_buffer_cursor_to_end (tbuffer);
  
  fd = open (file_name, O_RDONLY);
  if (fd >= 0)
    {
      guint8 data_buffer[FILE_READ_BUFFER_SIZE];
      gxk_text_buffer_insert (tbuffer, parse_tsm, indent_margin, file_name, fd_read_callback, &fd, data_buffer);
      close (fd);
    }
  else
    text_buffer_add_error (tbuffer, "Failed to open \"%s\": %s", file_name, g_strerror (errno));
}

void
gxk_text_buffer_cursor_to_start (GtkTextBuffer *tbuffer)
{
  GtkTextIter iter;
  
  g_return_if_fail (GTK_IS_TEXT_BUFFER (tbuffer));
  
  gtk_text_buffer_get_start_iter (tbuffer, &iter);
  gtk_text_buffer_place_cursor (tbuffer, &iter);
}

void
gxk_text_buffer_cursor_to_end (GtkTextBuffer *tbuffer)
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
  gboolean editable = FALSE, handled = TRUE;
  g_object_get (tview, "editable", &editable, NULL);
  if (editable)
    return FALSE;
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

/**
 * @param tview	a GtkTextView object
 *
 * Install key press handlers on a text view which
 * allow scrolling its contents into any direction.
 */
void
gxk_text_view_enter_browse_mode (GtkTextView *tview)
{
  g_return_if_fail (GTK_IS_TEXT_VIEW (tview));

  if (!g_signal_handler_find (tview, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, (void*) text_view_key_event, tview))
    {
      g_signal_connect (tview, "key_press_event", G_CALLBACK (text_view_key_event), tview);
      g_signal_connect (tview, "key_release_event", G_CALLBACK (text_view_key_event), tview);
    }
}

/**
 * @param tview	a GtkTextView object
 *
 * Deinstall key press handlers previously installed
 * with gxk_text_view_enter_browse_mode().
 */
void
gxk_text_view_leave_browse_mode (GtkTextView *tview)
{
  g_return_if_fail (GTK_IS_TEXT_VIEW (tview));

  if (g_signal_handler_find (tview, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, (void*) text_view_key_event, tview))
    g_signal_handlers_disconnect_by_func (tview, (void*) text_view_key_event, tview);
}

/**
 * @param tview	a GtkTextView object
 *
 * Move the insertion and selection bound mark to
 * the start of the text view's buffer and keep
 * the cursor on screen.
 */
void
gxk_text_view_cursor_to_start (GtkTextView *tview)
{
  GtkTextBuffer *tbuffer;

  g_return_if_fail (GTK_IS_TEXT_VIEW (tview));

  tbuffer = gtk_text_view_get_buffer (tview);
  gxk_text_buffer_cursor_to_start (tbuffer);
  gtk_text_view_scroll_to_mark (tview, gtk_text_buffer_get_insert (tbuffer), 0.0, TRUE, 0.0, 0.0);
}

/**
 * @param tview	a GtkTextView object
 *
 * Move the insertion and selection bound mark to
 * the end of the text view's buffer and keep
 * the cursor on screen.
 */
void
gxk_text_view_cursor_to_end (GtkTextView *tview)
{
  GtkTextBuffer *tbuffer;

  g_return_if_fail (GTK_IS_TEXT_VIEW (tview));

  tbuffer = gtk_text_view_get_buffer (tview);
  gxk_text_buffer_cursor_to_end (tbuffer);
  gtk_text_view_scroll_to_mark (tview, gtk_text_buffer_get_insert (tbuffer), 0.0, TRUE, 0.0, 0.0);
}

void
gxk_text_view_cursor_busy (GtkTextView *tview)
{
  g_return_if_fail (GTK_IS_TEXT_VIEW (tview));

  if (GTK_WIDGET_DRAWABLE (tview))
    {
      GdkCursor *cursor = gdk_cursor_new (GDK_WATCH);
      gdk_window_set_cursor (gtk_text_view_get_window (GTK_TEXT_VIEW (tview), GTK_TEXT_WINDOW_TEXT), cursor);
      gdk_cursor_unref (cursor);
      gdk_flush ();
    }
}

void
gxk_text_view_cursor_normal (GtkTextView *tview)
{
  g_return_if_fail (GTK_IS_TEXT_VIEW (tview));

  if (GTK_WIDGET_DRAWABLE (tview))
    {
      GdkCursor *cursor = gdk_cursor_new (GDK_XTERM);
      gdk_window_set_cursor (gtk_text_view_get_window (GTK_TEXT_VIEW (tview), GTK_TEXT_WINDOW_TEXT), cursor);
      gdk_cursor_unref (cursor);
      gdk_flush ();
    }
}

static gboolean
scroll_text_key_event (GtkWidget   *sctext,
                       GdkEventKey *event)
{
  if (event->type == GDK_KEY_PRESS)
    switch (event->keyval)
      {
      case GDK_Left: case GDK_KP_Left:
        if (event->state & GDK_MOD1_MASK)
          {
            navigate_back (sctext);
            return TRUE;
          }
      break;
    case GDK_Right: case GDK_KP_Right:
        if (event->state & GDK_MOD1_MASK)
          {
            navigate_forward (sctext);
            return TRUE;
          }
      break;
      case 'r': case 'R':
        if (event->state & GDK_CONTROL_MASK)
          {
            navigate_reload (sctext);
            return TRUE;
          }
      break;
    }
  return FALSE;
}

static void
scroll_text_patchup_size_request (GtkWidget      *scwin,
                                  GtkRequisition *requisition,
                                  GtkWidget      *sctext)
{
  GxkScrollTextFlags flags = (GxkScrollTextFlags) g_object_get_long (sctext, "GxkScrollTextFlags");
  if (!GTK_WIDGET_MAPPED (scwin))
    {
      /* provide initial size */
      if (!(flags & GXK_SCROLL_TEXT_HFIXED))
        requisition->width += 220;
      if (!(flags & GXK_SCROLL_TEXT_VFIXED))
        requisition->height += 100;
    }
}

static void
tnav_update_vpos (GtkAdjustment  *a,
                  TextNavigation *tnav)
{
  if (tnav->current)
    tnav->current->vpos = a->value > a->lower ? (a->value - a->lower) / (a->upper - a->lower) : 0;
}

/**
 * @param flags	scroll text flags
 * @param string	default contents
 *
 * Create a scrollable text view. Behaviour and apperance can
 * be tweaked by specifying various @a flags:
 * @li @c GXK_SCROLL_TEXT_MONO - use a fixed width font;
 * @li @c GXK_SCROLL_TEXT_SANS - use a sans serif font;
 * @li @c GXK_SCROLL_TEXT_SERIF - use a serif font;
 * @li @c GXK_SCROLL_TEXT_WRAP - allow word wrapping of @a string;
 * @li @c GXK_SCROLL_TEXT_CENTER - center @a string;
 * @li @c GXK_SCROLL_TEXT_WIDGET_BG - do not use white as background,
 * but keep the usual (grey) widget background;
 * @li @c GXK_SCROLL_TEXT_NAVIGATABLE - add a navigation bar and allow
 * the user to navigate through clickable links;
 * @li @c GXK_SCROLL_TEXT_EDITABLE - permit modifications of the text;
 * @li @c GXK_SCROLL_TEXT_HFIXED - make horizontal dimension unscrollable
 * @li @c GXK_SCROLL_TEXT_VFIXED - make vertical dimension unscrollable
 * @li @c GXK_SCROLL_TEXT_WIDGET_LOOK - this is a combination of flags
 * to adjust the scroll text to look like an ordinary GtkLabel,
 * which amounts to using a sans serif font, normal widget
 * background and allowing word wrapping.
 */
GtkWidget*
gxk_scroll_text_create (GxkScrollTextFlags flags,
                        const gchar       *string)
{
  GtkWidget *widget, *sctext, *scwin;
  GtkTextBuffer *tbuffer;
  GxkRadget *toolbar = NULL;

  gxk_text_buffer_init_custom ();

  /* sctext outer container */
  sctext = (GtkWidget*) g_object_new (GTK_TYPE_VBOX,
                                      "visible", TRUE,
                                      NULL);
  g_object_set_long (sctext, "GxkScrollTextFlags", flags);
  /* navigation toolbar */
  if (flags & GXK_SCROLL_TEXT_NAVIGATABLE)
    {
      toolbar = gxk_radget_create ("beast", "gxk-scroll-text-toolbar", NULL); // FIXME: domain name
      gtk_box_pack_start (GTK_BOX (sctext), (GtkWidget*) toolbar, FALSE, TRUE, 0);
    }
  else /* there's code depending on the "presence" of a widget in the toolbar slot */
    gtk_box_pack_start (GTK_BOX (sctext), (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT, NULL), FALSE, TRUE, 0);

  /* scrollable text area */
  scwin = (GtkWidget*) g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                                     "visible", TRUE,
                                     "hscrollbar_policy", (flags & GXK_SCROLL_TEXT_HFIXED) ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC,
                                     "vscrollbar_policy", (flags & GXK_SCROLL_TEXT_VFIXED) ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC,
                                     NULL);
  g_signal_connect_after (scwin, "size_request", G_CALLBACK (scroll_text_patchup_size_request), sctext);
  gtk_box_pack_start (GTK_BOX (sctext), scwin, TRUE, TRUE, 0);
  widget = (GtkWidget*) g_object_new (GTK_TYPE_TEXT_VIEW,
                                      "visible", TRUE,
                                      "parent", scwin,
                                      NULL);
  gtk_widget_grab_focus (widget);
  tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));

  /* navigation bits */
  if (toolbar)
    {
      GtkWidget *button;
      TextNavigation *tnav = navigation_from_sctext (sctext);
      tnav->vadjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scwin));
      g_object_connect (tnav->vadjustment, "signal::value_changed", tnav_update_vpos, tnav, NULL);
      g_signal_connect_swapped (tbuffer, "custom-activate", G_CALLBACK (navigate_link), sctext);
      tnav->backb = (GtkWidget*) gxk_radget_find (toolbar, "back-button");
      g_object_connect (g_object_ref (tnav->backb), "swapped_signal::clicked", navigate_back, sctext, NULL);
      gtk_widget_set_sensitive (tnav->backb, FALSE);
      tnav->forwardb = (GtkWidget*) gxk_radget_find (toolbar, "forward-button");
      g_object_connect (g_object_ref (tnav->forwardb), "swapped_signal::clicked", navigate_forward, sctext, NULL);
      gtk_widget_set_sensitive (tnav->forwardb, FALSE);
      g_object_connect (gxk_radget_find (toolbar, "reload-button"),
                        "swapped_signal::clicked", navigate_reload, sctext,
                        NULL);
      g_object_connect (gxk_radget_find (toolbar, "index-button"),
                        "swapped_signal::clicked", navigate_index, sctext,
                        NULL);
      button = (GtkWidget*) gxk_radget_find (toolbar, "find-button");
      g_object_connect (button,
                        "swapped_signal::clicked", navigate_find, sctext,
                        NULL);
      gtk_widget_set_sensitive (button, FALSE); // FIXME: implement Find
      tnav->refe = (GtkWidget*) gxk_radget_find (toolbar, "location-entry");
      g_object_connect (g_object_ref (tnav->refe),
                        "swapped_signal::activate", navigate_goto, sctext,
                        "swapped_signal::key_press_event", scroll_text_key_event, sctext,
                        NULL);
    }
  else
    g_signal_connect_swapped (tbuffer, "custom-activate", G_CALLBACK (navigate_urls), sctext);
  
  if (flags & GXK_SCROLL_TEXT_EDITABLE)
    g_object_set (widget,
                  "editable", TRUE,
                  "cursor_visible", TRUE,
                  NULL);
  else
    g_object_set (widget,
                  "editable", FALSE,
                  "cursor_visible", FALSE,
                  NULL);
  g_signal_connect_swapped (widget, "key_press_event", G_CALLBACK (scroll_text_key_event), sctext);
  gxk_text_view_enter_browse_mode (GTK_TEXT_VIEW (widget));
  if (flags & GXK_SCROLL_TEXT_WRAP)
    g_object_set (widget, "wrap_mode", GTK_WRAP_WORD, NULL);
  else
    g_object_set (widget, "wrap_mode", GTK_WRAP_NONE, NULL);
  if (flags & GXK_SCROLL_TEXT_CENTER)
    g_object_set (widget, "justification", GTK_JUSTIFY_CENTER, NULL);
  switch (flags & (GXK_SCROLL_TEXT_MONO | GXK_SCROLL_TEXT_SANS | GXK_SCROLL_TEXT_SERIF))
    {
    case GXK_SCROLL_TEXT_SANS:
      g_object_set_data ((GObject*) tbuffer, "family", (void*) "sans");
      break;
    case GXK_SCROLL_TEXT_SERIF:
      g_object_set_data ((GObject*) tbuffer, "family", (void*) "serif");
      break;
    default:
    case GXK_SCROLL_TEXT_MONO:
      g_object_set_data ((GObject*) tbuffer, "family", (void*) "monospace");
      break;
    }
  if (flags & GXK_SCROLL_TEXT_WIDGET_BG)
    gxk_widget_modify_base_as_bg (widget);

  gxk_scroll_text_append (sctext, string);
  
  return sctext;
}

GtkWidget*
gxk_scroll_text_create_for (GxkScrollTextFlags      flags,
                            GtkWidget              *parent)
{
  GtkWidget *sctext;
  g_return_val_if_fail (GTK_IS_CONTAINER (parent), NULL);
  sctext = gxk_scroll_text_create (flags, "");
  gtk_container_add (GTK_CONTAINER (parent), sctext);
  return sctext;
}

static void
text_tag_remove (GtkTextTag *tag,
                 gpointer    data)
{
  GSList **slist_p = (GSList**) data;
  if (g_object_get_int (tag, "gxk-text-tools-owned"))
    *slist_p = g_slist_prepend (*slist_p, tag);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 *
 * Clear the textual contents of this @a sctext and reset the indentation level.
 */
void
gxk_scroll_text_clear (GtkWidget *sctext)
{
  GtkTextView *tview;
  GtkTextBuffer *tbuffer;
  GtkTextTagTable *ttable;
  GtkTextIter iter1, iter2;
  GSList *node, *slist = NULL;

  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));

  tview = gxk_scroll_text_get_text_view (sctext);
  tbuffer = gtk_text_view_get_buffer (tview);
  ttable = gtk_text_buffer_get_tag_table (tbuffer);

  gtk_text_buffer_get_start_iter (tbuffer, &iter1);
  gtk_text_buffer_get_end_iter (tbuffer, &iter2);
  gtk_text_buffer_delete (tbuffer, &iter1, &iter2);
  gtk_text_tag_table_foreach (ttable, text_tag_remove, &slist);
  for (node = slist; node; node = node->next)
    gtk_text_tag_table_remove (ttable, (GtkTextTag*) node->data);
  g_slist_free (slist);
  g_object_set_int (tview, "indent", 0);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @param string	the new text to be displayed
 *
 * Replace the textual contents of this @a sctext with @a string.
 */
void
gxk_scroll_text_set (GtkWidget   *sctext,
                     const gchar *string)
{
  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));

  gxk_scroll_text_clear (sctext);
  gxk_scroll_text_append (sctext, string);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @param string	the new text to be displayed in tag-span-markup
 *
 * Replace the textual contents of this @a sctext with @a string, where
 * @a string is marked up with tag-span-markup.
 */
void
gxk_scroll_text_set_tsm (GtkWidget   *sctext,
                         const gchar *string)
{
  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));

  gxk_scroll_text_clear (sctext);
  gxk_scroll_text_append_tsm (sctext, string);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @param string	the text to be displayed
 *
 * Append @a string to the textual contents of this @a sctext.
 */
void
gxk_scroll_text_append (GtkWidget   *sctext,
                        const gchar *string)
{
  GtkTextView *tview;
  GtkTextBuffer *tbuffer;
  
  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));

  tview = gxk_scroll_text_get_text_view (sctext);
  tbuffer = gtk_text_view_get_buffer (tview);
  gxk_text_view_cursor_busy (tview);
  if (string)
    gxk_text_buffer_append_from_string (tbuffer, FALSE, g_object_get_int (tview, "indent"), strlen (string), string);
  gxk_text_buffer_add_textgets_to_view (tbuffer, tview);
  gxk_text_view_cursor_normal (tview);
  gxk_text_view_cursor_to_start (tview);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @param string	the text to be displayed in tag-span-markup
 *
 * Append @a string to the textual contents of this @a sctext, where
 * @a string is marked up with tag-span-markup.
 */
void
gxk_scroll_text_append_tsm (GtkWidget   *sctext,
                            const gchar *string)
{
  GtkTextView *tview;
  GtkTextBuffer *tbuffer;
  
  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));

  tview = gxk_scroll_text_get_text_view (sctext);
  tbuffer = gtk_text_view_get_buffer (tview);
  gxk_text_view_cursor_busy (tview);
  if (string)
    gxk_text_buffer_append_from_string (tbuffer, TRUE, g_object_get_int (tview, "indent"), strlen (string), string);
  gxk_text_buffer_add_textgets_to_view (tbuffer, tview);
  gxk_text_view_cursor_normal (tview);
  gxk_text_view_cursor_to_start (tview);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @param file_name	file holding the text to be displayed
 *
 * Append the contents of @a file_name to the textual
 * contents of this @a sctext.
 */
void
gxk_scroll_text_append_file (GtkWidget   *sctext,
                             const gchar *file_name)
{
  GtkTextView *tview;
  GtkTextBuffer *tbuffer;

  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));
  g_return_if_fail (file_name != NULL);

  tview = gxk_scroll_text_get_text_view (sctext);
  tbuffer = gtk_text_view_get_buffer (tview);
  gxk_text_view_cursor_busy (tview);
  gxk_text_buffer_append_from_file (tbuffer, FALSE, g_object_get_int (tview, "indent"), file_name);
  gxk_text_buffer_add_textgets_to_view (tbuffer, tview);
  gxk_text_view_cursor_normal (tview);
  gxk_text_view_cursor_to_start (tview);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @param file_name	file holding the text to be displayed in tag-span-markup
 *
 * Append the contents of @a file_name to the textual contents of
 * this @a sctext, where those contents are marked up with tag-span-markup.
 */
void
gxk_scroll_text_append_file_tsm (GtkWidget   *sctext,
                                 const gchar *file_name)
{
  GtkTextView *tview;
  GtkTextBuffer *tbuffer;

  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));
  g_return_if_fail (file_name != NULL);

  tview = gxk_scroll_text_get_text_view (sctext);
  tbuffer = gtk_text_view_get_buffer (tview);
  gxk_text_view_cursor_busy (tview);
  gxk_text_buffer_append_from_file (tbuffer, TRUE, g_object_get_int (tview, "indent"), file_name);
  gxk_text_buffer_add_textgets_to_view (tbuffer, tview);
  gxk_text_view_cursor_normal (tview);
  gxk_text_view_cursor_to_start (tview);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @param text_fmt	printf(3) style format string
 *
 * Append @a text_fmt to the textual contents of this @a sctext.
 */
void
gxk_scroll_text_aprintf (GtkWidget   *sctext,
                         const gchar *text_fmt,
                         ...)
{
  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));

  if (text_fmt)
    {
      va_list args;
      gchar *buffer;
      
      va_start (args, text_fmt);
      buffer = g_strdup_vprintf (text_fmt, args);
      va_end (args);

      gxk_scroll_text_append (sctext, buffer);
      g_free (buffer);
    }
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @return		a GtkTextView widget
 *
 * Return the internally used GtkTextView of this @a sctext.
 */
GtkTextView*
gxk_scroll_text_get_text_view (GtkWidget *sctext)
{
  GtkTextView *tview;
  GtkWidget *scwin;

  g_return_val_if_fail (GXK_IS_SCROLL_TEXT (sctext), NULL);

  scwin = ((GtkBoxChild*) GTK_BOX (sctext)->children->next->data)->widget;

  tview = GTK_TEXT_VIEW (GTK_BIN (scwin)->child);

  return tview;
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 *
 * Increment the global indentation level, which affects overall
 * indentation of text added with gxk_scroll_text_append() and friends.
 */
void
gxk_scroll_text_push_indent (GtkWidget *sctext)
{
  GtkTextView *tview;
  guint indent;

  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));

  tview = gxk_scroll_text_get_text_view (sctext);
  indent = g_object_get_int (tview, "indent");
  g_object_set_int (tview, "indent", indent + 2);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 *
 * Decrement the global indentation level after a previous
 * increment with gxk_scroll_text_push_indent().
 */
void
gxk_scroll_text_pop_indent (GtkWidget *sctext)
{
  GtkTextView *tview;
  guint indent;

  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));

  tview = gxk_scroll_text_get_text_view (sctext);
  indent = g_object_get_int (tview, "indent");
  if (indent)
    g_object_set_int (tview, "indent", indent - 2);
}

static void
navigation_reset_url (TextNavigation *tnav)
{
  g_free (tnav->proto);
  tnav->proto = g_strdup ("file");
  g_free (tnav->path);
  tnav->path = g_strdup ("/");
  g_free (tnav->file);
  tnav->file = g_strdup ("");
  g_free (tnav->anchor);
  tnav->anchor = NULL;
}

static gboolean
navigation_test_file (const gchar *dir,
                      const gchar *path,
                      const gchar *file)
{
  gchar *fname = g_strconcat (dir ? dir : "",
                              "/",
                              path ? path : "",
                              "/",
                              file,
                              NULL);
  gboolean check = g_file_test (fname, G_FILE_TEST_EXISTS);
  g_free (fname);
  return check;
}

static void
navigation_set_url (TextNavigation *tnav,
                    const gchar    *src_url,
                    gdouble         vert_frac)
{
  gchar *p, *url = g_strdup (src_url), *buffer = url;

  p = strchr (url, ':');
  if (p)
    {
      g_free (tnav->proto);
      tnav->proto = g_strndup (url, p - url);
      url = p + 1;
    }

  g_free (tnav->anchor);
  p = strrchr (url, '#');
  if (p)
    {
      *p++ = 0;
      tnav->anchor = g_strdup (p);
    }
  else
    tnav->anchor = NULL;

  p = strrchr (url, '/');
  if (p)
    {
      g_free (tnav->file);
      p += 1;
      tnav->file = g_strdup (p);
      *p = 0;
    }
  else if (url[0])
    {
      g_free (tnav->file);
      tnav->file = g_strdup (url);
      *url = 0;
    }

  if (url[0] == '/')
    {
      g_free (tnav->path);
      while (url[1] == '/')
        url++;
      if (url[strlen (url) - 1] == '/')
        tnav->path = g_strdup (url);
      else
        tnav->path = g_strconcat (url, "/", NULL);
    }
  else if (url[0])
    {
      p = g_strconcat (tnav->path, url, NULL);
      g_free (tnav->path);
      tnav->path = p;
    }
  /* here, url contains just path segment */

  /* patch up relative file names */
  if (url[0] != '/'
      && strcmp (tnav->proto, "file") == 0
      /* mozilla-alike behaviour also needs: && !navigation_test_file (NULL, tnav->path, tnav->file) */
      && TRUE)
    {
      GSList *slist;
      for (slist = tsm_paths; slist; slist = slist->next)
        if (navigation_test_file ((const char*) slist->data, url, tnav->file))
          {
            /* take first match */
            g_free (tnav->path);
            tnav->path = g_strconcat ((const char*) slist->data, url,
                                      url[0] && url[strlen (url) - 1] != '/' ? "/" : "",
                                      NULL);
            break;
          }
    }

  g_free (buffer);
  tnav->vert_frac = vert_frac;
}

static gchar*
navigation_strdup_url (TextNavigation *tnav)
{
  if (tnav->anchor)
    return g_strconcat (tnav->proto, ":", tnav->path, tnav->file, "#", tnav->anchor, NULL);
  else
    return g_strconcat (tnav->proto, ":", tnav->path, tnav->file, NULL);
}

static void
navigation_clear_fore_stack (TextNavigation *tnav)
{
  GSList *slist;
  for (slist = tnav->fore_stack; slist; slist = slist->next)
    {
      HEntry *hentry = (HEntry*) slist->data;
      g_free (hentry->url);
      g_free (hentry);
    }
  g_slist_free (tnav->fore_stack);
  tnav->fore_stack = NULL;
}

static void
navigation_update_widgets (TextNavigation *tnav)
{
  /* handle sensitivity */
  if (tnav->backb)
    gtk_widget_set_sensitive (tnav->backb, tnav->back_stack != NULL);
  if (tnav->forwardb)
    gtk_widget_set_sensitive (tnav->forwardb, tnav->fore_stack != NULL);
  /* update location */
  if (tnav->refe)
    gtk_entry_set_text (GTK_ENTRY (tnav->refe), tnav->current ? tnav->current->url : "");
}

static void
free_navigation (gpointer data)
{
  TextNavigation *tnav = (TextNavigation*) data;
  GSList *slist;
  if (tnav->backb)
    g_object_unref (tnav->backb);
  if (tnav->forwardb)
    g_object_unref (tnav->forwardb);
  if (tnav->refe)
    g_object_unref (tnav->refe);
  g_free (tnav->index);
  g_free (tnav->proto);
  g_free (tnav->path);
  g_free (tnav->file);
  g_free (tnav->anchor);
  if (tnav->current)
    {
      g_free (tnav->current->url);
      g_free (tnav->current);
    }
  navigation_clear_fore_stack (tnav);
  for (slist = tnav->back_stack; slist; slist = slist->next)
    {
      HEntry *hentry = (HEntry*) slist->data;
      g_free (hentry->url);
      g_free (hentry);
    }
  g_slist_free (tnav->back_stack);
  g_free (tnav);
}

static TextNavigation*
navigation_from_sctext (GtkWidget *sctext)
{
  TextNavigation *tnav = (TextNavigation*) g_object_get_data (G_OBJECT (sctext), "gxk-text-tools-navigation");
  if (!tnav)
    {
      tnav = g_new0 (TextNavigation, 1);
      tnav->sctext = sctext;
      tnav->index = g_strdup ("about:");
      navigation_reset_url (tnav);
      g_object_set_data_full (G_OBJECT (sctext), "gxk-text-tools-navigation", tnav, free_navigation);
    }
  return tnav;
}

enum {
  FILE_TYPE_UNKNOWN,
  FILE_TYPE_TSM,
  FILE_TYPE_DIR,
};

static guint
guess_file_type (const gchar *file_name)
{
  gint xerr, fd = open (file_name, O_RDONLY);
  if (fd >= 0)
    {
      guint8 buffer[FILE_READ_BUFFER_SIZE + 1];
      gint l;
      do
        l = read (fd, buffer, FILE_READ_BUFFER_SIZE);
      while (l < 0 && errno == EINTR);
      xerr = l < 0 ? errno : 0;
      close (fd);
      if (l)
        {
          gchar *p = (gchar*) buffer;
          buffer[l] = 0;
#if 0
          while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
            p++;
          if (strncmp (p, "<tag-span-markup>", 17) == 0)
            return FILE_TYPE_TSM;
#endif
          if (strstr (p, "<!DOCTYPE tag-span-markup") && strstr (p, "<tag-span-markup>"))
            return FILE_TYPE_TSM;
        }
    }
  else
    xerr = errno;
  if (xerr == EISDIR)
    return FILE_TYPE_DIR;
  return FILE_TYPE_UNKNOWN;
}

static void
g_string_add_xmlstr (GString     *gstring,
                     const gchar *str)
{
  const gchar *p;
  for (p = str; *p; p++)
    switch (*p)
      {
      case '<':  g_string_append (gstring, "&lt;");     break;
      case '>':  g_string_append (gstring, "&gt;");     break;
      case '&':  g_string_append (gstring, "&amp;");    break;
      case '"':  g_string_append (gstring, "&quot;");   break;
      case '\'': g_string_append (gstring, "&apos;");   break;
      default:   g_string_append_c (gstring, *p);
      }
}

static gboolean
adjust_vscroll_offset (gpointer data)
{
  GtkWidget *sctext = (GtkWidget*) data;
  TextNavigation *tnav;
  GDK_THREADS_ENTER ();
  tnav = navigation_from_sctext (sctext);
  if (GTK_WIDGET_REALIZED (sctext) && tnav->vert_frac >= 0)
    {
      GtkAdjustment *a = tnav->vadjustment;
      gdouble v = a->lower + tnav->vert_frac * (a->upper - a->lower);
      gtk_adjustment_set_value (a, CLAMP (v, a->lower, a->upper - a->page_size));
    }
  GDK_THREADS_LEAVE ();
  return FALSE;
}

static void
scroll_text_reload (GtkWidget *sctext)
{
  TextNavigation *tnav = navigation_from_sctext (sctext);

  gxk_scroll_text_clear (sctext);
  if (strcmp (tnav->proto, "about") == 0)
    gxk_scroll_text_set_tsm (sctext,
                             "<tag-span-markup>"
                             "<tagdef name='center' justification='center'/>"
                             "<span tag='center'><newline/>"
                             "ABOUT<newline/><newline/>"
                             "The &quot;tag-span-markup&quot; viewer is a compact<newline/>"
                             "xml markup browser, based on the GtkTextView and GtkTextBuffer<newline/>"
                             "facilities. The xml tags closely resemble GtkTextTag properties.</span>"
                             "</tag-span-markup>"
                             );
  else if (strcmp (tnav->proto, "file") == 0)
    {
      gchar *file = g_strconcat (tnav->path, tnav->file, NULL);
      guint file_guess = guess_file_type (file);
      if (file_guess == FILE_TYPE_TSM)
        gxk_scroll_text_append_file_tsm (sctext, file);
      else if (file_guess == FILE_TYPE_DIR)
        {
          struct dirent **flist;
          gchar *tmp;
          gint n;
          n = scandir (file, &flist, NULL, alphasort);
          if (n < 0)
            gxk_scroll_text_append_file (sctext, file); /* revert to "unknown file" */
          else
            {
              GString *gstring = g_string_new (
                                               "<tag-span-markup>\n"
                                               "<tagdef name='body' family='monospace'/>\n"
                                               "<tagdef name='hyperlink' underline='single' foreground='#0000ff'/>\n"
                                               "<span tag='body'><keep-space>\n"
                                               );
              while (n--)
                {
                  gchar istr[256];
                  g_snprintf (istr, sizeof (istr), "&lt;%lu&gt;                    ", flist[n]->d_ino);
                  g_string_append_printf (gstring, "  %.20s ", istr);
                  g_string_append (gstring, "<span tag='hyperlink'><xlink ref='");
                  tmp = g_strconcat (file, "/", flist[n]->d_name, NULL);
                  g_string_add_xmlstr (gstring, tmp);
                  g_free (tmp);
                  g_string_append (gstring, "'>");
                  g_string_add_xmlstr (gstring, flist[n]->d_name);
                  g_string_append (gstring, "</xlink></span>\n");
                  free (flist[n]);
                }
              free (flist);
              g_string_append (gstring, "</keep-space></span></tag-span-markup>");
              gxk_scroll_text_append_tsm (sctext, gstring->str);
              g_string_free (gstring, TRUE);
            }
        }
      else
        gxk_scroll_text_append_file (sctext, file);
      g_free (file);
      if (tnav->anchor)
        {
          GtkTextView *tview = gxk_scroll_text_get_text_view (sctext);
          GtkTextBuffer *tbuffer = gtk_text_view_get_buffer (tview);
          gchar *aname = g_strconcat ("#-", tnav->anchor, NULL);
          GtkTextMark *mark = gtk_text_buffer_get_mark (tbuffer, aname);
          GtkTextIter iter;
          g_free (aname);
          if (mark)
            gtk_text_buffer_get_iter_at_mark (tbuffer, &iter, mark);
          else
            gtk_text_buffer_get_end_iter (tbuffer, &iter);
          gtk_text_buffer_place_cursor (tbuffer, &iter);
          gtk_text_view_scroll_to_mark (tview, gtk_text_buffer_get_insert (tbuffer), 0.0, TRUE, 0.0, 0.0);
        }
    }
  else
    {
      gchar *loc = navigation_strdup_url (tnav);
      text_buffer_add_error (gtk_text_view_get_buffer (gxk_scroll_text_get_text_view (sctext)),
                             "Resource locator with unknown method: %s", loc);
      g_free (loc);
      gxk_text_view_cursor_to_end (gxk_scroll_text_get_text_view (sctext));
    }
  if (tnav->vert_frac >= 0)
    g_idle_add_full (G_PRIORITY_LOW + 100, adjust_vscroll_offset, g_object_ref (sctext), g_object_unref);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @param uri	resource locator
 *
 * Load and display the resource from @a uri without
 * altering the navigation history.
 */
void
gxk_scroll_text_display (GtkWidget   *sctext,
                         const gchar *uri)
{
  TextNavigation *tnav;
  
  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));
  g_return_if_fail (uri != NULL);
  
  tnav = navigation_from_sctext (sctext);
  navigation_set_url (tnav, uri, -1);

  scroll_text_reload (sctext);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @param uri	resource locator
 *
 * Relative to the url currently being displayed
 * load and display the possibly partial (relative)
 * url @a uri. Navigation history is affected.
 */
void
gxk_scroll_text_advance (GtkWidget   *sctext,
                         const gchar *uri)
{
  TextNavigation *tnav;
  HEntry *last;

  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));
  g_return_if_fail (uri != NULL);

  tnav = navigation_from_sctext (sctext);
  /* handle history */
  last = tnav->back_stack ? (HEntry*) tnav->back_stack->data : NULL;
  if (tnav->current)
    {
      if (last && strcmp (last->url, tnav->current->url) == 0)
        {
          g_free (tnav->current->url);
          g_free (tnav->current);
        }
      else
        {
          tnav->back_stack = g_slist_prepend (tnav->back_stack, tnav->current);
          last = tnav->current;
        }
      tnav->current = NULL;
    }
  navigation_clear_fore_stack (tnav);
  /* set new uri */
  navigation_set_url (tnav, uri, -1);
  /* prepare for next history */
  tnav->current = g_new (HEntry, 1);
  tnav->current->url = navigation_strdup_url (tnav);
  tnav->current->vpos = -1;
  /* dedup history */
  if (last && strcmp (last->url, tnav->current->url) == 0)
    {
      last = (HEntry*) g_slist_pop_head (&tnav->back_stack);
      g_free (last->url);
      g_free (last);
    }
  /* show away */
  scroll_text_reload (sctext);
  navigation_update_widgets (tnav);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @param uri	resource locator
 *
 * Load and display the url @a uri.
 * Navigation history is affected.
 */
void
gxk_scroll_text_enter (GtkWidget   *sctext,
                       const gchar *uri)
{
  TextNavigation *tnav;

  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));
  g_return_if_fail (uri != NULL);

  tnav = navigation_from_sctext (sctext);
  navigation_reset_url (tnav);
  gxk_scroll_text_advance (sctext, uri);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 * @param uri	resource locator
 *
 * Affect what uri is being displayed by pressing
 * on the "Index" navigation button.
 */
void
gxk_scroll_text_set_index (GtkWidget   *sctext,
                           const gchar *uri)
{
  TextNavigation *tnav;

  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));
  if (!uri || !uri[0])
    uri = "about:";

  tnav = navigation_from_sctext (sctext);
  g_free (tnav->index);
  tnav->index = g_strdup (uri);
}

/**
 * @param sctext	a scroll text widget as returned from gxk_scroll_text_create()
 *
 * Go back in navigation history as far as possible.
 */
void
gxk_scroll_text_rewind (GtkWidget *sctext)
{
  TextNavigation *tnav;

  g_return_if_fail (GXK_IS_SCROLL_TEXT (sctext));

  tnav = navigation_from_sctext (sctext);
  while (tnav->back_stack)
    {
      if (tnav->current)
        {
          HEntry *next = tnav->fore_stack ? (HEntry*) tnav->fore_stack->data : NULL;
          if (next && strcmp (next->url, tnav->current->url) == 0)
            {
              g_free (tnav->current->url);
              g_free (tnav->current);
            }
          else
            tnav->fore_stack = g_slist_prepend (tnav->fore_stack, tnav->current);
        }
      tnav->current = (HEntry*) g_slist_pop_head (&tnav->back_stack);
    }
  if (tnav->current)
    {
      navigation_reset_url (tnav);
      navigation_set_url (tnav, tnav->current->url, -1);
      scroll_text_reload (sctext);
    }
  navigation_update_widgets (tnav);
  gxk_text_view_cursor_to_start (gxk_scroll_text_get_text_view (sctext));
}

static void
navigate_back (GtkWidget *sctext)
{
  TextNavigation *tnav = navigation_from_sctext (sctext);
  if (tnav->back_stack)
    {
      if (tnav->current)
        {
          HEntry *next = tnav->fore_stack ? (HEntry*) tnav->fore_stack->data : NULL;
          if (next && strcmp (next->url, tnav->current->url) == 0)
            {
              g_free (tnav->current->url);
              g_free (tnav->current);
            }
          else
            tnav->fore_stack = g_slist_prepend (tnav->fore_stack, tnav->current);
        }
      tnav->current = (HEntry*) g_slist_pop_head (&tnav->back_stack);
      navigation_reset_url (tnav);
      navigation_set_url (tnav, tnav->current->url, tnav->current->vpos);
      scroll_text_reload (sctext);
      navigation_update_widgets (tnav);
    }
}

static void
navigate_forward (GtkWidget *sctext)
{
  TextNavigation *tnav = navigation_from_sctext (sctext);
  if (tnav->fore_stack)
    {
      if (tnav->current)
        {
          HEntry *last = tnav->back_stack ? (HEntry*) tnav->back_stack->data : NULL;
          if (last && strcmp (last->url, tnav->current->url) == 0)
            {
              g_free (tnav->current->url);
              g_free (tnav->current);
            }
          else
            tnav->back_stack = g_slist_prepend (tnav->back_stack, tnav->current);
        }
      tnav->current = (HEntry*) g_slist_pop_head (&tnav->fore_stack);
      navigation_reset_url (tnav);
      navigation_set_url (tnav, tnav->current->url, tnav->current->vpos);
      scroll_text_reload (sctext);
      navigation_update_widgets (tnav);
    }
}

static void
navigate_index (GtkWidget *sctext)
{
  TextNavigation *tnav = navigation_from_sctext (sctext);
  gxk_scroll_text_enter (sctext, tnav->index);
}

static void
navigate_find (GtkWidget *sctext)
{
  gxk_scroll_text_enter (sctext, "about:");
}

static void
navigate_reload (GtkWidget *sctext)
{
  TextNavigation *tnav = navigation_from_sctext (sctext);
  tnav->vert_frac = -1;
  scroll_text_reload (sctext);
}

static void
navigate_goto (GtkWidget *sctext)
{
  TextNavigation *tnav = navigation_from_sctext (sctext);
  const gchar *text = gtk_entry_get_text (GTK_ENTRY (tnav->refe));
  if (text && text[0])
    gxk_scroll_text_enter (sctext, text);
}

static bool
navigate_urls (GtkWidget   *sctext,
               const gchar *uri)
{
  if (strncmp (uri, "ftp:", 4) == 0 ||
      strncmp (uri, "http:", 5) == 0 ||
      strncmp (uri, "https:", 6) == 0 ||
      strncmp (uri, "mailto:", 7) == 0)
    {
      sfi_url_show (uri);
      return TRUE;
    }
  else
    return FALSE;
}

static void
navigate_link (GtkWidget   *sctext,
               const gchar *uri)
{
  if (!navigate_urls (sctext, uri))
    gxk_scroll_text_advance (sctext, uri);
}
