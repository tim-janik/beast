/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
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
#include "bstparamview.h"

#include "bstparam.h"


/* --- prototypes --- */
static void	bst_param_view_class_init	(BstParamViewClass	*klass);
static void	bst_param_view_init		(BstParamView		*pe);
static void	bst_param_view_destroy		(GtkObject		*object);
static void	bst_param_view_finalize		(GObject		*object);


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
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  bst_param_view_class = class;
  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = bst_param_view_finalize;

  object_class->destroy = bst_param_view_destroy;
}

static void
bst_param_view_init (BstParamView *param_view)
{
  param_view->item = 0;
  param_view->bparams = NULL;

  param_view->first_base_type = g_strdup ("BseObject");
  param_view->last_base_type = NULL;
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

  bst_param_view_set_item (param_view, 0);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_param_view_finalize (GObject *object)
{
  BstParamView *param_view = BST_PARAM_VIEW (object);

  bst_param_view_set_item (param_view, 0);

  g_free (param_view->first_base_type);
  g_free (param_view->last_base_type);

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

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget*
bst_param_view_new (SfiProxy item)
{
  GtkWidget *param_view;

  if (item)
    g_return_val_if_fail (BSE_IS_ITEM (item), NULL);

  param_view = gtk_widget_new (BST_TYPE_PARAM_VIEW, NULL);
  if (item)
    bst_param_view_set_item (BST_PARAM_VIEW (param_view), item);

  return param_view;
}

static void
param_view_reset_item (BstParamView *param_view)
{
  bst_param_view_set_item (param_view, 0);
}

void
bst_param_view_set_item (BstParamView *param_view,
			 SfiProxy      item)
{
  GSList *slist;

  g_return_if_fail (BST_IS_PARAM_VIEW (param_view));
  if (item)
    g_return_if_fail (BSE_IS_ITEM (item));

  if (param_view->item)
    {
      bse_proxy_disconnect (param_view->item,
			    "any_signal", param_view_reset_item, param_view,
			    NULL);
      param_view->item = 0;
      
      for (slist = param_view->bparams; slist; slist = slist->next)
	bst_param_set_proxy (slist->data, 0);
    }

  param_view->item = item;

  if (param_view->item)
    bse_proxy_connect (param_view->item,
		       "swapped_signal::release", param_view_reset_item, param_view,
		       NULL);
  
  bst_param_view_rebuild (param_view);
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
bst_param_view_rebuild (BstParamView *param_view)
{
  GtkWidget *param_box;
  const gchar **pstrings;
  GSList *slist;
  guint i, n;
  
  g_return_if_fail (BST_IS_PARAM_VIEW (param_view));

  bst_param_view_destroy_contents (param_view);

  if (!param_view->item)
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
  
  /* parameter fields, per bse class
   */
  pstrings = bse_proxy_list_properties (param_view->item, param_view->first_base_type, param_view->last_base_type, &n);
  for (i = 0; i < n; i++)
    if ((!param_view->reject_pattern || !g_pattern_match_string (param_view->reject_pattern, pstrings[i])) &&
	(!param_view->match_pattern || g_pattern_match_string (param_view->reject_pattern, pstrings[i])))
      {
	GParamSpec *pspec = bse_proxy_get_pspec (param_view->item, pstrings[i]);
	const gchar *param_group = sfi_pspec_get_group (pspec);

	if (sfi_pspec_test_hint (pspec, SFI_PARAM_SERVE_GUI) && (pspec->flags & G_PARAM_READABLE))
	  {
	    BstParam *bparam = bst_param_proxy_create (pspec, FALSE, NULL, param_view->item);
	    bst_param_pack_property (bparam, param_group ? param_view->container : param_view->nil_container);
	    param_view->bparams = g_slist_prepend (param_view->bparams, bparam);
	  }
      }
  for (slist = param_view->bparams; slist; slist = slist->next)
    bst_param_update (slist->data);
}
