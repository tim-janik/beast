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
#include "bstsongshell.h"

#include "bstparamview.h"
#include "bstapp.h"


/* --- prototypes --- */
static void	bst_song_shell_class_init	(BstSongShellClass	*klass);
static void	bst_song_shell_init		(BstSongShell		*pe);
static void	bst_song_shell_destroy		(GtkObject		*object);
static void	bst_song_shell_rebuild		(BstSuperShell		*super_shell);
static void	bst_song_shell_update		(BstSuperShell		*super_shell);
static void	bst_song_shell_operate		(BstSuperShell		*super_shell,
						 BstOps			 sop);
static gboolean	bst_song_shell_can_operate	(BstSuperShell		*super_shell,
						 BstOps			 sop);
static void	bst_song_shell_setup_super	(BstSuperShell		*super_shell,
						 BseSuper		*super);
static void	bst_song_shell_release_super	(BstSuperShell		*super_shell,
						 BseSuper		*super);


/* --- static variables --- */
static gpointer           parent_class = NULL;
static BstSongShellClass *bst_song_shell_class = NULL;


/* --- functions --- */
GtkType
bst_song_shell_get_type (void)
{
  static GtkType song_shell_type = 0;
  
  if (!song_shell_type)
    {
      GtkTypeInfo song_shell_info =
      {
	"BstSongShell",
	sizeof (BstSongShell),
	sizeof (BstSongShellClass),
	(GtkClassInitFunc) bst_song_shell_class_init,
	(GtkObjectInitFunc) bst_song_shell_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      song_shell_type = gtk_type_unique (BST_TYPE_SUPER_SHELL, &song_shell_info);
    }
  
  return song_shell_type;
}

static void
bst_song_shell_class_init (BstSongShellClass *class)
{
  GtkObjectClass *object_class;
  BstSuperShellClass *super_shell_class;

  object_class = GTK_OBJECT_CLASS (class);
  super_shell_class = BST_SUPER_SHELL_CLASS (class);

  bst_song_shell_class = class;
  parent_class = gtk_type_class (BST_TYPE_SUPER_SHELL);

  object_class->destroy = bst_song_shell_destroy;

  super_shell_class->setup_super = bst_song_shell_setup_super;
  super_shell_class->release_super = bst_song_shell_release_super;
  super_shell_class->operate = bst_song_shell_operate;
  super_shell_class->can_operate = bst_song_shell_can_operate;
  super_shell_class->rebuild = bst_song_shell_rebuild;
  super_shell_class->update = bst_song_shell_update;

  class->factories_path = "<BstSongShell>";
}

static void
bst_song_shell_init (BstSongShell *song_shell)
{
  song_shell->param_view = NULL;
  song_shell->instrument_view = NULL;
  song_shell->pattern_view = NULL;
  song_shell->play_list = NULL;
  song_shell->tooltips = gtk_tooltips_new ();
  gtk_object_ref (GTK_OBJECT (song_shell->tooltips));
  gtk_object_sink (GTK_OBJECT (song_shell->tooltips));
}

static void
bst_song_shell_destroy (GtkObject *object)
{
  BstSongShell *song_shell = BST_SONG_SHELL (object);
  BseSong *song = BSE_SONG (BST_SUPER_SHELL (song_shell)->super);
  
  if (song->sequencer != NULL)
    bse_source_clear_ochannels (BSE_SOURCE (song));
  
  gtk_container_foreach (GTK_CONTAINER (song_shell), (GtkCallback) gtk_widget_destroy, NULL);
  
  gtk_object_unref (GTK_OBJECT (song_shell->tooltips));
  song_shell->tooltips = NULL;
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_song_shell_new (BseSong *song)
{
  GtkWidget *song_shell;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  g_return_val_if_fail (bst_super_shell_from_super (BSE_SUPER (song)) == NULL, NULL);
  
  song_shell = gtk_widget_new (BST_TYPE_SONG_SHELL,
			       "super", song,
			       NULL);
  
  return song_shell;
}

static void
bst_song_shell_build (BstSongShell *song_shell)
{
  GtkWidget *notebook;
  BseSong *song = BSE_SONG (BST_SUPER_SHELL (song_shell)->super);

  song_shell->param_view = (BstParamView*) bst_param_view_new (BSE_OBJECT (song));
  gtk_widget_set (GTK_WIDGET (song_shell->param_view),
		  "signal::destroy", gtk_widget_destroyed, &song_shell->param_view,
		  "visible", TRUE,
		  NULL);
  song_shell->instrument_view = (BstItemView*) bst_instrument_view_new (song);
  gtk_widget_set (GTK_WIDGET (song_shell->instrument_view),
		  "signal::destroy", gtk_widget_destroyed, &song_shell->instrument_view,
		  "visible", TRUE,
		  NULL);
  song_shell->pattern_view = (BstItemView*) bst_pattern_view_new (song);
  gtk_widget_set (GTK_WIDGET (song_shell->pattern_view),
		  "signal::destroy", gtk_widget_destroyed, &song_shell->pattern_view,
		  "visible", TRUE,
		  NULL);
  song_shell->play_list = bst_play_list_new (song);
  gtk_widget_set (GTK_WIDGET (song_shell->play_list),
		  "signal::destroy", gtk_widget_destroyed, &song_shell->play_list,
		  "visible", TRUE,
		  NULL);
  
  notebook = gtk_widget_new (GTK_TYPE_NOTEBOOK,
			     "GtkNotebook::scrollable", FALSE,
			     "GtkNotebook::tab_border", 0,
			     "GtkNotebook::show_border", TRUE,
			     "GtkNotebook::enable_popup", FALSE,
			     "GtkNotebook::show_tabs", TRUE,
			     "GtkNotebook::tab_pos", GTK_POS_LEFT,
			     "GtkNotebook::tab_pos", GTK_POS_TOP,
			     "border_width", 5,
			     "parent", song_shell,
			     "visible", TRUE,
			     NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (song_shell->param_view),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", "Parameters",
					    "visible", TRUE,
					    NULL));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (song_shell->instrument_view),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", "Instruments",
					    "visible", TRUE,
					    NULL));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (song_shell->pattern_view),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", "Patterns",
					    "visible", TRUE,
					    NULL));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), song_shell->play_list,
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", "Arrangement",
					    "visible", TRUE,
					    NULL));
}

static void
bst_song_shell_setup_super (BstSuperShell *super_shell,
			    BseSuper      *super)
{
  BstSongShell *song_shell;
  
  song_shell = BST_SONG_SHELL (super_shell);
  
  BST_SUPER_SHELL_CLASS (parent_class)->setup_super (super_shell, super);

  if (super)
    bst_song_shell_build (song_shell);
}

static void
bst_song_shell_release_super (BstSuperShell *super_shell,
			      BseSuper      *super)
{
  BstSongShell *song_shell;
  
  song_shell = BST_SONG_SHELL (super_shell);
  
  gtk_container_foreach (GTK_CONTAINER (song_shell), (GtkCallback) gtk_widget_destroy, NULL);
  
  BST_SUPER_SHELL_CLASS (parent_class)->release_super (super_shell, super);
}

static void
bst_song_shell_update (BstSuperShell *super_shell)
{
  BstSongShell *song_shell;
  
  song_shell = BST_SONG_SHELL (super_shell);
  
  bst_param_view_update (song_shell->param_view);
  bst_item_view_update (song_shell->instrument_view);
  bst_item_view_update (song_shell->pattern_view);
  // bst_play_list_update (song_shell->play_list);
}

static void
bst_song_shell_rebuild (BstSuperShell *super_shell)
{
  BstSongShell *song_shell;
  
  song_shell = BST_SONG_SHELL (super_shell);
  
  bst_param_view_rebuild (song_shell->param_view);
  bst_item_view_rebuild (song_shell->instrument_view);
  bst_item_view_rebuild (song_shell->pattern_view);
  bst_play_list_rebuild (BST_PLAY_LIST (song_shell->play_list));
}

static void
bst_song_shell_operate (BstSuperShell *super_shell,
			BstOps         op)
{
  // BseSong *song = BSE_SONG (super_shell->super);
  BstSongShell *song_shell = BST_SONG_SHELL (super_shell);

  g_return_if_fail (bst_song_shell_can_operate (super_shell, op));
  
  switch (op)
    {
    case BST_OP_PATTERN_ADD:
    case BST_OP_PATTERN_DELETE:
    case BST_OP_PATTERN_EDITOR:
      bst_item_view_operate (song_shell->pattern_view, op);
      break;
    case BST_OP_INSTRUMENT_ADD:
    case BST_OP_INSTRUMENT_DELETE:
      bst_item_view_operate (song_shell->instrument_view, op);
      break;
    default:
      break;
    }

  bst_update_can_operate (GTK_WIDGET (song_shell));
}

static gboolean
bst_song_shell_can_operate (BstSuperShell *super_shell,
			    BstOps	   op)
{
  BstSongShell *song_shell = BST_SONG_SHELL (super_shell);
  // BseSong *song = BSE_SONG (super_shell->super);

  switch (op)
    {
    case BST_OP_PATTERN_ADD:
    case BST_OP_PATTERN_DELETE:
    case BST_OP_PATTERN_EDITOR:
      return (song_shell->pattern_view &&
	      bst_item_view_can_operate (song_shell->pattern_view, op));
    case BST_OP_INSTRUMENT_ADD:
    case BST_OP_INSTRUMENT_DELETE:
      return (song_shell->instrument_view &&
	      bst_item_view_can_operate (song_shell->instrument_view, op));
    default:
      return FALSE;
    }
}
