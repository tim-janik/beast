/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
static void	bst_item_view_init		(BstItemView		*self,
						 BstItemViewClass	*real_class);
static void	bst_item_view_destroy		(GtkObject		*object);
static void	bst_item_view_finalize		(GObject		*object);
static void	bst_item_view_create_tree	(BstItemView		*self);
static void	item_view_listen_on		(BstItemView		*self,
						 SfiProxy		 item);
static void	item_view_unlisten_on		(BstItemView		*self,
						 SfiProxy		 item);


/* --- columns --- */
enum {
  COL_SEQID,
  COL_NAME,
  COL_BLURB,
  N_COLS
};


/* --- static variables --- */
static gpointer		 parent_class = NULL;


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
  
  parent_class = gtk_type_class (GTK_TYPE_ALIGNMENT);
  
  G_OBJECT_CLASS (object_class)->finalize = bst_item_view_finalize;

  object_class->destroy = bst_item_view_destroy;
  
  class->n_ops = 0;
  class->ops = NULL;
  class->show_properties = TRUE;
  
  class->operate = NULL;
  class->can_operate = NULL;
  class->create_tree = bst_item_view_create_tree;
  class->listen_on = item_view_listen_on;
  class->unlisten_on = item_view_unlisten_on;
}

static void
button_action (GtkWidget *widget,
	       gpointer	  op)
{
  while (!BST_IS_ITEM_VIEW (widget))
    widget = widget->parent;
  bst_item_view_operate (BST_ITEM_VIEW (widget), GPOINTER_TO_UINT (op));
}

static void
bst_item_view_init (BstItemView      *self,
		    BstItemViewClass *ITEM_VIEW_CLASS)
{
  GtkWidget *tool_box;
  gboolean vpack = ITEM_VIEW_CLASS->horizontal_ops;
  guint i;

  /* action buttons */
  self->op_widgets = g_new0 (GtkWidget*, ITEM_VIEW_CLASS->n_ops);
  self->tools = g_object_new (GTK_TYPE_ALIGNMENT, /* don't want tool buttons to resize */
			      "visible", TRUE,
			      "xscale", 0.0,
			      "yscale", 0.0,
			      "xalign", vpack ? 0.0 : 0.5,
			      "yalign", vpack ? 0.5 : 0.0,
			      NULL);
  tool_box = g_object_new (vpack ? GTK_TYPE_HBOX : GTK_TYPE_VBOX,
			   "homogeneous", TRUE,
			   "spacing", 3,
			   "parent", self->tools,
			   NULL);
  for (i = 0; i < ITEM_VIEW_CLASS->n_ops; i++)
    {
      BstItemViewOp *bop = ITEM_VIEW_CLASS->ops + i;
      GtkWidget *label = g_object_new (GTK_TYPE_LABEL,
				       "label", bop->op_name,
				       NULL);
      self->op_widgets[i] = g_object_new (GTK_TYPE_BUTTON,
					  "can_focus", FALSE,
					  "parent", tool_box,
					  NULL);
      g_object_connect (self->op_widgets[i],
			"signal::clicked", button_action, GUINT_TO_POINTER (bop->op),
			"signal::destroy", gtk_widget_destroyed, &self->op_widgets[i],
			NULL);
      if (!bop->stock_icon)
	gtk_container_add (GTK_CONTAINER (self->op_widgets[i]), label);
      else
	g_object_new (GTK_TYPE_VBOX,
		      "homogeneous", FALSE,
		      "spacing", 0,
		      "child", gxk_stock_image (bop->stock_icon, BST_SIZE_BIG_BUTTON),
		      "child", label,
		      "parent", self->op_widgets[i],
		      NULL);
      if (bop->tooltip)
	gtk_tooltips_set_tip (GXK_TOOLTIPS, self->op_widgets[i], bop->tooltip, NULL);
    }
  gtk_widget_show_all (self->tools);
  
  /* pack list view + button box */
  self->paned = g_object_new (GTK_TYPE_VPANED,
			      "parent", self,
			      NULL);
  gxk_nullify_on_destroy (self->paned, &self->paned);
  
  /* property view */
  if (ITEM_VIEW_CLASS->show_properties)
    {
      self->pview = bst_param_view_new (0);
      bst_param_view_set_mask (BST_PARAM_VIEW (self->pview), "BseItem", 0, NULL, NULL);
      gxk_nullify_on_destroy (self->pview, &self->pview);
      gtk_paned_pack2 (self->paned, self->pview, TRUE, TRUE);
    }
  
  /* show the sutter */
  gtk_widget_show_all (GTK_WIDGET (self));
  
  self->item_type = NULL;
  self->container = 0;
  self->id_format = g_strdup ("%03u");
}

static void
bst_item_view_destroy (GtkObject *object)
{
  BstItemView *self = BST_ITEM_VIEW (object);

  bst_item_view_set_container (self, 0);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_item_view_finalize (GObject *object)
{
  BstItemView *self = BST_ITEM_VIEW (object);

  bst_item_view_set_container (self, 0);

  g_free (self->id_format);
  g_free (self->op_widgets);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
item_view_fill_value (BstItemView *self,
		      guint        column,
		      guint        row,
		      GValue      *value)
{
  guint seqid = row + 1;
  switch (column)
    {
      const gchar *string;
      SfiProxy item;
    case COL_SEQID:
      g_value_set_string_take_ownership (value, g_strdup_printf (self->id_format, seqid));
      break;
    case COL_NAME:
      item = bse_container_get_item (self->container, self->item_type, seqid);
      g_value_set_string (value, bse_item_get_name (item));
      break;
    case COL_BLURB:
      item = bse_container_get_item (self->container, self->item_type, seqid);
      bse_proxy_get (item, "blurb", &string, NULL);
      g_value_set_string (value, string ? string : "");
      break;
    }
}

void
bst_item_view_name_edited (BstItemView *self,
			   const gchar *strpath,
			   const gchar *text)
{
  g_return_if_fail (BST_IS_ITEM_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      guint seqid = row + 1;
      SfiProxy item = bse_container_get_item (self->container, self->item_type, seqid);
      if (item)
	bse_item_set_name (item, text);
    }
}

void
bst_item_view_blurb_edited (BstItemView *self,
			    const gchar *strpath,
			    const gchar *text)
{
  g_return_if_fail (BST_IS_ITEM_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      guint seqid = row + 1;
      SfiProxy item = bse_container_get_item (self->container, self->item_type, seqid);
      if (item)
	bse_proxy_set (item, "blurb", text, NULL);
    }
}

static void
bst_item_view_create_tree (BstItemView *self)
{
  GtkWidget *scwin;
  GtkTreeSelection *tsel;

  /* item list model */
  self->wlist = gxk_list_wrapper_new (N_COLS,
				      G_TYPE_STRING,	/* COL_SEQID */
				      G_TYPE_STRING,	/* COL_NAME */
				      G_TYPE_STRING	/* COL_BLURB */
				      );
  g_signal_connect_object (self->wlist, "fill-value",
			   G_CALLBACK (item_view_fill_value),
			   self, G_CONNECT_SWAPPED);
  
  /* item list view */
  scwin = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
			"hscrollbar_policy", GTK_POLICY_AUTOMATIC,
			"vscrollbar_policy", GTK_POLICY_ALWAYS,
			"border_width", 0,
			"shadow_type", GTK_SHADOW_IN,
			NULL);
  self->tree = g_object_new (GTK_TYPE_TREE_VIEW,
			     "can_focus", TRUE,
			     "model", self->wlist,
			     "border_width", 5,
			     "parent", scwin,
			     "height_request", BST_ITEM_VIEW_TREE_HEIGHT,
			     NULL);
  gxk_nullify_on_destroy (self->tree, &self->tree);
  tsel = gtk_tree_view_get_selection (self->tree);
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_BROWSE);
  gxk_tree_selection_force_browse (tsel, GTK_TREE_MODEL (self->wlist));

  /* add list view columns */
  gxk_tree_view_append_text_columns (self->tree, 1,
				     COL_SEQID, 0.0, "ID");
  gxk_tree_view_add_text_column (self->tree,
				 COL_NAME, 0.0, "Name", NULL,
				 bst_item_view_name_edited, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_text_column (self->tree,
				 COL_BLURB, 0.0, "Comment", NULL,
				 bst_item_view_blurb_edited, self, G_CONNECT_SWAPPED);

  /* make widgets visible */
  gtk_widget_show_all (scwin);
}

static void
pview_selection_changed (BstItemView *self)
{
  if (self->pview)
    bst_param_view_set_item (BST_PARAM_VIEW (self->pview),
			     bst_item_view_get_current (self));
}

static void
complete_tree (BstItemView *self)
{
  if (!self->wlist && !self->tree)
    {
      BST_ITEM_VIEW_GET_CLASS (self)->create_tree (self);
      if (self->tree)
	{
	  GtkWidget *widget = GTK_WIDGET (self->tree);
	  while (widget->parent)
	    widget = widget->parent;

	  /* update property editor */
	  g_object_connect (gtk_tree_view_get_selection (self->tree),
			    "swapped_object_signal::changed", pview_selection_changed, self,
			    NULL);

	  /* pack list view + button box */
	  if (!self->tools->parent)
	    {
	      gboolean vpack = BST_ITEM_VIEW_GET_CLASS (self)->horizontal_ops;
	      GtkWidget *lbox = g_object_new (vpack ? GTK_TYPE_VBOX : GTK_TYPE_HBOX,
					      "visible", TRUE,
					      "border_width", 3,
					      "spacing", 3,
					      NULL);
	      if (!vpack)
		gtk_box_pack_start (GTK_BOX (lbox), widget, TRUE, TRUE, 0);
	      gtk_box_pack_start (GTK_BOX (lbox), self->tools, FALSE, FALSE, 0);
	      if (vpack)
		gtk_box_pack_start (GTK_BOX (lbox), widget, TRUE, TRUE, 0);
	      gtk_paned_pack1 (self->paned, lbox, FALSE, FALSE);
	    }
	  else
	    gtk_paned_pack1 (self->paned, widget, FALSE, FALSE);

	  /* adapt param view */
	  pview_selection_changed (self);
	}
    }
}

static void
item_property_notify (SfiProxy     item,
		      const gchar *property_name,
		      BstItemView *self)
{
  if (self->wlist)
    {
      gint row = bse_item_get_seqid (item) - 1;
      gxk_list_wrapper_notify_change (GXK_LIST_WRAPPER (self->wlist), row);
    }
}

static void
item_changed (SfiProxy     item,
	      BstItemView *self)
{
  item_property_notify (item, NULL, self);
}

static void
item_view_listen_on (BstItemView *self,
		     SfiProxy     item)
{
  bse_proxy_connect (item,
		     "signal::seqid-changed", item_changed, self,
		     "signal::property-notify", item_property_notify, self,
		     NULL);
}

static void
item_view_unlisten_on (BstItemView *self,
		       SfiProxy     item)
{
  bse_proxy_disconnect (item,
			"any_signal", item_changed, self,
			"any_signal", item_property_notify, self,
			NULL);
}

static void
item_view_item_added (SfiProxy     container,
		      SfiProxy     item,
		      BstItemView *self)
{
  if (self->wlist && BSE_IS_ITEM (item) && bse_proxy_is_a (item, self->item_type))
    {
      gint row = bse_item_get_seqid (item) - 1;
      gxk_list_wrapper_notify_insert (self->wlist, row);
      BST_ITEM_VIEW_GET_CLASS (self)->listen_on (self, item);
      bst_item_view_can_operate (self, 0);
    }
}

static void
item_view_item_removed (SfiProxy     container,
			SfiProxy     item,
			gint         seqid,
			BstItemView *self)
{
  if (self->wlist && BSE_IS_ITEM (item) && bse_proxy_is_a (item, self->item_type))
    {
      gint row = seqid - 1;
      gxk_list_wrapper_notify_delete (self->wlist, row);
      BST_ITEM_VIEW_GET_CLASS (self)->unlisten_on (self, item);
      bst_update_can_operate (GTK_WIDGET (self));
    }
}

static void
bst_item_view_release_container (BstItemView  *item_view)
{
  bst_item_view_set_container (item_view, 0);
}

void
bst_item_view_set_container (BstItemView *self,
			     SfiProxy     new_container)
{
  g_return_if_fail (BST_IS_ITEM_VIEW (self));
  if (new_container)
    g_return_if_fail (BSE_IS_CONTAINER (new_container));

  if (self->container)
    {
      BseProxySeq *pseq = bse_container_list_items (self->container);
      guint i;
      for (i = 0; i < pseq->n_proxies; i++)
	if (bse_proxy_is_a (pseq->proxies[i], self->item_type))
	  BST_ITEM_VIEW_GET_CLASS (self)->unlisten_on (self, pseq->proxies[i]);
      bse_proxy_disconnect (self->container,
			    "any_signal", item_view_item_removed, self,
			    "any_signal", item_view_item_added, self,
			    "any_signal", bst_item_view_release_container, self,
			    NULL);
      if (self->wlist)
	gxk_list_wrapper_notify_clear (self->wlist);
      if (self->pview)
	bst_param_view_set_item (BST_PARAM_VIEW (self->pview), 0);
    }
  self->container = new_container;
  if (self->container)
    {
      BseProxySeq *pseq = bse_container_list_items (self->container);
      guint i;
      bse_proxy_connect (self->container,
			 "swapped_signal::release", bst_item_view_release_container, self,
			 "signal::item_added", item_view_item_added, self,
			 "signal::item_removed", item_view_item_removed, self,
			 NULL);
      complete_tree (self);
      for (i = 0; i < pseq->n_proxies; i++)
	if (bse_proxy_is_a (pseq->proxies[i], self->item_type))
	  {
	    BST_ITEM_VIEW_GET_CLASS (self)->listen_on (self, pseq->proxies[i]);
	    if (self->wlist)
	      gxk_list_wrapper_notify_append (self->wlist, 1);
	  }
    }
}

void
bst_item_view_select (BstItemView *self,
		      SfiProxy	   item)
{
  g_return_if_fail (BST_IS_ITEM_VIEW (self));
  g_return_if_fail (BSE_IS_ITEM (item));

  if (self->tree && bse_item_get_parent (item) == self->container)
    {
      gint row = bse_item_get_seqid (item) - 1;
      gxk_tree_selection_select_ipath (gtk_tree_view_get_selection (self->tree),
				       row, -1);
    }
}

SfiProxy
bst_item_view_get_current (BstItemView *self)
{
  SfiProxy item = 0;
  GtkTreeIter iter;

  g_return_val_if_fail (BST_IS_ITEM_VIEW (self), 0);

  if (self->tree && gtk_tree_selection_get_selected (gtk_tree_view_get_selection (self->tree),
						     NULL, &iter))
    {
      guint row = gxk_list_wrapper_get_index (self->wlist, &iter);
      guint seqid = row + 1;
      item = bse_container_get_item (self->container, self->item_type, seqid);
    }
  if (item)
    g_return_val_if_fail (BSE_IS_ITEM (item), 0);
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
