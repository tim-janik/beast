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
#include "bstsnetshell.h"

#include "bstparamview.h"
#include "bstapp.h"



/* --- prototypes --- */
static void	bst_snet_shell_class_init	(BstSNetShellClass	*klass);
static void	bst_snet_shell_init		(BstSNetShell		*pe);
static void	bst_snet_shell_destroy		(GtkObject		*object);
static void	bst_snet_shell_rebuild		(BstSuperShell		*super_shell);
static void	bst_snet_shell_update		(BstSuperShell		*super_shell);
static void	bst_snet_shell_operate		(BstSuperShell		*super_shell,
						 BstOps			 sop);
static gboolean	bst_snet_shell_can_operate	(BstSuperShell		*super_shell,
						 BstOps			 sop);
static void	bst_snet_shell_setup_super	(BstSuperShell		*super_shell,
						 BseSuper		*super);
static void	bst_snet_shell_release_super	(BstSuperShell		*super_shell,
						 BseSuper		*super);


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
  GtkObjectClass *object_class;
  BstSuperShellClass *super_shell_class;

  object_class = GTK_OBJECT_CLASS (class);
  super_shell_class = BST_SUPER_SHELL_CLASS (class);

  bst_snet_shell_class = class;
  parent_class = gtk_type_class (BST_TYPE_SUPER_SHELL);

  object_class->destroy = bst_snet_shell_destroy;

  super_shell_class->setup_super = bst_snet_shell_setup_super;
  super_shell_class->release_super = bst_snet_shell_release_super;
  super_shell_class->operate = bst_snet_shell_operate;
  super_shell_class->can_operate = bst_snet_shell_can_operate;
  super_shell_class->rebuild = bst_snet_shell_rebuild;
  super_shell_class->update = bst_snet_shell_update;

  class->factories_path = "<BstSNetShell>";
}

static void
bst_snet_shell_init (BstSNetShell *snet_shell)
{
  snet_shell->param_view = NULL;
  snet_shell->snet_router = NULL;
  snet_shell->tooltips = gtk_tooltips_new ();
  gtk_object_ref (GTK_OBJECT (snet_shell->tooltips));
  gtk_object_sink (GTK_OBJECT (snet_shell->tooltips));
}

static void
bst_snet_shell_destroy (GtkObject *object)
{
  BstSNetShell *snet_shell = BST_SNET_SHELL (object);
  BseSNet *snet = BSE_SNET (BST_SUPER_SHELL (snet_shell)->super);
  
  bse_source_clear_ochannels (BSE_SOURCE (snet));
  
  gtk_container_foreach (GTK_CONTAINER (snet_shell), (GtkCallback) gtk_widget_destroy, NULL);
  
  gtk_object_unref (GTK_OBJECT (snet_shell->tooltips));
  snet_shell->tooltips = NULL;
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_snet_shell_new (BseSNet *snet)
{
  GtkWidget *snet_shell;
  
  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  g_return_val_if_fail (bst_super_shell_from_super (BSE_SUPER (snet)) == NULL, NULL);
  
  snet_shell = gtk_widget_new (BST_TYPE_SNET_SHELL,
			       "super", snet,
			       NULL);
  
  return snet_shell;
}

static void
bst_snet_shell_build (BstSNetShell *snet_shell)
{
  GtkWidget *notebook;
  BseSNet *snet = BSE_SNET (BST_SUPER_SHELL (snet_shell)->super);

  snet_shell->param_view = (BstParamView*) bst_param_view_new (BSE_OBJECT (snet));
  gtk_widget_set (GTK_WIDGET (snet_shell->param_view),
		  "signal::destroy", gtk_widget_destroyed, &snet_shell->param_view,
		  "visible", TRUE,
		  NULL);
  snet_shell->snet_router = (BstSNetRouter*) bst_snet_router_new (snet);
  gtk_widget_set (GTK_WIDGET (snet_shell->snet_router),
		  "signal::destroy", gtk_widget_destroyed, &snet_shell->snet_router,
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
			     "parent", snet_shell,
			     "visible", TRUE,
			     NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (snet_shell->param_view),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", "Parameters",
					    "visible", TRUE,
					    NULL));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (snet_shell->snet_router),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", "Routing",
					    "visible", TRUE,
					    NULL));
}

static void
bst_snet_shell_setup_super (BstSuperShell *super_shell,
			    BseSuper      *super)
{
  BstSNetShell *snet_shell;
  
  snet_shell = BST_SNET_SHELL (super_shell);
  
  BST_SUPER_SHELL_CLASS (parent_class)->setup_super (super_shell, super);

  if (super)
    bst_snet_shell_build (snet_shell);
}

static void
bst_snet_shell_release_super (BstSuperShell *super_shell,
			      BseSuper      *super)
{
  BstSNetShell *snet_shell;
  
  snet_shell = BST_SNET_SHELL (super_shell);
  
  gtk_container_foreach (GTK_CONTAINER (snet_shell), (GtkCallback) gtk_widget_destroy, NULL);
  
  BST_SUPER_SHELL_CLASS (parent_class)->release_super (super_shell, super);
}

static void
bst_snet_shell_update (BstSuperShell *super_shell)
{
  BstSNetShell *snet_shell;
  
  snet_shell = BST_SNET_SHELL (super_shell);
  
  bst_param_view_update (snet_shell->param_view);
  bst_snet_router_update (snet_shell->snet_router);
}

static void
bst_snet_shell_rebuild (BstSuperShell *super_shell)
{
  BstSNetShell *snet_shell;
  
  snet_shell = BST_SNET_SHELL (super_shell);
  
  bst_param_view_rebuild (snet_shell->param_view);
  bst_snet_router_rebuild (snet_shell->snet_router);
}

static void
bst_snet_shell_operate (BstSuperShell *super_shell,
			BstOps         op)
{
  BseSNet *snet = BSE_SNET (super_shell->super);
  BstSNetShell *snet_shell = BST_SNET_SHELL (super_shell);
  BseMaster *master;

  g_return_if_fail (bst_snet_shell_can_operate (super_shell, op));
  
  master = bst_app_get_master (BST_APP (gtk_widget_get_toplevel (GTK_WIDGET (super_shell))));
  
  switch (op)
    {
    case BST_OP_PLAY:
      // FIXME: bse_master_add_source (master, BSE_SOURCE (snet), BSE_SNET_OCHANNEL_STEREO);
      break;
    case BST_OP_STOP:
      bse_source_clear_ochannels (BSE_SOURCE (snet));
      break;
    default:
      break;
    }

  bst_update_can_operate (GTK_WIDGET (snet_shell));
}

static gboolean
bst_snet_shell_can_operate (BstSuperShell *super_shell,
			    BstOps	   op)
{
  BstSNetShell *snet_shell = BST_SNET_SHELL (super_shell);
  BseSNet *snet = BSE_SNET (super_shell->super);

  switch (op)
    {
    case BST_OP_PLAY:
      return snet != NULL;
    case BST_OP_STOP:
      return snet != NULL;
    default:
      return FALSE;
    }
}
