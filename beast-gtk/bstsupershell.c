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
#include "bstsupershell.h"


enum {
  ARG_0,
  ARG_SUPER
};


/* --- prototypes --- */
static void	bst_super_shell_class_init	(BstSuperShellClass	*klass);
static void	bst_super_shell_init		(BstSuperShell		*super_shell);
static void	bst_super_shell_destroy		(GtkObject		*object);
static void	bst_super_shell_finalize	(GObject		*object);
static void	bst_super_shell_set_arg		(GtkObject		*object,
						 GtkArg			*arg,
						 guint		         arg_id);
static void	bst_super_shell_get_arg		(GtkObject		*object,
						 GtkArg			*arg,
						 guint		         arg_id);
static void	bst_super_shell_setup_super	(BstSuperShell		*super_shell,
						 BswProxy     		 super);
static void	bst_super_shell_release_super	(BstSuperShell		*super_shell,
						 BswProxy		 super);


/* --- static variables --- */
static GtkVBoxClass	  *parent_class = NULL;
static GQuark		   quark_super_shell = 0;
static BstSuperShellClass *bst_super_shell_class = NULL;


/* --- functions --- */
GtkType
bst_super_shell_get_type (void)
{
  static GtkType super_shell_type = 0;
  
  if (!super_shell_type)
    {
      GtkTypeInfo super_shell_info =
      {
	"BstSuperShell",
	sizeof (BstSuperShell),
	sizeof (BstSuperShellClass),
	(GtkClassInitFunc) bst_super_shell_class_init,
	(GtkObjectInitFunc) bst_super_shell_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      super_shell_type = gtk_type_unique (GTK_TYPE_VBOX, &super_shell_info);
    }
  
  return super_shell_type;
}

static void
bst_super_shell_class_init (BstSuperShellClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  bst_super_shell_class = class;
  parent_class = g_type_class_peek_parent (class);

  quark_super_shell = g_quark_from_static_string ("BstSuperShell");

  gobject_class->finalize = bst_super_shell_finalize;
  object_class->set_arg = bst_super_shell_set_arg;
  object_class->get_arg = bst_super_shell_get_arg;
  object_class->destroy = bst_super_shell_destroy;

  class->setup_super = bst_super_shell_setup_super;
  class->release_super = bst_super_shell_release_super;
  class->operate = NULL;
  class->can_operate = NULL;
  class->rebuild = NULL;
  class->update = NULL;

  gtk_object_add_arg_type ("BstSuperShell::super", GTK_TYPE_UINT, GTK_ARG_READWRITE | GTK_ARG_CONSTRUCT, ARG_SUPER);
}

static void
bst_super_shell_init (BstSuperShell *super_shell)
{
  super_shell->accel_group = gtk_accel_group_new ();
  super_shell->super = 0;
  super_shell->name_set_id = 0;
  
  gtk_widget_set (GTK_WIDGET (super_shell),
		  "homogeneous", FALSE,
		  "spacing", 0,
		  "border_width", 0,
		  NULL);
}

static void
bst_super_shell_destroy (GtkObject *object)
{
  BstSuperShell *super_shell = BST_SUPER_SHELL (object);

  if (super_shell->super)
    {
      bsw_source_clear_outputs (super_shell->super);
      bst_super_shell_set_super (super_shell, 0);
    }
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_super_shell_finalize (GObject *object)
{
  BstSuperShell *super_shell = BST_SUPER_SHELL (object);

  gtk_accel_group_unref (super_shell->accel_group);
  super_shell->accel_group = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bst_super_shell_set_arg (GtkObject *object,
			 GtkArg    *arg,
			 guint      arg_id)
{
  BstSuperShell *super_shell;

  super_shell = BST_SUPER_SHELL (object);

  switch (arg_id)
    {
    case ARG_SUPER:
      bst_super_shell_set_super (super_shell, GTK_VALUE_UINT (*arg));
      break;
    default:
      break;
    }
}

static void
bst_super_shell_get_arg (GtkObject *object,
			 GtkArg    *arg,
			 guint      arg_id)
{
  BstSuperShell *super_shell;

  super_shell = BST_SUPER_SHELL (object);

  switch (arg_id)
    {
    case ARG_SUPER:
      GTK_VALUE_UINT (*arg) = super_shell->super;
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static void
bst_super_shell_name_set (BstSuperShell *super_shell,
			  GParamSpec	*pspec,
			  BseSuper	*super)
{
  GtkWidget *widget;

  widget = GTK_WIDGET (super_shell);
  if (widget->parent && GTK_IS_NOTEBOOK (widget->parent))
    {
      gchar *name = g_strconcat (BSE_OBJECT_TYPE_NAME (super),
				 ":\n",
				 bsw_item_get_name (BSE_OBJECT_ID (super)),
				 NULL);
      
      widget = gtk_notebook_get_tab_label (GTK_NOTEBOOK (widget->parent), widget);
      if (widget)
	gtk_widget_set (widget,
			"label", name,
			/* "xalign", 0.0, */
			NULL);
      g_free (name);
    }
}

static void
bst_super_shell_setup_super (BstSuperShell *super_shell,
			     BswProxy       super)
{
  bse_object_set_qdata (bse_object_from_id (super), quark_super_shell, super_shell);
  super_shell->name_set_id = g_signal_connect_data (bse_object_from_id (super), "notify::name",
						    G_CALLBACK (bst_super_shell_name_set), super_shell, NULL,
						    G_CONNECT_SWAPPED);
  BST_SUPER_SHELL_GET_CLASS (super_shell)->rebuild (super_shell);
}

static void
bst_super_shell_release_super (BstSuperShell *super_shell,
			       BswProxy       super)
{
  g_signal_handler_disconnect (bse_object_from_id (super), super_shell->name_set_id);
  super_shell->name_set_id = 0;
  bse_object_set_qdata (bse_object_from_id (super), quark_super_shell, NULL);
  gtk_container_foreach (GTK_CONTAINER (super_shell), (GtkCallback) gtk_widget_destroy, NULL);
}

void
bst_super_shell_set_super (BstSuperShell *super_shell,
			   BswProxy       super)
{
  g_return_if_fail (BST_IS_SUPER_SHELL (super_shell));
  if (super)
    {
      g_return_if_fail (BSW_IS_SUPER (super));
      g_return_if_fail (bst_super_shell_from_super (super) == 0);
    }
  
  if (super != super_shell->super)
    {
      if (super_shell->super)
	{
	  BST_SUPER_SHELL_GET_CLASS (super_shell)->release_super (super_shell, super_shell->super);
	  bsw_item_unuse (super_shell->super);
	}
      super_shell->super = super;
      if (super_shell->super)
	{
	  bsw_item_use (super_shell->super);
	  BST_SUPER_SHELL_GET_CLASS (super_shell)->setup_super (super_shell, super_shell->super);
	  bst_super_shell_name_set (super_shell, NULL, bse_object_from_id (super));
	}
    }
}

void
bst_super_shell_update (BstSuperShell *super_shell)
{
  g_return_if_fail (BST_IS_SUPER_SHELL (super_shell));

  bst_super_shell_name_set (super_shell, NULL, bse_object_from_id (super_shell->super));
      
  if (BST_SUPER_SHELL_GET_CLASS (super_shell)->update)
    BST_SUPER_SHELL_GET_CLASS (super_shell)->update (super_shell);
}

void
bst_super_shell_update_parent (BstSuperShell *super_shell)
{
  g_return_if_fail (BST_IS_SUPER_SHELL (super_shell));

  bst_super_shell_name_set (super_shell, NULL, bse_object_from_id (super_shell->super));
}

BstSuperShell*
bst_super_shell_from_super (BswProxy super)
{
  BstSuperShell *super_shell;

  g_return_val_if_fail (BSW_IS_SUPER (super), NULL);

  super_shell = bse_object_get_qdata (bse_object_from_id (super), quark_super_shell);
  if (super_shell)
    g_return_val_if_fail (BST_IS_SUPER_SHELL (super_shell), NULL);

  return super_shell;
}

void
bst_super_shell_operate (BstSuperShell *super_shell,
			 BstOps         op)
{
  g_return_if_fail (BST_IS_SUPER_SHELL (super_shell));
  g_return_if_fail (bst_super_shell_can_operate (super_shell, op));
  
  gtk_widget_ref (GTK_WIDGET (super_shell));

  BST_SUPER_SHELL_GET_CLASS (super_shell)->operate (super_shell, op);

  bst_update_can_operate (GTK_WIDGET (super_shell));

  gtk_widget_unref (GTK_WIDGET (super_shell));
}

gboolean
bst_super_shell_can_operate (BstSuperShell *super_shell,
			     BstOps         op)
{
  gboolean can_do;

  g_return_val_if_fail (BST_IS_SUPER_SHELL (super_shell), FALSE);

  gtk_widget_ref (GTK_WIDGET (super_shell));

  if (BST_SUPER_SHELL_GET_CLASS (super_shell)->operate &&
      BST_SUPER_SHELL_GET_CLASS (super_shell)->can_operate)
    can_do = BST_SUPER_SHELL_GET_CLASS (super_shell)->can_operate (super_shell, op);
  else
    can_do = FALSE;

  gtk_widget_unref (GTK_WIDGET (super_shell));

  return can_do;
}
