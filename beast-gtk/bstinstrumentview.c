/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
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
#include "bstinstrumentview.h"



/* --- prototypes --- */
static void	bst_instrument_view_class_init	(BstInstrumentViewClass	*klass);
static void	bst_instrument_view_init	(BstInstrumentView	*instrument_view);
static void	bst_instrument_view_operate	(BstItemView		*item_view,
						 BstOps			 op);
static gboolean	bst_instrument_view_can_operate	(BstItemView		*item_view,
						 BstOps			 op);


/* --- instrument ops --- */
static BstItemViewOp instrument_view_ops[] = {
  { "Add",		BST_OP_INSTRUMENT_ADD,		BST_STOCK_INSTRUMENT	},
  { "Delete",		BST_OP_INSTRUMENT_DELETE,	BST_STOCK_TRASHCAN	},
};
static guint n_instrument_view_ops = sizeof (instrument_view_ops) / sizeof (instrument_view_ops[0]);


/* --- static variables --- */
static gpointer		       parent_class = NULL;
static BstInstrumentViewClass *bst_instrument_view_class = NULL;


/* --- functions --- */
GtkType
bst_instrument_view_get_type (void)
{
  static GtkType instrument_view_type = 0;
  
  if (!instrument_view_type)
    {
      GtkTypeInfo instrument_view_info =
      {
	"BstInstrumentView",
	sizeof (BstInstrumentView),
	sizeof (BstInstrumentViewClass),
	(GtkClassInitFunc) bst_instrument_view_class_init,
	(GtkObjectInitFunc) bst_instrument_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      instrument_view_type = gtk_type_unique (BST_TYPE_ITEM_VIEW, &instrument_view_info);
    }
  
  return instrument_view_type;
}

static void
bst_instrument_view_class_init (BstInstrumentViewClass *class)
{
  GtkObjectClass *object_class;
  BstItemViewClass *item_view_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  item_view_class = BST_ITEM_VIEW_CLASS (class);
  
  bst_instrument_view_class = class;
  parent_class = gtk_type_class (BST_TYPE_ITEM_VIEW);
  
  item_view_class->can_operate = bst_instrument_view_can_operate;
  item_view_class->operate = bst_instrument_view_operate;
  item_view_class->n_ops = n_instrument_view_ops;
  item_view_class->ops = instrument_view_ops;
  item_view_class->default_param_view_height = 300;
}

static void
bst_instrument_view_init (BstInstrumentView *instrument_view)
{
  BstItemView *item_view = BST_ITEM_VIEW (instrument_view);

  item_view->item_type = BSE_TYPE_INSTRUMENT;
  bst_item_view_set_id_format (item_view, "%02X");
}

GtkWidget*
bst_instrument_view_new (BseSong *song)
{
  GtkWidget *instrument_view;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  
  instrument_view = gtk_widget_new (BST_TYPE_INSTRUMENT_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (instrument_view), BSE_OBJECT_ID (song));
  
  return instrument_view;
}

void
bst_instrument_view_operate (BstItemView *item_view,
			     BstOps       op)
{
  BstInstrumentView *instrument_view = BST_INSTRUMENT_VIEW (item_view);
  SfiProxy song = item_view->container;

  g_return_if_fail (bst_instrument_view_can_operate (item_view, op));

  switch (op)
    {
      SfiProxy item;
      gchar *string;
    case BST_OP_INSTRUMENT_ADD:
      item = bsw_song_create_instrument (song);
      string = g_strdup_printf ("Instrument-%02X", bsw_item_get_seqid (item));
      bse_proxy_set (item, "uname", string, NULL);
      g_free (string);
      bst_item_view_select (item_view, item);
      break;
    case BST_OP_INSTRUMENT_DELETE:
      item = bst_item_view_get_current (item_view);
      bsw_song_remove_instrument (song, item);
      break;
    default:
      break;
    }
  
  bst_update_can_operate (GTK_WIDGET (instrument_view));
}

gboolean
bst_instrument_view_can_operate (BstItemView *item_view,
				 BstOps	   op)
{
  BstInstrumentView *instrument_view = BST_INSTRUMENT_VIEW (item_view);
  
  g_return_val_if_fail (BST_IS_INSTRUMENT_VIEW (instrument_view), FALSE);
  
  switch (op)
    {
      SfiProxy item;
    case BST_OP_INSTRUMENT_ADD:
      return TRUE;
    case BST_OP_INSTRUMENT_DELETE:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    default:
      return FALSE;
    }
}
