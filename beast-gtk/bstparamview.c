/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Tim Janik and Red Hat, Inc.
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
#include "bstparamview.h"



/* --- prototypes --- */
static void	bst_param_view_class_init	(BstParamViewClass	*klass);
static void	bst_param_view_init		(BstParamView		*pe);
static void	bst_param_view_destroy		(GtkObject		*object);


/* --- static variables --- */
static gpointer           parent_class = NULL;
static BstParamViewClass *bst_param_view_class = NULL;


/* --- functions --- */
GtkType
bst_param_view_get_type (void)
{
  static GtkType param_view_type = 0;
  
  if (!param_view_type)
    {
      GtkTypeInfo param_view_info =
      {
	"BstParamView",
	sizeof (BstParamView),
	sizeof (BstParamViewClass),
	(GtkClassInitFunc) bst_param_view_class_init,
	(GtkObjectInitFunc) bst_param_view_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      param_view_type = gtk_type_unique (GTK_TYPE_VWRAP_BOX, &param_view_info);
    }
  
  return param_view_type;
}

static void
bst_param_view_class_init (BstParamViewClass *class)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (class);

  bst_param_view_class = class;
  parent_class = gtk_type_class (GTK_TYPE_VWRAP_BOX);

  object_class->destroy = bst_param_view_destroy;
}

static void
bst_param_view_init (BstParamView *param_view)
{
  param_view->object = NULL;
  param_view->bparams = NULL;

  param_view->base_type = BSE_TYPE_OBJECT;
  param_view->object_type = 0;
  param_view->reject_pattern = NULL;
  param_view->match_pattern = NULL;

  param_view->tooltips = gtk_tooltips_new ();
  gtk_object_ref (GTK_OBJECT (param_view->tooltips));
  gtk_object_sink (GTK_OBJECT (param_view->tooltips));
}

static void
bst_param_view_destroy_contents (BstParamView *param_view)
{
  gtk_container_foreach (GTK_CONTAINER (param_view), (GtkCallback) gtk_widget_destroy, NULL);
  g_slist_free (param_view->bparams);
  param_view->bparams = NULL;
}

static void
bst_param_view_destroy (GtkObject *object)
{
  BstParamView *param_view;

  g_return_if_fail (object != NULL);

  param_view = BST_PARAM_VIEW (object);

  bst_param_view_destroy_contents (param_view);

  bst_param_view_set_object (param_view, NULL);

  if (param_view->reject_pattern)
    {
      g_pattern_spec_free (param_view->reject_pattern);
      param_view->reject_pattern = NULL;
    }
  if (param_view->match_pattern)
    {
      g_pattern_spec_free (param_view->match_pattern);
      param_view->match_pattern = NULL;
    }

  gtk_object_unref (GTK_OBJECT (param_view->tooltips));
  param_view->tooltips = NULL;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_param_view_new (BseObject *object)
{
  GtkWidget *param_view;

  if (object)
    g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);

  param_view = gtk_widget_new (BST_TYPE_PARAM_VIEW, NULL);
  if (object)
    bst_param_view_set_object (BST_PARAM_VIEW (param_view), object);

  return param_view;
}

static void
param_view_reset_object (BstParamView *param_view)
{
  bst_param_view_set_object (param_view, NULL);
}

void
bst_param_view_set_object (BstParamView *param_view,
			   BseObject    *object)
{
  GSList *slist;

  g_return_if_fail (BST_IS_PARAM_VIEW (param_view));
  if (object)
    g_return_if_fail (BSE_IS_OBJECT (object));

  if (param_view->object)
    {
      bse_object_remove_notifiers_by_func (param_view->object,
					   param_view_reset_object,
					   param_view);
      param_view->object = NULL;
      
      for (slist = param_view->bparams; slist; slist = slist->next)
	bst_param_set_object (slist->data, NULL);
    }

  param_view->object = object;

  if (param_view->object)
    bse_object_add_data_notifier (param_view->object,
				  "destroy",
				  param_view_reset_object,
				  param_view);

  bst_param_view_rebuild (param_view);
}

void
bst_param_view_set_mask (BstParamView   *param_view,
			 GType           base_type,
			 GType           param_object_type,
			 const gchar    *reject_pattern,
			 const gchar    *match_pattern)
{
  g_return_if_fail (BST_IS_PARAM_VIEW (param_view));

  if (param_view->reject_pattern)
    g_pattern_spec_free (param_view->reject_pattern);
  param_view->reject_pattern = reject_pattern ? g_pattern_spec_new (reject_pattern) : NULL;

  if (param_view->match_pattern)
    g_pattern_spec_free (param_view->match_pattern);
  param_view->match_pattern = match_pattern ? g_pattern_spec_new (match_pattern) : NULL;

  param_view->base_type = base_type;
  param_view->object_type = param_object_type;
}

void
bst_param_view_rebuild (BstParamView *param_view)
{
  BseObject *object;
  GtkWidget *param_box;
  BseObjectClass *class;
  GSList *slist, *class_list = NULL;

  g_return_if_fail (BST_IS_PARAM_VIEW (param_view));

  bst_param_view_destroy_contents (param_view);

  if (!param_view->object)
    return;
  
  param_box = GTK_WIDGET (param_view);
  gtk_widget_set (param_box,
		  "visible", TRUE,
		  "homogeneous", FALSE,
		  "border_width", 5,
		  "hspacing", 5,
		  "aspect_ratio", 0.0,
		  NULL);
  
  object = BSE_OBJECT (param_view->object);
  class = BSE_OBJECT_GET_CLASS (object);
  
  /* parameter fields, per bse class
   */
  while (class && g_type_is_a (BSE_CLASS_TYPE (class), param_view->base_type))
    {
      if (!param_view->object_type || g_type_is_a (param_view->object_type, BSE_CLASS_TYPE (class)))
	class_list = g_slist_prepend (class_list, class);
      class = g_type_class_peek_parent (class);
    }
  
  for (slist = class_list; slist; slist = slist->next)
    {
      guint i, n;
      
      class = slist->data;
      n = bse_object_class_get_n_param_specs (class);
      for (i = 0; i < n; i++)
	{
	  GQuark param_group;
	  BseParamSpec *pspec = bse_object_class_get_param_spec (class, i, &param_group);
	  
	  if (pspec->any.flags & BSE_PARAM_SERVE_GUI &&
	      pspec->any.flags & BSE_PARAM_READABLE &&
	      (!param_view->reject_pattern ||
	       !g_pattern_match_string (param_view->reject_pattern, pspec->any.name)) &&
	      (!param_view->match_pattern ||
	       g_pattern_match_string (param_view->match_pattern, pspec->any.name)))
	    {
	      BstParam *bparam;
	      
	      bparam = bst_param_create (object,
					 BSE_TYPE_OBJECT,
					 pspec,
					 param_group,
					 param_box,
					 GTK_TOOLTIPS (param_view->tooltips));
	      param_view->bparams = g_slist_prepend (param_view->bparams, bparam);
	    }
	}
    }
  g_slist_free (class_list);
  
  /* FIXME: work around a gtk bug (<= 1.2.6)
   */
  if (GTK_WIDGET_REALIZED (param_view) && !GTK_CHECK_VERSION (1, 2, 7))
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (param_view));
      
      gtk_container_check_resize (GTK_CONTAINER (toplevel));
      gtk_container_check_resize (GTK_CONTAINER (toplevel));
      gtk_container_check_resize (GTK_CONTAINER (toplevel));
      gtk_widget_queue_resize (toplevel);
    }
  
  bst_param_view_update (param_view);
}

void
bst_param_view_update (BstParamView *param_view)
{
  GSList *slist;

  g_return_if_fail (BST_IS_PARAM_VIEW (param_view));

  for (slist = param_view->bparams; slist; slist = slist->next)
    bst_param_get (slist->data);
}
