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
#include "bstsupershell.h"
#include "bstactivatable.h"


enum {
  PROP_0,
  PROP_SUPER
};


/* --- prototypes --- */
static void	bst_super_shell_class_init	(BstSuperShellClass	*klass);
static void	bst_super_shell_init		(BstSuperShell		*super_shell);
static void	bst_super_shell_destroy		(GtkObject		*object);
static void	bst_super_shell_finalize	(GObject		*object);
static void	bst_super_shell_set_property	(GObject         	*object,
						 guint           	 prop_id,
						 const GValue    	*value,
						 GParamSpec      	*pspec);
static void	bst_super_shell_get_property	(GObject         	*object,
						 guint           	 prop_id,
						 GValue          	*value,
						 GParamSpec      	*pspec);
static gchar*   bst_super_shell_get_title	(BstSuperShell		*super_shell);
static void	bst_super_shell_setup_super	(BstSuperShell		*super_shell,
						 SfiProxy     		 super);
static void	bst_super_shell_release_super	(BstSuperShell		*super_shell,
						 SfiProxy		 super);
static void     bst_super_shell_activate        (BstActivatable         *activatable,
                                                 gulong                  action);
static gboolean bst_super_shell_can_activate    (BstActivatable         *activatable,
                                                 gulong                  action);


/* --- static variables --- */
static GtkVBoxClass	  *parent_class = NULL;
static BstSuperShellClass *bst_super_shell_class = NULL;


/* --- functions --- */
GType
bst_super_shell_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (BstSuperShellClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bst_super_shell_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (BstSuperShell),
        0,      /* n_preallocs */
        (GInstanceInitFunc) bst_super_shell_init,
      };
      type = g_type_register_static (GTK_TYPE_VBOX, "BstSuperShell", &type_info, 0);
      bst_type_implement_activatable (type,
                                      bst_super_shell_activate,
                                      bst_super_shell_can_activate,
                                      NULL);
    }
  return type;
}

static void
bst_super_shell_class_init (BstSuperShellClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  bst_super_shell_class = class;
  parent_class = g_type_class_peek_parent (class);

  gobject_class->set_property = bst_super_shell_set_property;
  gobject_class->get_property = bst_super_shell_get_property;
  gobject_class->finalize = bst_super_shell_finalize;

  object_class->destroy = bst_super_shell_destroy;

  class->get_title = bst_super_shell_get_title;
  class->setup_super = bst_super_shell_setup_super;
  class->release_super = bst_super_shell_release_super;
  class->rebuild = NULL;

  g_object_class_install_property (gobject_class,
				   PROP_SUPER,
				   sfi_pspec_proxy ("super", NULL, NULL, SFI_PARAM_DEFAULT));
}

static void
bst_super_shell_init (BstSuperShell *self)
{
  self->accel_group = gtk_accel_group_new ();
  self->super = 0;
  gtk_widget_set (GTK_WIDGET (self),
		  "homogeneous", FALSE,
		  "spacing", 0,
		  "border_width", 0,
		  NULL);
}

static void
bst_super_shell_set_property (GObject         *object,
			      guint            prop_id,
			      const GValue    *value,
			      GParamSpec      *pspec)
{
  BstSuperShell *self = BST_SUPER_SHELL (object);

  switch (prop_id)
    {
    case PROP_SUPER:
      bst_super_shell_set_super (self, sfi_value_get_proxy (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_super_shell_get_property (GObject         *object,
			      guint            prop_id,
			      GValue          *value,
			      GParamSpec      *pspec)
{
  BstSuperShell *self = BST_SUPER_SHELL (object);

  switch (prop_id)
    {
    case PROP_SUPER:
      sfi_value_set_proxy (value, self->super);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_super_shell_destroy (GtkObject *object)
{
  BstSuperShell *self = BST_SUPER_SHELL (object);

  if (self->super)
    {
      bse_source_clear_outputs (self->super);
      bst_super_shell_set_super (self, 0);
    }
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_super_shell_finalize (GObject *object)
{
  BstSuperShell *self = BST_SUPER_SHELL (object);

  gtk_accel_group_unref (self->accel_group);
  self->accel_group = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gchar*
bst_super_shell_get_title (BstSuperShell *self)
{
  if (self->super)
    return g_strconcat (bse_item_get_type (self->super),
                        ":\n",
                        bse_item_get_name (self->super),
                        NULL);
  else
    return g_strdup ("BstSuperShell");
}

static void
bst_super_shell_name_set (BstSuperShell *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  if (widget->parent && GTK_IS_NOTEBOOK (widget->parent))
    {
      gchar *name = BST_SUPER_SHELL_GET_CLASS (self)->get_title (self);

      widget = gtk_notebook_get_tab_label (GTK_NOTEBOOK (widget->parent), widget);
      if (widget)
	gtk_widget_set (widget,
			"label", name,
			NULL);
      g_free (name);
    }
}

static void
bst_super_shell_setup_super (BstSuperShell *self,
			     SfiProxy       super)
{
  bse_proxy_connect (super,
		     "swapped_signal::property-notify::uname", bst_super_shell_name_set, self,
		     NULL);
  BST_SUPER_SHELL_GET_CLASS (self)->rebuild (self);
}

static void
bst_super_shell_release_super (BstSuperShell *self,
			       SfiProxy       super)
{
  bse_proxy_disconnect (super,
			"any_signal::property-notify::uname", bst_super_shell_name_set, self,
			NULL);
  gtk_container_foreach (GTK_CONTAINER (self), (GtkCallback) gtk_widget_destroy, NULL);
}

void
bst_super_shell_set_super (BstSuperShell *self,
			   SfiProxy       super)
{
  g_return_if_fail (BST_IS_SUPER_SHELL (self));
  if (super)
    g_return_if_fail (BSE_IS_SUPER (super));
  
  if (super != self->super)
    {
      if (self->super)
	{
	  BST_SUPER_SHELL_GET_CLASS (self)->release_super (self, self->super);
	  bse_item_unuse (self->super);
	}
      self->super = super;
      if (self->super)
	{
	  bse_item_use (self->super);
	  BST_SUPER_SHELL_GET_CLASS (self)->setup_super (self, self->super);
	  bst_super_shell_name_set (self);
	}
    }
}

void
bst_super_shell_update_label (BstSuperShell *self)
{
  g_return_if_fail (BST_IS_SUPER_SHELL (self));

  bst_super_shell_name_set (self);
}

static void
bst_super_shell_activate (BstActivatable *activatable,
                          gulong          action)
{
  bst_widget_update_activatable (activatable);
}

static gboolean
bst_super_shell_can_activate (BstActivatable *activatable,
                              gulong          action)
{
  return FALSE;
}
