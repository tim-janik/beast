/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
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
      
      param_view_type = gtk_type_unique (GTK_TYPE_VBOX, &param_view_info);
    }
  
  return param_view_type;
}

static void
bst_param_view_class_init (BstParamViewClass *class)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (class);

  bst_param_view_class = class;
  parent_class = gtk_type_class (GTK_TYPE_VBOX);

  object_class->destroy = bst_param_view_destroy;
}

static void
bst_param_view_init (BstParamView *param_view)
{
  param_view->object = NULL;
  param_view->bparams = NULL;
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

  gtk_object_unref (GTK_OBJECT (param_view->tooltips));
  param_view->tooltips = NULL;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_param_view_new (BseObject *object)
{
  GtkWidget *param_view;

  g_return_val_if_fail (BSE_IS_OBJECT (object), NULL);

  param_view = gtk_widget_new (BST_TYPE_PARAM_VIEW, NULL);
  bst_param_view_set_object (BST_PARAM_VIEW (param_view), object);

  bst_param_view_rebuild (BST_PARAM_VIEW (param_view));

  return param_view;
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
      bse_object_unref (param_view->object);
      param_view->object = NULL;
    }
  param_view->object = object;
  if (param_view->object)
    bse_object_ref (param_view->object);

  for (slist = param_view->bparams; slist; slist = slist->next)
    bst_param_set_object (slist->data, object);
}

void
bst_param_view_rebuild (BstParamView *param_view)
{
  BseObject *object;
  GtkWidget *param_box;
  BseObjectClass *class;
  GSList *slist, *class_list = NULL;

  g_return_if_fail (BST_IS_PARAM_VIEW (param_view));

  param_box = GTK_WIDGET (param_view);

  bst_param_view_destroy_contents (param_view);

  if (!param_view->object)
    return;

  object = BSE_OBJECT (param_view->object);
  class = BSE_OBJECT_GET_CLASS (object);
  
  gtk_widget_set (param_box,
		  "homogeneous", FALSE,
		  "spacing", 0,
		  "border_width", 5,
		  NULL);
  
  /* parameter fields, per bse class
   */
  while (class)
    {
      class_list = g_slist_prepend (class_list, class);
      class = bse_type_class_parent (class);
    }
  
  for (slist = class_list; slist; slist = slist->next)
    {
      BseParamSpec **pspec_p;
      
      class = slist->data;
      
      pspec_p = class->param_specs;
      if (pspec_p)
	while (*pspec_p)
	  {
	    if ((*pspec_p)->any.flags & BSE_PARAM_SERVE_GUI &&
		(*pspec_p)->any.flags & BSE_PARAM_READABLE)
	      {
		BstParam *bparam;
		
		bparam = bst_param_create (object,
					   BSE_TYPE_OBJECT,
					   *pspec_p,
					   GTK_BOX (param_box),
					   GTK_TOOLTIPS (param_view->tooltips));
		param_view->bparams = g_slist_prepend (param_view->bparams, bparam);
	      }
	    pspec_p++;
	  }
    }
  g_slist_free (class_list);
  
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
