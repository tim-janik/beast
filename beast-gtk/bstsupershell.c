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
#include "bstparamview.h"
#include "bsttrackview.h"
#include "bstpartview.h"
#include "bstbusmixer.h"
#include "bstbusview.h"
#include "bstwaveview.h"
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
bst_super_shell_update_label (BstSuperShell *self)
{
  g_return_if_fail (BST_IS_SUPER_SHELL (self));

  GtkWidget *widget = GTK_WIDGET (self);
  GtkWidget *tab = gxk_notebook_descendant_get_tab (widget);
  GtkWidget *box = tab ? GTK_BIN (tab)->child : NULL;
  GtkWidget *label = tab ? gtk_box_get_nth_child (GTK_BOX (box), 1) : NULL;
  if (GTK_IS_LABEL (label) && self->super)
    {
      /* discriminate super */
      const gchar *stock, *name = bse_item_get_name (self->super);
      gchar *tip;
      if (BSE_IS_WAVE_REPO (self->super))
        {
          name = _("Waves");
          tip = g_strdup (_("Wave Repository"));
          stock = BST_STOCK_MINI_WAVE_REPO;
        }
      else if (BSE_IS_SONG (self->super))
        {
          tip = g_strdup_printf (_("Song: %s"), name);
          stock = BST_STOCK_MINI_SONG;
        }
      else if (BSE_IS_MIDI_SYNTH (self->super))
        {
          tip = g_strdup_printf (_("MIDI Synthesizer: %s"), name);
          stock = BST_STOCK_MINI_MIDI_SYNTH;
        }
      else
        {
          tip = g_strdup_printf (_("Synthesizer: %s"), name);
          stock = BST_STOCK_MINI_CSYNTH;
        }
      /* update tooltip */
      gxk_widget_set_tooltip (tab, tip);
      g_free (tip);
      /* update image */
      GtkWidget *image = gtk_box_get_nth_child (GTK_BOX (box), 0);
      if (GTK_IS_IMAGE (image))
        {
          gchar *ostock = NULL;
          GtkIconSize isize = 0;
          gtk_image_get_stock (GTK_IMAGE (image), &ostock, &isize);
          if (!ostock || strcmp (ostock, stock) != 0 || isize != GXK_ICON_SIZE_TABULATOR)
            gtk_image_set_from_stock (GTK_IMAGE (image), stock, GXK_ICON_SIZE_TABULATOR);
        }
      /* update label */
      if (BSE_IS_WAVE_REPO (self->super))
        name = _("Waves");
      else if (self->super)
        name = bse_item_get_name (self->super);
      g_object_set (label, "label", name, NULL);
    }
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
          bse_proxy_disconnect (self->super,
                                "any_signal::property-notify::uname", bst_super_shell_update_label, self,
                                NULL);
          gtk_container_foreach (GTK_CONTAINER (self), (GtkCallback) gtk_widget_destroy, NULL);
	  bse_item_unuse (self->super);
	}
      self->super = super;
      if (self->super)
	{
	  bse_item_use (self->super);
          bse_proxy_connect (self->super,
                             "swapped_signal::property-notify::uname", bst_super_shell_update_label, self,
                             NULL);
          super_shell_add_views (self);
	  bst_super_shell_update_label (self);
	}
    }
}

GtkWidget*
bst_super_shell_create_label (BstSuperShell *super_shell)
{
  GtkWidget *ev = g_object_new (GTK_TYPE_EVENT_BOX, NULL);
  GtkWidget *image = gtk_image_new();
  GtkWidget *label = g_object_new (GTK_TYPE_LABEL,
                                   "width_request", BST_TAB_WIDTH ? BST_TAB_WIDTH : -1,
                                   NULL);
  GtkWidget *box = g_object_new (GTK_TYPE_HBOX, "parent", ev, NULL);
  gtk_box_pack_start (GTK_BOX (box), image, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_widget_show_all (ev);
  return ev;
}

static void
super_shell_build_song (BstSuperShell *self,
                        GtkNotebook   *notebook)
{
  SfiProxy song = self->super;

  gtk_notebook_append_page (notebook,
                            bst_track_view_new (song),
			    g_object_new (GTK_TYPE_LABEL, "visible", TRUE,
                                          "label", _("Tracks"),
                                          NULL));
  gtk_notebook_append_page (notebook,
                            bst_bus_mixer_new (song),
                            g_object_new (GTK_TYPE_LABEL, "visible", TRUE,
                                          "label", _("Mixer"),
                                          NULL));
  gtk_notebook_append_page (notebook,
                            bst_param_view_new (song),
                            g_object_new (GTK_TYPE_LABEL, "visible", TRUE,
                                          "label", _("Parameters"),
                                          NULL));
  gtk_notebook_append_page (notebook,
                            bst_part_view_new (song),
                            g_object_new (GTK_TYPE_LABEL, "visible", TRUE,
                                          "label", _("Parts"),
                                          NULL));
  if (BST_DBG_EXT)
    gtk_notebook_append_page (notebook,
                              bst_bus_view_new (song),
                              g_object_new (GTK_TYPE_LABEL, "visible", TRUE,
                                            "label", _("Busses"),
                                            NULL));
  if (BST_DBG_EXT)
    gtk_notebook_append_page (notebook,
                              gtk_widget_get_toplevel (GTK_WIDGET (bst_snet_router_build_page (song))),
                              g_object_new (GTK_TYPE_LABEL, "visible", TRUE,
                                            "label", _("Routing"),
                                            NULL));
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
                              g_object_new (GTK_TYPE_LABEL, "label", _("Rack"), NULL));
  if (bse_snet_supports_user_synths (snet) || BST_DBG_EXT)
    gtk_notebook_append_page (notebook,
                              gtk_widget_get_toplevel (GTK_WIDGET (bst_snet_router_build_page (snet))),
                              g_object_new (GTK_TYPE_LABEL, "visible", TRUE,
                                            "label", _("Routing"),
                                            NULL));
  param_view = bst_param_view_new (snet);
  gtk_notebook_append_page (notebook,
                            bst_param_view_new (snet),
                            g_object_new (GTK_TYPE_LABEL, "visible", TRUE,
                                          "label", _("Parameters"),
                                          NULL));
}

static void
super_shell_build_wave_repo (BstSuperShell *self,
                             GtkNotebook   *notebook)
{
  SfiProxy wrepo = self->super;

  gtk_notebook_append_page (notebook,
                            bst_wave_view_new (wrepo),
                            g_object_new (GTK_TYPE_LABEL, "visible", TRUE,
                                          "label", _("Waves"),
                                          NULL));
  gtk_notebook_append_page (notebook,
                            bst_param_view_new (wrepo),
                            g_object_new (GTK_TYPE_LABEL, "visible", TRUE,
                                          "label", _("Parameters"),
                                          NULL));
}

static GtkNotebook*
create_notebook (BstSuperShell *self)
{
  GtkNotebook *notebook = g_object_new (GTK_TYPE_NOTEBOOK,
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
  g_object_connect (notebook, "signal_after::switch-page", gxk_widget_viewable_changed, NULL, NULL);
  return notebook;
}

static void
super_shell_add_views (BstSuperShell *self)
{
  if (BSE_IS_SONG (self->super))
    super_shell_build_song (self, create_notebook (self));
  else if (BSE_IS_WAVE_REPO (self->super))
    super_shell_build_wave_repo (self, create_notebook (self));
  else /* BSE_IS_SNET (self->super) */
    super_shell_build_snet (self, create_notebook (self));
}
