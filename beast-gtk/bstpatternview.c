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
#include "bstpatternview.h"

#include "bstpatterndialog.h"
#include "bststatusbar.h"



/* --- prototypes --- */
static void	bst_pattern_view_class_init	(BstPatternViewClass	*klass);
static void	bst_pattern_view_init		(BstPatternView		*pattern_view);
static void	bst_pattern_view_operate	(BstItemView		*item_view,
						 BstOps			 op);
static gboolean	bst_pattern_view_can_operate	(BstItemView		*item_view,
						 BstOps			 op);


/* --- pattern ops --- */
static BstItemViewOp pattern_view_ops[] = {
  { "Add",		BST_OP_PATTERN_ADD,	BST_ICON_PATTERN,	},
  { "Delete",		BST_OP_PATTERN_DELETE,	BST_ICON_TRASHCAN,	},
  { "Editor...",	BST_OP_PATTERN_EDITOR,	BST_ICON_PATTERN_TOOL,	},
};
static guint n_pattern_view_ops = sizeof (pattern_view_ops) / sizeof (pattern_view_ops[0]);


/* --- static variables --- */
static gpointer		    parent_class = NULL;
static BstPatternViewClass *bst_pattern_view_class = NULL;


/* --- functions --- */
GtkType
bst_pattern_view_get_type (void)
{
  static GtkType pattern_view_type = 0;
  
  if (!pattern_view_type)
    {
      GtkTypeInfo pattern_view_info =
      {
	"BstPatternView",
	sizeof (BstPatternView),
	sizeof (BstPatternViewClass),
	(GtkClassInitFunc) bst_pattern_view_class_init,
	(GtkObjectInitFunc) bst_pattern_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      pattern_view_type = gtk_type_unique (BST_TYPE_ITEM_VIEW, &pattern_view_info);
    }
  
  return pattern_view_type;
}

static void
bst_pattern_view_class_init (BstPatternViewClass *class)
{
  GtkObjectClass *object_class;
  BstItemViewClass *item_view_class;

  object_class = GTK_OBJECT_CLASS (class);
  item_view_class = BST_ITEM_VIEW_CLASS (class);
  
  bst_pattern_view_class = class;
  parent_class = gtk_type_class (BST_TYPE_ITEM_VIEW);

  item_view_class->can_operate = bst_pattern_view_can_operate;
  item_view_class->operate = bst_pattern_view_operate;
  item_view_class->n_ops = n_pattern_view_ops;
  item_view_class->ops = pattern_view_ops;
}

static void
bst_pattern_view_init (BstPatternView *pattern_view)
{
  BST_ITEM_VIEW (pattern_view)->item_type = BSE_TYPE_PATTERN;
}

GtkWidget*
bst_pattern_view_new (BseSong *song)
{
  GtkWidget *pattern_view;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  
  pattern_view = gtk_widget_new (BST_TYPE_PATTERN_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (pattern_view), BSE_CONTAINER (song));
  
  return pattern_view;
}

static void
popup_pattern_dialog (BstPatternView *pattern_view)
{
  BseItem *pattern;
  GtkWidget *pd;

  pattern = bst_item_view_get_current (BST_ITEM_VIEW (pattern_view));
  pd = bst_pattern_dialog_new (BSE_PATTERN (pattern));

  gtk_signal_connect_object_while_alive (GTK_OBJECT (pattern_view),
					 "destroy",
					 G_CALLBACK (gtk_widget_destroy),
					 GTK_OBJECT (pd));
  gtk_widget_set (pd,
		  "allow_shrink", TRUE,
		  "visible", TRUE,
		  NULL);
}

void
bst_pattern_view_operate (BstItemView *item_view,
			  BstOps       op)
{
  BseSong *song;
  BstPatternView *pattern_view = BST_PATTERN_VIEW (item_view);
  
  g_return_if_fail (bst_pattern_view_can_operate (item_view, op));

  song = BSE_SONG (item_view->container);
  
  switch (op)
    {
      BseItem *item;
      
    case BST_OP_PATTERN_ADD:
      item = bse_container_new_item (BSE_CONTAINER (song), BSE_TYPE_PATTERN, NULL);
      bse_pattern_group_insert_pattern (bse_song_get_default_pattern_group (song),
					BSE_PATTERN (item),
					-1);
      bst_item_view_select (item_view, item);
      break;
    case BST_OP_PATTERN_DELETE:
      item = bst_item_view_get_current (BST_ITEM_VIEW (pattern_view));
      bse_container_remove_item (BSE_CONTAINER (song), item);
      break;
    case BST_OP_PATTERN_EDITOR:
      popup_pattern_dialog (pattern_view);
      break;
    default:
      break;
    }
  
  bst_update_can_operate (GTK_WIDGET (pattern_view));
}

gboolean
bst_pattern_view_can_operate (BstItemView *item_view,
			      BstOps	   op)
{
  BseSong *song;
  BstPatternView *pattern_view = BST_PATTERN_VIEW (item_view);

  g_return_val_if_fail (BST_IS_PATTERN_VIEW (pattern_view), FALSE);
  
  song = BSE_SONG (item_view->container);

  switch (op)
    {
    case BST_OP_PATTERN_ADD:
      return TRUE;
    case BST_OP_PATTERN_DELETE:
      return g_list_length (song->patterns) > 1;
    case BST_OP_PATTERN_EDITOR:
      return bst_item_view_get_current (item_view) != NULL;
    default:
      return FALSE;
    }
}
