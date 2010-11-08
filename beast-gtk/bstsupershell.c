/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#include "bstsupershell.h"
#include "bstparamview.h"
#include "bsttrackview.h"
#include "bstpartview.h"
#include "bstbusmixer.h"
#include "bstbusview.h"
#include "bstwaveview.h"
#include "bstsoundfontview.h"
#include "bstrackview.h"
#include "bstsnetrouter.h"
#include "bstgconfig.h"
#include <string.h>

enum {
  PROP_0,
  PROP_SUPER
};


/* --- prototypes --- */
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
static void     super_shell_add_views           (BstSuperShell          *self);


/* --- static variables --- */
static BstSuperShellClass *bst_super_shell_class = NULL;


/* --- functions --- */
G_DEFINE_TYPE (BstSuperShell, bst_super_shell, GTK_TYPE_VBOX);

static void
bst_super_shell_class_init (BstSuperShellClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  bst_super_shell_class = class;

  gobject_class->set_property = bst_super_shell_set_property;
  gobject_class->get_property = bst_super_shell_get_property;
  gobject_class->finalize = bst_super_shell_finalize;

  object_class->destroy = bst_super_shell_destroy;

  g_object_class_install_property (gobject_class,
				   PROP_SUPER,
				   sfi_pspec_proxy ("super", NULL, NULL, SFI_PARAM_STANDARD));
}

static void
bst_super_shell_init (BstSuperShell *self)
{
  self->super = 0;
  gtk_widget_set (GTK_WIDGET (self),
                  "visible", TRUE,
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
    bst_super_shell_set_super (self, 0);
  
  GTK_OBJECT_CLASS (bst_super_shell_parent_class)->destroy (object);
}

static void
bst_super_shell_finalize (GObject *object)
{
  // BstSuperShell *self = BST_SUPER_SHELL (object);

  G_OBJECT_CLASS (bst_super_shell_parent_class)->finalize (object);
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
          gtk_container_foreach (GTK_CONTAINER (self), (GtkCallback) gtk_widget_destroy, NULL);
	  bse_item_unuse (self->super);
	}
      self->super = super;
      if (self->super)
	{
	  bse_item_use (self->super);
          super_shell_add_views (self);
	}
    }
}

GtkWidget*
bst_super_shell_create_label (BstSuperShell *super_shell)
{
  return gxk_notebook_create_tabulator ("SuperShell", NULL, NULL);
}

static void
super_shell_build_song (BstSuperShell *self,
                        GtkNotebook   *notebook)
{
  SfiProxy song = self->super;

  gtk_notebook_append_page (notebook,
                            bst_track_view_new (song),
                            gxk_notebook_create_tabulator (_("Tracks"), BST_STOCK_TRACKS, _("Tracks contain instrument definitions and parts with notes")));
  gtk_notebook_append_page (notebook,
                            bst_bus_mixer_new (song),
                            gxk_notebook_create_tabulator (_("Mixer"), BST_STOCK_MIXER, _("Mix track outputs, adjust volume and add effects")));
  gtk_notebook_append_page (notebook,
                            bst_param_view_new (song),
                            gxk_notebook_create_tabulator (_("Properties"), BST_STOCK_PROPERTIES, _("Adjust overall song behaviour")));
  if (BST_DBG_EXT)
    gtk_notebook_append_page (notebook,
                              bst_part_view_new (song),
                              gxk_notebook_create_tabulator (_("Parts"), BST_STOCK_PART, NULL));
  if (BST_DBG_EXT)
    gtk_notebook_append_page (notebook,
                              bst_bus_view_new (song),
                              gxk_notebook_create_tabulator (_("Busses"), BST_STOCK_BUS, NULL));
  if (BST_DBG_EXT)
    gtk_notebook_append_page (notebook,
                              gtk_widget_get_toplevel (GTK_WIDGET (bst_snet_router_build_page (song))),
                              gxk_notebook_create_tabulator (_("Routing"), BST_STOCK_MESH, NULL));
}

static void
super_shell_build_snet (BstSuperShell *self,
                        GtkNotebook   *notebook)
{
  SfiProxy snet = self->super;
  GtkWidget *param_view;

  if (BST_DBG_EXT && bse_snet_supports_user_synths (snet))
    gtk_notebook_append_page (notebook,
                              gtk_widget_get_toplevel (bst_rack_view_new (snet)),
                              gxk_notebook_create_tabulator (_("Rack"), NULL, NULL));
  if (bse_snet_supports_user_synths (snet) || BST_DBG_EXT)
    gtk_notebook_append_page (notebook,
                              gtk_widget_get_toplevel (GTK_WIDGET (bst_snet_router_build_page (snet))),
                              gxk_notebook_create_tabulator (_("Routing"), BST_STOCK_MESH, _("Add, edit and connect synthesizer mesh components")));
  param_view = bst_param_view_new (snet);
  gtk_notebook_append_page (notebook,
                            bst_param_view_new (snet),
                            gxk_notebook_create_tabulator (_("Properties"), BST_STOCK_PROPERTIES, _("Adjust overall synthesizer behaviour")));
}

static void
super_shell_build_wave_repo (BstSuperShell *self,
                             GtkNotebook   *notebook)
{
  SfiProxy wrepo = self->super;

  gtk_notebook_append_page (notebook,
                            bst_wave_view_new (wrepo),
                            gxk_notebook_create_tabulator (_("Waves"), BST_STOCK_MINI_WAVE_REPO, NULL));
  gtk_notebook_append_page (notebook,
                            bst_param_view_new (wrepo),
                            gxk_notebook_create_tabulator (_("Properties"), BST_STOCK_PROPERTIES, NULL));
}

static void
super_shell_build_sound_font_repo (BstSuperShell *self,
                                   GtkNotebook   *notebook)
{
  SfiProxy sfrepo = self->super;

  gtk_notebook_append_page (notebook,
                            bst_sound_font_view_new (sfrepo),
                            gxk_notebook_create_tabulator (_("Sound Fonts"), BST_STOCK_MINI_WAVE_REPO, NULL));
  gtk_notebook_append_page (notebook,
                            bst_param_view_new (sfrepo),
                            gxk_notebook_create_tabulator (_("Properties"), BST_STOCK_PROPERTIES, NULL));
}

static GtkNotebook*
create_notebook (BstSuperShell *self)
{
  GtkNotebook *notebook = g_object_new (GXK_TYPE_NOTEBOOK,
                                        "scrollable", FALSE,
                                        "tab_border", 0,
                                        "show_border", TRUE,
                                        "enable_popup", FALSE,
                                        "show_tabs", TRUE,
                                        "tab_pos", GTK_POS_TOP,
                                        "border_width", 3,
                                        "parent", self,
                                        "visible", TRUE,
                                        NULL);
  return notebook;
}

static void
super_shell_add_views (BstSuperShell *self)
{
  if (BSE_IS_SONG (self->super))
    super_shell_build_song (self, create_notebook (self));
  else if (BSE_IS_WAVE_REPO (self->super))
    super_shell_build_wave_repo (self, create_notebook (self));
  else if (BSE_IS_SOUND_FONT_REPO (self->super))
    super_shell_build_sound_font_repo (self, create_notebook (self));
  else /* BSE_IS_SNET (self->super) */
    super_shell_build_snet (self, create_notebook (self));
}
