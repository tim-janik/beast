/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000, 2001 Tim Janik and Red Hat, Inc.
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
#include "bstradiotools.h"

#include "bsttexttools.h"
#include <ctype.h>


/* --- signals --- */
enum {
  SIGNAL_SET_TOOL,
  SIGNAL_LAST
};

struct _BstRadioToolEntry
{
  guint             tool_id;
  BstRadioToolFlags flags;
  gchar            *name;
  gchar            *tip;
  gchar            *blurb;
  BswIcon          *icon;
  gchar            *stock_icon;
};


/* --- prototypes --- */
static void	  bst_radio_tools_class_init		(BstRadioToolsClass	*klass);
static void	  bst_radio_tools_init			(BstRadioTools		*rtools,
							 BstRadioToolsClass     *class);
static void	  bst_radio_tools_destroy		(GtkObject		*object);
static void	  bst_radio_tools_do_set_tool		(BstRadioTools		*rtools,
							 guint         		 tool_id);


/* --- static variables --- */
static gpointer            parent_class = NULL;
static guint               radio_tools_signals[SIGNAL_LAST] = { 0 };


/* --- functions --- */
GtkType
bst_radio_tools_get_type (void)
{
  static GtkType radio_tools_type = 0;
  
  if (!radio_tools_type)
    {
      GtkTypeInfo radio_tools_info =
      {
	"BstRadioTools",
	sizeof (BstRadioTools),
	sizeof (BstRadioToolsClass),
	(GtkClassInitFunc) bst_radio_tools_class_init,
	(GtkObjectInitFunc) bst_radio_tools_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      radio_tools_type = gtk_type_unique (GTK_TYPE_OBJECT, &radio_tools_info);
    }
  
  return radio_tools_type;
}

static void
bst_radio_tools_class_init (BstRadioToolsClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->destroy = bst_radio_tools_destroy;
  
  class->set_tool = bst_radio_tools_do_set_tool;

  radio_tools_signals[SIGNAL_SET_TOOL] =
    gtk_signal_new ("set_tool",
		    GTK_RUN_LAST | GTK_RUN_NO_RECURSE,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (BstRadioToolsClass, set_tool),
		    bst_marshal_NONE__UINT,
		    GTK_TYPE_NONE,
		    1, GTK_TYPE_UINT);
}

static void
bst_radio_tools_init (BstRadioTools      *rtools,
		      BstRadioToolsClass *class)
{
  // GtkObject *object = GTK_OBJECT (rtools);

  rtools->block_tool_id = FALSE;
  rtools->tool_id = 0;
  rtools->n_tools = 0;
  rtools->tools = NULL;
  rtools->widgets = NULL;
}

static void
bst_radio_tools_destroy (GtkObject *object)
{
  BstRadioTools *rtools = BST_RADIO_TOOLS (object);

  bst_radio_tools_clear_tools (rtools);

  while (rtools->widgets)
    gtk_widget_destroy (rtools->widgets->data);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

BstRadioTools*
bst_radio_tools_new (void)
{
  GtkObject *object;
  
  object = gtk_object_new (BST_TYPE_RADIO_TOOLS, NULL);
  
  return BST_RADIO_TOOLS (object);
}

static void
bst_radio_tools_do_set_tool (BstRadioTools *rtools,
			     guint          tool_id)
{
  GSList *slist, *next;

  rtools->block_tool_id = TRUE;
  for (slist = rtools->widgets; slist; slist = next)
    {
      GtkWidget *widget = slist->data;

      next = slist->next;
      if (GTK_IS_TOGGLE_BUTTON (widget))
	{
	  tool_id = GPOINTER_TO_UINT (gtk_object_get_user_data (GTK_OBJECT (widget)));
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), tool_id == rtools->tool_id);
	}
    }
  rtools->block_tool_id = FALSE;
}

void
bst_radio_tools_set_tool (BstRadioTools *rtools,
			  guint          tool_id)
{
  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));

  if (!rtools->block_tool_id)
    {
      /* emit the signal unconditionally if we don't have tools yet */
      if (!rtools->n_tools || rtools->tool_id != tool_id)
	{
	  rtools->tool_id = tool_id;
	  gtk_signal_emit (GTK_OBJECT (rtools), radio_tools_signals[SIGNAL_SET_TOOL], tool_id);
	}
    }
}

void
bst_radio_tools_add_category (BstRadioTools    *rtools,
			      guint             tool_id,
			      BseCategory      *category,
			      BstRadioToolFlags flags)
{
  gchar *tip;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (category != NULL);
  g_return_if_fail (flags != 0);

#if 0
  guint i, next_uc = 0;

  /* strip first namespace prefix from type name */
  name = g_type_name (category->type);
  for (i = 0; name[i] != 0; i++)
    if (i && toupper(name[i]) == name[i])
      {
	next_uc = i;
	break;
      }
  if (toupper(name[0]) == name[0] && next_uc > 0)
    name += next_uc;
#endif
  
  tip = g_strconcat (category->category + category->lindex + 1,
		     " [",
		     g_type_name (category->type),
		     "]",
		     NULL);
  bst_radio_tools_add_tool (rtools,
			    tool_id,
			    category->category + category->lindex + 1,
			    tip,
			    bse_type_blurb (category->type),
			    category->icon,
			    flags);
  g_free (tip);
}

static void
bst_radio_tools_add_tool_any (BstRadioTools    *rtools,
			      guint             tool_id,
			      const gchar      *tool_name,
			      const gchar      *tool_tip,
			      const gchar      *tool_blurb,
			      BswIcon          *tool_icon,
			      const gchar      *stock_icon,
			      BstRadioToolFlags flags)
{
  guint i;

  if (!tool_tip)
    tool_tip = tool_name;
  if (!tool_blurb)
    tool_blurb = tool_tip;

  i = rtools->n_tools++;
  rtools->tools = g_renew (BstRadioToolEntry, rtools->tools, rtools->n_tools);
  rtools->tools[i].tool_id = tool_id;
  rtools->tools[i].name = g_strdup (tool_name);
  rtools->tools[i].tip = g_strdup (tool_tip);
  rtools->tools[i].blurb = g_strdup (tool_blurb);
  rtools->tools[i].icon = tool_icon ? bsw_icon_ref (tool_icon) : NULL;
  rtools->tools[i].stock_icon = g_strdup (stock_icon);
  rtools->tools[i].flags = flags;
}

void
bst_radio_tools_add_tool (BstRadioTools    *rtools,
			  guint             tool_id,
			  const gchar      *tool_name,
			  const gchar      *tool_tip,
			  const gchar      *tool_blurb,
			  BswIcon          *tool_icon,
			  BstRadioToolFlags flags)
{
  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (tool_name != NULL);
  g_return_if_fail (flags != 0);

  bst_radio_tools_add_tool_any (rtools, tool_id, tool_name, tool_tip, tool_blurb, tool_icon, NULL, flags);
}

void
bst_radio_tools_add_stock_tool (BstRadioTools    *rtools,
				guint             tool_id,
				const gchar      *tool_name,
				const gchar      *tool_tip,
				const gchar      *tool_blurb,
				const gchar	 *stock_icon,
				BstRadioToolFlags flags)
{
  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (tool_name != NULL);
  g_return_if_fail (flags != 0);

  bst_radio_tools_add_tool_any (rtools, tool_id, tool_name, tool_tip, tool_blurb, NULL, stock_icon, flags);
}

void
bst_radio_tools_clear_tools (BstRadioTools *rtools)
{
  guint i;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));

  for (i = 0; i < rtools->n_tools; i++)
    {
      g_free (rtools->tools[i].name);
      g_free (rtools->tools[i].tip);
      g_free (rtools->tools[i].blurb);
      if (rtools->tools[i].icon)
	bsw_icon_unref (rtools->tools[i].icon);
      g_free (rtools->tools[i].stock_icon);
    }
  rtools->n_tools = 0;
  g_free (rtools->tools);
  rtools->tools = NULL;
}

static void
rtools_widget_destroyed (BstRadioTools *rtools,
			 GtkWidget     *widget)
{
  rtools->widgets = g_slist_remove (rtools->widgets, widget);
}

static void
rtools_toggle_toggled (BstRadioTools   *rtools,
		       GtkToggleButton *toggle)
{
  guint tool_id;

  if (!rtools->block_tool_id)
    {
      tool_id = GPOINTER_TO_UINT (gtk_object_get_user_data (GTK_OBJECT (toggle)));
      bst_radio_tools_set_tool (rtools, toggle->active ? tool_id : 0);
      /* enforce depressed state */
      if (rtools->tool_id == tool_id && !toggle->active)
	{
	  rtools->block_tool_id = TRUE;
	  gtk_toggle_button_set_active (toggle, TRUE);
	  rtools->block_tool_id = FALSE;
	}
    }
}

void
bst_radio_tools_build_toolbar (BstRadioTools *rtools,
			       BstToolbar    *toolbar)
{
  guint i;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (BST_IS_TOOLBAR (toolbar));

  for (i = 0; i < rtools->n_tools; i++)
    {
      GtkWidget *button, *image = NULL;

      if (!(rtools->tools[i].flags & BST_RADIO_TOOLS_TOOLBAR))
	continue;

      if (rtools->tools[i].icon)
	image = bst_image_from_icon (rtools->tools[i].icon, BST_SIZE_TOOLBAR);
      else if (rtools->tools[i].stock_icon)
	image = bst_image_from_stock (rtools->tools[i].stock_icon, BST_SIZE_TOOLBAR);
      if (!image)
	image = bst_image_from_stock (BST_STOCK_NOICON, BST_SIZE_TOOLBAR);
      button = bst_toolbar_append (toolbar, BST_TOOLBAR_TOGGLE,
				   rtools->tools[i].name,
				   rtools->tools[i].tip,
				   image);
      g_object_set (button,
		    "user_data", GUINT_TO_POINTER (rtools->tools[i].tool_id),
		    NULL);
      g_object_connect (button,
			"swapped_signal::toggled", rtools_toggle_toggled, rtools,
			"swapped_signal::destroy", rtools_widget_destroyed, rtools,
			NULL);
      rtools->widgets = g_slist_prepend (rtools->widgets, button);
    }

  BST_RADIO_TOOLS_GET_CLASS (rtools)->set_tool (rtools, rtools->tool_id);
}

void
bst_radio_tools_build_gtk_toolbar (BstRadioTools *rtools,
				   GtkToolbar    *toolbar)
{
  guint i;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (GTK_IS_TOOLBAR (toolbar));

  for (i = 0; i < rtools->n_tools; i++)
    {
      GtkWidget *button, *image = NULL;

      if (!(rtools->tools[i].flags & BST_RADIO_TOOLS_TOOLBAR))
	continue;

      if (rtools->tools[i].icon)
	image = bst_image_from_icon (rtools->tools[i].icon, BST_SIZE_TOOLBAR);
      else if (rtools->tools[i].stock_icon)
	image = bst_image_from_stock (rtools->tools[i].stock_icon, BST_SIZE_TOOLBAR);
      if (!image)
	image = bst_image_from_stock (BST_STOCK_NOICON, BST_SIZE_TOOLBAR);
      button = gtk_toolbar_append_element (toolbar,
					   GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
					   rtools->tools[i].name,
					   rtools->tools[i].tip, NULL,
					   image,
					   NULL, NULL);
      g_object_set (button,
		    "user_data", GUINT_TO_POINTER (rtools->tools[i].tool_id),
		    NULL);
      g_object_connect (button,
			"swapped_signal::toggled", rtools_toggle_toggled, rtools,
			"swapped_signal::destroy", rtools_widget_destroyed, rtools,
			NULL);
      rtools->widgets = g_slist_prepend (rtools->widgets, button);
    }

  BST_RADIO_TOOLS_GET_CLASS (rtools)->set_tool (rtools, rtools->tool_id);
}

static void
toggle_apply_blurb (GtkToggleButton *toggle,
		    GtkWidget       *text)
{
  gpointer tool_id = gtk_object_get_data (GTK_OBJECT (toggle), "user_data");
  gpointer blurb_id = gtk_object_get_data (GTK_OBJECT (text), "user_data");

  if (tool_id == blurb_id && !toggle->active)
    {
      bst_scroll_text_set (text, NULL);
      g_object_set_data (G_OBJECT (text), "user_data", GUINT_TO_POINTER (~0));
    }
  else if (toggle->active && tool_id != blurb_id)
    {
      bst_scroll_text_set (text, gtk_object_get_data (GTK_OBJECT (toggle), "blurb"));
      g_object_set_data (G_OBJECT (text), "user_data", tool_id);
    }
}

GtkWidget*
bst_radio_tools_build_palette (BstRadioTools *rtools,
			       gboolean       show_descriptions,
			       GtkReliefStyle relief)
{
  GtkWidget *vbox, *table, *text = NULL;
  guint i, n = 0, column = 5;
  
  g_return_val_if_fail (BST_IS_RADIO_TOOLS (rtools), NULL);
  
  vbox = gtk_widget_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "homogeneous", FALSE,
			 "resize_mode", GTK_RESIZE_QUEUE,
			 NULL);
  table = gtk_widget_new (GTK_TYPE_TABLE,
			  "visible", TRUE,
			  "homogeneous", TRUE,
			  NULL);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  if (show_descriptions)
    {
      text = bst_scroll_text_create (0, NULL);
      g_object_set_data (G_OBJECT (text), "user_data", GUINT_TO_POINTER (~0));
      gtk_widget_ref (text);
      gtk_object_sink (GTK_OBJECT (text));
    }
  
  for (i = 0; i < rtools->n_tools; i++)
    {
      GtkWidget *button, *image = NULL;
      
      if (!(rtools->tools[i].flags & BST_RADIO_TOOLS_PALETTE))
	continue;
      
      if (rtools->tools[i].icon)
	image = bst_image_from_icon (rtools->tools[i].icon, BST_SIZE_PALETTE);
      else if (rtools->tools[i].stock_icon)
	image = bst_image_from_stock (rtools->tools[i].stock_icon, BST_SIZE_PALETTE);
      if (!image)
	image = bst_image_from_stock (BST_STOCK_NOICON, BST_SIZE_PALETTE);
      button = g_object_connect (gtk_widget_new (GTK_TYPE_TOGGLE_BUTTON,
						 "visible", TRUE,
						 "draw_indicator", FALSE,
						 "relief", relief,
						 "child", image,
						 "user_data", GUINT_TO_POINTER (rtools->tools[i].tool_id),
						 NULL),
				 "swapped_signal::toggled", rtools_toggle_toggled, rtools,
				 "swapped_signal::destroy", rtools_widget_destroyed, rtools,
				 text ? "signal::toggled" : NULL, toggle_apply_blurb, text,
				 NULL);
      gtk_tooltips_set_tip (BST_TOOLTIPS, button, rtools->tools[i].tip, NULL);
      gtk_object_set_data_full (GTK_OBJECT (button), "blurb", g_strdup (rtools->tools[i].blurb), g_free);
      rtools->widgets = g_slist_prepend (rtools->widgets, button);
      gtk_table_attach (GTK_TABLE (table),
			button,
			n % column, n % column + 1,
			n / column, n / column + 1,
			GTK_SHRINK, GTK_SHRINK, // GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			0, 0);
      n++;
    }
  
  if (n && text)
    {
      GtkWidget *hbox;

      hbox = gtk_widget_new (GTK_TYPE_HBOX,
			     "visible", TRUE,
			     NULL);
      gtk_box_pack_start (GTK_BOX (hbox), text, TRUE, TRUE, 5);
      gtk_widget_new (GTK_TYPE_FRAME,
		      "visible", TRUE,
		      "label", "Description",
		      "label_xalign", 0.5,
		      "border_width", 5,
		      "parent", vbox,
		      "child", hbox,
		      "width_request", 1,
		      "height_request", 100,
		      NULL);
    }
  if (text)
    gtk_widget_unref (text);
  
  BST_RADIO_TOOLS_GET_CLASS (rtools)->set_tool (rtools, rtools->tool_id);
  
  return vbox;
}
