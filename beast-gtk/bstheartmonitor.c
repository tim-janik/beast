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
#include "bstheartmonitor.h"



/* --- prototypes --- */
static void	bst_heart_monitor_class_init	(BstHeartMonitorClass	*klass);
static void	bst_heart_monitor_init		(BstHeartMonitor	*pe);
static void	bst_heart_monitor_destroy	(GtkObject		*object);


/* --- static variables --- */
static gpointer              parent_class = NULL;
static BstHeartMonitorClass *bst_heart_monitor_class = NULL;


/* --- functions --- */
GtkType
bst_heart_monitor_get_type (void)
{
  static GtkType heart_monitor_type = 0;
  
  if (!heart_monitor_type)
    {
      GtkTypeInfo heart_monitor_info =
      {
	"BstHeartMonitor",
	sizeof (BstHeartMonitor),
	sizeof (BstHeartMonitorClass),
	(GtkClassInitFunc) bst_heart_monitor_class_init,
	(GtkObjectInitFunc) bst_heart_monitor_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      heart_monitor_type = gtk_type_unique (GTK_TYPE_VBOX, &heart_monitor_info);
    }
  
  return heart_monitor_type;
}

static void
bst_heart_monitor_class_init (BstHeartMonitorClass *class)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (class);

  bst_heart_monitor_class = class;
  parent_class = gtk_type_class (GTK_TYPE_VBOX);

  object_class->destroy = bst_heart_monitor_destroy;
}

static void
bst_heart_monitor_init (BstHeartMonitor *hmon)
{
  hmon->heart = NULL;
  hmon->param_view = gtk_widget_new (BST_TYPE_PARAM_VIEW, NULL);
  bst_param_view_set_mask (BST_PARAM_VIEW (hmon->param_view), BSE_TYPE_HEART, 0, NULL, NULL);
  gtk_widget_show (hmon->param_view);
  gtk_container_add (GTK_CONTAINER (hmon), hmon->param_view);
}

static void
bst_heart_monitor_destroy (GtkObject *object)
{
  BstHeartMonitor *hmon = BST_HEART_MONITOR (object);

  bst_heart_monitor_set_heart (hmon, NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_heart_monitor_new (BseHeart *heart)
{
  GtkWidget *hmon;

  g_return_val_if_fail (BSE_IS_HEART (heart), NULL);

  hmon = gtk_widget_new (BST_TYPE_HEART_MONITOR, NULL);
  bst_heart_monitor_set_heart (BST_HEART_MONITOR (hmon), heart);

  return hmon;
}

static void
heart_monitor_reset_object (BstHeartMonitor *hmon)
{
  bst_heart_monitor_set_heart (hmon, NULL);
}

void
bst_heart_monitor_set_heart (BstHeartMonitor *hmon,
			     BseHeart        *heart)
{
  g_return_if_fail (BST_IS_HEART_MONITOR (hmon));
  if (heart)
    {
      g_return_if_fail (BSE_IS_HEART (heart));
      g_return_if_fail (bst_heart_monitor_from_heart (heart) == NULL);
    }

  if (hmon->heart)
    {
      bse_object_set_data (BSE_OBJECT (hmon->heart), "BstHeartMonitor", NULL);
      bst_param_view_set_object (BST_PARAM_VIEW (hmon->param_view), NULL);
      bse_object_remove_notifiers_by_func (hmon->heart,
					   heart_monitor_reset_object,
					   hmon);
      hmon->heart = NULL;
    }
  hmon->heart = heart;
  if (hmon->heart)
    {
      bse_object_set_data (BSE_OBJECT (hmon->heart), "BstHeartMonitor", hmon);
      bse_object_add_data_notifier (hmon->heart,
				    "destroy",
				    heart_monitor_reset_object,
				    hmon);
      bst_param_view_set_object (BST_PARAM_VIEW (hmon->param_view), BSE_OBJECT (hmon->heart));
    }
}

BstHeartMonitor*
bst_heart_monitor_from_heart (BseHeart *heart)
{
  BstHeartMonitor *hmon;

  g_return_val_if_fail (BSE_IS_HEART (heart), NULL);

  hmon = bse_object_get_data (BSE_OBJECT (heart), "BstHeartMonitor");

  return hmon ? BST_HEART_MONITOR (hmon) : NULL;
}

void
bst_heart_monitor_rebuild (BstHeartMonitor *hmon)
{
  BseObject *object;
  BseObjectClass *class;
  
  g_return_if_fail (BST_IS_HEART_MONITOR (hmon));
  
  if (!hmon->heart)
    return;

  object = BSE_OBJECT (hmon->heart);
  class = BSE_OBJECT_GET_CLASS (object);
  
  bst_heart_monitor_update (hmon);
}

void
bst_heart_monitor_update (BstHeartMonitor *hmon)
{
  g_return_if_fail (BST_IS_HEART_MONITOR (hmon));

  bst_param_view_update (BST_PARAM_VIEW (hmon->param_view));
}
