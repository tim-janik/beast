/* BEAST - Better Audio System
 * Copyright (C) 2002 Tim Janik
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
#include "bstrackeditor.h"
#if 0
#include "bstrackitem.h"
#endif


/* --- include sources --- */
#include "bstrackeditor-covers.cc"


/* --- prototypes --- */
static void	bst_rack_editor_class_init	(BstRackEditorClass	*klass);
static void	bst_rack_editor_init		(BstRackEditor		*rack_editor);
static void	bst_rack_editor_destroy		(GtkObject		*object);
static void	update_covers			(BstRackEditor		*ed);
static void	update_items			(BstRackEditor		*ed);
static void	create_rack_item		(BstRackEditor		*ed,
						 guint			 entry_id);


/* --- static variables --- */
static gpointer		    parent_class = NULL;


/* --- functions --- */
GType
bst_rack_editor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstRackEditorClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_rack_editor_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstRackEditor),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_rack_editor_init,
      };

      type = g_type_register_static (GTK_TYPE_VBOX,
				     "BstRackEditor",
				     &type_info, GTypeFlags (0));
    }

  return type;
}

static void
bst_rack_editor_class_init (BstRackEditorClass *klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy = bst_rack_editor_destroy;
}

static void
toggle_edit_mode (BstRackEditor *ed)
{
  bst_rack_table_set_edit_mode (ed->rtable, !ed->rtable->edit_mode);
}

static void
bst_rack_editor_init (BstRackEditor *ed)
{
  ed->pocket = 0;
  GtkWidget *edchild =
    (GtkWidget*) g_object_new (GTK_TYPE_LABEL,
                               "label", _("The rack editor is still highly experimental code and guaranteed "
                                          "to be broken by future releases. So don't rely on rack editor contents "
                                          "to look similarly in future versions and be prepared to work around "
                                          "possible instabilities in the code (i.e. save your projects regularly)."),
                               "visible", TRUE,
                               "justify", GTK_JUSTIFY_CENTER,
                               "wrap", TRUE,
                               NULL);
  gtk_box_pack_start (GTK_BOX (ed), edchild, FALSE, FALSE, 3);
  ed->button_edit = (GtkWidget*) g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                                               "visible", TRUE,
                                               NULL);
  gtk_box_pack_start (GTK_BOX (ed), ed->button_edit, FALSE, TRUE, 0);
  g_object_new (GTK_TYPE_LABEL,
		"label", _("_Edit"),
		"use_underline", TRUE,
		"visible", TRUE,
		"parent", ed->button_edit,
		NULL);
  g_object_connect (ed->button_edit,
		    "swapped_signal::destroy", g_nullify_pointer, &ed->button_edit,
		    "swapped_signal::clicked", toggle_edit_mode, ed,
		    NULL);

  ed->rtable = (BstRackTable*) g_object_new (BST_TYPE_RACK_TABLE,
                                             "visible", TRUE,
                                             "parent", ed,
                                             NULL);
  g_object_connect (ed->rtable,
		    "swapped_signal::destroy", g_nullify_pointer, &ed->rtable,
		    "swapped_signal::edit-mode-changed", update_covers, ed,
		    NULL);
  gtk_table_resize (GTK_TABLE (ed->rtable), 20, 30);
  update_covers (ed);
}

static void
bst_rack_editor_destroy (GtkObject *object)
{
  BstRackEditor *ed = BST_RACK_EDITOR (object);

  bst_rack_editor_set_rack_view (ed, 0);

  while (ed->plate_list)
    {
      GSList *slist = ed->plate_list;

      ed->plate_list = slist->next;
      gtk_widget_destroy ((GtkWidget*) slist->data);
      g_slist_free_1 (slist);
    }
  while (ed->item_list)
    {
      GSList *slist = ed->item_list;

      ed->item_list = slist->next;
      gtk_widget_destroy ((GtkWidget*) slist->data);
      g_slist_free_1 (slist);
    }
  
  /* chain parent class' handler */
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
pocket_entry_changed (BstRackEditor *ed,
		      guint	     entry_id)
{
#if 0
  GSList *slist;
  BstRackItem *item = NULL;
  const gchar *controller;

  for (slist = ed->item_list; slist; slist = slist->next)
    {
      item = slist->data;
      if (item->entry == entry_id)
	break;
    }
  if (!slist)
    item = NULL;
  controller = bse_data_pocket_get_string (ed->pocket, entry_id, "property-controller");
  if (item && !controller)
    {
      ed->item_list = g_slist_remove (ed->item_list, item);
      gtk_widget_destroy (GTK_WIDGET (item));
    }
  else if (!item && controller)
    create_rack_item (ed, entry_id);
#endif
}

static void
pocket_remove (BstRackEditor *ed)
{
  bst_rack_editor_set_rack_view (ed, 0);
}

void
bst_rack_editor_set_rack_view (BstRackEditor *ed,
			       SfiProxy      pocket)
{
  g_return_if_fail (BST_IS_RACK_EDITOR (ed));
  if (pocket)
    g_return_if_fail (BSE_IS_DATA_POCKET (pocket));

  if (ed->pocket)
    {
      bse_proxy_disconnect (ed->pocket,
			    "any_signal", pocket_remove, ed,
			    "any_signal", pocket_entry_changed, ed,
			    NULL);
    }
  ed->pocket = pocket;
  if (ed->pocket)
    {
      bse_proxy_connect (ed->pocket,
			 "swapped_signal::release", pocket_remove, ed,
			 "swapped_signal::entry_changed", pocket_entry_changed, ed,
			 "swapped_signal::entry_removed", pocket_entry_changed, ed,
			 NULL);
      update_items (ed);
    }
}

void
bst_rack_editor_add_property (BstRackEditor *ed,
			      SfiProxy       item,
			      const gchar   *property_name)
{
  g_return_if_fail (BST_IS_RACK_EDITOR (ed));
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (property_name != NULL);

  if (ed->pocket)
    {
      GParamSpec *pspec = bse_proxy_get_pspec (item, property_name);

      if (pspec)
	{
	  guint id = bse_data_pocket_create_entry (ed->pocket);
	  bse_data_pocket_set_string (ed->pocket, id, "property-controller", NULL);
	  bse_data_pocket_set_object (ed->pocket, id, "property-object", item);
	  bse_data_pocket_set_string (ed->pocket, id, "property-name", property_name);
	}
    }
}

static void
update_covers (BstRackEditor *ed)
{
  if (ed->rtable->edit_mode)
    {
      while (ed->plate_list)
	{
	  GSList *slist = ed->plate_list;

	  ed->plate_list = slist->next;
	  gtk_widget_destroy ((GtkWidget*) slist->data);
	  g_slist_free_1 (slist);
	}
    }
  else if (!ed->plate_list)
    ed->plate_list = rack_cover_add_plates (ed->rtable);
}

static void
update_items (BstRackEditor *ed)
{
  GSList *slist = NULL;
  guint i;

  while (ed->item_list)
    {
      GSList *slist = ed->item_list;

      ed->item_list = slist->next;
      gtk_widget_destroy ((GtkWidget*) slist->data);
      g_slist_free_1 (slist);
    }

  bst_rack_table_set_edit_mode (ed->rtable, TRUE);
  i = bse_data_pocket_get_n_entries (ed->pocket);
  while (i--)
    slist = g_slist_prepend (slist, GUINT_TO_POINTER (bse_data_pocket_get_nth_entry_id (ed->pocket, i)));
  while (slist)
    {
      GSList *tmp = slist;
      guint entry_id = GPOINTER_TO_UINT (tmp->data);
      const gchar *controller;

      slist = tmp->next;
      controller = bse_data_pocket_get_string (ed->pocket, entry_id, "property-controller");
      if (controller)
	create_rack_item (ed, entry_id);
      g_slist_free_1 (tmp);
    }
  bst_rack_table_set_edit_mode (ed->rtable, FALSE);
}

static void
create_rack_item (BstRackEditor *ed,
		  guint          entry_id)
{
#if 0
  gboolean edit_mode = ed->rtable->edit_mode;
  GtkWidget *item = (GtkWidget*) g_object_new (BST_TYPE_RACK_ITEM,
				  "visible", TRUE,
				  NULL);

  if (!edit_mode)
    bst_rack_table_set_edit_mode (ed->rtable, TRUE);
  bst_rack_child_set_info (item, -1, -2, 4, 4);
  bst_rack_item_set_property (BST_RACK_ITEM (item), ed->pocket, entry_id);
  gtk_container_add (GTK_CONTAINER (ed->rtable), item);
  ed->item_list = g_slist_prepend (ed->item_list, item);
  if (!edit_mode)
    bst_rack_table_set_edit_mode (ed->rtable, FALSE);
#endif
}
