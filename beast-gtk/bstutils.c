/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#include "bstutils.h"

#include "bstmenus.h"
#include "bsttrackview.h"
#include "bstwaveview.h"
#include "bstpartview.h"
#include "bstpianoroll.h"
#include "bsteventroll.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>


/* --- generated enums --- */
#include "bstenum_arrays.c"     /* enum string value arrays plus include directives */


/* --- prototypes --- */
static void     _bst_init_idl                   (void);


/* --- variables --- */
static GtkIconFactory *stock_icon_factory = NULL;


/* --- functions --- */
void
_bst_init_utils (void)
{
  g_assert (stock_icon_factory == NULL);
  
  stock_icon_factory = gtk_icon_factory_new ();
  gtk_icon_factory_add_default (stock_icon_factory);
  
  /* initialize generated type ids */
  {
    static struct {
      gchar            *type_name;
      GType             parent;
      GType            *type_id;
      gconstpointer     pointer1;
    } builtin_info[] = {
#include "bstenum_list.c"       /* type entries */
    };
    guint i;
    for (i = 0; i < sizeof (builtin_info) / sizeof (builtin_info[0]); i++)
      {
        GType type_id = 0;
        
        if (builtin_info[i].parent == G_TYPE_ENUM)
          type_id = g_enum_register_static (builtin_info[i].type_name, builtin_info[i].pointer1);
        else if (builtin_info[i].parent == G_TYPE_FLAGS)
          type_id = g_flags_register_static (builtin_info[i].type_name, builtin_info[i].pointer1);
        else
          g_assert_not_reached ();
        g_assert (g_type_name (type_id) != NULL);
        *builtin_info[i].type_id = type_id;
      }
  }
  
  /* initialize IDL types */
  _bst_init_idl ();
  
  /* initialize stock icons (included above) */
  {
    /* generated stock icons */
#include "beast-gtk/icons/bst-stock-gen.c"
    
    gxk_stock_register_icons (G_N_ELEMENTS (stock_icons), stock_icons);
  }
  
  /* initialize stock actions */
  {
    static const GxkStockItem stock_items[] = {
      { BST_STOCK_CLONE,                "_Clone",       GTK_STOCK_COPY,                 },
      { BST_STOCK_DEFAULT_REVERT,       "_Defaults",    GTK_STOCK_UNDO,                 },
      { BST_STOCK_LOAD,                 "_Load",        NULL,                           },
      { BST_STOCK_OVERWRITE,            "_Overwrite",   GTK_STOCK_SAVE,                 },
      { BST_STOCK_REVERT,               "_Revert",      GTK_STOCK_UNDO,                 },
    };
    gxk_stock_register_items (G_N_ELEMENTS (stock_items), stock_items);
  }
}

#include "beast-gtk/dialogs/beast-xml-zfiles.c"
void
_bst_init_gadgets (void)
{
  gchar *text;
  gxk_gadget_define_widget_type (BST_TYPE_TRACK_VIEW);
  gxk_gadget_define_widget_type (BST_TYPE_WAVE_VIEW);
  gxk_gadget_define_widget_type (BST_TYPE_PART_VIEW);
  gxk_gadget_define_widget_type (BST_TYPE_PIANO_ROLL);
  gxk_gadget_define_widget_type (BST_TYPE_EVENT_ROLL);
  gxk_gadget_define_widget_type (BST_TYPE_ZOOMED_WINDOW);
  text = gxk_zfile_uncompress (BST_GADGETS_STANDARD_SIZE, BST_GADGETS_STANDARD_DATA, G_N_ELEMENTS (BST_GADGETS_STANDARD_DATA));
  gxk_gadget_parse_text ("beast", text, -1, NULL);
  g_free (text);
  text = gxk_zfile_uncompress (BST_GADGETS_BEAST_SIZE, BST_GADGETS_BEAST_DATA, G_N_ELEMENTS (BST_GADGETS_BEAST_DATA));
  gxk_gadget_parse_text ("beast", text, -1, NULL);
  g_free (text);
}

GtkWidget*
bst_stock_button (const gchar *stock_id)
{
  GtkWidget *w = gtk_button_new_from_stock (stock_id);
  gtk_widget_show_all (w);
  return w;
}

GtkWidget*
bst_stock_dbutton (const gchar *stock_id)
{
  GtkWidget *w = bst_stock_button (stock_id);
  g_object_set (w, "can-default", TRUE, NULL);
  return w;
}

GtkWidget*
bst_stock_icon_button (const gchar *stock_id)
{
  GtkWidget *w = g_object_new (GTK_TYPE_BUTTON,
                               "visible", TRUE,
                               "child", gtk_image_new_from_stock (stock_id, GXK_ICON_SIZE_BUTTON),
                               "can-focus", FALSE,
                               NULL);
  gtk_widget_show_all (w);
  return w;
}

void
bst_stock_register_icon (const gchar    *stock_id,
                         guint           bytes_per_pixel,
                         guint           width,
                         guint           height,
                         guint           rowstride,
                         const guint8   *pixels)
{
  g_return_if_fail (bytes_per_pixel == 3 || bytes_per_pixel == 4);
  g_return_if_fail (width > 0 && height > 0 && rowstride >= width * bytes_per_pixel);
  
  if (!gtk_icon_factory_lookup (stock_icon_factory, stock_id))
    {
      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data (g_memdup (pixels, rowstride * height * bytes_per_pixel),
                                                    GDK_COLORSPACE_RGB, bytes_per_pixel == 4,
                                                    8, width, height,
                                                    width * bytes_per_pixel,
                                                    (GdkPixbufDestroyNotify) g_free, NULL);
      GtkIconSet *iset = gtk_icon_set_new_from_pixbuf (pixbuf);
      g_object_unref (pixbuf);
      gtk_icon_factory_add (stock_icon_factory, stock_id, iset);
      gtk_icon_set_unref (iset);
    }
}


/* --- beast/bsw specific extensions --- */
void
bst_status_eprintf (BseErrorType error,
                    const gchar *message_fmt,
                    ...)
{
  gchar *buffer;
  va_list args;
  
  va_start (args, message_fmt);
  buffer = g_strdup_vprintf (message_fmt, args);
  va_end (args);
  
  if (error)
    gxk_status_set (GXK_STATUS_ERROR, buffer, bse_error_blurb (error));
  else
    gxk_status_set (GXK_STATUS_DONE, buffer, NULL);
  g_free (buffer);
}

typedef struct {
  GtkWindow *window;
  SfiProxy   proxy;
  gchar     *title1;
  gchar     *title2;
} TitleSync;

static void
sync_title (TitleSync *tsync)
{
  const gchar *name = bse_item_get_name (tsync->proxy);
  gchar *s;
  
  s = g_strconcat (tsync->title1, name ? name : "<NULL>", tsync->title2, NULL);
  g_object_set (tsync->window, "title", s, NULL);
  g_free (s);
}

static void
free_title_sync (gpointer data)
{
  TitleSync *tsync = data;
  
  bse_proxy_disconnect (tsync->proxy,
                        "any_signal", sync_title, tsync,
                        NULL);
  g_free (tsync->title1);
  g_free (tsync->title2);
  g_free (tsync);
}

void
bst_window_sync_title_to_proxy (gpointer     window,
                                SfiProxy     proxy,
                                const gchar *title_format)
{
  gchar *p;
  
  g_return_if_fail (GTK_IS_WINDOW (window));
  if (proxy)
    {
      g_return_if_fail (BSE_IS_ITEM (proxy));
      g_return_if_fail (title_format != NULL);
      /* g_return_if_fail (strstr (title_format, "%s") != NULL); */
    }
  
  p = title_format ? strstr (title_format, "%s") : NULL;
  if (proxy && p)
    {
      TitleSync *tsync = g_new0 (TitleSync, 1);
      
      tsync->window = window;
      tsync->proxy = proxy;
      tsync->title1 = g_strndup (title_format, p - title_format);
      tsync->title2 = g_strdup (p + 2);
      bse_proxy_connect (tsync->proxy,
                         "swapped_signal::property-notify::uname", sync_title, tsync,
                         NULL);
      g_object_set_data_full (window, "bst-title-sync", tsync, free_title_sync);
      sync_title (tsync);
    }
  else
    {
      g_object_set_data (window, "bst-title-sync", NULL);
      g_object_set (window, "title", title_format, NULL);
    }
}


/* --- packing utilities --- */
#define SPACING 3
static void
bst_util_pack (GtkWidget   *widget,
               const gchar *location,
               guint        spacing,
               va_list      args)
{
  GtkBox *box = GTK_BOX (widget);
  while (location)
    {
      gchar *t, **toks = g_strsplit (location, ":", -1);
      guint border = 0, padding = 0, i = 0;
      gboolean fill = FALSE, expand = FALSE, start = TRUE;
      GtkWidget *child;
      t = toks[i++];
      if (t && t[0] >= '0' && t[0] <= '9')
        {
          border = g_ascii_strtoull (t, NULL, 10);
          t = toks[i++];
        }
      if (t && t[0] == '+')
        {
          expand = TRUE;
          t = toks[i++];
        }
      if (t && t[0] == '*')
        {
          expand = fill = TRUE;
          t = toks[i++];
        }
      if (t && t[0] == 'H')
        {
          gtk_box_set_homogeneous (box, TRUE);
          expand = fill = TRUE;
          t = toks[i++];
        }
      if (t && t[0] == 's')
        {
          start = TRUE;
          t = toks[i++];
        }
      if (t && t[0] == 'e')
        {
          start = FALSE;
          t = toks[i++];
        }
      if (t && t[0] == 'p')
        {
          padding = g_ascii_strtoull (t + 1, NULL, 10);
          t = toks[i++];
        }
      g_strfreev (toks);
      child = va_arg (args, GtkWidget*);
      if (child)
        {
          if (border)
            child = g_object_new (GTK_TYPE_ALIGNMENT,
                                  "child", child,
                                  "border_width", border * spacing,
                                  NULL);
          if (start)
            gtk_box_pack_start (box, child, expand, fill, padding);
          else
            gtk_box_pack_end (box, child, expand, fill, padding);
        }
      location = va_arg (args, gchar*);
    }
}

GtkWidget*
bst_vpack (const gchar *first_location,
           ...)
{
  va_list args;
  GtkWidget *box = g_object_new (GTK_TYPE_VBOX,
                                 "spacing", SPACING,
                                 NULL);
  va_start (args, first_location);
  bst_util_pack (box, first_location, SPACING, args);
  va_end (args);
  gtk_widget_show_all (box);
  return box;
}

GtkWidget*
bst_hpack (const gchar *first_location,
           ...)
{
  va_list args;
  GtkWidget *box = g_object_new (GTK_TYPE_HBOX,
                                 "spacing", SPACING,
                                 NULL);
  va_start (args, first_location);
  bst_util_pack (box, first_location, SPACING, args);
  va_end (args);
  gtk_widget_show_all (box);
  return box;
}

GtkWidget*
bst_vpack0 (const gchar *first_location,
            ...)
{
  va_list args;
  GtkWidget *box = g_object_new (GTK_TYPE_VBOX,
                                 "spacing", 0,
                                 NULL);
  va_start (args, first_location);
  bst_util_pack (box, first_location, SPACING, args);
  va_end (args);
  gtk_widget_show_all (box);
  return box;
}

GtkWidget*
bst_hpack0 (const gchar *first_location,
            ...)
{
  va_list args;
  GtkWidget *box = g_object_new (GTK_TYPE_HBOX,
                                 "spacing", 0,
                                 NULL);
  va_start (args, first_location);
  bst_util_pack (box, first_location, SPACING, args);
  va_end (args);
  gtk_widget_show_all (box);
  return box;
}


void
bst_action_list_add_cat (GxkActionList          *alist,
                         BseCategory            *cat,
                         guint                   skip_levels,
                         const gchar            *stock_fallback,
                         GxkActionCheck          acheck,
                         GxkActionExec           aexec,
                         gpointer                user_data)
{
  const gchar *p, *stock_id;

  if (cat->icon && (cat->icon->width + cat->icon->height) > 0)  // FIXME: need NULL icons
    {
      bst_stock_register_icon (cat->category, cat->icon->bytes_per_pixel,
                               cat->icon->width, cat->icon->height,
                               cat->icon->width * cat->icon->bytes_per_pixel,
                               cat->icon->pixels->bytes);
      stock_id = cat->category;
    }
  else
    stock_id = stock_fallback;

  p = cat->category[0] == '/' ? cat->category + 1 : cat->category;      // FIXME: needs i18n
  while (skip_levels--)
    {
      const gchar *d = strchr (p, '/');
      p = d ? d + 1 : p;
    }

  gxk_action_list_add_translated (alist, cat->category, p, NULL, NULL,
                                  cat->category_id, stock_id,
                                  acheck, aexec, user_data);
}

GxkActionList*
bst_action_list_from_cats (BseCategorySeq         *cseq,
                           guint                   skip_levels,
                           const gchar            *stock_fallback,
                           GxkActionCheck          acheck,
                           GxkActionExec           aexec,
                           gpointer                user_data)
{
  GxkActionList *alist = gxk_action_list_create ();
  guint i;

  g_return_val_if_fail (cseq != NULL, alist);

  for (i = 0; i < cseq->n_cats; i++)
    bst_action_list_add_cat (alist, cseq->cats[i], skip_levels, stock_fallback, acheck, aexec, user_data);
  return alist;
}


/* --- field mask --- */
static GQuark gmask_quark = 0;
typedef struct {
  GtkWidget   *parent;
  GtkWidget   *prompt;
  GtkWidget   *aux1;
  GtkWidget   *aux2;            /* auto-expand */
  GtkWidget   *aux3;
  GtkWidget   *ahead;
  GtkWidget   *action;
  GtkWidget   *atail;
  gchar       *tip;
  guint        column : 16;
  guint        gpack : 8;
} GMask;
#define GMASK_GET(o)    ((GMask*) g_object_get_qdata (G_OBJECT (o), gmask_quark))

static void
gmask_destroy (gpointer data)
{
  GMask *gmask = data;
  
  if (gmask->parent)
    g_object_unref (gmask->parent);
  if (gmask->prompt)
    g_object_unref (gmask->prompt);
  if (gmask->aux1)
    g_object_unref (gmask->aux1);
  if (gmask->aux2)
    g_object_unref (gmask->aux2);
  if (gmask->aux3)
    g_object_unref (gmask->aux3);
  if (gmask->ahead)
    g_object_unref (gmask->ahead);
  if (gmask->atail)
    g_object_unref (gmask->atail);
  g_free (gmask->tip);
  g_free (gmask);
}

static gpointer
gmask_form (GtkWidget   *parent,
            GtkWidget   *action,
            BstGMaskPack gpack)
{
  GMask *gmask;
  
  g_return_val_if_fail (GTK_IS_TABLE (parent), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (action), NULL);
  
  if (!gmask_quark)
    gmask_quark = g_quark_from_static_string ("GMask");
  
  gmask = GMASK_GET (action);
  g_return_val_if_fail (gmask == NULL, NULL);
  
  gmask = g_new0 (GMask, 1);
  g_object_set_qdata_full (G_OBJECT (action), gmask_quark, gmask, gmask_destroy);
  gmask->parent = g_object_ref (parent);
  gtk_object_sink (GTK_OBJECT (parent));
  gmask->action = action;
  gpack = CLAMP (gpack, BST_GMASK_FIT, BST_GMASK_CENTER);
  gmask->gpack = gpack;
  
  return action;
}

/**
 * bst_gmask_container_create
 * @border_width:     Border width of this GUI mask
 * @dislodge_columns: Provide expandable space between columns
 * @RETURNS:          GUI field mask container
 *
 * Create a container capable to hold GUI field masks.
 * This is the container to be passed into bst_gmask_form().
 * In case multiple field mask columns are packed into the
 * container (by using bst_gmask_set_column() on the filed
 * masks), @dislodge_columns specifies whether the field
 * mask columns are to be closely aligned.
 */
GtkWidget*
bst_gmask_container_create (guint    border_width,
                            gboolean dislodge_columns)
{
  GtkWidget *container = gtk_widget_new (GTK_TYPE_TABLE,
                                         "visible", TRUE,
                                         "homogeneous", FALSE,
                                         "n_columns", 2,
                                         "border_width", border_width,
                                         NULL);
  if (dislodge_columns)
    g_object_set_data (G_OBJECT (container), "GMask-dislodge", GUINT_TO_POINTER (TRUE));
  
  return container;
}

/**
 * bst_gmask_form
 * @gmask_container: container created with bst_gmask_container_create()
 * @action:          valid #GtkWidget
 * @gpack:           #BstGMaskPack packing type
 * @RETURNS:         a new GUI field mask
 *
 * Create a new GUI field mask with @action as action widget.
 * Each GUI field mask consists of an action widget which may
 * be neighboured by pre and post action widgets, the action
 * widget is usually something like a #GtkEntry input widget.
 * Also, most field masks have a prompt widget, usually a
 * #GtkLabel, labeling the field mask with a name.
 * Optionally, up to three auxillary widgets are supported
 * per field mask, layed out between the prompt and the
 * action widgets.
 * The second auxillary widget will expand if additional
 * space is available. Other layout details are configured
 * through the @gpack packing type:
 @* %BST_GMASK_FIT - the action widget is not expanded,
 @* %BST_GMASK_FILL - the action widget can expand within the action column,
 @* %BST_GMASK_INTERLEAVE - allow the action widget to expand across auxillary
 * columns if it requests that much space,
 @* %BST_GMASK_BIG - force expansion of the action widget across all possible
 * columns up to the prompt,
 @* %BST_GMASK_CENTER - center the action widget within space across all possible
 * columns up to the prompt.
 */
BstGMask*
bst_gmask_form (GtkWidget   *gmask_container,
                GtkWidget   *action,
                BstGMaskPack gpack)
{
  return gmask_form (gmask_container, action, gpack);
}

/**
 * bst_gmask_set_tip
 * @mask:     valid #BstGMask
 * @tip_text: tooltip text
 *
 * Set the tooltip text of this GUI field @mask.
 */
void
bst_gmask_set_tip (BstGMask    *mask,
                   const gchar *tip_text)
{
  GMask *gmask;
  
  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  
  g_free (gmask->tip);
  gmask->tip = g_strdup (tip_text);
}

/**
 * bst_gmask_set_prompt
 * @mask:     valid #BstGMask
 * @widget:   valid #GtkWidget
 *
 * Set the prompt widget of this GUI field @mask.
 */
void
bst_gmask_set_prompt (BstGMask *mask,
                      gpointer  widget)
{
  GMask *gmask;
  
  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  if (gmask->prompt)
    g_object_unref (gmask->prompt);
  gmask->prompt = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

/**
 * bst_gmask_set_aux1
 * @mask:     valid #BstGMask
 * @widget:   valid #GtkWidget
 *
 * Set the first auxillary widget of this GUI field @mask.
 */
void
bst_gmask_set_aux1 (BstGMask *mask,
                    gpointer  widget)
{
  GMask *gmask;
  
  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  if (gmask->aux1)
    g_object_unref (gmask->aux1);
  gmask->aux1 = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

/**
 * bst_gmask_set_aux2
 * @mask:     valid #BstGMask
 * @widget:   valid #GtkWidget
 *
 * Set the second auxillary widget of this GUI field @mask.
 * In contrast to the first and third auxillary widget, this
 * one is expanded if extra space is available.
 */
void
bst_gmask_set_aux2 (BstGMask *mask,
                    gpointer  widget)
{
  GMask *gmask;
  
  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  if (gmask->aux2)
    g_object_unref (gmask->aux2);
  gmask->aux2 = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

/**
 * bst_gmask_set_aux3
 * @mask:     valid #BstGMask
 * @widget:   valid #GtkWidget
 *
 * Set the third auxillary widget of this GUI field @mask.
 */
void
bst_gmask_set_aux3 (BstGMask *mask,
                    gpointer  widget)
{
  GMask *gmask;
  
  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  if (gmask->aux3)
    g_object_unref (gmask->aux3);
  gmask->aux3 = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

/**
 * bst_gmask_set_ahead
 * @mask:     valid #BstGMask
 * @widget:   valid #GtkWidget
 *
 * Set the pre action widget of this GUI field @mask.
 */
void
bst_gmask_set_ahead (BstGMask *mask,
                     gpointer  widget)
{
  GMask *gmask;
  
  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  if (gmask->ahead)
    g_object_unref (gmask->ahead);
  gmask->ahead = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

/**
 * bst_gmask_set_atail
 * @mask:     valid #BstGMask
 * @widget:   valid #GtkWidget
 *
 * Set the post action widget of this GUI field @mask.
 */
void
bst_gmask_set_atail (BstGMask *mask,
                     gpointer  widget)
{
  GMask *gmask;
  
  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  if (gmask->atail)
    g_object_unref (gmask->atail);
  gmask->atail = g_object_ref (widget);
  gtk_object_sink (GTK_OBJECT (widget));
}

/**
 * bst_gmask_set_column
 * @mask:     valid #BstGMask
 * @column:   column number
 *
 * Set the field mask column. By default all field masks are
 * packed into column 0, so that only vertical packing occours.
 */
void
bst_gmask_set_column (BstGMask *mask,
                      guint     column)
{
  GMask *gmask;
  
  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  
  gmask->column = column;
}

/**
 * bst_gmask_get_prompt
 * @mask:     valid #BstGMask
 * @RETURNS:  the requested #GtkWidget or %NULL
 *
 * Retrieve the prompt widget of this GUI field @mask.
 */
GtkWidget*
bst_gmask_get_prompt (BstGMask *mask)
{
  GMask *gmask;
  
  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);
  
  return gmask->prompt;
}

/**
 * bst_gmask_get_aux1
 * @mask:     valid #BstGMask
 * @RETURNS:  the requested #GtkWidget or %NULL
 *
 * Retrieve the first auxillary widget of this GUI field @mask.
 */
GtkWidget*
bst_gmask_get_aux1 (BstGMask *mask)
{
  GMask *gmask;
  
  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);
  
  return gmask->aux1;
}

/**
 * bst_gmask_get_aux2
 * @mask:     valid #BstGMask
 * @RETURNS:  the requested #GtkWidget or %NULL
 *
 * Retrieve the second auxillary widget of this GUI field @mask.
 */
GtkWidget*
bst_gmask_get_aux2 (BstGMask *mask)
{
  GMask *gmask;
  
  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);
  
  return gmask->aux2;
}

/**
 * bst_gmask_get_aux3
 * @mask:     valid #BstGMask
 * @RETURNS:  the requested #GtkWidget or %NULL
 *
 * Retrieve the third auxillary widget of this GUI field @mask.
 */
GtkWidget*
bst_gmask_get_aux3 (BstGMask *mask)
{
  GMask *gmask;
  
  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);
  
  return gmask->aux3;
}

/**
 * bst_gmask_get_ahead
 * @mask:     valid #BstGMask
 * @RETURNS:  the requested #GtkWidget or %NULL
 *
 * Retrieve the pre action widget of this GUI field @mask.
 */
GtkWidget*
bst_gmask_get_ahead (BstGMask *mask)
{
  GMask *gmask;
  
  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);
  
  return gmask->ahead;
}

/**
 * bst_gmask_get_action
 * @mask:     valid #BstGMask
 * @RETURNS:  the requested #GtkWidget or %NULL
 *
 * Retrieve the action widget of this GUI field @mask.
 */
GtkWidget*
bst_gmask_get_action (BstGMask *mask)
{
  GMask *gmask;
  
  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);
  
  return gmask->action;
}

/**
 * bst_gmask_get_atail
 * @mask:     valid #BstGMask
 * @RETURNS:  the requested #GtkWidget or %NULL
 *
 * Retrieve the post action widget of this GUI field @mask.
 */
GtkWidget*
bst_gmask_get_atail (BstGMask *mask)
{
  GMask *gmask;
  
  g_return_val_if_fail (GTK_IS_WIDGET (mask), NULL);
  gmask = GMASK_GET (mask);
  g_return_val_if_fail (gmask != NULL, NULL);
  
  return gmask->atail;
}

/**
 * bst_gmask_foreach
 * @mask:     valid #BstGMask
 * @func:     foreach function as: void func(GtkWidget*, gpointer data);
 * @data:     data passed in to @func
 *
 * Invoke @func() with each of the widgets set for this
 * field mask.
 */
void
bst_gmask_foreach (BstGMask *mask,
                   gpointer  func,
                   gpointer  data)
{
  GMask *gmask;
  GtkCallback callback = func;
  
  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  g_return_if_fail (func != NULL);
  
  if (gmask->prompt)
    callback (gmask->prompt, data);
  if (gmask->aux1)
    callback (gmask->aux1, data);
  if (gmask->aux2)
    callback (gmask->aux2, data);
  if (gmask->aux3)
    callback (gmask->aux3, data);
  if (gmask->ahead)
    callback (gmask->ahead, data);
  if (gmask->atail)
    callback (gmask->atail, data);
  if (gmask->action)
    callback (gmask->action, data);
}

static GtkWidget*
get_toplevel_and_set_tip (GtkWidget   *widget,
                          GtkTooltips *tooltips,
                          const gchar *tip)
{
  GtkWidget *last;
  
  if (!widget)
    return NULL;
  else if (!tooltips || !tip)
    return gtk_widget_get_toplevel (widget);
  do
    {
      if (!GTK_WIDGET_NO_WINDOW (widget))
        {
          gtk_tooltips_set_tip (tooltips, widget, tip, NULL);
          return gtk_widget_get_toplevel (widget);
        }
      last = widget;
      widget = last->parent;
    }
  while (widget);
  /* need to create a tooltips sensitive parent */
  widget = gtk_widget_new (GTK_TYPE_EVENT_BOX,
                           "visible", TRUE,
                           "child", last,
                           NULL);
  gtk_tooltips_set_tip (tooltips, widget, tip, NULL);
  return widget;
}

static guint
table_max_bottom_row (GtkTable *table,
                      guint     min_col,
                      guint     max_col)
{
  guint max_bottom = 0;
  GList *list;
  
  for (list = table->children; list; list = list->next)
    {
      GtkTableChild *child = list->data;
      
      if (child->left_attach >= min_col && child->right_attach <= max_col)
        max_bottom = MAX (max_bottom, child->bottom_attach);
    }
  return max_bottom;
}

/**
 * bst_gmask_pack
 * @mask:     valid #BstGMask
 *
 * After the GUI field mask is fully configured, by setting
 * all associated widgets on it, column tooltip text, etc.,
 * this function actually packs it into its container. The
 * field mask setters shouldn't be used after this point.
 */
void
bst_gmask_pack (BstGMask *mask)
{
  GtkWidget *prompt, *aux1, *aux2, *aux3, *ahead, *action, *atail;
  GtkTable *table;
  gboolean dummy_aux2 = FALSE;
  guint row, n, c, dislodge_columns;
  GMask *gmask;
  
  g_return_if_fail (GTK_IS_WIDGET (mask));
  gmask = GMASK_GET (mask);
  g_return_if_fail (gmask != NULL);
  
  /* GUI mask layout:
   * row: |Prompt|Aux1| Aux2 |Aux3| PreAction#Action#PostAction|
   * FILL: allocate all possible (Pre/Post)Action space to the action widget
   * INTERLEAVE: allow the action widget to facilitate unused Aux2/Aux3 space
   * BIG: allocate maximum (left extendeded) possible space to Action
   * Aux2 expands automatically
   */
  
  /* retrieve children and set tips */
  prompt = get_toplevel_and_set_tip (gmask->prompt, GXK_TOOLTIPS, gmask->tip);
  aux1 = get_toplevel_and_set_tip (gmask->aux1, GXK_TOOLTIPS, gmask->tip);
  aux2 = get_toplevel_and_set_tip (gmask->aux2, GXK_TOOLTIPS, gmask->tip);
  aux3 = get_toplevel_and_set_tip (gmask->aux3, GXK_TOOLTIPS, gmask->tip);
  ahead = get_toplevel_and_set_tip (gmask->ahead, GXK_TOOLTIPS, gmask->tip);
  action = get_toplevel_and_set_tip (gmask->action, GXK_TOOLTIPS, gmask->tip);
  atail = get_toplevel_and_set_tip (gmask->atail, GXK_TOOLTIPS, gmask->tip);
  dislodge_columns = g_object_get_data (G_OBJECT (gmask->parent), "GMask-dislodge") != NULL;
  table = GTK_TABLE (gmask->parent);
  
  /* ensure expansion happens outside of columns */
  if (dislodge_columns)
    {
      gchar *dummy_name = g_strdup_printf ("GMask-dummy-dislodge-%u", MAX (gmask->column, 1) - 1);
      GtkWidget *dislodge = g_object_get_data (G_OBJECT (table), dummy_name);
      
      if (!dislodge)
        {
          dislodge = g_object_new (GTK_TYPE_ALIGNMENT, "visible", TRUE, NULL);
          g_object_set_data_full (G_OBJECT (table), dummy_name, g_object_ref (dislodge), g_object_unref);
          c = MAX (gmask->column, 1) * 6;
          gtk_table_attach (table, dislodge, c - 1, c, 0, 1, GTK_EXPAND, 0, 0, 0);
        }
      g_free (dummy_name);
    }
  
  /* pack gmask children, options: GTK_EXPAND, GTK_SHRINK, GTK_FILL */
  c = 6 * gmask->column;
  row = table_max_bottom_row (table, c, c + 5);
  if (prompt)
    {
      gtk_table_attach (table, prompt, c, c + 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
      gtk_table_set_col_spacing (table, c, 2); /* seperate prompt from rest */
    }
  c++;
  if (aux1)
    gtk_table_attach (table, aux1, c, c + 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  c++;
  if (!aux2 && !dislodge_columns)
    {
      gchar *dummy_name = g_strdup_printf ("GMask-dummy-aux2-%u", gmask->column);
      
      aux2 = g_object_get_data (G_OBJECT (table), dummy_name);
      
      /* need to have at least 1 (dummy) aux2-child per table column to eat up
       * expanding space in this column if !dislodge_columns
       */
      if (!aux2)
        {
          aux2 = gtk_widget_new (GTK_TYPE_ALIGNMENT, "visible", TRUE, NULL);
          g_object_set_data_full (G_OBJECT (table), dummy_name, g_object_ref (aux2), g_object_unref);
        }
      else
        aux2 = NULL;
      g_free (dummy_name);
      dummy_aux2 = TRUE;
    }
  if (aux2)
    {
      gtk_table_attach (table, aux2,
                        c, c + 1,
                        row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
      if (dummy_aux2)
        aux2 = NULL;
    }
  c++;
  if (aux3)
    gtk_table_attach (table, aux3, c, c + 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  c++;
  /* pack action with head and tail widgets closely together */
  if (ahead || atail)
    {
      action = gtk_widget_new (GTK_TYPE_HBOX,
                               "visible", TRUE,
                               "child", action,
                               NULL);
      if (ahead)
        gtk_container_add_with_properties (GTK_CONTAINER (action), ahead,
                                           "position", 0,
                                           "expand", FALSE,
                                           NULL);
      if (atail)
        gtk_box_pack_end (GTK_BOX (action), atail, FALSE, TRUE, 0);
    }
  n = c;
  if (gmask->gpack == BST_GMASK_BIG || gmask->gpack == BST_GMASK_CENTER ||
      gmask->gpack == BST_GMASK_INTERLEAVE)     /* extend action to the left when possible */
    {
      if (!aux3)
        {
          n--;
          if (!aux2)
            {
              n--;
              if (!aux1 && (gmask->gpack == BST_GMASK_BIG ||
                            gmask->gpack == BST_GMASK_CENTER))
                {
                  n--;
                  if (!prompt)
                    n--;
                }
            }
        }
    }
  if (gmask->gpack == BST_GMASK_FIT ||
      gmask->gpack == BST_GMASK_INTERLEAVE) /* align to right without expansion */
    action = gtk_widget_new (GTK_TYPE_ALIGNMENT,
                             "visible", TRUE,
                             "child", action,
                             "xalign", 1.0,
                             "xscale", 0.0,
                             "yscale", 0.0,
                             NULL);
  else if (gmask->gpack == BST_GMASK_CENTER)
    action = gtk_widget_new (GTK_TYPE_ALIGNMENT,
                             "visible", TRUE,
                             "child", action,
                             "xalign", 0.5,
                             "yalign", 0.5,
                             "xscale", 0.0,
                             "yscale", 0.0,
                             NULL);
  gtk_table_attach (table, action,
                    n, c + 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL,
                    GTK_FILL,
                    0, 0);
  gtk_table_set_col_spacing (table, c - 1, 2); /* seperate action from rest */
  c = 6 * gmask->column;
  if (c)
    gtk_table_set_col_spacing (table, c - 1, 5); /* spacing between columns */
}

/**
 * bst_gmask_quick
 * @gmask_container: container created with bst_gmask_container_create()
 * @column:          column number for bst_gmask_set_column()
 * @prompt:          valid #GtkWidget for bst_gmask_set_prompt()
 * @action:          valid #GtkWidget as with bst_gmask_form()
 * @tip_text:        text for bst_gmask_set_tip()
 * @RETURNS:         an already packed GUI field mask
 *
 * Shorthand to form a GUI field mask in @column of type %BST_GMASK_FILL,
 * with @prompt and @tip_text. Note that this function already calls
 * bst_gmask_pack(), so the returned field mask already can't be modified
 * anymore.
 */
BstGMask*
bst_gmask_quick (GtkWidget   *gmask_container,
                 guint        column,
                 const gchar *prompt,
                 gpointer     action,
                 const gchar *tip_text)
{
  gpointer mask = bst_gmask_form (gmask_container, action, BST_GMASK_FILL);
  
  if (prompt)
    bst_gmask_set_prompt (mask, g_object_new (GTK_TYPE_LABEL,
                                              "visible", TRUE,
                                              "label", prompt,
                                              NULL));
  if (tip_text)
    bst_gmask_set_tip (mask, tip_text);
  bst_gmask_set_column (mask, column);
  bst_gmask_pack (mask);
  
  return mask;
}


/* --- named children --- */
static GQuark quark_container_named_children = 0;
typedef struct {
  GData *qdata;
} NChildren;
static void
nchildren_free (gpointer data)
{
  NChildren *children = data;
  
  g_datalist_clear (&children->qdata);
  g_free (children);
}
static void
destroy_nchildren (GtkWidget *container)
{
  g_object_set_qdata (G_OBJECT (container), quark_container_named_children, NULL);
}
void
bst_container_set_named_child (GtkWidget *container,
                               GQuark     qname,
                               GtkWidget *child)
{
  NChildren *children;
  
  g_return_if_fail (GTK_IS_CONTAINER (container));
  g_return_if_fail (qname > 0);
  g_return_if_fail (GTK_IS_WIDGET (child));
  if (child)
    g_return_if_fail (gtk_widget_is_ancestor (child, container));
  
  if (!quark_container_named_children)
    quark_container_named_children = g_quark_from_static_string ("BstContainer-named_children");
  
  children = g_object_get_qdata (G_OBJECT (container), quark_container_named_children);
  if (!children)
    {
      children = g_new (NChildren, 1);
      g_datalist_init (&children->qdata);
      g_object_set_qdata_full (G_OBJECT (container), quark_container_named_children, children, nchildren_free);
      g_object_connect (container,
                        "signal::destroy", destroy_nchildren, NULL,
                        NULL);
    }
  g_object_ref (child);
  g_datalist_id_set_data_full (&children->qdata, qname, child, g_object_unref);
}

GtkWidget*
bst_container_get_named_child (GtkWidget *container,
                               GQuark     qname)
{
  NChildren *children;
  
  g_return_val_if_fail (GTK_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (qname > 0, NULL);
  
  children = quark_container_named_children ? g_object_get_qdata (G_OBJECT (container), quark_container_named_children) : NULL;
  if (children)
    {
      GtkWidget *child = g_datalist_id_get_data (&children->qdata, qname);
      
      if (child && !gtk_widget_is_ancestor (child, container))
        {
          /* got removed meanwhile */
          g_datalist_id_set_data (&children->qdata, qname, NULL);
          child = NULL;
        }
      return child;
    }
  return NULL;
}

GtkWidget*
bst_xpm_view_create (const gchar **xpm,
                     GtkWidget    *colormap_widget)
{
  GtkWidget *pix;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  
  g_return_val_if_fail (xpm != NULL, NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (colormap_widget), NULL);
  
  pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gtk_widget_get_colormap (colormap_widget),
                                                  &mask, NULL, (gchar**) xpm);
  pix = gtk_pixmap_new (pixmap, mask);
  gdk_pixmap_unref (pixmap);
  gdk_pixmap_unref (mask);
  gtk_widget_set (pix,
                  "visible", TRUE,
                  NULL);
  return pix;
}


/* --- file utils --- */
#include <sfi/sfistore.h>

static gboolean
song_name_scanner (SfiRStore      *rstore,
                   gpointer        data)
{
  if (g_scanner_peek_next_token (rstore->scanner) == G_TOKEN_STRING)
    {
      g_scanner_get_next_token (rstore->scanner);
      if (strncmp (rstore->scanner->value.v_string, "BseSong::", 9) == 0)
        {
          gchar **rval = data;
          *rval = g_strdup (rstore->scanner->value.v_string + 9);
          return FALSE;
        }
    }
  return TRUE;
}

gchar*
bst_file_scan_song_name (const gchar *file)
{
  SfiRStore *rstore;
  
  g_return_val_if_fail (file != NULL, NULL);
  
  rstore = sfi_rstore_new_open (file);
  if (rstore)
    {
      gchar *name = NULL;
      sfi_rstore_quick_scan (rstore, "container-child", song_name_scanner, &name);
      sfi_rstore_destroy (rstore);
      return name;
    }
  else
    return NULL;
}


/* --- source file key scans --- */
#include "bstdebugkeys.defs"
#ifndef BST_DEBUG_KEYS
#  define BST_DEBUG_KEYS        /* none */
#endif
static const gchar *debug_keys[] = { BST_LOG_SCAN_KEYS NULL };
const gchar**
_bst_log_scan_keys (void)
{
  return debug_keys;
}


/* --- generated marshallers --- */
#include "bstmarshal.c"


/* --- IDL pspecs --- */
#define sfidl_pspec_Int(group, name, nick, blurb, dflt, min, max, step, hints)  \
  sfi_pspec_set_group (sfi_pspec_int (name, nick, blurb, dflt, min, max, step, hints), group)
#define sfidl_pspec_Int_default(group, name)                                            \
  sfi_pspec_set_group (sfi_pspec_int (name, NULL, NULL, 0, G_MININT, G_MAXINT, 256, SFI_PARAM_DEFAULT), group)
#define sfidl_pspec_UInt(group, name, nick, blurb, dflt, hints) \
  sfi_pspec_set_group (sfi_pspec_int (name, nick, blurb, dflt, 0, G_MAXINT, 1, hints), group)
#define sfidl_pspec_Real(group, name, nick, blurb, dflt, min, max, step, hints) \
  sfi_pspec_set_group (sfi_pspec_real (name, nick, blurb, dflt, min, max, step, hints), group)
#define sfidl_pspec_Real_default(group, name)                                           \
  sfi_pspec_set_group (sfi_pspec_real (name, NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 10, SFI_PARAM_DEFAULT), group)
#define sfidl_pspec_Bool(group, name, nick, blurb, dflt, hints)                 \
  sfi_pspec_set_group (sfi_pspec_bool (name, nick, blurb, dflt, hints), group)
#define sfidl_pspec_Bool_default(group, name)                                           \
  sfi_pspec_set_group (sfi_pspec_bool (name, NULL, NULL, FALSE, SFI_PARAM_DEFAULT), group)
#define sfidl_pspec_Note(group, name, nick, blurb, dflt, hints)                 \
  sfi_pspec_set_group (sfi_pspec_note (name, nick, blurb, dflt, hints), group)
#define sfidl_pspec_String(group, name, nick, blurb, dflt, hints)                       \
  sfi_pspec_set_group (sfi_pspec_string (name, nick, blurb, dflt, hints), group)
#define sfidl_pspec_String_default(group, name)                                 \
  sfi_pspec_set_group (sfi_pspec_string (name, NULL, NULL, NULL, SFI_PARAM_DEFAULT), group)
#define sfidl_pspec_Proxy_default(group, name)                                          \
  sfi_pspec_set_group (sfi_pspec_proxy (name, NULL, NULL, SFI_PARAM_DEFAULT), group)
#define sfidl_pspec_Seq(group, name, nick, blurb, hints, element_pspec)         \
  sfi_pspec_set_group (sfi_pspec_seq (name, nick, blurb, element_pspec, hints), group)
#define sfidl_pspec_Rec(group, name, nick, blurb, hints, fields)                        \
  sfi_pspec_set_group (sfi_pspec_rec (name, nick, blurb, fields, hints), group)
#define sfidl_pspec_Rec_default(group, name, fields)                                    \
  sfi_pspec_set_group (sfi_pspec_rec (name, NULL, NULL, fields, SFI_PARAM_DEFAULT), group)
#define sfidl_pspec_BBlock(group, name, nick, blurb, hints)                             \
  sfi_pspec_set_group (sfi_pspec_bblock (name, nick, blurb, hints), group)
/* --- generated type IDs and SFIDL types --- */
#include "bstgentypes.c"        /* type id defs */
