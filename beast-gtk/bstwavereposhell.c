/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000-2001, 2003 Tim Janik
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
#include "bstwavereposhell.h"

#include "bstparamview.h"
#include "bstapp.h"


/* --- prototypes --- */
static void	bst_wave_repo_shell_class_init	(BstWaveRepoShellClass	*klass);
static void	bst_wave_repo_shell_init	(BstWaveRepoShell	*wshell);
static void	bst_wave_repo_shell_rebuild	(BstSuperShell		*super_shell);
static void	bst_wave_repo_shell_operate	(BstSuperShell		*super_shell,
						 BstOps			 sop);
static gboolean	bst_wave_repo_shell_can_operate	(BstSuperShell		*super_shell,
						 BstOps			 sop);
static gchar*   bst_wave_repo_shell_get_title   (BstSuperShell          *super_shell);


/* --- static variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
GType
bst_wave_repo_shell_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstWaveRepoShellClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_wave_repo_shell_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstWaveRepoShell),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_wave_repo_shell_init,
      };
      type = g_type_register_static (BST_TYPE_SUPER_SHELL, "BstWaveRepoShell", &type_info, 0);
    }
  return type;
}

static void
bst_wave_repo_shell_class_init (BstWaveRepoShellClass *class)
{
  BstSuperShellClass *super_shell_class = BST_SUPER_SHELL_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  super_shell_class->get_title = bst_wave_repo_shell_get_title;
  super_shell_class->operate = bst_wave_repo_shell_operate;
  super_shell_class->can_operate = bst_wave_repo_shell_can_operate;
  super_shell_class->rebuild = bst_wave_repo_shell_rebuild;
}

static void
bst_wave_repo_shell_init (BstWaveRepoShell *wshell)
{
  wshell->param_view = NULL;
  wshell->wave_view = NULL;
}

static void
bst_wave_repo_shell_rebuild (BstSuperShell *super_shell)
{
  BstWaveRepoShell *wshell = BST_WAVE_REPO_SHELL (super_shell);
  SfiProxy wrepo = super_shell->super;
  GtkWidget *notebook;

  g_return_if_fail (wshell->param_view == NULL);

  wshell->param_view = (BstParamView*) bst_param_view_new (wrepo);
  g_object_set (GTK_WIDGET (wshell->param_view),
		"visible", TRUE,
		NULL);
  g_object_connect (GTK_WIDGET (wshell->param_view),
		    "signal::destroy", gtk_widget_destroyed, &wshell->param_view,
		    NULL);
  wshell->wave_view = (BstItemView*) bst_wave_view_new (wrepo);
  g_object_set (GTK_WIDGET (wshell->wave_view),
		"visible", TRUE,
		NULL);
  g_object_connect (GTK_WIDGET (wshell->wave_view),
		    "signal::destroy", gtk_widget_destroyed, &wshell->wave_view,
		    NULL);
  
  notebook = g_object_connect (g_object_new (GTK_TYPE_NOTEBOOK,
					     "scrollable", FALSE,
					     "tab_border", 0,
					     "show_border", TRUE,
					     "enable_popup", FALSE,
					     "show_tabs", TRUE,
					     "tab_pos", GTK_POS_TOP,
					     "border_width", 5,
					     "parent", wshell,
					     "visible", TRUE,
					     NULL),
			       "signal_after::switch-page", gxk_widget_viewable_changed, NULL,
			       NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (wshell->wave_view),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", "Waves",
					    "visible", TRUE,
					    NULL));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (wshell->param_view),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", "Parameters",
					    "visible", TRUE,
					    NULL));
}

static gchar*
bst_wave_repo_shell_get_title (BstSuperShell *super_shell)
{
  // BstWaveRepoShell *self = BST_WAVE_REPO_SHELL (super_shell);

  return g_strdup ("Waves");
}

static void
bst_wave_repo_shell_operate (BstSuperShell *super_shell,
			     BstOps         op)
{
  BstWaveRepoShell *wshell = BST_WAVE_REPO_SHELL (super_shell);

  g_return_if_fail (bst_wave_repo_shell_can_operate (super_shell, op));
  
  switch (op)
    {
    case BST_OP_WAVE_LOAD:
    case BST_OP_WAVE_LOAD_LIB:
    case BST_OP_WAVE_DELETE:
    case BST_OP_WAVE_EDITOR:
      bst_item_view_operate (wshell->wave_view, op);
      break;
    default:
      break;
    }

  bst_update_can_operate (GTK_WIDGET (wshell));
}

static gboolean
bst_wave_repo_shell_can_operate (BstSuperShell *super_shell,
				 BstOps	        op)
{
  BstWaveRepoShell *wshell = BST_WAVE_REPO_SHELL (super_shell);

  switch (op)
    {
    case BST_OP_WAVE_LOAD:
    case BST_OP_WAVE_LOAD_LIB:
    case BST_OP_WAVE_DELETE:
    case BST_OP_WAVE_EDITOR:
      return (wshell->wave_view &&
              bst_item_view_can_operate (wshell->wave_view, op));
    default:
      return FALSE;
    }
}
