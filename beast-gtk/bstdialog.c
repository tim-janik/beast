/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bstdialog.h"

#include	"bststatusbar.h"
#include	<string.h>

#define	DEFAULT_TITLE	"Auxillary Dialog"


/* --- properties --- */
enum {
  PROP_0,
  PROP_POINTER,
  PROP_ALIVE_OBJECT,
  PROP_FLAGS,
  PROP_TITLE
};


/* --- prototypes --- */
static void	bst_dialog_class_init		(BstDialogClass	  *class);
static void	bst_dialog_init			(BstDialog	  *dialog);
static void	bst_dialog_destroy		(GtkObject	  *object);
static void	bst_dialog_finalize		(GObject	  *object);
static void	bst_dialog_show			(GtkWidget	  *widget);
static void	bst_dialog_hide			(GtkWidget	  *widget);
static gboolean bst_dialog_delete_event		(GtkWidget	  *widget,
						 GdkEventAny	  *event);
static gboolean	bst_dialog_enter_notify_event	(GtkWidget	  *widget,
						 GdkEventCrossing *event);
static void	bst_dialog_set_property		(GObject	  *object,
						 guint		   prop_id,
						 const GValue	  *value,
						 GParamSpec	  *pspec);
static void	bst_dialog_get_property		(GObject	  *object,
						 guint		   prop_id,
						 GValue		  *value,
						 GParamSpec	  *pspec);


/* --- variables --- */
static gpointer		 parent_class = NULL;
static GSList		*enter_stack = NULL;


/* --- functions --- */
GtkType
bst_dialog_get_type (void)
{
  static GtkType dialog_type = 0;

  if (!dialog_type)
    {
      GtkTypeInfo dialog_info =
      {
	"BstDialog",
	sizeof (BstDialog),
	sizeof (BstDialogClass),
	(GtkClassInitFunc) bst_dialog_class_init,
	(GtkObjectInitFunc) bst_dialog_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      dialog_type = gtk_type_unique (GTK_TYPE_WINDOW, &dialog_info);
    }

  return dialog_type;
}

static void
bst_dialog_class_init (BstDialogClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = bst_dialog_finalize;
  gobject_class->set_property = bst_dialog_set_property;
  gobject_class->get_property = bst_dialog_get_property;
  
  object_class->destroy = bst_dialog_destroy;

  widget_class->show = bst_dialog_show;
  widget_class->hide = bst_dialog_hide;
  widget_class->delete_event = bst_dialog_delete_event;
  widget_class->enter_notify_event = bst_dialog_enter_notify_event;

  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_POINTER,
				   g_param_spec_pointer ("pointer", NULL, NULL,
							 G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_ALIVE_OBJECT,
				   g_param_spec_object ("alive_object", NULL, NULL,
							GTK_TYPE_OBJECT,
							G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_FLAGS,
				   g_param_spec_flags ("flags", NULL, NULL,
						       BST_TYPE_DIALOG_FLAGS, 0,
						       G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_TITLE,
				   g_param_spec_string ("title", NULL, NULL,
							NULL, G_PARAM_READWRITE));
}

static void
bst_dialog_init (BstDialog *dialog)
{
  GtkWindow *window = GTK_WINDOW (dialog);

  dialog->proxy = 0;
  dialog->title1 = NULL;
  dialog->title2 = NULL;
  dialog->flags = 0;
  dialog->pointer_loc = NULL;
  dialog->alive_object = NULL;
  bst_dialog_set_title (dialog, DEFAULT_TITLE);

  /* main box */
  dialog->mbox = g_object_new (GTK_TYPE_VBOX,
			       "visible", TRUE,
			       "homogeneous", FALSE,
			       "spacing", 0,
			       "parent", window,
			       NULL);
  g_signal_connect_swapped (dialog->mbox, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->mbox);

  /* user vbox */
  dialog->vbox = g_object_new (GTK_TYPE_VBOX,
			       "visible", TRUE,
			       "homogeneous", FALSE,
			       "spacing", 0,
			       "parent", dialog->mbox,
			       NULL);
  g_signal_connect_swapped (dialog->vbox, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->vbox);

  /* status bar */
  dialog->status_bar = bst_status_bar_create ();
  g_signal_connect_swapped (dialog->status_bar, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->status_bar);
  gtk_box_pack_end (GTK_BOX (dialog->mbox), dialog->status_bar, FALSE, FALSE, 0);

  /* button box */
  dialog->hbox = g_object_new (GTK_TYPE_HBOX,
			       "visible", FALSE,
			       "homogeneous", TRUE,
			       "spacing", BST_INNER_PADDING,
			       "border_width", BST_INNER_PADDING,
			       NULL);
  g_signal_connect_swapped (dialog->hbox, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->hbox);
  gtk_box_pack_end (GTK_BOX (dialog->mbox), dialog->hbox, FALSE, TRUE, BST_INNER_PADDING);

  /* separator */
  dialog->sep = g_object_new (GTK_TYPE_HSEPARATOR,
			      "visible", FALSE,
			      NULL);
  g_signal_connect_swapped (dialog->sep, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->sep);
  gtk_box_pack_end (GTK_BOX (dialog->mbox), dialog->sep, FALSE, FALSE, BST_INNER_PADDING);
}

static void
bst_dialog_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  BstDialog *dialog = BST_DIALOG (object);

  switch (prop_id)
    {
      gchar *string;
      guint old_flags;
    case PROP_ALIVE_OBJECT:
      if (dialog->alive_object)
	g_signal_handlers_disconnect_by_func (dialog->alive_object, gtk_widget_destroy, dialog);
      dialog->alive_object = g_value_get_object (value);
      if (dialog->alive_object && G_OBJECT (dialog->alive_object)->ref_count)
	g_signal_connect_swapped (dialog->alive_object, "destroy", G_CALLBACK (gtk_widget_destroy), dialog);
      else
	dialog->alive_object = NULL;
      break;
    case PROP_POINTER:
      dialog->pointer_loc = g_value_get_pointer (value);
      break;
    case PROP_TITLE:
      string = g_value_get_string (value);
      if (!string)
	string = "";
      if (!GTK_WIDGET_VISIBLE (dialog))
	{
	  if (GTK_WIDGET_REALIZED (dialog))
	    gtk_widget_unrealize (GTK_WIDGET (dialog));
	  gtk_window_set_role (GTK_WINDOW (dialog), string);
	}
      string = g_strconcat (string, ": BEAST", NULL);
      g_object_set (dialog, "GtkWindow::title", string, NULL);
      g_free (string);
      break;
    case PROP_FLAGS:
      old_flags = dialog->flags;
      dialog->flags = g_value_get_flags (value);
      if (dialog->status_bar)
	{
	  if (dialog->flags & BST_DIALOG_STATUS)
	    gtk_widget_show (dialog->status_bar);
	  else
	    gtk_widget_hide (dialog->status_bar);
	}
      gtk_window_set_modal (GTK_WINDOW (dialog), dialog->flags & BST_DIALOG_MODAL);
      /* this is a bit hackish, we feature flags that can't be unset */
      if (!(old_flags & BST_DIALOG_DELETE_BUTTON) &&
	  (dialog->flags & BST_DIALOG_DELETE_BUTTON))
	{
	  /* we synthesize a delete event instead of hiding/destroying
	   * directly, because derived classes may override delete_event
	   */
	  bst_dialog_default_action (dialog, BST_STOCK_CLOSE, gtk_toplevel_delete, NULL);
	}
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_dialog_get_property (GObject     *object,
			 guint        prop_id,
			 GValue      *value,
			 GParamSpec  *pspec)
{
  BstDialog *dialog = BST_DIALOG (object);

  switch (prop_id)
    {
    case PROP_FLAGS:
      g_value_set_flags (value, dialog->flags);
      break;
    case PROP_ALIVE_OBJECT:
      g_value_set_object (value, dialog->alive_object);
      break;
    case PROP_POINTER:
      g_value_set_pointer (value, dialog->pointer_loc);
      break;
    case PROP_TITLE:
      g_value_set_string (value, GTK_WINDOW (dialog)->title);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_dialog_destroy (GtkObject *object)
{
  BstDialog *dialog = BST_DIALOG (object);

  bst_dialog_sync_title_to_proxy (dialog, 0, NULL);

  enter_stack = g_slist_remove (enter_stack, dialog);

  if (dialog->pointer_loc)
    *dialog->pointer_loc = NULL;

  g_object_set (dialog,
		"alive_object", NULL,
		"pointer", NULL,
		NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_dialog_finalize (GObject *object)
{
  BstDialog *dialog = BST_DIALOG (object);

  bst_dialog_sync_title_to_proxy (dialog, 0, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget*
bst_dialog_new (gpointer       pointer_loc,
		GtkObject     *alive_object,
		BstDialogFlags flags,
		const gchar   *title,
		GtkWidget     *child)
{
  GtkWidget *dialog;

  dialog = g_object_new (BST_TYPE_DIALOG,
			 "pointer", pointer_loc,
			 "alive_object", alive_object,
			 "flags", flags,
			 "title", title ? title : DEFAULT_TITLE,
			 NULL);
  bst_dialog_set_title (BST_DIALOG (dialog), title);
  if (child)
    gtk_container_add (GTK_CONTAINER (BST_DIALOG (dialog)->vbox), child);

  return dialog;
}

void
bst_dialog_set_title (BstDialog   *dialog,
		      const gchar *title)
{
  g_return_if_fail (BST_IS_DIALOG (dialog));

  g_object_set (dialog, "title", title, NULL);
}

GtkWidget*
bst_dialog_get_child (BstDialog *dialog)
{
  GtkBoxChild *child;
  GtkBox *box;

  g_return_val_if_fail (BST_IS_DIALOG (dialog), NULL);

  /* return the single child that was passed to bst_dialog_new() if any */
  box = dialog->vbox ? GTK_BOX (dialog->vbox) : NULL;
  child = box && box->children ? box->children->data : NULL;

  return child ? child->widget : NULL;
}

static void
bst_dialog_show (GtkWidget *widget)
{
  BstDialog *dialog = BST_DIALOG (widget);
  
  if (dialog->flags & BST_DIALOG_POPUP_POS)
    g_object_set (dialog, "window_position", GTK_WIN_POS_MOUSE, NULL);
  else
    g_object_set (dialog, "window_position", GTK_WIN_POS_NONE, NULL);
  
  gtk_window_set_focus (GTK_WINDOW (dialog), NULL);
  if (dialog->default_widget)
    gtk_widget_grab_default (dialog->default_widget);

  GTK_WIDGET_CLASS (parent_class)->show (widget);
}

static void
bst_dialog_hide (GtkWidget *widget)
{
  // BstDialog *dialog = BST_DIALOG (widget);
  
  GTK_WIDGET_CLASS (parent_class)->hide (widget);
}

static gboolean
bst_dialog_delete_event (GtkWidget   *widget,
			 GdkEventAny *event)
{
  BstDialog *dialog = BST_DIALOG (widget);

  if (dialog->flags & BST_DIALOG_HIDE_ON_DELETE)
    gtk_widget_hide (GTK_WIDGET (dialog));

  if (dialog->flags & BST_DIALOG_HIDE_ON_DELETE)
    return TRUE;
  else
    return FALSE;
}

static gboolean
bst_dialog_enter_notify_event (GtkWidget        *widget,
			       GdkEventCrossing *event)
{
  BstDialog *dialog = BST_DIALOG (widget);
  GtkWidget *event_widget = gtk_get_event_widget ((GdkEvent*) event);

  if (event_widget == widget && event->detail != GDK_NOTIFY_INFERIOR)
    {
      enter_stack = g_slist_remove (enter_stack, dialog);
      enter_stack = g_slist_prepend (enter_stack, dialog);
    }
  return FALSE;
}

BstDialog*
bst_dialog_get_status_window (void)
{
  GSList *slist;

  for (slist = enter_stack; slist; slist = slist->next)
    {
      BstDialog *dialog = BST_DIALOG (slist->data);

      if (dialog->status_bar && GTK_WIDGET_DRAWABLE (dialog->status_bar))
	return dialog;
    }
  return NULL;
}

static void
sync_title (BstDialog *dialog)
{
  gchar *s, *name = bsw_item_get_name (dialog->proxy);

  s = g_strconcat (dialog->title1, name ? name : "<NULL>", dialog->title2, NULL);
  g_object_set (dialog, "title", s, NULL);
  g_free (s);
}

void
bst_dialog_sync_title_to_proxy (BstDialog   *dialog,
				BswProxy     proxy,
				const gchar *title_format)
{
  g_return_if_fail (BST_IS_DIALOG (dialog));
  if (proxy)
    {
      g_return_if_fail (BSW_IS_ITEM (proxy));
      g_return_if_fail (title_format != NULL);
      g_return_if_fail (strstr (title_format, "%s") != NULL);
    }

  if (dialog->proxy)
    {
      bsw_proxy_disconnect (dialog->proxy,
			    "any_signal", sync_title, dialog,
			    NULL);
      g_free (dialog->title1);
      g_free (dialog->title2);
      if (!proxy)
	g_object_set (dialog, "title", DEFAULT_TITLE, NULL);
    }

  dialog->proxy = proxy;
  dialog->title1 = NULL;
  dialog->title2 = NULL;

  if (dialog->proxy)
    {
      gchar *p = strstr (title_format, "%s");

      if (p)	/* asserted above */
	{
	  bsw_proxy_connect (dialog->proxy,
			     "swapped_signal::notify::name", sync_title, dialog,
			     NULL);
	  dialog->title1 = g_strndup (title_format, p - title_format);
	  dialog->title2 = g_strdup (p + 2);
	  sync_title (dialog);
	}
    }
}

GtkWidget*
bst_dialog_action_multi (BstDialog          *dialog,
			 const gchar        *action,
			 gpointer            callback,
			 gpointer            data,
			 const gchar        *icon_stock_id,
			 BstDialogMultiFlags multi_mode)
{
  GtkWidget *button = NULL;

  g_return_val_if_fail (BST_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (action != NULL, NULL);

  if (dialog->sep)
    gtk_widget_show (dialog->sep);
  if (dialog->hbox)
    {
      GtkWidget *alignment, *hbox, *image = icon_stock_id ? bst_image_from_stock (icon_stock_id, BST_SIZE_BUTTON) : NULL;

      if (!image)
	image = bst_image_from_stock (action, BST_SIZE_BUTTON);

      /* setup button */
      button = g_object_new (GTK_TYPE_BUTTON,
			     "can_default", TRUE,
			     "parent", dialog->hbox,
			     NULL);
      if (callback)
	g_signal_connect_data (button, "clicked",
			       callback, data, NULL,
			       (multi_mode & BST_DIALOG_MULTI_SWAPPED) ? G_CONNECT_SWAPPED : 0);

      /* setup button contents */
      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_container_add (GTK_CONTAINER (button), alignment);
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_container_add (GTK_CONTAINER (alignment), hbox);
      if (image)
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (hbox),
			  g_object_new (GTK_TYPE_LABEL,
					"label", bst_stock_action (action),
					"use_underline", TRUE,
					NULL),
			  FALSE, TRUE, 0);
      gtk_widget_show_all (button);

      gtk_widget_show (dialog->hbox);

      if (multi_mode & BST_DIALOG_MULTI_DEFAULT)
	{
	  if (dialog->default_widget)
	    g_signal_handlers_disconnect_by_func (dialog->default_widget, g_nullify_pointer, &dialog->default_widget);
	  dialog->default_widget = button;
	  g_signal_connect_swapped (dialog->default_widget, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->default_widget);
	}
    }
  return button;
}
