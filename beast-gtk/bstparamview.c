/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
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
GType
bst_param_view_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstParamViewClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_param_view_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstParamView),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_param_view_init,
      };

      type = g_type_register_static (GTK_TYPE_VBOX,
				     "BstParamView",
				     &type_info, 0);
    }

  return type;
}

static void
bst_param_view_class_init (BstParamViewClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  bst_param_view_class = class;
  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bst_param_view_destroy;
}

static void
bst_param_view_init (BstParamView *param_view)
{
  param_view->object = 0;
  param_view->bparams = NULL;

  param_view->first_base_type = BSE_TYPE_OBJECT;
  param_view->last_base_type = 0;
  param_view->reject_pattern = NULL;
  param_view->match_pattern = NULL;
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

  bst_param_view_set_object (param_view, 0);

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

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_param_view_new (BswProxy object)
{
  GtkWidget *param_view;

  if (object)
    g_return_val_if_fail (BSW_IS_OBJECT (object), NULL);

  param_view = gtk_widget_new (BST_TYPE_PARAM_VIEW, NULL);
  if (object)
    bst_param_view_set_object (BST_PARAM_VIEW (param_view), object);

  return param_view;
}

static void
param_view_reset_object (BstParamView *param_view)
{
  bst_param_view_set_object (param_view, 0);
}

void
bst_param_view_set_object (BstParamView *param_view,
			   BswProxy      object)
{
  GSList *slist;

  g_return_if_fail (BST_IS_PARAM_VIEW (param_view));
  if (object)
    g_return_if_fail (BSW_IS_OBJECT (object));

  if (param_view->object)
    {
      bsw_proxy_disconnect (param_view->object,
			    "any_signal", param_view_reset_object, param_view,
			    NULL);
      param_view->object = 0;
      
      for (slist = param_view->bparams; slist; slist = slist->next)
	bst_param_set_object (slist->data, NULL);
    }

  param_view->object = object;

  if (param_view->object)
    bsw_proxy_connect (param_view->object,
		       "swapped_signal::destroy", param_view_reset_object, param_view,
		       NULL);
  
  bst_param_view_rebuild (param_view);
}

void
bst_param_view_set_mask (BstParamView   *param_view,
			 GType           first_base_type,
			 GType           last_base_type,
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

  param_view->first_base_type = first_base_type;
  param_view->last_base_type = last_base_type;
}

void
bst_param_view_rebuild (BstParamView *param_view)
{
  BseObject *object;
  GtkWidget *param_box;
  GParamSpec **pspecs;
  guint i, n;
  
  g_return_if_fail (BST_IS_PARAM_VIEW (param_view));

  bst_param_view_destroy_contents (param_view);

  if (!param_view->object)
    return;
  
  param_box = GTK_WIDGET (param_view);

  if (0)	/* want Wrap boxes */
    {
      param_view->container = g_object_new (GTK_TYPE_HWRAP_BOX,
					    "visible", TRUE,
					    "homogeneous", FALSE,
					    "border_width", 5,
					    "hspacing", 5,
					    "aspect_ratio", 0.0,
					    "parent", param_view,
					    NULL);
      param_view->nil_container = param_view->container;
    }
  else
    {
      GtkScrolledWindow *scrolled_window;
      GtkWidget *viewport, *separator;

      param_view->nil_container = g_object_new (GTK_TYPE_VBOX,
						"visible", TRUE,
						"homogeneous", FALSE,
						"border_width", 5,
						NULL);
      gtk_box_pack_start (GTK_BOX (param_view), param_view->nil_container, FALSE, TRUE, 0);
      separator = g_object_new (GTK_TYPE_HSEPARATOR,
				"visible", TRUE,
				NULL);
      gtk_box_pack_start (GTK_BOX (param_view), separator, FALSE, FALSE, 0);
      scrolled_window = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
				      "visible", TRUE,
				      "hscrollbar_policy", GTK_POLICY_NEVER,
				      "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
				      "parent", param_view,
				      NULL);
      viewport = g_object_new (GTK_TYPE_VIEWPORT,
			       "visible", TRUE,
			       "shadow_type", GTK_SHADOW_NONE,
			       "hadjustment", gtk_scrolled_window_get_hadjustment (scrolled_window),
			       "vadjustment", gtk_scrolled_window_get_vadjustment (scrolled_window),
			       "parent", scrolled_window,
			       NULL);
      bst_widget_request_aux_info (viewport);
      param_view->container = g_object_new (GTK_TYPE_VBOX,
					    "visible", TRUE,
					    "homogeneous", FALSE,
					    "border_width", 5,
					    "parent", viewport,
					    NULL);
    }
    
  g_object_connect (param_view->container,
		    "swapped_signal::destroy", g_nullify_pointer, &param_view->container,
		    NULL);
  g_object_connect (param_view->nil_container,
		    "swapped_signal::destroy", g_nullify_pointer, &param_view->nil_container,
		    NULL);
  
  object = bse_object_from_id (param_view->object);
  
  /* parameter fields, per bse class
   */
  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (object), &n);
  for (i = 0; i < n; i++)
    {
      GParamSpec *pspec = pspecs[i];
      gchar *param_group = bse_param_spec_get_group (pspec);
      
      if ((param_view->first_base_type && !g_type_is_a (pspec->owner_type, param_view->first_base_type)) ||
	  (param_view->last_base_type && !g_type_is_a (param_view->last_base_type, pspec->owner_type)))
	continue;
      
      if ((pspec->flags & BSE_PARAM_SERVE_GUI) &&
	  (pspec->flags & BSE_PARAM_READABLE) &&
	  (!param_view->reject_pattern ||
	   !g_pattern_match_string (param_view->reject_pattern, pspec->name)) &&
	  (!param_view->match_pattern ||
	   g_pattern_match_string (param_view->match_pattern, pspec->name)))
	{
	  BstParam *bparam;
	  
	  bparam = bst_param_create (object,
				     BSE_TYPE_OBJECT,
				     pspec,
				     param_group,
				     param_group ? param_view->container : param_view->nil_container,
				     BST_TOOLTIPS);
	  param_view->bparams = g_slist_prepend (param_view->bparams, bparam);
	}
    }
  g_free (pspecs);
  
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
