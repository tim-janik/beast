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
#include "bstsnetshell.h"

#include "bstparamview.h"
#include "bstapp.h"



/* --- prototypes --- */
static void	bst_snet_shell_class_init	(BstSNetShellClass	*klass);
static void	bst_snet_shell_init		(BstSNetShell		*pe);
static void	bst_snet_shell_rebuild		(BstSuperShell		*super_shell);
static void	bst_snet_shell_update		(BstSuperShell		*super_shell);
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
  super_shell_class->update = bst_snet_shell_update;

  class->factories_path = "<BstSNetShell>";
}

static void
bst_snet_shell_init (BstSNetShell *snet_shell)
{
  snet_shell->param_view = NULL;
  snet_shell->snet_router = NULL;
}

static void
zoomed_add_xpm (BstZoomedWindow *zoomed)
{
  if (!GTK_BIN (zoomed->toggle_button)->child)
    {
      GtkWidget *widget = GTK_WIDGET (zoomed);
      GdkPixmap *pixmap;
      GdkBitmap *mask;
      GtkWidget *pix;
      static const gchar *zoom_xpm[] = {
	"12 12 2 1", "  c None", "# c #000000",
	"####  ####  ",
	"##      ##  ",
	"# #    # #  ",
	"#  ####  #  ",
	"   #  #     ",
	"   #  #     ",
	"#  ####  #  ",
	"# #    # #  ",
	"##      ##  ",
	"####  ####  ",
	"            ",
	"            ",
      };

      pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
					     &mask,
					     NULL,
					     (gchar**) zoom_xpm);
      pix = gtk_pixmap_new (pixmap, mask);
      gdk_pixmap_unref (pixmap);
      gdk_pixmap_unref (mask);

      gtk_widget_set (pix,
		      "visible", TRUE,
		      "parent", zoomed->toggle_button,
		      NULL);
    }
}

static void
bst_snet_shell_rebuild (BstSuperShell *super_shell)
{
  BstSNetShell *snet_shell = BST_SNET_SHELL (super_shell);
  BseSNet *snet = bse_object_from_id (super_shell->super);
  GtkWidget *notebook, *zoomed_window;
  GtkWidget *snet_router_box;

  g_return_if_fail (snet_shell->param_view == NULL);

  snet_shell->param_view = (BstParamView*) bst_param_view_new (BSE_OBJECT (snet));
  g_object_set (GTK_WIDGET (snet_shell->param_view),
		"visible", TRUE,
		NULL);
  g_object_connect (GTK_WIDGET (snet_shell->param_view),
		    "signal::destroy", gtk_widget_destroyed, &snet_shell->param_view,
		    NULL);

  snet_router_box = gtk_widget_new (GTK_TYPE_VBOX,
				    "visible", TRUE,
				    "homogeneous", FALSE,
				    "spacing", 3,
				    "border_width", 5,
				    NULL);
  snet_shell->snet_router = (BstSNetRouter*) bst_snet_router_new (BSE_OBJECT_ID (snet));
  gtk_box_pack_start (GTK_BOX (snet_router_box), snet_shell->snet_router->toolbar, FALSE, TRUE, 0);
  zoomed_window = g_object_connect (gtk_widget_new (BST_TYPE_ZOOMED_WINDOW,
						    "visible", TRUE,
						    "hscrollbar_policy", GTK_POLICY_ALWAYS,
						    "vscrollbar_policy", GTK_POLICY_ALWAYS,
						    "parent", snet_router_box,
						    NULL),
				    "swapped_signal::zoom", bst_snet_router_adjust_region, snet_shell->snet_router,
				    "swapped_signal::zoom", gtk_false, NULL,
				    "signal::realize", zoomed_add_xpm, NULL,
				    NULL);
  g_object_set (GTK_WIDGET (snet_shell->snet_router),
		"visible", TRUE,
		"parent", zoomed_window,
		NULL);
  g_object_connect (GTK_WIDGET (snet_shell->snet_router),
		    "signal::destroy", gtk_widget_destroyed, &snet_shell->snet_router,
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
					     "parent", snet_shell,
					     "visible", TRUE,
					     NULL),
			       "signal_after::switch-page", gtk_widget_viewable_changed, NULL,
			       NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (snet_shell->param_view),
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", "Parameters",
					    "visible", TRUE,
					    NULL));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), snet_router_box,
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", "Routing",
					    "visible", TRUE,
					    NULL));
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
