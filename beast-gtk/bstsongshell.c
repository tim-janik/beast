/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#include "bstsongshell.h"

#include "bstparamview.h"
#include "bstactivatable.h"
#include "bstapp.h"


/* --- prototypes --- */
static void	bst_song_shell_class_init	  (BstSongShellClass	  *klass);
static void     bst_song_shell_init_activatable   (BstActivatableIface    *iface,
                                                   gpointer                iface_data);
static void	bst_song_shell_init		  (BstSongShell		  *pe);
static void	bst_song_shell_rebuild		  (BstSuperShell          *super_shell);
static void	bst_song_shell_activate		  (BstActivatable         *activatable,
                                                   gulong                  action);
static gboolean	bst_song_shell_can_activate	  (BstActivatable         *activatable,
                                                   gulong                  action);
static void     bst_song_shell_request_update     (BstActivatable         *activatable);
static void     bst_song_shell_update_activatable (BstActivatable         *activatable);


/* --- static variables --- */
static gpointer           parent_class = NULL;
static BstSongShellClass *bst_song_shell_class = NULL;


/* --- functions --- */
GType
bst_song_shell_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (BstSongShellClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bst_song_shell_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (BstSongShell),
        0,      /* n_preallocs */
        (GInstanceInitFunc) bst_song_shell_init,
      };
      static const GInterfaceInfo activatable_info = {
        (GInterfaceInitFunc) bst_song_shell_init_activatable,           /* interface_init */
        NULL,                                                           /* interface_finalize */
        NULL                                                            /* interface_data */
      };
      type = g_type_register_static (BST_TYPE_SUPER_SHELL, "BstSongShell", &type_info, 0);
      g_type_add_interface_static (type, BST_TYPE_ACTIVATABLE, &activatable_info);
    }
  return type;
}

static void
bst_song_shell_class_init (BstSongShellClass *class)
{
  BstSuperShellClass *super_shell_class = BST_SUPER_SHELL_CLASS (class);

  bst_song_shell_class = class;
  parent_class = g_type_class_peek_parent (class);

  super_shell_class->rebuild = bst_song_shell_rebuild;

  class->factories_path = "<BstSongShell>";
}

static void
bst_song_shell_init_activatable (BstActivatableIface *iface,
                                 gpointer             iface_data)
{
  iface->activate = bst_song_shell_activate;
  iface->can_activate = bst_song_shell_can_activate;
  iface->request_update = bst_song_shell_request_update;
  iface->update = bst_song_shell_update_activatable;
}

static void
bst_song_shell_init (BstSongShell *song_shell)
{
  song_shell->param_view = NULL;
  song_shell->track_view = NULL;
  song_shell->part_view = NULL;
  song_shell->snet_router = NULL;
}

static void
bst_song_shell_rebuild (BstSuperShell *super_shell)
{
  BstSongShell *song_shell = BST_SONG_SHELL (super_shell);
  SfiProxy song = super_shell->super;
  GtkWidget *notebook;

  g_return_if_fail (song_shell->param_view == NULL);

  song_shell->param_view = (BstParamView*) bst_param_view_new (song);
  g_object_connect (GTK_WIDGET (song_shell->param_view),
		    "signal::destroy", gtk_widget_destroyed, &song_shell->param_view,
		    NULL);
  song_shell->track_view = (BstItemView*) bst_track_view_new (song);
  g_object_connect (GTK_WIDGET (song_shell->track_view),
		    "signal::destroy", gtk_widget_destroyed, &song_shell->track_view,
		    NULL);
  song_shell->part_view = g_object_new (BST_TYPE_PART_VIEW, NULL);
  bst_item_view_set_container (song_shell->part_view, super_shell->super);
  g_object_connect (GTK_WIDGET (song_shell->part_view),
		    "swapped_signal::destroy", g_nullify_pointer, &song_shell->part_view,
		    NULL);
  
  notebook = g_object_connect (g_object_new (GTK_TYPE_NOTEBOOK,
					     "scrollable", FALSE,
					     "tab_border", 0,
					     "show_border", TRUE,
					     "enable_popup", FALSE,
					     "show_tabs", TRUE,
					     "tab_pos", GTK_POS_LEFT,
					     "tab_pos", GTK_POS_TOP,
					     "border_width", 5,
					     "parent", song_shell,
					     "visible", TRUE,
					     NULL),
			       "signal_after::switch-page", gxk_widget_viewable_changed, NULL,
			       NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (song_shell->track_view),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", _("Tracks"),
					    "visible", TRUE,
					    NULL));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (song_shell->param_view),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", _("Parameters"),
					    "visible", TRUE,
					    NULL));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (song_shell->part_view),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", _("Parts"),
					    "visible", TRUE,
					    NULL));
  if (BST_DBG_EXT)
    {
      song_shell->snet_router = bst_snet_router_build_page (super_shell->super);
      g_object_connect (song_shell->snet_router,
			"signal::destroy", gtk_widget_destroyed, &song_shell->snet_router,
			NULL);
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), gtk_widget_get_toplevel (GTK_WIDGET (song_shell->snet_router)),
				gtk_widget_new (GTK_TYPE_LABEL,
						"label", _("Routing"),
						"visible", TRUE,
						NULL));
    }
}

static void
bst_song_shell_activate (BstActivatable *activatable,
                         gulong          action)
{
  BstSongShell *self = BST_SONG_SHELL (activatable);
  // BseSong *song = BSE_SONG (super_shell->super);

  if (action >= BST_ACTION_PART_FIRST && action <= BST_ACTION_PART_LAST)
    bst_activatable_activate (BST_ACTIVATABLE (self->part_view), action);
  else if (action >= BST_ACTION_TRACK_FIRST && action <= BST_ACTION_TRACK_LAST)
    bst_activatable_activate (BST_ACTIVATABLE (self->track_view), action);
  bst_widget_update_activatable (activatable);
}

static gboolean
bst_song_shell_can_activate (BstActivatable *activatable,
                             gulong          action)
{
  BstSongShell *self = BST_SONG_SHELL (activatable);
  // BseSong *song = BSE_SONG (super_shell->super);

  if (action >= BST_ACTION_PART_FIRST && action <= BST_ACTION_PART_LAST && self->part_view)
    return bst_activatable_can_activate (BST_ACTIVATABLE (self->part_view), action);
  else if (action >= BST_ACTION_TRACK_FIRST && action <= BST_ACTION_TRACK_LAST && self->track_view)
    return bst_activatable_can_activate (BST_ACTIVATABLE (self->track_view), action);
  else
    return FALSE;
}

static void
bst_song_shell_request_update (BstActivatable *activatable)
{
  BstSongShell *self = BST_SONG_SHELL (activatable);
  /* chain to normal handler */
  bst_activatable_default_request_update (activatable);
  /* add activatable children */
  if (self->part_view)
    bst_activatable_update_enqueue (BST_ACTIVATABLE (self->part_view));
  if (self->track_view)
    bst_activatable_update_enqueue (BST_ACTIVATABLE (self->track_view));
}

static void
bst_song_shell_update_activatable (BstActivatable *activatable)
{
  // BstSongShell *self = BST_SONG_SHELL (activatable);

  /* no original actions to update */
}
