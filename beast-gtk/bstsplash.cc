/* BEAST - Better Audio System
 * Copyright (C) 2002-2003 Tim Janik
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
#include "bstsplash.h"
#include <gdk/gdkkeysyms.h>
#include "topconfig.h"
#include <string.h>


/* --- prototypes --- */
static void	bst_splash_class_init		(BstSplashClass	  *klass);
static void	bst_splash_init			(BstSplash	  *splash);
static void	bst_splash_finalize		(GObject	  *object);
static gint     bst_splash_button_press         (GtkWidget        *widget,
                                                 GdkEventButton   *event);
static gboolean bst_splash_key_press_event      (GtkWidget        *widget,
                                                 GdkEventKey      *event);
static void     bst_splash_hide                 (GtkWidget        *widget);
static void	bst_splash_show			(GtkWidget	  *widget);
static void	bst_splash_unrealize		(GtkWidget	  *widget);
static gboolean bst_splash_delete_event		(GtkWidget	  *widget,
						 GdkEventAny	  *event);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
GtkType
bst_splash_get_type (void)
{
  static GtkType splash_type = 0;
  
  if (!splash_type)
    {
      GtkTypeInfo splash_info =
      {
	(char*) "BstSplash",
	sizeof (BstSplash),
	sizeof (BstSplashClass),
	(GtkClassInitFunc) bst_splash_class_init,
	(GtkObjectInitFunc) bst_splash_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      splash_type = gtk_type_unique (GTK_TYPE_WINDOW, &splash_info);
    }
  
  return splash_type;
}

static void
bst_splash_class_init (BstSplashClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  gobject_class->finalize = bst_splash_finalize;
  
  widget_class->delete_event = bst_splash_delete_event;
  widget_class->key_press_event = bst_splash_key_press_event;
  widget_class->button_press_event = bst_splash_button_press;
  widget_class->hide = bst_splash_hide;
  widget_class->show = bst_splash_show;
  widget_class->unrealize = bst_splash_unrealize;
}

static void
bst_splash_init (BstSplash *self)
{
  GtkWindow *window = GTK_WINDOW (self);
  GtkWidget *any;

  /* window setup */
  g_object_set (window,
		"window_position", GTK_WIN_POS_CENTER,
                "events", GDK_BUTTON_PRESS_MASK,
		NULL);

  /* main vbox */
  self->vbox = (GtkWidget*) g_object_new (GTK_TYPE_VBOX,
                                          "visible", TRUE,
                                          "spacing", 0,
                                          "parent", window,
                                          NULL);
  gxk_nullify_in_object (self, &self->vbox);
  
  /* splash vbox */
  self->splash_box = (GtkWidget*) g_object_new (GTK_TYPE_VBOX,
                                                "visible", TRUE,
                                                "spacing", 0,
                                                NULL);
  gxk_nullify_in_object (self, &self->splash_box);
  gtk_box_pack_start (GTK_BOX (self->vbox), self->splash_box, TRUE, TRUE, 0);

  /* progress bar */
  self->pbar = (GtkProgressBar*) g_object_new (GTK_TYPE_PROGRESS_BAR,
                                               "visible", TRUE,
                                               NULL);
  gxk_nullify_in_object (self, &self->pbar);
  gtk_box_pack_end (GTK_BOX (self->vbox), GTK_WIDGET (self->pbar), FALSE, TRUE, 0);
  gtk_progress_set_show_text (GTK_PROGRESS (self->pbar), FALSE);

  /* item label */
  any = (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT,
                                   "visible", TRUE,
                                   "xscale", 1.0,
                                   NULL);
  gtk_box_pack_end (GTK_BOX (self->vbox), any, FALSE, TRUE, 3);
  self->item = (GtkWidget*) g_object_new (GTK_TYPE_LABEL,
                                          "visible", TRUE,
                                          "label", "Item",           /* untranslated template name */
                                          "width_request", 1,
                                          "parent", any,
                                          NULL);
  gxk_nullify_in_object (self, &self->item);

  /* entity label */
  any = (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT,
                                   "visible", TRUE,
                                   "xscale", 1.0,
                                   NULL);
  gtk_box_pack_end (GTK_BOX (self->vbox), any, FALSE, TRUE, 5);
  self->entity = (GtkWidget*) g_object_new (GTK_TYPE_LABEL,
                                            "visible", TRUE,
                                            "label", "Entity",       /* untranslated template name */
                                            "width_request", 1,
                                            "parent", any,
                                            NULL);
  gxk_nullify_in_object (self, &self->entity);
}

static void
bst_splash_finalize (GObject *object)
{
  // BstSplash *splash = BST_SPLASH (object);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
bst_splash_delete_event (GtkWidget   *widget,
			 GdkEventAny *event)
{
  gtk_widget_hide (widget);

  return TRUE;
}

static gboolean
bst_splash_key_press_event (GtkWidget        *widget,
                            GdkEventKey      *event)
{
  if (event->keyval == GDK_Escape)
    {
      /* trigger delete event */
      gxk_toplevel_delete (widget);
      return TRUE;
    }
  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
}

static gint
bst_splash_button_press (GtkWidget        *widget,
                         GdkEventButton   *event)
{
  gtk_widget_hide (widget);

  return TRUE;
}

static void
bst_splash_hide (GtkWidget *widget)
{
  BstSplash *self = BST_SPLASH (widget);
  gtk_progress_bar_set_text (self->pbar, NULL);
  gtk_progress_bar_set_orientation (self->pbar, GTK_PROGRESS_LEFT_TO_RIGHT);
  gtk_progress_bar_set_fraction (self->pbar, 0);
  if (self->timer_id)
    {
      g_source_remove (self->timer_id);
      self->timer_id = 0;
    }
  GTK_WIDGET_CLASS (parent_class)->hide (widget);
}

static void
bst_splash_show (GtkWidget *widget)
{
  BstSplash *self = BST_SPLASH (widget);

  self->item_count = 0;
  GTK_WIDGET_CLASS (parent_class)->show (widget);
}

static void
bst_splash_unrealize (GtkWidget *widget)
{
  BstSplash *self = BST_SPLASH (widget);

  if (BST_DBG_EXT) /* && self->item_count > self->max_items */
    g_message ("BstSplash: seen %u/%u items (%+d)",
	       self->item_count, self->max_items,
	       self->item_count - self->max_items);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

void
bst_splash_set_title (GtkWidget      *widget,
                      const gchar    *title)
{
  gtk_window_set_title (GTK_WINDOW (widget), title);
}

GtkWidget*
bst_splash_new (const gchar *role,
		guint        splash_width,
		guint        splash_height,
		guint        max_items)
{
  GtkWidget *splash = (GtkWidget*) g_object_new (BST_TYPE_SPLASH, NULL);
  
  /* set title and role */
  gtk_window_set_role (GTK_WINDOW (splash), role);
  gtk_window_set_title (GTK_WINDOW (splash), role);
  BST_SPLASH (splash)->max_items = max_items;
  g_object_set (BST_SPLASH (splash)->splash_box,
		"width_request", splash_width,
		"height_request", splash_height,
		NULL);

  return splash;
}

void
bst_splash_show_grab (GtkWidget *widget)
{
  BstSplash *self = BST_SPLASH (widget);

  if (!GTK_WIDGET_VISIBLE (widget))
    {
      gtk_widget_show (widget);
      if (!self->has_grab)
        {
          self->has_grab = TRUE;
          gtk_grab_add (widget);
        }

      GDK_THREADS_LEAVE ();
      while (!GTK_WIDGET_MAPPED (widget))
	g_main_iteration (TRUE);
      while (g_main_pending ())
	g_main_iteration (FALSE);
      GDK_THREADS_ENTER ();
    }
}

void
bst_splash_release_grab (GtkWidget      *widget)
{
  BstSplash *self = BST_SPLASH (widget);
  if (self->has_grab)
    {
      self->has_grab = FALSE;
      gtk_grab_remove (widget);
    }
}

void
bst_splash_update_entity (GtkWidget   *widget,
			  const gchar *format,
			  ...)
{
  BstSplash *self;
  va_list args;
  gchar *text;
  
  g_return_if_fail (BST_IS_SPLASH (widget));
  
  self = BST_SPLASH (widget);
  va_start (args, format);
  text = g_strdup_vprintf (format, args);
  va_end (args);

  gtk_label_set_text (GTK_LABEL (self->entity), text);
  gtk_label_set_text (GTK_LABEL (self->item), NULL);
  g_free (text);
}

void
bst_splash_update_item (GtkWidget   *widget,
			const gchar *format,
			...)
{
  BstSplash *self;
  va_list args;
  gchar *text;
  
  g_return_if_fail (BST_IS_SPLASH (widget));
  
  self = BST_SPLASH (widget);
  va_start (args, format);
  text = g_strdup_vprintf (format, args);
  va_end (args);

  gtk_label_set_text (GTK_LABEL (self->item), text);
  g_free (text);

  if (GTK_WIDGET_VISIBLE (self))
    {
      gfloat frac = self->item_count++;
      frac /= self->max_items;
      gtk_progress_bar_set_fraction (self->pbar, MIN (frac, 1.0));
      bst_splash_update ();
      if (0)
	g_usleep (1000 * 250);
    }
}

void
bst_splash_update (void)
{
  GDK_THREADS_LEAVE ();
  while (g_main_pending ())
    g_main_iteration (FALSE);
  GDK_THREADS_ENTER ();
}

void
bst_splash_set_text (GtkWidget   *widget,
		     const gchar *format,
		     ...)
{
  BstSplash *self;
  va_list args;
  gchar *text;
  
  g_return_if_fail (BST_IS_SPLASH (widget));
  
  va_start (args, format);
  text = g_strdup_vprintf (format, args);
  va_end (args);
  
  self = BST_SPLASH (widget);
  gtk_container_foreach (GTK_CONTAINER (self->splash_box), (GtkCallback) gtk_widget_destroy, NULL);
  if (text)
    {
      gchar *str = text;
      while (str)
	{
	  GtkWidget *label;
	  gchar *p = strchr (str, '\n');
	  if (p)
	    *p++ = 0;
	  label = (GtkWidget*) g_object_new (GTK_TYPE_LABEL,
                                             "visible", TRUE,
                                             "use_markup", TRUE,
                                             "label", str,
                                             NULL);
	  gtk_box_pack_start (GTK_BOX (self->splash_box), label, TRUE, TRUE, 0);
	  str = p;
	}
    }
  g_free (text);
  if (GTK_WIDGET_VISIBLE (self))
    bst_splash_update ();
}

void
bst_splash_set_animation (GtkWidget          *widget,
			  GdkPixbufAnimation *anim)
{
  BstSplash *self;

  g_return_if_fail (BST_IS_SPLASH (widget));

  self = BST_SPLASH (widget);
  gtk_container_foreach (GTK_CONTAINER (self->splash_box), (GtkCallback) gtk_widget_destroy, NULL);
  if (anim)
    {
      GtkWidget *image = (GtkWidget*) g_object_new (GTK_TYPE_IMAGE,
				       "visible", TRUE,
                                       "parent",
                                       g_object_new (GTK_TYPE_FRAME,
                                                     "visible", TRUE,
                                                     "shadow-type", GTK_SHADOW_IN,
                                                     "parent", g_object_new (GTK_TYPE_ALIGNMENT,
                                                                             "visible", TRUE,
                                                                             "xscale", 0.0,
                                                                             "yscale", 0.0,
                                                                             "border-width", 10,
                                                                             NULL),
                                                     NULL),
				       NULL);
      gtk_box_pack_start (GTK_BOX (self->splash_box), gtk_widget_get_toplevel (image), TRUE, TRUE, 0);
      if (gdk_pixbuf_animation_is_static_image (anim))
	gtk_image_set_from_pixbuf (GTK_IMAGE (image), gdk_pixbuf_animation_get_static_image (anim));
      else
	gtk_image_set_from_animation (GTK_IMAGE (image), anim);
    }
  if (GTK_WIDGET_VISIBLE (self))
    bst_splash_update ();
}

static const gchar*
splash_select_string (BstSplash *self)
{
  guint nth = rand () % self->n_rand_strings;
  gchar *string = self->strings[nth];
  self->n_rand_strings -= 1;
  if (self->n_rand_strings < 1)
    self->n_rand_strings = self->n_strings;
  else
    {
      self->strings[nth] = self->strings[self->n_rand_strings];
      self->strings[self->n_rand_strings] = string;
    }
  return string;
}

static gboolean
about_timer (gpointer data)
{
  BstSplash *self = (BstSplash*) data;
  float delta = 0.02;
  GDK_THREADS_ENTER ();
  self->aprogress += delta;
  if (self->aprogress >= 2)
    {
      gtk_progress_bar_set_text (self->pbar, splash_select_string (self));
      self->aprogress = 0;
    }
  gtk_progress_bar_set_orientation (self->pbar, self->aprogress < 1 ? GTK_PROGRESS_LEFT_TO_RIGHT : GTK_PROGRESS_RIGHT_TO_LEFT);
  gtk_progress_bar_set_fraction (self->pbar, self->aprogress < 1 ? self->aprogress : 2 - self->aprogress);
  GDK_THREADS_LEAVE ();
  return TRUE;
}

void
bst_splash_animate_strings (GtkWidget      *splash,
                            const gchar   **strings)
{
  BstSplash *self = BST_SPLASH (splash);
  g_strfreev (self->strings);
  self->strings = g_strdupv ((gchar**) strings);
  self->n_strings = 0;
  while (self->strings[self->n_strings])
    self->n_strings++;
  self->n_rand_strings = self->n_strings;
  if (!self->timer_id)
    self->timer_id = g_timeout_add (40, about_timer, self);
  self->aprogress = 0;
  gtk_progress_bar_set_orientation (self->pbar, GTK_PROGRESS_LEFT_TO_RIGHT);
  gtk_progress_bar_set_fraction (self->pbar, self->aprogress);
  gtk_progress_bar_set_text (self->pbar, splash_select_string (self));
}
