/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#include	"bstitemview.h"

#include	"bstparamview.h"



/* --- prototypes --- */
static void	bst_item_view_class_init	(BstItemViewClass	*klass);
static void	bst_item_view_init		(BstItemView		*item_view,
						 BstItemViewClass	*real_class);
static void	bst_item_view_destroy		(GtkObject		*object);
static void	bst_item_view_finalize		(GObject		*object);


/* --- item clist --- */
enum {
  CLIST_SEQID,
  CLIST_NAME,
  CLIST_BLURB,
  CLIST_N_COLUMNS
};
static gchar *clist_titles[CLIST_N_COLUMNS] = {
  "SeqId",
  "Name",
  "Blurb",
};


/* --- static variables --- */
static gpointer		 parent_class = NULL;
static BstItemViewClass *bst_item_view_class = NULL;


/* --- functions --- */
GtkType
bst_item_view_get_type (void)
{
  static GtkType item_view_type = 0;
  
  if (!item_view_type)
    {
      GtkTypeInfo item_view_info =
      {
	"BstItemView",
	sizeof (BstItemView),
	sizeof (BstItemViewClass),
	(GtkClassInitFunc) bst_item_view_class_init,
	(GtkObjectInitFunc) bst_item_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      item_view_type = gtk_type_unique (GTK_TYPE_ALIGNMENT, &item_view_info);
    }
  
  return item_view_type;
}

static void
bst_item_view_class_init (BstItemViewClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  
  bst_item_view_class = class;
  parent_class = gtk_type_class (GTK_TYPE_ALIGNMENT);
  
  G_OBJECT_CLASS (object_class)->finalize = bst_item_view_finalize;

  object_class->destroy = bst_item_view_destroy;
  
  class->n_ops = 0;
  class->ops = NULL;
  
  class->operate = NULL;
  class->can_operate = NULL;
  class->default_param_view_height = 0;
}

static void
bst_item_view_init (BstItemView      *item_view,
		    BstItemViewClass *real_class)
{
  item_view->paned = gtk_widget_new (GTK_TYPE_VPANED,
				     "visible", TRUE,
				     "parent", item_view,
				     NULL);
  gtk_widget_ref (item_view->paned);

  item_view->item_type = 0;
  item_view->container = 0;
  item_view->id_format = g_strdup ("%03u");
  item_view->item_list_pos = 0;
  item_view->item_clist = NULL;
  item_view->param_view = NULL;
  item_view->op_widgets = g_new0 (GtkWidget*, real_class->n_ops);
  gtk_container_set_resize_mode (GTK_CONTAINER (item_view), GTK_RESIZE_QUEUE);
}

static void
bst_item_view_destroy_contents (BstItemView *item_view)
{
  gtk_container_foreach (GTK_CONTAINER (item_view->paned), (GtkCallback) gtk_widget_destroy, NULL);
}

static void
bst_item_view_destroy (GtkObject *object)
{
  BstItemView *item_view;
  
  g_return_if_fail (object != NULL);
  
  item_view = BST_ITEM_VIEW (object);
  
  bst_item_view_destroy_contents (item_view);
  
  bst_item_view_set_container (item_view, 0);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_item_view_finalize (GObject *object)
{
  BstItemView *item_view;
  
  g_return_if_fail (object != NULL);
  
  item_view = BST_ITEM_VIEW (object);
  
  g_free (item_view->id_format);
  g_free (item_view->op_widgets);
  
  gtk_widget_unref (item_view->paned);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bst_item_view_item_changed (BstItemView *item_view,
			    BswProxy     item)
{
  GtkCList *clist;
  gint row;
  
  clist = GTK_CLIST (item_view->item_clist);
  row = gtk_clist_find_row_from_data (clist, (gpointer) item);
  if (row >= 0)
    {
      gchar *string, *str;
      
      string = g_strdup_printf (item_view->id_format, bse_item_get_seqid (bse_object_from_id (item)));
      str = NULL;
      if (!gtk_clist_get_text (clist, row, CLIST_SEQID, &str) || strcmp (str, string))
	gtk_clist_set_text (clist, row, CLIST_SEQID, string);
      g_free (string);
      string = bsw_item_get_name (item);
      if (!string)
	string = "";
      str = NULL;
      if (!gtk_clist_get_text (clist, row, CLIST_NAME, &str) || strcmp (str, string))
	gtk_clist_set_text (clist, row, CLIST_NAME, string);
      string = NULL;
      g_object_get (bse_object_from_id (item), "blurb", &string, NULL);
      if (!string)
	string = g_strdup ("");
      if (!gtk_clist_get_text (clist, row, CLIST_BLURB, &str) || strcmp (str, string))
	gtk_clist_set_text (clist, row, CLIST_BLURB, string);
      g_free (string);
    }
}

static void
bst_item_view_item_param_changed (BstItemView *item_view,
				  GParamSpec  *pspec,
				  BswProxy     item)
{
  bst_item_view_item_changed (item_view, item);
}

static void
bst_item_view_build_param_view (BstItemView *item_view)
{
  BswProxy container = item_view->container;
  BswProxy item = 0;
  BswIterProxy *iter;
  
  g_return_if_fail (item_view->param_view == NULL);
  
  for (iter = bsw_container_list_items (container); bsw_iter_n_left (iter); bsw_iter_next (iter))
    if (g_type_is_a (bsw_proxy_type (bsw_iter_get_proxy (iter)), item_view->item_type))
      item = bsw_iter_get_proxy (iter);
  bsw_iter_free (iter);

  if (BSW_IS_ITEM (item))
    {
      gint default_param_view_height = BST_ITEM_VIEW_GET_CLASS (item_view)->default_param_view_height;

      item_view->param_view = bst_param_view_new (item);
      g_object_set (item_view->param_view,
		    "visible", TRUE,
		    default_param_view_height > 0 ? "height_request" : NULL, default_param_view_height,
		    NULL);
      g_object_connect (item_view->param_view,
			"signal::destroy", gtk_widget_destroyed, &item_view->param_view,
			NULL);
      (item_view->item_list_pos ? gtk_paned_pack1 : gtk_paned_pack2) (GTK_PANED (item_view->paned),
								      item_view->param_view,
								      TRUE,
								      TRUE);
      bst_param_view_set_object (BST_PARAM_VIEW (item_view->param_view),
				 bst_item_view_get_current (item_view));
    }
}

static void
bst_item_view_item_added (BstItemView  *item_view,
			  BseItem      *item,
			  BseContainer *container)
{
  if (BSE_IS_ITEM (item) && g_type_is_a (BSE_OBJECT_TYPE (item), item_view->item_type))
    {
      static gchar *text[CLIST_N_COLUMNS] = { 0, };
      gint row;
      GtkCList *clist = GTK_CLIST (item_view->item_clist);
      
      bsw_proxy_connect (BSE_OBJECT_ID (item),
			 "swapped_signal::seqid_changed", bst_item_view_item_changed, item_view,
			 "swapped_signal::notify", bst_item_view_item_param_changed, item_view,
			 NULL);
      row = gtk_clist_insert (clist, -1, text);
      gtk_clist_set_row_data (clist, row, (gpointer) BSE_OBJECT_ID (item));
      bst_item_view_item_changed (item_view, BSE_OBJECT_ID (item));
      
      if (!item_view->param_view)
	bst_item_view_build_param_view (item_view);
      
      bst_item_view_can_operate (item_view, 0);
    }
}

static void
bst_item_view_item_removed (BstItemView  *item_view,
			    BseItem	 *item,
			    BseContainer *container)
{
  if (BSE_IS_ITEM (item) && g_type_is_a (BSE_OBJECT_TYPE (item), item_view->item_type))
    {
      gint row;
      GtkCList *clist = GTK_CLIST (item_view->item_clist);
      
      bsw_proxy_disconnect (BSE_OBJECT_ID (item),
			    "any_signal", bst_item_view_item_changed, item_view,
			    "any_signal", bst_item_view_item_param_changed, item_view,
			    NULL);
      
      row = gtk_clist_find_row_from_data (clist, (gpointer) BSE_OBJECT_ID (item));
      if (row >= 0)
	gtk_clist_remove (clist, row);
      
      bst_update_can_operate (GTK_WIDGET (item_view));
    }
}

static void
bst_item_view_release_container (BstItemView  *item_view)
{
  bst_item_view_set_container (item_view, 0);
}

void
bst_item_view_set_container (BstItemView *item_view,
			     BswProxy     new_container)
{
  BswProxy container;
  BswIterProxy* iter;
  
  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));
  if (new_container)
    g_return_if_fail (BSW_IS_CONTAINER (new_container));

  if (item_view->container)
    {
      container = item_view->container;
      item_view->container = 0;
      
      bst_item_view_destroy_contents (item_view);
      
      for (iter = bsw_container_list_items (container); bsw_iter_n_left (iter); bsw_iter_next (iter))
	if (g_type_is_a (bsw_proxy_type (bsw_iter_get_proxy (iter)), item_view->item_type))
	  bsw_proxy_disconnect (bsw_iter_get_proxy (iter),
				"any_signal", bst_item_view_item_changed, item_view,
				"any_signal", bst_item_view_item_param_changed, item_view,
				NULL);
      bsw_iter_free (iter);

      g_object_disconnect (bse_object_from_id (container),
			   "any_signal", bst_item_view_item_removed, item_view,
			   "any_signal", bst_item_view_item_added, item_view,
			   "any_signal", bst_item_view_release_container, item_view,
			   NULL);
    }

  if (new_container)
    g_return_if_fail (bsw_item_get_parent (new_container) != 0);
  
  item_view->container = new_container;

  if (item_view->container)
    {
      container = item_view->container;
      
      g_object_connect (bse_object_from_id (container),
			"swapped_signal::set_parent", bst_item_view_release_container, item_view,
			"swapped_signal::item_added", bst_item_view_item_added, item_view,
			"swapped_signal::item_removed", bst_item_view_item_removed, item_view,
			NULL);
      for (iter = bsw_container_list_items (container); bsw_iter_n_left (iter); bsw_iter_next (iter))
	if (g_type_is_a (bsw_proxy_type (bsw_iter_get_proxy (iter)), item_view->item_type))
	  bsw_proxy_connect (bsw_iter_get_proxy (iter),
			     "swapped_signal::seqid_changed", bst_item_view_item_changed, item_view,
			     "swapped_signal::notify", bst_item_view_item_param_changed, item_view,
			     NULL);
      bsw_iter_free (iter);

      bst_item_view_rebuild (item_view);
    }
}

static void
bst_item_view_selection_changed (BstItemView *item_view)
{
  GtkCList *clist = GTK_CLIST (item_view->item_clist);

  if (item_view->param_view)
    bst_param_view_set_object (BST_PARAM_VIEW (item_view->param_view),
			       bst_item_view_get_current (item_view));

  gtk_clist_moveto_selection (clist);
}

static void
button_action (GtkWidget *widget,
	       gpointer	  op)
{
  while (!BST_IS_ITEM_VIEW (widget))
    widget = widget->parent;
  bst_item_view_operate (BST_ITEM_VIEW (widget), GPOINTER_TO_UINT (op));
}

void
bst_item_view_rebuild (BstItemView *item_view)
{
  BswProxy container;
  GtkCList *clist;
  GtkWidget *vbox, *list_box;
  guint i;
  
  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));
  
  bst_item_view_destroy_contents (item_view);
  
  container = item_view->container;

  /* list box, containing list and action buttons
   */
  list_box = gtk_widget_new (GTK_TYPE_HBOX,
			     "homogeneous", FALSE,
			     "spacing", 5,
			     "border_width", 5,
			     "visible", TRUE,
			     NULL);
  (item_view->item_list_pos ? gtk_paned_pack2 : gtk_paned_pack1) (GTK_PANED (item_view->paned),
								  list_box,
								  FALSE,
								  FALSE);
  
  /* action buttons
   */
  vbox = gtk_widget_new (GTK_TYPE_VBOX,
			 "homogeneous", TRUE,
			 "spacing", 5,
			 "border_width", 0,
			 "visible", TRUE,
			 NULL);
  gtk_box_pack_end (GTK_BOX (list_box),
		    gtk_widget_new (GTK_TYPE_ALIGNMENT, /* don't want vexpand */
				    "visible", TRUE,
				    "xscale", 0.0,
				    "yscale", 0.0,
				    "xalign", 0.0,
				    "yalign", 0.0,
				    "child", vbox,
				    NULL),
		    FALSE, FALSE, 0);
  
  for (i = 0; i < BST_ITEM_VIEW_GET_CLASS (item_view)->n_ops; i++)
    {
      BstItemViewOp *bop = BST_ITEM_VIEW_GET_CLASS (item_view)->ops + i;
      GtkWidget *label;

      item_view->op_widgets[i] = g_object_connect (gtk_widget_new (GTK_TYPE_BUTTON,
								   "visible", TRUE,
								   "parent", vbox,
								   NULL),
						   "signal::clicked", button_action, GUINT_TO_POINTER (bop->op),
						   "signal::destroy", gtk_widget_destroyed, &item_view->op_widgets[i],
						   NULL);
      label = gtk_widget_new (GTK_TYPE_LABEL,
			      "visible", TRUE,
			      "label", bop->op_name,
			      bop->stock_icon ? NULL : "parent", item_view->op_widgets[i],
			      NULL);
      if (bop->stock_icon)
	gtk_widget_new (GTK_TYPE_VBOX,
			"visible", TRUE,
			"homogeneous", FALSE,
			"spacing", 0,
			"child", bst_image_from_stock (bop->stock_icon, BST_SIZE_BIG_BUTTON),
			"child", label,
			"parent", item_view->op_widgets[i],
			NULL);
    }
  
  /* item list
   */
  item_view->item_clist =
    g_object_connect (gtk_widget_new (GTK_TYPE_CLIST,
				      "n_columns", CLIST_N_COLUMNS,
				      "selection_mode", GTK_SELECTION_BROWSE,
				      "titles_active", FALSE,
				      "border_width", 0,
				      "height_request", 60,
				      "visible", TRUE,
				      "parent", gtk_widget_new (GTK_TYPE_SCROLLED_WINDOW,
								"visible", TRUE,
								"hscrollbar_policy", GTK_POLICY_AUTOMATIC,
								"vscrollbar_policy", GTK_POLICY_AUTOMATIC,
								"parent", list_box,
								NULL),
				      NULL),
		      "signal::destroy", gtk_widget_destroyed, &item_view->item_clist,
		      "swapped_signal::select_row", bst_item_view_selection_changed, item_view,
		      "signal_after::size_allocate", gtk_clist_moveto_selection, NULL,
		      "signal_after::map", gtk_clist_moveto_selection, NULL,
		      NULL);
  clist = GTK_CLIST (item_view->item_clist);
  gtk_clist_set_column_title (clist, CLIST_SEQID, clist_titles[CLIST_SEQID]);
  gtk_clist_set_column_title (clist, CLIST_NAME, clist_titles[CLIST_NAME]);
  gtk_clist_set_column_title (clist, CLIST_BLURB, clist_titles[CLIST_BLURB]);
  gtk_clist_set_column_auto_resize (clist, CLIST_NAME, TRUE);
  gtk_clist_column_titles_show (clist);
  gtk_clist_column_titles_passive (clist);
  
  /* param view
   */
  bst_item_view_build_param_view (item_view);
  
  bst_item_view_update (item_view);
  bst_item_view_can_operate (item_view, 0);
}

void
bst_item_view_update (BstItemView *item_view)
{
  BswProxy container;
  GtkCList *clist;
  BswIterProxy *iter;

  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));
  
  container = item_view->container;
  clist = GTK_CLIST (item_view->item_clist);
  
  gtk_clist_freeze (clist);
  gtk_clist_clear (clist);
  
  for (iter = bsw_container_list_items (container); bsw_iter_n_left (iter); bsw_iter_next (iter))
    {
      BswProxy item = bsw_iter_get_proxy (iter);

      if (g_type_is_a (bsw_proxy_type (item), item_view->item_type))
	{
	  static gchar *text[CLIST_N_COLUMNS] = { NULL, };
	  gint row;
	  
	  row = gtk_clist_insert (clist, 0, text);
	  gtk_clist_set_row_data (clist, row, (gpointer) item);
	  bst_item_view_item_changed (item_view, item);
	}
    }
  bsw_iter_free (iter);
  
  gtk_clist_thaw (clist);
  
  /* update item_view->param_view */
  bst_item_view_selection_changed (item_view);
}

void
bst_item_view_select (BstItemView *item_view,
		      BswProxy	   item)
{
  GtkCList *clist;
  gint row;
  
  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));
  g_return_if_fail (BSW_IS_ITEM (item));
  g_return_if_fail (bsw_item_get_parent (item) == item_view->container);
  
  clist = GTK_CLIST (item_view->item_clist);
  row = gtk_clist_find_row_from_data (clist, (gpointer) item);
  if (row >= 0)
    {
      gtk_clist_freeze (clist);
      gtk_clist_undo_selection (clist);
      gtk_clist_unselect_all (clist);
      gtk_clist_select_row (clist, row, 0);
      gtk_clist_thaw (clist);
    }
}

BswProxy
bst_item_view_get_current (BstItemView *item_view)
{
  BswProxy item = 0;
  GtkCList *clist;
  
  g_return_val_if_fail (BST_IS_ITEM_VIEW (item_view), 0);
  
  clist = GTK_CLIST (item_view->item_clist);
  
  if (clist->selection)
    item = (BswProxy) gtk_clist_get_row_data (clist, GPOINTER_TO_INT (clist->selection->data));
  
  if (item)
    g_return_val_if_fail (BSW_IS_ITEM (item), 0);
  
  return item;
}

void
bst_item_view_set_id_format (BstItemView *item_view,
			     const gchar *id_format)
{
  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));
  g_return_if_fail (id_format != NULL);

  g_free (item_view->id_format);
  item_view->id_format = g_strdup (id_format);
}

void
bst_item_view_operate (BstItemView *item_view,
		       BstOps	    op)
{
  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));
  g_return_if_fail (bst_item_view_can_operate (item_view, op));
  
  gtk_widget_ref (GTK_WIDGET (item_view));
  
  BST_ITEM_VIEW_GET_CLASS (item_view)->operate (item_view, op);
  
  bst_update_can_operate (GTK_WIDGET (item_view));
  
  gtk_widget_unref (GTK_WIDGET (item_view));
}

gboolean
bst_item_view_can_operate (BstItemView *item_view,
			   BstOps	op)
{
  gboolean can_do;
  guint i;
  
  g_return_val_if_fail (BST_IS_ITEM_VIEW (item_view), FALSE);
  
  gtk_widget_ref (GTK_WIDGET (item_view));
  
  if (BST_ITEM_VIEW_GET_CLASS (item_view)->operate &&
      BST_ITEM_VIEW_GET_CLASS (item_view)->can_operate)
    can_do = BST_ITEM_VIEW_GET_CLASS (item_view)->can_operate (item_view, op);
  else
    can_do = FALSE;
  
  /* update action buttons */
  for (i = 0; i < BST_ITEM_VIEW_GET_CLASS (item_view)->n_ops; i++)
    {
      BstItemViewOp *bop = BST_ITEM_VIEW_GET_CLASS (item_view)->ops + i;
      
      if (bop->op == op &&
	  item_view->op_widgets[i])
	{
	  gtk_widget_set_sensitive (item_view->op_widgets[i], can_do);
	  break;
	}
    }
  
  gtk_widget_unref (GTK_WIDGET (item_view));
  
  return can_do;
}

void
bst_item_view_set_layout (BstItemView *item_view,
			  gboolean     horizontal,
			  guint        pos)
{
  GtkWidget *child1, *child2;

  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));

  if (item_view->item_list_pos)
    {
      child2 = GTK_PANED (item_view->paned)->child1;
      child1 = GTK_PANED (item_view->paned)->child2;
    }
  else
    {
      child1 = GTK_PANED (item_view->paned)->child1;
      child2 = GTK_PANED (item_view->paned)->child2;
    }
  if (child1)
    {
      gtk_widget_ref (child1);
      gtk_container_remove (GTK_CONTAINER (child1->parent), child1);
    }
  if (child2)
    {
      gtk_widget_ref (child2);
      gtk_container_remove (GTK_CONTAINER (child2->parent), child2);
    }
  gtk_widget_destroy (item_view->paned);
  gtk_widget_unref (item_view->paned);

  item_view->item_list_pos = pos ? 1 : 0;

  item_view->paned = gtk_widget_new (horizontal ? GTK_TYPE_HPANED : GTK_TYPE_VPANED,
				     "visible", TRUE,
				     "parent", item_view,
				     NULL);
  gtk_widget_ref (item_view->paned);
  if (child1)
    {
      (item_view->item_list_pos ? gtk_paned_pack2 : gtk_paned_pack1) (GTK_PANED (item_view->paned),
								      child1,
								      FALSE,
								      FALSE);
      gtk_widget_unref (child1);
    }
  if (child2)
    {
      (item_view->item_list_pos ? gtk_paned_pack1 : gtk_paned_pack2) (GTK_PANED (item_view->paned),
								      child2,
								      TRUE,
								      TRUE);
      gtk_widget_unref (child2);
    }
}
