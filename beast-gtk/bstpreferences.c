/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999-2002 Tim Janik and Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bstpreferences.h"
#include "bstgconfig.h"
#include "bstskinconfig.h"
#include "bstkeybindings.h"
#include "bstpatternctrl.h"
#include "bstparam.h"


/* --- prototypes --- */
static void	  bst_preferences_destroy		(GtkObject		*object);
static GtkWidget* bst_preferences_build_rec_editor	(SfiRec			*rec,
							 SfiRecFields		 fields,
							 SfiRing	       **bparam_list);

/* --- functions --- */
G_DEFINE_TYPE (BstPreferences, bst_preferences, GTK_TYPE_VBOX);

static void
bst_preferences_class_init (BstPreferencesClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  object_class->destroy = bst_preferences_destroy;
}

static void
bst_preferences_init (BstPreferences *self)
{
  BstKeyBindingItemSeq *iseq;
  BstKeyBinding *kbinding;
  GParamSpec *pspec;
  GtkWidget *pchild;
  SfiRec *rec;
  
  self->notebook = g_object_new (GTK_TYPE_NOTEBOOK,
                                 "visible", TRUE,
                                 "parent", self,
                                 "tab_pos", GTK_POS_TOP,
                                 "scrollable", FALSE,
                                 "can_focus", TRUE,
                                 "border_width", 5,
                                 NULL);
  gxk_nullify_in_object (self, &self->notebook);
  g_object_connect (self->notebook,
		    "signal_after::switch-page", gxk_widget_viewable_changed, NULL,
		    NULL);
  
  pspec = bst_gconfig_pspec ();
  self->rec_gconfig = bst_gconfig_to_rec (bst_gconfig_get_global ());
  pchild = bst_preferences_build_rec_editor (self->rec_gconfig, sfi_pspec_get_rec_fields (pspec), &self->params_gconfig);
  gxk_notebook_append (self->notebook, pchild, "BEAST");
  
  kbinding = bst_pattern_controller_bindings();
  iseq = bst_key_binding_get_item_seq (kbinding);
  self->box_pview = bst_key_binding_box (kbinding->n_funcs, kbinding->funcs);
  bst_key_binding_box_set (self->box_pview, iseq);
  bst_key_binding_item_seq_free (iseq);
  gxk_notebook_append (self->notebook, self->box_pview, _("Key Bindings"));
  
  pspec = bst_skin_config_pspec ();
  self->rec_skin = bst_skin_config_to_rec (bst_skin_config_get_global ());
  pchild = bst_preferences_build_rec_editor (self->rec_skin, sfi_pspec_get_rec_fields (pspec), &self->params_skin);
  gxk_notebook_append (self->notebook, pchild, _("Skin"));
  
  pspec = bse_proxy_get_pspec (BSE_SERVER, "bse-preferences");
  self->bsepspec = g_param_spec_ref (pspec);
  bse_proxy_get (BSE_SERVER, "bse-preferences", &rec, NULL);
  self->bserec = sfi_rec_copy_deep (rec);
  pchild = bst_preferences_build_rec_editor (self->bserec, sfi_pspec_get_rec_fields (pspec), &self->bseparams);
  gxk_notebook_append (self->notebook, pchild, "BSE");
}

static void
bst_preferences_destroy (GtkObject *object)
{
  BstPreferences *self = BST_PREFERENCES (object);
  
  if (self->rec_gconfig)
    {
      sfi_rec_unref (self->rec_gconfig);
      self->rec_gconfig = NULL;
    }
  sfi_ring_free (self->params_gconfig);
  self->params_gconfig = NULL;
  
  if (self->rec_skin)
    {
      sfi_rec_unref (self->rec_skin);
      self->rec_skin = NULL;
    }
  sfi_ring_free (self->params_skin);
  self->params_skin = NULL;
  
  if (self->bsepspec)
    {
      g_param_spec_unref (self->bsepspec);
      self->bsepspec = NULL;
    }
  if (self->bserec)
    {
      sfi_rec_unref (self->bserec);
      self->bserec = NULL;
    }
  sfi_ring_free (self->bseparams);
  self->bseparams = NULL;
  
  GTK_OBJECT_CLASS (bst_preferences_parent_class)->destroy (object);
}

static GtkWidget*
bst_preferences_build_rec_editor (SfiRec      *rec,
				  SfiRecFields fields,
				  SfiRing    **param_list)
{
  GtkWidget *parent;
  SfiRing *ring, *params = NULL;
  guint i;
  
  g_return_val_if_fail (rec != NULL, NULL);
  
  parent = g_object_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "homogeneous", FALSE,
			 "border_width", 5,
			 NULL);
  for (i = 0; i < fields.n_fields; i++)
    {
      GParamSpec *pspec = fields.fields[i];
      if (sfi_pspec_check_option (pspec, "G"))     /* GUI representable */
	{
	  GxkParam *param = bst_param_new_rec (pspec, rec);
	  bst_param_create_gmask (param, NULL, parent);
	  params = sfi_ring_append (params, param);
	}
    }
  for (ring = params; ring; ring = sfi_ring_walk (ring, params))
    gxk_param_update (ring->data);
  if (param_list)
    *param_list = params;
  else
    sfi_ring_free (params);
  return parent;
}

static void
bst_preferences_update_params (BstPreferences *self)
{
  SfiRing *ring;
  for (ring = self->params_gconfig; ring; ring = sfi_ring_walk (ring, self->params_gconfig))
    gxk_param_update (ring->data);
  for (ring = self->params_skin; ring; ring = sfi_ring_walk (ring, self->params_skin))
    gxk_param_update (ring->data);
  for (ring = self->bseparams; ring; ring = sfi_ring_walk (ring, self->bseparams))
    gxk_param_update (ring->data);
}

void
bst_preferences_revert (BstPreferences *self)
{
  BstKeyBindingItemSeq *iseq;
  BstKeyBinding *kbinding;
  SfiRec *rec, *crec;
  
  g_return_if_fail (BST_IS_PREFERENCES (self));
  
  rec = bst_gconfig_to_rec (bst_gconfig_get_global ());
  crec = sfi_rec_copy_deep (rec);
  sfi_rec_unref (rec);
  sfi_rec_swap_fields (self->rec_gconfig, crec);
  sfi_rec_unref (crec);

  kbinding = bst_pattern_controller_bindings();
  iseq = bst_key_binding_get_item_seq (kbinding);
  bst_key_binding_box_set (self->box_pview, iseq);
  bst_key_binding_item_seq_free (iseq);

  rec = bst_skin_config_to_rec (bst_skin_config_get_global ());
  crec = sfi_rec_copy_deep (rec);
  sfi_rec_unref (rec);
  sfi_rec_swap_fields (self->rec_skin, crec);
  sfi_rec_unref (crec);

  bse_proxy_get (BSE_SERVER, "bse-preferences", &rec, NULL);
  crec = sfi_rec_copy_deep (rec);
  sfi_rec_swap_fields (self->bserec, crec);
  sfi_rec_unref (crec);

  bst_preferences_update_params (self);
}

void
bst_preferences_default_revert (BstPreferences *self)
{
  BstKeyBindingItemSeq *iseq;
  BstKeyBinding *kbinding;
  SfiRec *rec;
  
  g_return_if_fail (BST_IS_PREFERENCES (self));
  
  rec = sfi_rec_new ();
  sfi_rec_validate (rec, sfi_pspec_get_rec_fields (bst_gconfig_pspec ()));
  sfi_rec_swap_fields (self->rec_gconfig, rec);
  sfi_rec_unref (rec);

  kbinding = bst_pattern_controller_bindings();
  iseq = bst_key_binding_get_item_seq (bst_pattern_controller_default_bindings());
  bst_key_binding_box_set (self->box_pview, iseq);
  bst_key_binding_item_seq_free (iseq);

  rec = sfi_rec_new ();
  sfi_rec_validate (rec, sfi_pspec_get_rec_fields (bst_skin_config_pspec ()));
  sfi_rec_swap_fields (self->rec_skin, rec);
  sfi_rec_unref (rec);
  
  rec = sfi_rec_new ();
  sfi_rec_validate (rec, sfi_pspec_get_rec_fields (self->bsepspec));
  sfi_rec_swap_fields (self->bserec, rec);
  sfi_rec_unref (rec);
  
  bst_preferences_update_params (self);
}

void
bst_preferences_apply (BstPreferences *self)
{
  BstKeyBindingItemSeq *iseq;
  BstKeyBinding *kbinding;
  g_return_if_fail (BST_IS_PREFERENCES (self));
  
  bst_gconfig_apply (self->rec_gconfig);

  kbinding = bst_pattern_controller_bindings();
  iseq = bst_key_binding_box_get (self->box_pview);
  bst_key_binding_set_item_seq (kbinding, iseq);
  bst_key_binding_item_seq_free (iseq);

  bst_skin_config_apply (self->rec_skin, NULL);

  bse_proxy_set (BSE_SERVER, "bse-preferences", self->bserec, NULL);

  bst_preferences_revert (self);
}

void
bst_preferences_save (BstPreferences *self)
{
  BseErrorType error = 0;
  gchar *file_name;
  GSList *slist = NULL;

  g_return_if_fail (BST_IS_PREFERENCES (self));
  
  bse_server_save_preferences (BSE_SERVER);
  
  file_name = BST_STRDUP_RC_FILE ();
  error = bst_rc_dump (file_name);
  if (error)
    g_warning ("failed to save rc-file \"%s\": %s", file_name, bse_error_blurb (error));
  g_free (file_name);

  file_name = g_strdup (bst_key_binding_rcfile ());
  slist = g_slist_append (slist, bst_pattern_controller_bindings());
  error = bst_key_binding_dump (file_name, slist);
  if (error)
    g_warning ("failed to save skinrc \"%s\": %s", file_name, bse_error_blurb (error));
  g_slist_free (slist);

  file_name = g_strdup (bst_skin_config_rcfile ());
  error = bst_skin_dump (file_name);
  if (error)
    g_warning ("failed to save skinrc \"%s\": %s", file_name, bse_error_blurb (error));
  g_free (file_name);
}

void
bst_preferences_create_buttons (BstPreferences *self,
				GxkDialog      *dialog)
{
  GtkWidget *widget;
  
  g_return_if_fail (BST_IS_PREFERENCES (self));
  g_return_if_fail (GXK_IS_DIALOG (dialog));
  g_return_if_fail (self->apply == NULL);
  
  /* Apply
   */
  self->apply = g_object_connect (gxk_dialog_default_action (dialog, BST_STOCK_APPLY, NULL, NULL),
                                  "swapped_signal::clicked", bst_preferences_apply, self,
                                  "swapped_signal::clicked", bst_preferences_save, self,
                                  "swapped_signal::destroy", g_nullify_pointer, &self->apply,
                                  NULL);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, self->apply,
			"Apply and save the preference values. Some values may only take effect after "
			"restart while others can be locked against modifcation during "
			"playback.",
			NULL);
  
  /* Revert
   */
  widget = gxk_dialog_action_swapped (dialog, BST_STOCK_REVERT, bst_preferences_revert, self);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, widget,
			"Revert to the currently active values.",
			NULL);
  
  /* Default Revert
   */
  widget = gxk_dialog_action_swapped (dialog, BST_STOCK_DEFAULT_REVERT, bst_preferences_default_revert, self);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, widget,
			"Revert to hardcoded default values (factory settings).",
			NULL);
  
  /* Close
   */
  widget = gxk_dialog_action (dialog, BST_STOCK_CLOSE, gxk_toplevel_delete, NULL);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, widget,
			"Discard changes and close dialog.",
			NULL);
}
