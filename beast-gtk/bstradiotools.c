/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000-2003 Tim Janik and Red Hat, Inc.
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
#include "bstradiotools.h"

#include "bstcatalog.h"

#include <ctype.h>


struct _BstRadioToolEntry
{
  guint          tool_id;
  BstCatalogTool tool;
  BseIcon       *icon;
  guint          flags;
};

#define	BST_RADIO_TOOLS_FREE_FLAG	(1 << 30)


/* --- prototypes --- */
static void	  bst_radio_tools_class_init		(BstRadioToolsClass	*klass);
static void	  bst_radio_tools_init			(BstRadioTools		*rtools,
							 BstRadioToolsClass     *class);
static void	  bst_radio_tools_real_dispose		(GObject		*object);
static void	  bst_radio_tools_finalize		(GObject		*object);
static void	  bst_radio_tools_do_set_tool		(BstRadioTools		*rtools,
							 guint         		 tool_id);


/* --- static variables --- */
static gpointer parent_class = NULL;
static guint    signal_set_tool = 0;


/* --- functions --- */
GType
bst_radio_tools_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (BstRadioToolsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bst_radio_tools_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (BstRadioTools),
        0,      /* n_preallocs */
        (GInstanceInitFunc) bst_radio_tools_init,
      };
      type = g_type_register_static (G_TYPE_OBJECT,
                                     "BstRadioTools",
                                     &type_info, 0);
    }
  return type;
}

static void
bst_radio_tools_class_init (BstRadioToolsClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->dispose = bst_radio_tools_real_dispose;
  gobject_class->finalize = bst_radio_tools_finalize;
  
  class->set_tool = bst_radio_tools_do_set_tool;
  
  signal_set_tool = g_signal_new ("set-tool",
                                  G_OBJECT_CLASS_TYPE (class),
                                  G_SIGNAL_RUN_LAST | GTK_RUN_NO_RECURSE,
                                  G_STRUCT_OFFSET (BstRadioToolsClass, set_tool),
                                  NULL, NULL,
                                  bst_marshal_NONE__UINT,
                                  G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
bst_radio_tools_init (BstRadioTools      *self,
                      BstRadioToolsClass *class)
{
  self->block_tool_id = 0;
  self->tool_id = 0;
  self->n_tools = 0;
  self->tools = NULL;
  self->widgets = NULL;
  self->accel_group = gtk_accel_group_new ();
}

static void
bst_radio_tools_real_dispose (GObject *object)
{
  BstRadioTools *self = BST_RADIO_TOOLS (object);
  
  bst_radio_tools_clear_tools (self);
  
  while (self->widgets)
    gtk_widget_destroy (self->widgets->data);
  
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bst_radio_tools_finalize (GObject *object)
{
  BstRadioTools *self = BST_RADIO_TOOLS (object);
  
  bst_radio_tools_clear_tools (self);
  
  while (self->widgets)
    gtk_widget_destroy (self->widgets->data);

  g_object_unref (self->accel_group);
  self->accel_group = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

BstRadioTools*
bst_radio_tools_new (void)
{
  BstRadioTools *rtools = g_object_new (BST_TYPE_RADIO_TOOLS, NULL);
  
  return rtools;
}

static void
bst_radio_tools_do_set_tool (BstRadioTools *rtools,
                             guint          tool_id)
{
  GSList *slist, *next;
  
  rtools->block_tool_id++;
  for (slist = rtools->widgets; slist; slist = next)
    {
      GtkWidget *widget = slist->data;
      
      next = slist->next;
      if (GTK_IS_TOGGLE_BUTTON (widget))
        {
          tool_id = g_object_get_long (widget, "user_data");
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), tool_id == rtools->tool_id);
        }
      else if (gxk_toolbar_choice_is_item (widget))
        {
          tool_id = g_object_get_long (widget, "user_data");
          if (tool_id == rtools->tool_id &&
              !gxk_toolbar_choice_is_selected (widget))
            gxk_toolbar_choice_select (widget);
        }
    }
  rtools->block_tool_id--;
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
          g_signal_emit (rtools, signal_set_tool, 0, tool_id);
        }
    }
}

void
bst_radio_tools_add_category (BstRadioTools     *rtools,
                              guint              tool_id,
                              BseCategory       *category,
                              BstRadioToolsFlags flags)
{
  gchar *tip;
  
  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (category != NULL);
  
  tip = g_strconcat (category->category + category->lindex + 1,
                     " [", category->type, "]",
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
bst_radio_tools_add_tool_generic (BstRadioTools     *self,
				  guint              tool_id,
				  const gchar       *tool_name,
				  const gchar       *stock_icon,
				  const gchar       *tool_accel,
				  const gchar       *tool_tip,
				  const gchar       *tool_blurb,
				  BseIcon           *tool_icon,
				  BstRadioToolsFlags flags)
{
  guint i;

  if (!tool_tip)
    tool_tip = tool_name;
  if (!tool_blurb)
    tool_blurb = tool_tip;

  flags |= BST_RADIO_TOOLS_FREE_FLAG;

  i = self->n_tools++;
  self->tools = g_renew (BstRadioToolEntry, self->tools, self->n_tools);
  self->tools[i].tool_id = tool_id;
  self->tools[i].tool.cat_key = NULL;
  self->tools[i].tool.name = g_strdup (tool_name);
  self->tools[i].tool.stock_id = g_strdup (stock_icon);
  self->tools[i].tool.accelerator = g_strdup (tool_accel);
  self->tools[i].tool.tooltip = g_strdup (tool_tip);
  self->tools[i].tool.blurb = g_strdup (tool_blurb);
  self->tools[i].icon = tool_icon ? bse_icon_copy_shallow (tool_icon) : NULL;
  self->tools[i].flags = flags;
}

static void
bst_radio_tools_add_catalog_tool (BstRadioTools     *self,
				  guint              tool_id,
				  const gchar       *cat_key,
				  BstRadioToolsFlags flags)
{
  const BstCatalogTool *tool;
  BstCatalogTool dummy = { 0, };
  guint i;

  tool = bst_catalog_get_tool (cat_key);
  if (tool)
    flags &= ~BST_RADIO_TOOLS_FREE_FLAG;
  else
    {
      dummy.name = g_strdup (cat_key);
      tool = &dummy;
      flags |= ~BST_RADIO_TOOLS_FREE_FLAG;
    }

  i = self->n_tools++;
  self->tools = g_renew (BstRadioToolEntry, self->tools, self->n_tools);
  self->tools[i].tool_id = tool_id;
  self->tools[i].tool = *tool;
  self->tools[i].tool.cat_key = NULL;
  self->tools[i].icon = NULL;
  self->tools[i].flags = flags;
}

void
bst_radio_tools_add_tools (BstRadioTools *self,
			   guint          n_tools,
			   const BstTool *tools)
{
  guint i;

  g_return_if_fail (BST_IS_RADIO_TOOLS (self));

  for (i = 0; i < n_tools; i++)
    bst_radio_tools_add_catalog_tool (self, tools[i].tool_id, tools[i].cat_key, tools[i].flags);
}

void
bst_radio_tools_add_tool (BstRadioTools     *self,
                          guint              tool_id,
                          const gchar       *tool_name,
                          const gchar       *tool_tip,
                          const gchar       *tool_blurb,
                          BseIcon           *tool_icon,
                          BstRadioToolsFlags flags)
{
  g_return_if_fail (BST_IS_RADIO_TOOLS (self));
  g_return_if_fail (tool_name != NULL);
  
  bst_radio_tools_add_tool_generic (self, tool_id, tool_name, NULL, NULL, tool_tip, tool_blurb, tool_icon, flags);
}

void
bst_radio_tools_add_stock_tool (BstRadioTools     *self,
                                guint              tool_id,
                                const gchar       *tool_name,
                                const gchar       *tool_tip,
                                const gchar       *tool_blurb,
                                const gchar       *stock_icon,
                                BstRadioToolsFlags flags)
{
  g_return_if_fail (BST_IS_RADIO_TOOLS (self));
  g_return_if_fail (tool_name != NULL);
  g_return_if_fail (flags != 0);
  
  bst_radio_tools_add_tool_generic (self, tool_id, tool_name, stock_icon, NULL, tool_tip, tool_blurb, NULL, flags);
}

void
bst_radio_tools_clear_tools (BstRadioTools *self)
{
  guint i;
  
  g_return_if_fail (BST_IS_RADIO_TOOLS (self));
  
  for (i = 0; i < self->n_tools; i++)
    {
      if (self->tools[i].flags & BST_RADIO_TOOLS_FREE_FLAG)
	{
	  g_free (self->tools[i].tool.cat_key);
	  g_free (self->tools[i].tool.name);
	  g_free (self->tools[i].tool.stock_id);
	  g_free (self->tools[i].tool.accelerator);
	  g_free (self->tools[i].tool.tooltip);
	  g_free (self->tools[i].tool.blurb);
	}
      if (self->tools[i].icon)
        bse_icon_free (self->tools[i].icon);
    }
  self->n_tools = 0;
  g_free (self->tools);
  self->tools = NULL;
}

void
bst_radio_tools_destroy (BstRadioTools *self)
{
  g_return_if_fail (BST_IS_RADIO_TOOLS (self));
  
  g_object_run_dispose (G_OBJECT (self));
}

static void
rtools_widget_destroyed (BstRadioTools *self,
                         GtkWidget     *widget)
{
  self->widgets = g_slist_remove (self->widgets, widget);
}

static void
rtools_toggle_toggled (BstRadioTools   *self,
                       GtkToggleButton *toggle)
{
  guint tool_id;
  
  if (!self->block_tool_id)
    {
      GdkEvent *event = gtk_get_current_event ();
      tool_id = GPOINTER_TO_UINT (gtk_object_get_user_data (GTK_OBJECT (toggle)));
      /* ignore untoggeling through the GUI (button release on depressed toggle) */
      if (toggle->active ||
          (gtk_get_event_widget (event) == GTK_WIDGET (toggle) &&
           event->type == GDK_BUTTON_RELEASE))
        bst_radio_tools_set_tool (self, tool_id);
      else
        bst_radio_tools_set_tool (self, 0);
      /* enforce depressed state in case tool_id didn't change */
      if (self->tool_id == tool_id && !toggle->active)
        {
          self->block_tool_id++;
          gtk_toggle_button_set_active (toggle, TRUE);
          self->block_tool_id--;
        }
    }
}

void
bst_radio_tools_build_toolbar (BstRadioTools *self,
                               GxkToolbar    *toolbar)
{
  guint i;
  
  g_return_if_fail (BST_IS_RADIO_TOOLS (self));
  g_return_if_fail (GXK_IS_TOOLBAR (toolbar));
  
  for (i = 0; i < self->n_tools; i++)
    {
      GtkWidget *button, *image = NULL;
      
      if (!(self->tools[i].flags & BST_RADIO_TOOLS_TOOLBAR))
        continue;
      
      if (self->tools[i].icon)
        image = bst_image_from_icon (self->tools[i].icon, BST_SIZE_TOOLBAR);
      else if (self->tools[i].tool.stock_id)
        image = gxk_stock_image (self->tools[i].tool.stock_id, BST_SIZE_TOOLBAR);
      if (!image)
        image = gxk_stock_image (BST_STOCK_NO_ICON, BST_SIZE_TOOLBAR);
      button = gxk_toolbar_append (toolbar, GXK_TOOLBAR_TOGGLE,
                                   self->tools[i].tool.name,
                                   self->tools[i].tool.tooltip,
                                   image);
      g_object_set (button,
                    "user_data", GUINT_TO_POINTER (self->tools[i].tool_id),
                    NULL);
      g_object_connect (button,
                        "swapped_signal::toggled", rtools_toggle_toggled, self,
                        "swapped_signal::destroy", rtools_widget_destroyed, self,
                        NULL);
      self->widgets = g_slist_prepend (self->widgets, button);
      if (self->tools[i].tool.accelerator)
	{
	  guint accelerator_key = 0;
	  GdkModifierType accelerator_mods = 0;
	  gtk_accelerator_parse (self->tools[i].tool.accelerator, &accelerator_key, &accelerator_mods);
	  if (accelerator_key)
	    gtk_widget_add_accelerator (button, "clicked", self->accel_group,
					accelerator_key, accelerator_mods, GTK_ACCEL_VISIBLE);
	}
    }
  
  BST_RADIO_TOOLS_GET_CLASS (self)->set_tool (self, self->tool_id);
}

static void
rtools_choice_func (gpointer       data,
                    guint          tool_id)
{
  BstRadioTools *self = BST_RADIO_TOOLS (data);
  
  if (!self->block_tool_id)
    bst_radio_tools_set_tool (self, tool_id);
}

void
bst_radio_tools_build_toolbar_choice (BstRadioTools *self,
                                      GxkToolbar    *toolbar)
{
  GtkWidget *choice_widget;
  guint i;
  
  g_return_if_fail (BST_IS_RADIO_TOOLS (self));
  g_return_if_fail (GXK_IS_TOOLBAR (toolbar));
  
  choice_widget = gxk_toolbar_append_choice (toolbar, GXK_TOOLBAR_TRUNC_BUTTON,
                                             rtools_choice_func, self, NULL);
  self->block_tool_id++;
  for (i = 0; i < self->n_tools; i++)
    {
      GtkWidget *item, *image = NULL;
      
      if (!(self->tools[i].flags & BST_RADIO_TOOLS_TOOLBAR))
        continue;
      
      if (self->tools[i].icon)
        image = bst_image_from_icon (self->tools[i].icon, BST_SIZE_TOOLBAR);
      else if (self->tools[i].tool.stock_id)
        image = gxk_stock_image (self->tools[i].tool.stock_id, BST_SIZE_TOOLBAR);
      if (!image)
        image = gxk_stock_image (BST_STOCK_NO_ICON, BST_SIZE_TOOLBAR);
      item = gxk_toolbar_choice_add (choice_widget,
                                     self->tools[i].tool.name,
                                     self->tools[i].tool.tooltip,
                                     image,
                                     self->tools[i].tool_id);
      g_object_set_long (item, "user_data", self->tools[i].tool_id);
      g_object_connect (item,
                        "swapped_signal::destroy", rtools_widget_destroyed, self,
                        NULL);
      self->widgets = g_slist_prepend (self->widgets, item);
      if (self->tools[i].tool.accelerator)
	{
	  guint accelerator_key = 0;
	  GdkModifierType accelerator_mods = 0;
	  gtk_accelerator_parse (self->tools[i].tool.accelerator, &accelerator_key, &accelerator_mods);
	  if (accelerator_key)
	    gtk_widget_add_accelerator (item, "activate", self->accel_group,
					accelerator_key, accelerator_mods, GTK_ACCEL_VISIBLE);
	}
    }
  self->block_tool_id--;
  
  BST_RADIO_TOOLS_GET_CLASS (self)->set_tool (self, self->tool_id);
}

static void
tool2text (BstRadioTools *self,
	   guint          tool_id,
	   GtkWidget	 *text)
{
  guint i;
  gxk_scroll_text_set (text, NULL);
  for (i = 0; i < self->n_tools; i++)
    if (self->tools[i].tool_id == tool_id)
      {
	gxk_scroll_text_set (text, self->tools[i].tool.blurb);
	break;
      }
}

static void
tool2label (BstRadioTools *self,
	    guint          tool_id,
	    GtkWidget	  *label)
{
  guint i;
  gtk_label_set (GTK_LABEL (label), NULL);
  for (i = 0; i < self->n_tools; i++)
    if (self->tools[i].tool_id == tool_id)
      {
	gtk_label_set (GTK_LABEL (label), self->tools[i].tool.name);
	break;
      }
}

GtkWidget*
bst_radio_tools_build_palette (BstRadioTools *self,
			       GtkWidget     *selector,
                               gboolean       show_descriptions,
                               GtkReliefStyle relief)
{
  GtkWidget *vbox, *table, *hbox, *label, *text = NULL;
  guint i, n = 0, column = 5;
  
  g_return_val_if_fail (BST_IS_RADIO_TOOLS (self), NULL);
  
  vbox = gtk_widget_new (GTK_TYPE_VBOX,
                         "visible", TRUE,
                         "homogeneous", FALSE,
			 "spacing", 3,
                         NULL);
  table = gtk_widget_new (GTK_TYPE_TABLE,
                          "visible", TRUE,
                          "homogeneous", TRUE,
                          NULL);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  /* add tool name label */
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "homogeneous", FALSE,
			 "spacing", 3,
			 NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  if (selector)
    gtk_box_pack_start (GTK_BOX (hbox), selector, FALSE, TRUE, 0);
  label = gtk_widget_new (GTK_TYPE_LABEL,
			  "visible", TRUE,
			  "parent", hbox,
			  "width_request", 1,	/* packed auto-expand */
			  NULL);
  g_signal_connect_object (self, "set_tool", G_CALLBACK (tool2label), label, G_CONNECT_AFTER);
  tool2label (self, self->tool_id, label);
  /* create description view */
  if (show_descriptions)
    {
      text = gxk_scroll_text_create (GXK_SCROLL_TEXT_WIDGET_LOOK, NULL);
      gtk_widget_ref (text);
      gtk_object_sink (GTK_OBJECT (text));
      g_signal_connect_object (self, "set_tool", G_CALLBACK (tool2text), text, G_CONNECT_AFTER);
      tool2text (self, self->tool_id, text);
    }
  
  for (i = 0; i < self->n_tools; i++)
    {
      GtkWidget *button, *image = NULL;
      
      if (!(self->tools[i].flags & BST_RADIO_TOOLS_PALETTE))
        continue;
      
      if (self->tools[i].icon)
        image = bst_image_from_icon (self->tools[i].icon, BST_SIZE_PALETTE);
      else if (self->tools[i].tool.stock_id)
        image = gxk_stock_image (self->tools[i].tool.stock_id, BST_SIZE_PALETTE);
      if (!image)
        image = gxk_stock_image (BST_STOCK_NO_ICON, BST_SIZE_PALETTE);
      button = g_object_connect (gtk_widget_new (GTK_TYPE_TOGGLE_BUTTON,
                                                 "visible", TRUE,
                                                 "draw_indicator", FALSE,
                                                 "relief", relief,
                                                 "child", image,
                                                 "user_data", GUINT_TO_POINTER (self->tools[i].tool_id),
                                                 NULL),
                                 "swapped_signal::toggled", rtools_toggle_toggled, self,
                                 "swapped_signal::destroy", rtools_widget_destroyed, self,
                                 NULL);
      if (self->tools[i].tool.accelerator)
	{
	  guint accelerator_key = 0;
	  GdkModifierType accelerator_mods = 0;
	  gtk_accelerator_parse (self->tools[i].tool.accelerator, &accelerator_key, &accelerator_mods);
	  if (accelerator_key)
	    gtk_widget_add_accelerator (button, "clicked", self->accel_group,
					accelerator_key, accelerator_mods, GTK_ACCEL_VISIBLE);
	}
      gtk_tooltips_set_tip (GXK_TOOLTIPS, button, self->tools[i].tool.tooltip, NULL);
      gtk_object_set_data_full (GTK_OBJECT (button), "blurb", g_strdup (self->tools[i].tool.blurb), g_free);
      self->widgets = g_slist_prepend (self->widgets, button);
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
                      "label", _("Description"),
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
  
  BST_RADIO_TOOLS_GET_CLASS (self)->set_tool (self, self->tool_id);
  
  return vbox;
}
