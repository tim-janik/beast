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
#include "bstsnetshell.h"

#include "bstparamview.h"
#include "bstapp.h"



/* --- prototypes --- */
static void	bst_snet_shell_class_init	(BstSNetShellClass	*klass);
static void	bst_snet_shell_init		(BstSNetShell		*pe);
static void	bst_snet_shell_rebuild		(BstSuperShell		*super_shell);
static void	bst_snet_shell_operate		(BstSuperShell		*super_shell,
						 BstOps			 sop);
static gboolean	bst_snet_shell_can_operate	(BstSuperShell		*super_shell,
						 BstOps			 sop);


/* --- static variables --- */
static gpointer           parent_class = NULL;
static BstSNetShellClass *bst_snet_shell_class = NULL;


/* --- functions --- */
GtkType
bst_snet_shell_get_type (void)
{
  static GtkType snet_shell_type = 0;
  
  if (!snet_shell_type)
    {
      GtkTypeInfo snet_shell_info =
      {
	"BstSNetShell",
	sizeof (BstSNetShell),
	sizeof (BstSNetShellClass),
	(GtkClassInitFunc) bst_snet_shell_class_init,
	(GtkObjectInitFunc) bst_snet_shell_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      snet_shell_type = gtk_type_unique (BST_TYPE_SUPER_SHELL, &snet_shell_info);
    }
  
  return snet_shell_type;
}

static void
bst_snet_shell_class_init (BstSNetShellClass *class)
{
  BstSuperShellClass *super_shell_class = BST_SUPER_SHELL_CLASS (class);

  bst_snet_shell_class = class;
  parent_class = g_type_class_peek_parent (class);

  super_shell_class->operate = bst_snet_shell_operate;
  super_shell_class->can_operate = bst_snet_shell_can_operate;
  super_shell_class->rebuild = bst_snet_shell_rebuild;

  class->factories_path = "<BstSNetShell>";
}

static void
bst_snet_shell_init (BstSNetShell *snet_shell)
{
  snet_shell->param_view = NULL;
  snet_shell->snet_router = NULL;
}

static void
bst_snet_shell_rebuild (BstSuperShell *super_shell)
{
  BstSNetShell *snet_shell = BST_SNET_SHELL (super_shell);
  SfiProxy snet = super_shell->super;
  GtkWidget *notebook;

  g_return_if_fail (snet_shell->param_view == NULL);

  /* notebook
   */
  notebook = g_object_new (GTK_TYPE_NOTEBOOK,
			   "scrollable", FALSE,
			   "tab_border", 0,
			   "show_border", TRUE,
			   "enable_popup", FALSE,
			   "show_tabs", TRUE,
			   "tab_pos", GTK_POS_LEFT,
			   "tab_pos", GTK_POS_TOP,
			   "border_width", 5,
			   "parent", snet_shell,
			   "visible", TRUE,
			   NULL);
  g_object_connect (notebook,
		    "signal_after::switch-page", gxk_widget_viewable_changed, NULL,
		    NULL);

  /* router */
  if (bse_snet_supports_user_synths (snet) || BST_DBG_EXT)
    {
      snet_shell->snet_router = bst_snet_router_build_page (super_shell->super);
      g_object_connect (GTK_WIDGET (snet_shell->snet_router),
			"signal::destroy", gtk_widget_destroyed, &snet_shell->snet_router,
			NULL);
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), gtk_widget_get_toplevel (GTK_WIDGET (snet_shell->snet_router)),
				gtk_widget_new (GTK_TYPE_LABEL,
						"label", "Routing",
						"visible", TRUE,
						NULL));
    }
  /* parameters */
  snet_shell->param_view = (BstParamView*) bst_param_view_new (snet);
  g_object_set (GTK_WIDGET (snet_shell->param_view),
		"visible", TRUE,
		NULL);
  g_object_connect (GTK_WIDGET (snet_shell->param_view),
		    "signal::destroy", gtk_widget_destroyed, &snet_shell->param_view,
		    NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (snet_shell->param_view),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", "Parameters",
					    "visible", TRUE,
					    NULL));
}

static void
bst_snet_shell_operate (BstSuperShell *super_shell,
			BstOps         op)
{
  // BseSNet *snet = BSE_SNET (super_shell->super);
  BstSNetShell *snet_shell = BST_SNET_SHELL (super_shell);

  g_return_if_fail (bst_snet_shell_can_operate (super_shell, op));
  
  switch (op)
    {
    default:
      break;
    }

  bst_update_can_operate (GTK_WIDGET (snet_shell));
}

static gboolean
bst_snet_shell_can_operate (BstSuperShell *super_shell,
			    BstOps	   op)
{
  // BstSNetShell *snet_shell = BST_SNET_SHELL (super_shell);
  // BseSNet *snet = BSE_SNET (super_shell->super);

  switch (op)
    {
    default:
      return FALSE;
    }
}
