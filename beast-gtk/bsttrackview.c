/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
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
#include "bsttrackview.h"



/* --- prototypes --- */
static void	bst_track_view_class_init	(BstTrackViewClass	*klass);
static void	bst_track_view_init		(BstTrackView		*self);
static void	bst_track_view_operate		(BstItemView		*item_view,
						 BstOps			 op);
static gboolean	bst_track_view_can_operate	(BstItemView		*item_view,
						 BstOps			 op);


/* --- track ops --- */
static BstItemViewOp track_view_ops[] = {
  { "Add",		BST_OP_TRACK_ADD,	BST_STOCK_TRACK		},
  { "Delete",		BST_OP_TRACK_DELETE,	BST_STOCK_TRASHCAN	},
};
static guint n_track_view_ops = sizeof (track_view_ops) / sizeof (track_view_ops[0]);


/* --- static variables --- */
static gpointer		       parent_class = NULL;
static BstTrackViewClass *bst_track_view_class = NULL;


/* --- functions --- */
GtkType
bst_track_view_get_type (void)
{
  static GtkType track_view_type = 0;
  
  if (!track_view_type)
    {
      GtkTypeInfo track_view_info =
      {
	"BstTrackView",
	sizeof (BstTrackView),
	sizeof (BstTrackViewClass),
	(GtkClassInitFunc) bst_track_view_class_init,
	(GtkObjectInitFunc) bst_track_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      track_view_type = gtk_type_unique (BST_TYPE_ITEM_VIEW, &track_view_info);
    }
  
  return track_view_type;
}

static void
bst_track_view_class_init (BstTrackViewClass *class)
{
  GtkObjectClass *object_class;
  BstItemViewClass *item_view_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  item_view_class = BST_ITEM_VIEW_CLASS (class);
  
  bst_track_view_class = class;
  parent_class = gtk_type_class (BST_TYPE_ITEM_VIEW);
  
  item_view_class->can_operate = bst_track_view_can_operate;
  item_view_class->operate = bst_track_view_operate;
  item_view_class->n_ops = n_track_view_ops;
  item_view_class->ops = track_view_ops;
  item_view_class->default_param_view_height = 300;
}

static void
bst_track_view_init (BstTrackView *track_view)
{
  BstItemView *item_view = BST_ITEM_VIEW (track_view);

  item_view->item_type = BSE_TYPE_TRACK;
  bst_item_view_set_id_format (item_view, "%02X");
}

GtkWidget*
bst_track_view_new (BseSong *song)
{
  GtkWidget *track_view;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  
  track_view = gtk_widget_new (BST_TYPE_TRACK_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (track_view), BSE_OBJECT_ID (song));
  
  return track_view;
}

void
bst_track_view_operate (BstItemView *item_view,
			BstOps       op)
{
  BstTrackView *track_view = BST_TRACK_VIEW (item_view);
  BswProxy song = item_view->container;

  g_return_if_fail (bst_track_view_can_operate (item_view, op));

  switch (op)
    {
      BswProxy item;
      gchar *string;
    case BST_OP_TRACK_ADD:
      item = bsw_song_create_track (song);
      string = g_strdup_printf ("Track-%02X", bsw_item_get_seqid (item));
      bsw_proxy_set (item, "uname", string, NULL);
      g_free (string);
      bst_item_view_select (item_view, item);
      break;
    case BST_OP_TRACK_DELETE:
      item = bst_item_view_get_current (item_view);
      bsw_song_remove_track (song, item);
      break;
    default:
      break;
    }
  
  bst_update_can_operate (GTK_WIDGET (track_view));
}

gboolean
bst_track_view_can_operate (BstItemView *item_view,
			    BstOps	   op)
{
  BstTrackView *track_view = BST_TRACK_VIEW (item_view);
  
  g_return_val_if_fail (BST_IS_TRACK_VIEW (track_view), FALSE);
  
  switch (op)
    {
      BswProxy item;
    case BST_OP_TRACK_ADD:
      return TRUE;
    case BST_OP_TRACK_DELETE:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    default:
      return FALSE;
    }
}
