/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
 * Copyright (C) 2009 Stefan Westerfeld
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
#include "bstsoundfontview.h"
#include "bstsoundfontpresetview.h"
#include "bstfiledialog.h"

/* --- prototypes --- */

static void     sound_font_view_action_exec           (gpointer                data,
                                                       gulong                  action);
static gboolean sound_font_view_action_check          (gpointer                data,
                                                       gulong                  action,
                                                       guint64                 action_stamp);


/* --- sound font actions --- */

enum {
  ACTION_LOAD_SOUND_FONT,
  ACTION_LOAD_SOUND_FONT_LIB,
  ACTION_DELETE_SOUND_FONT,
  ACTION_SOUND_FONT_LAST
};
static const GxkStockAction sound_font_view_actions[] = {
  { N_("Load..."),  NULL,       N_("Load a new sound font file from disk"),
    ACTION_LOAD_SOUND_FONT,     BST_STOCK_LOAD,	},
  { N_("Lib..."),   NULL,       N_("Load a sound font file from library paths"),
    ACTION_LOAD_SOUND_FONT_LIB,	BST_STOCK_LOAD_LIB, },
  { N_("Delete"),   NULL,       N_("Delete the currently selected sound font from project"),
    ACTION_DELETE_SOUND_FONT,   BST_STOCK_TRASHCAN, },
};


/* --- functions --- */

G_DEFINE_TYPE (BstSoundFontView, bst_sound_font_view, BST_TYPE_ITEM_VIEW);

static void
bst_sound_font_view_class_init (BstSoundFontViewClass *class)
{
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (class);

  item_view_class->item_type = "BseSoundFont";
}

static void
sound_font_selection_changed (BstSoundFontView *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  bst_item_view_set_container (BST_ITEM_VIEW (self->preset_view), bst_item_view_get_current (iview));
}

static void
bst_sound_font_view_init (BstSoundFontView *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  /* complete GUI */
  GxkRadget *radget = gxk_radget_complete (GTK_WIDGET (self), "beast", "sound-font-view", NULL);
  gxk_widget_publish_actions (self, "sound-font-view-actions",
                              G_N_ELEMENTS (sound_font_view_actions), sound_font_view_actions,
                              NULL, sound_font_view_action_check, sound_font_view_action_exec);
  /* setup tree view */
  GtkTreeView *tview = gxk_radget_find (radget, "tree-view");
  bst_item_view_complete_tree (iview, tview);

  g_object_connect (gtk_tree_view_get_selection (tview),
                    "swapped_object_signal::changed", sound_font_selection_changed, self,
                    NULL);


  /* setup preset view */
  GtkTreeView *pview = gxk_radget_find (radget, "preset-view");
  self->preset_view = BST_SOUND_FONT_PRESET_VIEW (bst_sound_font_preset_view_new());
  bst_item_view_complete_tree (BST_ITEM_VIEW (self->preset_view), pview);
}

GtkWidget*
bst_sound_font_view_new (SfiProxy sfrepo)
{
  GtkWidget *sound_font_view;

  g_return_val_if_fail (BSE_IS_SOUND_FONT_REPO (sfrepo), NULL);

  sound_font_view = gtk_widget_new (BST_TYPE_SOUND_FONT_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (sound_font_view), sfrepo);

  return sound_font_view;
}

SfiProxy
bst_sound_font_view_get_preset (BstSoundFontView *self)
{
  return bst_item_view_get_current (BST_ITEM_VIEW (self->preset_view));
}

static void
sound_font_view_action_exec (gpointer                data,
                             gulong                  action)
{
  BstSoundFontView *self = BST_SOUND_FONT_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  SfiProxy sfrepo = item_view->container;
  switch (action)
    {
      SfiProxy item;
    case ACTION_LOAD_SOUND_FONT:
      bst_file_dialog_popup_load_sound_font (item_view, BST_ITEM_VIEW (self)->container, FALSE);
      break;
    case ACTION_LOAD_SOUND_FONT_LIB:
      bst_file_dialog_popup_load_sound_font (item_view, BST_ITEM_VIEW (self)->container, TRUE);
      break;
    case ACTION_DELETE_SOUND_FONT:
      item = bst_item_view_get_current (BST_ITEM_VIEW (self));
      bse_sound_font_repo_remove_sound_font (sfrepo, item);
      break;
    default:
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
sound_font_view_action_check (gpointer                data,
                              gulong                  action,
                              guint64                 action_stamp)
{
  BstSoundFontView *self = BST_SOUND_FONT_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);

  switch (action)
    {
    case ACTION_LOAD_SOUND_FONT:
    case ACTION_LOAD_SOUND_FONT_LIB:
      return TRUE;
    case ACTION_DELETE_SOUND_FONT:
      return bst_item_view_get_current (item_view) != 0;
    default:
      return FALSE;
    }
}
