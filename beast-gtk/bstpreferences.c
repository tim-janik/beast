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
#include "bstparam.h"


/* --- prototypes --- */
static void	  bst_preferences_class_init		(BstPreferencesClass	*klass);
static void	  bst_preferences_init			(BstPreferences		*prefs);
static void	  bst_preferences_destroy		(GtkObject		*object);
static GtkWidget* bst_preferences_build_rec_editor	(SfiRec			*rec,
							 SfiRecFields		 fields,
							 SfiRing	       **bparam_list);


/* --- static variables --- */
static gpointer             parent_class = NULL;


/* --- functions --- */
GtkType
bst_preferences_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstPreferencesClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_preferences_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstPreferences),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_preferences_init,
      };

      type = g_type_register_static (GTK_TYPE_VBOX, "BstPreferences", &type_info, 0);
    }
  return type;
}

static void
bst_preferences_class_init (BstPreferencesClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bst_preferences_destroy;
}

static void
bst_preferences_init (BstPreferences *prefs)
{
  GParamSpec *pspec;
  GtkWidget *pchild;
  SfiRec *rec;

  prefs->notebook = g_object_new (GTK_TYPE_NOTEBOOK,
				  "visible", TRUE,
				  "parent", prefs,
				  "tab_pos", GTK_POS_TOP,
				  "scrollable", FALSE,
				  "can_focus", TRUE,
				  "border_width", 5,
				  NULL);
  gxk_nullify_on_destroy (prefs->notebook, &prefs->notebook);
  g_object_connect (prefs->notebook,
		    "signal_after::switch-page", gxk_widget_viewable_changed, NULL,
		    NULL);

  pspec = bst_gconfig_pspec ();
  prefs->bstrec = bst_gconfig_to_rec (bst_global_config);
  pchild = bst_preferences_build_rec_editor (prefs->bstrec, sfi_pspec_get_rec_fields (pspec), &prefs->bstparams);
  gxk_notebook_append (prefs->notebook, pchild, "BEAST");

  pspec = bse_proxy_get_pspec (BSE_SERVER, "bse-preferences");
  prefs->bsepspec = g_param_spec_ref (pspec);
  bse_proxy_get (BSE_SERVER, "bse-preferences", &rec, NULL);
  prefs->bserec = sfi_rec_copy_deep (rec);
  pchild = bst_preferences_build_rec_editor (prefs->bserec, sfi_pspec_get_rec_fields (pspec), &prefs->bseparams);
  gxk_notebook_append (prefs->notebook, pchild, "BSE");
}

static void
bst_preferences_destroy (GtkObject *object)
{
  BstPreferences *prefs = BST_PREFERENCES (object);

  if (prefs->bstrec)
    {
      sfi_rec_unref (prefs->bstrec);
      prefs->bstrec = NULL;
    }
  sfi_ring_free (prefs->bstparams);
  prefs->bstparams = NULL;

  if (prefs->bsepspec)
    {
      g_param_spec_unref (prefs->bsepspec);
      prefs->bsepspec = NULL;
    }
  if (prefs->bserec)
    {
      sfi_rec_unref (prefs->bserec);
      prefs->bserec = NULL;
    }
  sfi_ring_free (prefs->bseparams);
  prefs->bseparams = NULL;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static GtkWidget*
bst_preferences_build_rec_editor (SfiRec      *rec,
				  SfiRecFields fields,
				  SfiRing    **bparam_list)
{
  GtkWidget *parent;
  SfiRing *ring, *bparams = NULL;
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
      if (sfi_pspec_test_hint (pspec, SFI_PARAM_SERVE_GUI))
	{
	  BstParam *bparam = bst_param_rec_create (pspec, FALSE, NULL, rec);
	  bst_param_pack_property (bparam, parent);
	  bparams = sfi_ring_append (bparams, bparam);
	}
    }
  for (ring = bparams; ring; ring = sfi_ring_walk (ring, bparams))
    bst_param_update (ring->data);
  if (bparam_list)
    *bparam_list = bparams;
  else
    sfi_ring_free (bparams);
  return parent;
}

static void
bst_preferences_update (BstPreferences *prefs)
{
  SfiRing *ring;
  for (ring = prefs->bstparams; ring; ring = sfi_ring_walk (ring, prefs->bstparams))
    bst_param_update (ring->data);
  for (ring = prefs->bseparams; ring; ring = sfi_ring_walk (ring, prefs->bseparams))
    bst_param_update (ring->data);
}

void
bst_preferences_revert (BstPreferences *prefs)
{
  SfiRec *rec, *crec;

  g_return_if_fail (BST_IS_PREFERENCES (prefs));

  rec = bst_gconfig_to_rec (bst_global_config);
  crec = sfi_rec_copy_deep (rec);
  sfi_rec_unref (rec);
  sfi_rec_swap_fields (prefs->bstrec, crec);
  sfi_rec_unref (crec);
  bst_preferences_update (prefs);

  bse_proxy_get (BSE_SERVER, "bse-preferences", &rec, NULL);
  crec = sfi_rec_copy_deep (rec);
  sfi_rec_swap_fields (prefs->bserec, crec);
  sfi_rec_unref (crec);
  bst_preferences_update (prefs);
}

void
bst_preferences_apply (BstPreferences *prefs)
{
  g_return_if_fail (BST_IS_PREFERENCES (prefs));

  bst_gconfig_apply (prefs->bstrec);
  bse_proxy_set (BSE_SERVER, "bse-preferences", prefs->bserec, NULL);
  bst_preferences_revert (prefs);
}

void
bst_preferences_default_revert (BstPreferences *prefs)
{
  SfiRec *rec;

  g_return_if_fail (BST_IS_PREFERENCES (prefs));

  rec = sfi_rec_new ();
  sfi_rec_validate (rec, sfi_pspec_get_rec_fields (bst_gconfig_pspec ()));
  sfi_rec_swap_fields (prefs->bstrec, rec);
  sfi_rec_unref (rec);

  rec = sfi_rec_new ();
  sfi_rec_validate (rec, sfi_pspec_get_rec_fields (prefs->bsepspec));
  sfi_rec_swap_fields (prefs->bserec, rec);
  sfi_rec_unref (rec);

  bst_preferences_update (prefs);
}

void
bst_preferences_save (BstPreferences *prefs)
{
  BseErrorType error = 0;
  gchar *file_name;

  g_return_if_fail (BST_IS_PREFERENCES (prefs));

  file_name = BST_STRDUP_RC_FILE ();
  bse_server_save_preferences (BSE_SERVER);
  error = bst_rc_dump (file_name);
  if (error)
    g_warning ("failed to save rc-file \"%s\": %s", file_name, bse_error_blurb (error));
  g_free (file_name);
}

void
bst_preferences_create_buttons (BstPreferences *prefs,
				GxkDialog      *dialog)
{
  GtkWidget *widget;

  g_return_if_fail (BST_IS_PREFERENCES (prefs));
  g_return_if_fail (GXK_IS_DIALOG (dialog));
  g_return_if_fail (prefs->apply == NULL);

  /* Apply
   */
  prefs->apply = g_object_connect (gxk_dialog_default_action (dialog, BST_STOCK_APPLY, NULL, NULL),
				   "swapped_signal::clicked", bst_preferences_apply, prefs,
				   "swapped_signal::clicked", bst_preferences_save, prefs,
				   "swapped_signal::destroy", g_nullify_pointer, &prefs->apply,
				   NULL);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, prefs->apply,
			"Apply and save the preference values. Some values may only take effect after "
			"restart while others can be locked against modifcation during "
			"playback.",
			NULL);

  /* Revert
   */
  widget = gxk_dialog_action_swapped (dialog, BST_STOCK_REVERT, bst_preferences_revert, prefs);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, widget,
			"Revert to the currently active values.",
			NULL);

  /* Default Revert
   */
  widget = gxk_dialog_action_swapped (dialog, BST_STOCK_DEFAULT_REVERT, bst_preferences_default_revert, prefs);
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
