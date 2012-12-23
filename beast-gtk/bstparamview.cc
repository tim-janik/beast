/* BEAST - Better Audio System
 * Copyright (C) 1998-2002 Tim Janik
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
#include "bstparamview.hh"

#include "bstparam.hh"


/* --- prototypes --- */
static void	bst_param_view_destroy		(GtkObject		*object);
static void	bst_param_view_finalize		(GObject		*object);


/* --- static variables --- */
static BstParamViewClass *bst_param_view_class = NULL;


/* --- functions --- */
G_DEFINE_TYPE (BstParamView, bst_param_view, GTK_TYPE_VBOX);

static void
bst_param_view_class_init (BstParamViewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);

  bst_param_view_class = klass;

  gobject_class->finalize = bst_param_view_finalize;

  object_class->destroy = bst_param_view_destroy;
}

static void
bst_param_view_init (BstParamView *self)
{
  self->item = 0;
  self->params = NULL;

  self->first_base_type = g_strdup ("BseObject");
  self->last_base_type = NULL;
  self->reject_pattern = NULL;
  self->match_pattern = NULL;
  gtk_widget_show (GTK_WIDGET (self));
}

static void
bst_param_view_destroy_contents (BstParamView *self)
{
  gtk_container_foreach (GTK_CONTAINER (self), (GtkCallback) gtk_widget_destroy, NULL);
  while (self->params)
    gxk_param_destroy ((GxkParam*) g_slist_pop_head (&self->params));
}

static void
bst_param_view_destroy (GtkObject *object)
{
  BstParamView *self = BST_PARAM_VIEW (object);

  bst_param_view_destroy_contents (self);

  bst_param_view_set_item (self, 0);

  GTK_OBJECT_CLASS (bst_param_view_parent_class)->destroy (object);
}

static void
bst_param_view_finalize (GObject *object)
{
  BstParamView *self = BST_PARAM_VIEW (object);

  bst_param_view_set_item (self, 0);

  g_free (self->first_base_type);
  g_free (self->last_base_type);

  if (self->reject_pattern)
    {
      g_pattern_spec_free (self->reject_pattern);
      self->reject_pattern = NULL;
    }
  if (self->match_pattern)
    {
      g_pattern_spec_free (self->match_pattern);
      self->match_pattern = NULL;
    }

  G_OBJECT_CLASS (bst_param_view_parent_class)->finalize (object);
}

GtkWidget*
bst_param_view_new (SfiProxy item)
{
  GtkWidget *self = gtk_widget_new (BST_TYPE_PARAM_VIEW, NULL);

  if (item)
    g_return_val_if_fail (BSE_IS_ITEM (item), NULL);

  if (item)
    bst_param_view_set_item (BST_PARAM_VIEW (self), item);

  return self;
}

static void
param_view_reset_item (BstParamView *self)
{
  bst_param_view_set_item (self, 0);
}

void
bst_param_view_set_item (BstParamView *self,
			 SfiProxy      item)
{
  GSList *slist;

  g_return_if_fail (BST_IS_PARAM_VIEW (self));
  if (item)
    g_return_if_fail (BSE_IS_ITEM (item));

  if (item == self->item)
    return;

  if (self->item)
    {
      bse_proxy_disconnect (self->item,
			    "any_signal", param_view_reset_item, self,
			    NULL);
      self->item = 0;
      
      for (slist = self->params; slist; slist = slist->next)
	bst_param_set_proxy ((GxkParam*) slist->data, 0);
    }

  self->item = item;

  if (self->item)
    bse_proxy_connect (self->item,
		       "swapped_signal::release", param_view_reset_item, self,
		       NULL);
  
  bst_param_view_rebuild (self);
}

void
bst_param_view_set_mask (BstParamView *param_view,
			 const gchar  *first_base_type,
			 const gchar  *last_base_type,
			 const gchar  *reject_pattern,
			 const gchar  *match_pattern)
{
  g_return_if_fail (BST_IS_PARAM_VIEW (param_view));

  if (param_view->reject_pattern)
    g_pattern_spec_free (param_view->reject_pattern);
  param_view->reject_pattern = reject_pattern ? g_pattern_spec_new (reject_pattern) : NULL;

  if (param_view->match_pattern)
    g_pattern_spec_free (param_view->match_pattern);
  param_view->match_pattern = match_pattern ? g_pattern_spec_new (match_pattern) : NULL;

  g_free (param_view->first_base_type);
  param_view->first_base_type = g_strdup (first_base_type);
  g_free (param_view->last_base_type);
  param_view->last_base_type = g_strdup (last_base_type);
}

void
bst_param_view_apply_defaults (BstParamView *self)
{
  g_return_if_fail (BST_IS_PARAM_VIEW (self));

  if (self->item)
    {
      GSList *slist;
      bse_item_group_undo (self->item, "Reset to defaults");
      for (slist = self->params; slist; slist = slist->next)
        gxk_param_apply_default ((GxkParam*) slist->data);
      bse_item_ungroup_undo (self->item);
    }
}

void
bst_param_view_rebuild (BstParamView *self)
{
  GtkBox *pbox;
  GtkWidget *ncontainer = NULL, *gcontainer = NULL;
  const gchar **pstrings;
  GSList *slist;
  gint border_width = 5;
  guint i, n;
  
  g_return_if_fail (BST_IS_PARAM_VIEW (self));

  bst_param_view_destroy_contents (self);

  pbox = GTK_BOX (self);
  if (!self->item)
    return;

  /* create parameter fields */
  pstrings = bse_proxy_list_properties (self->item, self->first_base_type, self->last_base_type, &n);
  for (i = 0; i < n; i++)
    if ((!self->reject_pattern || !g_pattern_match_string (self->reject_pattern, pstrings[i])) &&
	(!self->match_pattern || g_pattern_match_string (self->reject_pattern, pstrings[i])))
      {
        GParamSpec *pspec = bse_proxy_get_pspec (self->item, pstrings[i]);
	const gchar *param_group = sfi_pspec_get_group (pspec);
	if (sfi_pspec_check_option (pspec, "G") && /* GUI representable */
            ((pspec->flags & G_PARAM_WRITABLE) || BST_DVL_HINTS))
	  {
            GxkParam *param = bst_param_new_proxy (pspec, self->item);
            if (param_group)
              {
                if (!gcontainer)
                  gcontainer = (GtkWidget*) g_object_new (GTK_TYPE_VBOX, NULL);
                bst_param_create_gmask (param, NULL, gcontainer);
              }
            else
              {
                if (!ncontainer)
                  ncontainer = (GtkWidget*) g_object_new (GTK_TYPE_VBOX, NULL);
                bst_param_create_gmask (param, NULL, ncontainer);
              }
            self->params = g_slist_prepend (self->params, param);
	  }
      }

  /* pack groupless parameters */
  if (ncontainer)
    {
      g_object_set (ncontainer,
		    "visible", TRUE,
		    "homogeneous", FALSE,
		    "border_width", border_width,
		    NULL);
      gtk_box_pack_start (pbox, ncontainer, FALSE, TRUE, 0);
    }

  /* pack grouped parameters */
  if (gcontainer)
    {
      if (ncontainer)
	gtk_box_pack_start (pbox, (GtkWidget*) g_object_new (GTK_TYPE_HSEPARATOR,
                                                             "visible", TRUE,
                                                             NULL),
			    FALSE, FALSE, 0);
      g_object_set (gcontainer,
		    "visible", TRUE,
		    "homogeneous", FALSE,
		    "border_width", border_width,
		    NULL);
      GtkWidget *scwin = gxk_scrolled_window_create (gcontainer, GTK_SHADOW_NONE, 1, 0.8);
      gtk_container_add (GTK_CONTAINER (self), scwin);
    }
  
  /* refresh parameter fields */
  for (slist = self->params; slist; slist = slist->next)
    gxk_param_update ((GxkParam*) slist->data);
}
