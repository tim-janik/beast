// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstpreferences.hh"
#include "bstgconfig.hh"
#include "bstskinconfig.hh"
#include "bstkeybindings.hh"
#include "bstmsgabsorb.hh"
#include "bstpatternctrl.hh"
#include "topconfig.h" /* BST_VERSION */
#include "bstparam.hh"


/* --- prototypes --- */
static void	  bst_preferences_destroy		(GtkObject		*object);
static GtkWidget* bst_preferences_build_rec_editor	(SfiRec			*rec,
							 SfiRecFields		 fields,
							 SfiRing	       **bparam_list);

/* --- functions --- */
G_DEFINE_TYPE (BstPreferences, bst_preferences, GTK_TYPE_VBOX);

static void
bst_preferences_class_init (BstPreferencesClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
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

  self->notebook = (GtkNotebook*) g_object_new (GXK_TYPE_NOTEBOOK,
                                                "visible", TRUE,
                                                "parent", self,
                                                "tab_pos", GTK_POS_TOP,
                                                "scrollable", FALSE,
                                                "can_focus", TRUE,
                                                "border_width", 5,
                                                NULL);
  gxk_nullify_in_object (self, &self->notebook);

  pspec = bst_gconfig_pspec ();
  self->rec_gconfig = bst_gconfig_to_rec (bst_gconfig_get_global ());
  pchild = bst_preferences_build_rec_editor (self->rec_gconfig, sfi_pspec_get_rec_fields (pspec), &self->params_gconfig);
  gxk_notebook_append (self->notebook, pchild, "BEAST", FALSE);

  kbinding = bst_pattern_controller_piano_keys();
  iseq = bst_key_binding_get_item_seq (kbinding);
  self->box_piano_keys = bst_key_binding_box (kbinding->binding_name, kbinding->n_funcs, kbinding->funcs, TRUE);
  bst_key_binding_box_set (self->box_piano_keys, iseq);
  bst_key_binding_item_seq_free (iseq);
  gxk_notebook_append (self->notebook, self->box_piano_keys, _("Piano Keys"), FALSE);

  kbinding = bst_pattern_controller_generic_keys();
  iseq = bst_key_binding_get_item_seq (kbinding);
  self->box_generic_keys = bst_key_binding_box (kbinding->binding_name, kbinding->n_funcs, kbinding->funcs, FALSE);
  bst_key_binding_box_set (self->box_generic_keys, iseq);
  bst_key_binding_item_seq_free (iseq);
  gxk_notebook_append (self->notebook, self->box_generic_keys, _("Generic Keys"), FALSE);

  self->box_msg_absorb_config = bst_msg_absorb_config_box();
  bst_msg_absorb_config_box_set (self->box_msg_absorb_config, bst_msg_absorb_config_get_global());
  gxk_notebook_append (self->notebook, self->box_msg_absorb_config, _("Messages"), FALSE);

  pspec = bst_skin_config_pspec ();
  self->rec_skin = bst_skin_config_to_rec (bst_skin_config_get_global ());
  pchild = bst_preferences_build_rec_editor (self->rec_skin, sfi_pspec_get_rec_fields (pspec), &self->params_skin);
  gxk_notebook_append (self->notebook, pchild, _("Skin"), FALSE);

  pspec = bse_proxy_get_pspec (BSE_SERVER, "bse-preferences");
  self->bsepspec = g_param_spec_ref (pspec);
  bse_proxy_get (BSE_SERVER, "bse-preferences", &rec, NULL);
  self->bserec = sfi_rec_copy_deep (rec);
  pchild = bst_preferences_build_rec_editor (self->bserec, sfi_pspec_get_rec_fields (pspec), &self->bseparams);
  gxk_notebook_append (self->notebook, pchild, "BSE", FALSE);
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
  SfiRing *ring, *params = NULL;
  guint i;

  g_return_val_if_fail (rec != NULL, NULL);

  GtkWidget *vbox = (GtkWidget*) g_object_new (GTK_TYPE_VBOX,
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
	  bst_param_create_gmask (param, NULL, vbox);
	  params = sfi_ring_append (params, param);
	}
    }
  for (ring = params; ring; ring = sfi_ring_walk (ring, params))
    gxk_param_update ((GxkParam*) ring->data);
  if (param_list)
    *param_list = params;
  else
    sfi_ring_free (params);

  return gxk_scrolled_window_create (vbox, GTK_SHADOW_NONE, 1, 0.8);
}

static void
bst_preferences_update_params (BstPreferences *self)
{
  SfiRing *ring;
  for (ring = self->params_gconfig; ring; ring = sfi_ring_walk (ring, self->params_gconfig))
    gxk_param_update ((GxkParam*) ring->data);
  for (ring = self->params_skin; ring; ring = sfi_ring_walk (ring, self->params_skin))
    gxk_param_update ((GxkParam*) ring->data);
  for (ring = self->bseparams; ring; ring = sfi_ring_walk (ring, self->bseparams))
    gxk_param_update ((GxkParam*) ring->data);
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

  kbinding = bst_pattern_controller_piano_keys();
  iseq = bst_key_binding_get_item_seq (kbinding);
  bst_key_binding_box_set (self->box_piano_keys, iseq);
  bst_key_binding_item_seq_free (iseq);

  kbinding = bst_pattern_controller_generic_keys();
  iseq = bst_key_binding_get_item_seq (kbinding);
  bst_key_binding_box_set (self->box_generic_keys, iseq);
  bst_key_binding_item_seq_free (iseq);

  bst_msg_absorb_config_box_set (self->box_msg_absorb_config, bst_msg_absorb_config_get_global());

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
  bst_gconfig_set_rec_rc_version (rec, BST_VERSION);
  sfi_rec_swap_fields (self->rec_gconfig, rec);
  sfi_rec_unref (rec);

  kbinding = bst_pattern_controller_piano_keys();
  (void) kbinding;
  iseq = bst_key_binding_get_item_seq (bst_pattern_controller_default_piano_keys());
  bst_key_binding_box_set (self->box_piano_keys, iseq);
  bst_key_binding_item_seq_free (iseq);

  kbinding = bst_pattern_controller_generic_keys();
  iseq = bst_key_binding_get_item_seq (bst_pattern_controller_default_generic_keys());
  bst_key_binding_box_set (self->box_generic_keys, iseq);
  bst_key_binding_item_seq_free (iseq);

  BstMsgAbsorbStringSeq empty_mas_seq = { 0, };
  bst_msg_absorb_config_box_set (self->box_msg_absorb_config, &empty_mas_seq);

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

  kbinding = bst_pattern_controller_piano_keys();
  iseq = bst_key_binding_box_get (self->box_piano_keys);
  bst_key_binding_set_item_seq (kbinding, iseq);
  bst_key_binding_item_seq_free (iseq);

  kbinding = bst_pattern_controller_generic_keys();
  iseq = bst_key_binding_box_get (self->box_generic_keys);
  bst_key_binding_set_item_seq (kbinding, iseq);
  bst_key_binding_item_seq_free (iseq);

  BstMsgAbsorbStringSeq *mass = bst_msg_absorb_config_box_get (self->box_msg_absorb_config);
  SfiSeq *seq = bst_msg_absorb_string_seq_to_seq (mass);
  bst_msg_absorb_config_apply (seq);
  sfi_seq_unref (seq);

  bst_skin_config_apply (self->rec_skin, NULL);

  bse_proxy_set (BSE_SERVER, "bse-preferences", self->bserec, NULL);

  bst_preferences_revert (self);
}

void
bst_preferences_load_rc_files (void)
{
  gchar *file_name = BST_STRDUP_RC_FILE ();
  BseErrorType error;
  GSList *slist = NULL;

  bst_rc_parse (file_name);
  g_free (file_name);

  bst_skin_parse (bst_skin_config_rcfile ());

  slist = g_slist_append (slist, bst_pattern_controller_piano_keys());
  // slist = g_slist_append (slist, bst_pattern_controller_generic_keys());
  error = bst_key_binding_parse (bst_key_binding_rcfile (), slist);
  if (error == BSE_ERROR_FILE_NOT_FOUND)
    {
      /* try loading fallback table */
      gchar *file = g_strconcat (BST_PATH_KEYS, G_DIR_SEPARATOR_S, "keyrc.us", NULL);
      error = bst_key_binding_parse (file, slist);
      g_free (file);
    }
  g_slist_free (slist);
}

static gboolean successfull_rc_dump = FALSE;

gboolean
bst_preferences_saved (void)
{
  return successfull_rc_dump;
}

void
bst_preferences_save (BstPreferences *self)
{
  BseErrorType error = BseErrorType (0);
  gchar *file_name;
  GSList *slist = NULL;

  g_return_if_fail (BST_IS_PREFERENCES (self));

  bse_server_save_preferences (BSE_SERVER);

  file_name = BST_STRDUP_RC_FILE ();
  error = bst_rc_dump (file_name);
  if (error)
    g_warning ("failed to save rc-file \"%s\": %s", file_name, bse_error_blurb (error));
  else
    successfull_rc_dump = TRUE;
  g_free (file_name);

  file_name = g_strdup (bst_key_binding_rcfile ());
  slist = g_slist_append (slist, bst_pattern_controller_piano_keys());
  // slist = g_slist_append (slist, bst_pattern_controller_generic_keys());
  error = bst_key_binding_dump (file_name, slist);
  if (error)
    g_warning ("failed to save keyrc \"%s\": %s", file_name, bse_error_blurb (error));
  g_slist_free (slist);

  bst_msg_absorb_config_save();

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
  self->apply = gxk_dialog_default_action (dialog, BST_STOCK_APPLY, NULL, NULL);
  g_object_connect (self->apply,
                    "swapped_signal::clicked", bst_preferences_apply, self,
                    "swapped_signal::clicked", bst_preferences_save, self,
                    "swapped_signal::destroy", g_nullify_pointer, &self->apply,
                    NULL);
  gxk_widget_set_tooltip (self->apply,
                          "Apply and save the preference values. Some values may only take effect after "
                          "restart while others can be locked against modifcation during "
                          "playback.");

  /* Revert
   */
  widget = gxk_dialog_action_swapped (dialog, BST_STOCK_REVERT, (void*) bst_preferences_revert, self);
  gxk_widget_set_tooltip (widget, "Revert dialog changes to the currently active values.");

  /* Default Revert
   */
  widget = gxk_dialog_action_swapped (dialog, BST_STOCK_DEFAULT_REVERT, (void*) bst_preferences_default_revert, self);
  gxk_widget_set_tooltip (widget, "Revert to hardcoded default values (factory settings).");

  /* Close
   */
  widget = (GtkWidget*) gxk_dialog_action (dialog, BST_STOCK_DISMISS, (void*) gxk_toplevel_delete, NULL);
  gxk_widget_set_tooltip (widget, "Discard changes and close dialog.");
}
