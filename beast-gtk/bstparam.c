#define sfi_pspec_require_options g_param_spec_provides_options
/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
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
#include "bstparam.h"

/* xframe rack view support */
#include "bstxframe.h"
#include "bstrackeditor.h"
#include "bstapp.h"
#include "bstdial.h"
#include "bstlogadjustment.h"
#include "bstsequence.h"


/* --- prototypes --- */
static BstParamImpl* bst_param_lookup_impl	(GParamSpec	 *pspec,
						 gboolean	  rack_widget,
						 const gchar	 *name,
						 BstParamBinding *binding);


/* --- variable --- */
static GQuark quark_null_group = 0;
static GQuark quark_param_choice_values = 0;


/* --- param implementation utils --- */
void
_bst_init_params (void)
{
  g_assert (quark_null_group == 0);

  quark_null_group = g_quark_from_static_string ("bst-param-null-group");
  quark_param_choice_values = g_quark_from_static_string ("bst-param-choice-values");
}


gboolean
bst_param_xframe_check_button (BstParam *bparam,
			       guint     button)
{
  g_return_val_if_fail (bparam != NULL, FALSE);

  if (bparam->binding->rack_item)
    {
      SfiProxy item = bparam->binding->rack_item (bparam);

      if (BSE_IS_ITEM (item))
	{
	  SfiProxy project = bse_item_get_project (item);
	  
	  if (project)
	    {
	      BstApp *app = bst_app_find (project);
	      
	      if (app && app->rack_editor && BST_RACK_EDITOR (app->rack_editor)->rtable->edit_mode)
		{
		  if (button == 1)
		    bst_rack_editor_add_property (BST_RACK_EDITOR (app->rack_editor), item, bparam->pspec->name);
		  return TRUE;
		}
	    }
	}
    }
  return FALSE;
}

gboolean
bst_param_entry_key_press (GtkEntry    *entry,
			   GdkEventKey *event)
{
  GtkEditable *editable = GTK_EDITABLE (entry);
  gboolean intercept = FALSE;

  if (event->state & GDK_MOD1_MASK)
    switch (event->keyval)
      {
      case 'b': /* check gtk_move_backward_word() */
	intercept = gtk_editable_get_position (editable) <= 0;
	break;
      case 'd': /* gtk_delete_forward_word() */
	intercept = TRUE;
	break;
      case 'f': /* check gtk_move_forward_word() */
	intercept = gtk_editable_get_position (editable) >= entry->text_length;
	break;
      default:
	break;
      }
  return intercept;
}

gboolean
bst_param_ensure_focus (GtkWidget *widget)
{
  gtk_widget_grab_focus (widget);
  return FALSE;
}

/* --- BParam functions --- */
static void
bst_param_update_sensitivity (BstParam *bparam)
{
  bparam->writable = (!bparam->readonly &&
		      bparam->editable &&
		      (!bparam->binding->check_writable ||
		       bparam->binding->check_writable (bparam)));

  if (BST_PARAM_IS_GMASK (bparam) && bparam->gdata.gmask)
    bst_gmask_set_sensitive (bparam->gdata.gmask, bparam->writable);
  else if (!BST_PARAM_IS_GMASK (bparam) && bparam->gdata.widget)
    gtk_widget_set_sensitive (bparam->gdata.widget, bparam->writable);
}

BstParam*
bst_param_alloc (BstParamImpl *impl,
		 GParamSpec   *pspec)
{
  BstParam *bparam;
  GType itype, vtype;

  g_return_val_if_fail (impl != NULL, NULL);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (!impl->create_gmask ^ !impl->create_widget, NULL);

  bparam = g_new0 (BstParam, 1);
  bparam->pspec = g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  bparam->impl = impl;
  vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
  itype = sfi_category_type (bparam->impl->scat);
  if (!itype)
    itype = vtype;
  g_value_init (&bparam->value, itype);
  bparam->column = 0;
  bparam->readonly = (!(bparam->impl->flags & BST_PARAM_EDITABLE) ||
		      !(pspec->flags & G_PARAM_WRITABLE) ||
		      sfi_pspec_check_option (pspec, "ro"));       /* read-only check */
  if (!bparam->readonly)
    bparam->readonly = !g_type_is_a (itype, vtype);
  bparam->writable = FALSE;
  bparam->editable = TRUE;
  bparam->updating = FALSE;
  bparam->gdata.widget = NULL;
  return bparam;
}

void
bst_param_update (BstParam *bparam)
{
  GtkWidget *action = NULL;
  gboolean updating;
  
  g_return_if_fail (bparam != NULL);
  
  updating = bparam->updating;
  bparam->updating = TRUE;
  
  if (BST_PARAM_IS_GMASK (bparam) && bparam->gdata.gmask)
    action = bst_gmask_get_action (bparam->gdata.gmask);
  else if (!BST_PARAM_IS_GMASK (bparam) && bparam->gdata.widget)
    action = bparam->gdata.widget;
  
  bparam->binding->get_value (bparam, &bparam->value);
  
  if (action)
    bparam->impl->update (bparam, action);
  
  bparam->writable = FALSE;
  bst_param_update_sensitivity (bparam);
  
  bparam->updating = updating;
}

void
bst_param_apply_value (BstParam *bparam)
{
  g_return_if_fail (bparam != NULL);

  if (bparam->updating)
    {
      g_warning ("not applying value from parameter \"%s\" currently in update mode",
		 bparam->impl->name);
      return;
    }
  bparam->binding->set_value (bparam, &bparam->value);
}

void
bst_param_apply_default (BstParam *bparam)
{
  g_return_if_fail (bparam != NULL);

  if (!bparam->updating && bparam->writable)
    {
      GValue value = { 0, };
      g_value_init (&value, G_VALUE_TYPE (&bparam->value));
      g_param_value_set_default (bparam->pspec, &value);
      bparam->binding->set_value (bparam, &value);
      g_value_unset (&value);
    }
}

void
bst_param_set_editable (BstParam *bparam,
			gboolean  editable)
{
  g_return_if_fail (bparam != NULL);

  bparam->editable = editable != FALSE;
  bst_param_update_sensitivity (bparam);
}

const gchar*
bst_param_get_name (BstParam *bparam)
{
  g_return_val_if_fail (bparam != NULL, NULL);

  return bparam->pspec->name;
}

const gchar*
bst_param_get_view_name (BstParam *bparam)
{
  g_return_val_if_fail (bparam != NULL, NULL);

  return bparam->impl->name;
}

static GtkWidget*
bparam_make_container (GtkWidget *parent,
		       GQuark     quark_group)
{
  GtkWidget *container;

  container = bst_container_get_named_child (parent, quark_group ? quark_group : quark_null_group);
  if (!container || !GTK_IS_CONTAINER (container))
    {
      GtkWidget *any;
      container = bst_gmask_container_create (quark_group ? 5 : 0, FALSE);
      if (quark_group)
	any = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "label", g_quark_to_string (quark_group),
			      "child", container,
			      NULL);
      else
	any = container;
      if (GTK_IS_BOX (parent))
	gtk_box_pack_start (GTK_BOX (parent), any, FALSE, TRUE, 0);
      else if (GTK_IS_WRAP_BOX (parent))
	gtk_container_add_with_properties (GTK_CONTAINER (parent), any,
					   "hexpand", TRUE, "hfill", TRUE,
					   "vexpand", FALSE, "vfill", TRUE,
					   NULL);
      else
	gtk_container_add (GTK_CONTAINER (parent), any);
      bst_container_set_named_child (parent, quark_group ? quark_group : quark_null_group, container);
    }
  return container;
}

static gchar*
bst_param_create_tooltip (BstParam *bparam)
{
  gchar *tooltip = g_param_spec_get_blurb (bparam->pspec);
  if (!BST_DVL_HINTS)
    tooltip = g_strdup (tooltip);
  else if (tooltip)
    tooltip = g_strdup_printf ("(%s): %s", g_param_spec_get_name (bparam->pspec), tooltip);
  else
    tooltip = g_strdup_printf ("(%s)", g_param_spec_get_name (bparam->pspec));
  return tooltip;
}

void
bst_param_pack_property (BstParam       *bparam,
			 GtkWidget      *parent)
{
  const gchar *group;
  gchar *tooltip;

  g_return_if_fail (bparam != NULL);
  g_return_if_fail (GTK_IS_CONTAINER (parent));
  g_return_if_fail (bparam->gdata.gmask == NULL);
  g_return_if_fail (BST_PARAM_IS_GMASK (bparam));

  bparam->updating = TRUE;
  group = sfi_pspec_get_group (bparam->pspec);
  parent = bparam_make_container (parent, group ? g_quark_from_string (group) : 0);
  tooltip = bst_param_create_tooltip (bparam);
  bparam->gdata.gmask = bparam->impl->create_gmask (bparam, tooltip, parent);
  g_free (tooltip);
  bst_gmask_ref (bparam->gdata.gmask);
  bst_gmask_set_column (bparam->gdata.gmask, bparam->column);
  bst_gmask_pack (bparam->gdata.gmask);
  bparam->updating = FALSE;
  bst_param_update_sensitivity (bparam); /* bst_param_update (bparam); */
}

GtkWidget*
bst_param_rack_widget (BstParam *bparam)
{
  gchar *tooltip;

  g_return_val_if_fail (bparam != NULL, NULL);
  g_return_val_if_fail (bparam->gdata.widget == NULL, NULL);
  g_return_val_if_fail (!BST_PARAM_IS_GMASK (bparam), NULL);

  bparam->updating = TRUE;
  tooltip = bst_param_create_tooltip (bparam);
  bparam->gdata.widget = bparam->impl->create_widget (bparam, tooltip);
  g_free (tooltip);
  g_object_ref (bparam->gdata.widget);
  bparam->updating = FALSE;
  bst_param_update_sensitivity (bparam); /* bst_param_update (bparam); */
  return gtk_widget_get_toplevel (bparam->gdata.widget);
}

void
bst_param_destroy (BstParam *bparam)
{
  g_return_if_fail (bparam != NULL);
  g_return_if_fail (bparam->binding != NULL);

  if (bparam->binding->destroy)
    bparam->binding->destroy (bparam);
  bparam->binding = NULL;
  if (BST_PARAM_IS_GMASK (bparam) && bparam->gdata.gmask)
    {
      bst_gmask_destroy (bparam->gdata.gmask);
      bst_gmask_unref (bparam->gdata.gmask);
    }
  else if (!BST_PARAM_IS_GMASK (bparam) && bparam->gdata.widget)
    {
      gtk_widget_destroy (bparam->gdata.widget);
      g_object_unref (bparam->gdata.widget);
    }
  g_param_spec_unref (bparam->pspec);
  g_value_unset (&bparam->value);
  g_free (bparam);
}


/* --- dummy binding --- */
static void
dummy_binding_set_value (BstParam       *bparam,
                         const GValue   *value)
{
}

static void
dummy_binding_get_value (BstParam       *bparam,
                         GValue         *value)
{
}

static BstParamBinding dummy_binding = {
  dummy_binding_set_value,
  dummy_binding_get_value,
};
BstParamBinding *bst_dummy_binding = &dummy_binding;


/* --- proxy binding --- */
static void
proxy_binding_set_value (BstParam       *bparam,
			 const GValue   *value)
{
  SfiProxy proxy = bparam->mdata[0].v_long;
  if (proxy)
    sfi_glue_proxy_set_property (bparam->mdata[0].v_long, bparam->pspec->name, value);
}

static void
proxy_binding_get_value (BstParam       *bparam,
			 GValue         *value)
{
  SfiProxy proxy = bparam->mdata[0].v_long;
  if (proxy)
    {
      const GValue *cvalue = sfi_glue_proxy_get_property (bparam->mdata[0].v_long, bparam->pspec->name);
      if (cvalue)
	g_value_transform (cvalue, value);
      else
	g_value_reset (value);
    }
  else
    g_value_reset (value);
}

static SfiProxy
proxy_binding_rack_item (BstParam *bparam)
{
  return bparam->mdata[0].v_long;
}

static void
proxy_binding_weakref (gpointer data,
		       SfiProxy junk)
{
  BstParam *bparam = data;
  bparam->mdata[0].v_long = 0;
  bparam->mdata[1].v_long = 0;	/* already disconnected */
  bparam->binding = bst_dummy_binding;
}

static void
proxy_binding_destroy (BstParam *bparam)
{
  SfiProxy proxy = bparam->mdata[0].v_long;
  if (proxy)
    {
      sfi_glue_signal_disconnect (proxy, bparam->mdata[1].v_long);
      sfi_glue_proxy_weak_unref (proxy, proxy_binding_weakref, bparam);
      bparam->mdata[0].v_long = 0;
      bparam->mdata[1].v_long = 0;
      bparam->binding = bst_dummy_binding;
    }
}

static BseProxySeq*
proxy_binding_list_proxies (BstParam *bparam)
{
  SfiProxy proxy = bparam->mdata[0].v_long;
  if (proxy)
    {
      BseProxySeq *pseq = bse_item_list_proxies (proxy, bparam->pspec->name);
      if (pseq)	/* need to return "newly allocated" proxy list */
	return bse_proxy_seq_copy_shallow (pseq);
    }
  return NULL;
}

static BstParamBinding bst_proxy_binding = {
  proxy_binding_set_value,
  proxy_binding_get_value,
  proxy_binding_destroy,
  NULL,	/* check_writable */
  proxy_binding_rack_item,
  proxy_binding_list_proxies,
};

BstParamBinding*
bst_param_binding_proxy (void)
{
  return &bst_proxy_binding;
}

BstParam*
bst_param_proxy_create (GParamSpec  *pspec,
			gboolean     rack_widget,
			const gchar *view_name,
			SfiProxy     proxy)
{
  BstParamImpl *impl;
  BstParam *bparam;

  g_return_val_if_fail (BSE_IS_ITEM (proxy), NULL);

  impl = bst_param_lookup_impl (pspec, rack_widget, view_name, &bst_proxy_binding);
  bparam = bst_param_alloc (impl, pspec);
  bparam->binding = &bst_proxy_binding;
  bparam->mdata[0].v_long = 0;
  bst_param_set_proxy (bparam, proxy);
  return bparam;
}

void
bst_param_set_proxy (BstParam *bparam,
		     SfiProxy  proxy)
{
  g_return_if_fail (bparam != NULL);
  g_return_if_fail (bparam->binding == &bst_proxy_binding);

  proxy_binding_destroy (bparam);
  bparam->binding = &bst_proxy_binding;
  bparam->mdata[0].v_long = proxy;
  if (proxy)
    {
      gchar *sig = g_strconcat ("property-notify::", bparam->pspec->name, NULL);
      bparam->mdata[1].v_long = sfi_glue_signal_connect_swapped (proxy, sig, bst_param_update, bparam);
      g_free (sig);
      sfi_glue_proxy_weak_ref (proxy, proxy_binding_weakref, bparam);
    }
}


/* --- record binding --- */
static void
record_binding_set_value (BstParam     *bparam,
			  const GValue *value)
{
  sfi_rec_set (bparam->mdata[0].v_pointer, bparam->pspec->name, value);
}

static void
record_binding_get_value (BstParam *bparam,
			  GValue   *value)
{
  const GValue *cvalue = sfi_rec_get (bparam->mdata[0].v_pointer, bparam->pspec->name);
  if (cvalue)
    g_value_transform (cvalue, value);
  else
    g_value_reset (value);
}

static void
record_binding_destroy (BstParam *bparam)
{
  sfi_rec_unref (bparam->mdata[0].v_pointer);
  bparam->mdata[0].v_pointer = NULL;
}

static BstParamBinding bst_record_binding = {
  record_binding_set_value,
  record_binding_get_value,
  record_binding_destroy,
  NULL,	/* check_writable */
};

BstParamBinding*
bst_param_binding_rec (void)
{
  return &bst_record_binding;
}

BstParam*
bst_param_rec_create (GParamSpec  *pspec,
		      gboolean     rack_widget,
		      const gchar *view_name,
		      SfiRec      *rec)
{
  BstParamImpl *impl;
  BstParam *bparam;

  g_return_val_if_fail (rec != NULL, NULL);

  impl = bst_param_lookup_impl (pspec, rack_widget, view_name, &bst_record_binding);
  bparam = bst_param_alloc (impl, pspec);
  bparam->binding = &bst_record_binding;
  bparam->mdata[0].v_pointer = sfi_rec_ref (rec);
  return bparam;
}


/* --- value binding --- */
static void
value_binding_set_value (BstParam     *bparam,
                         const GValue *value)
{
  BstParamValueNotify notify = bparam->mdata[0].v_pointer;
  sfi_value_copy_shallow (value, &bparam->value);
  if (notify)
    notify (bparam->mdata[1].v_pointer, bparam);
}

static void
value_binding_get_value (BstParam *bparam,
                         GValue   *value)
{
  sfi_value_copy_shallow (&bparam->value, value);
}

static void
value_binding_destroy (BstParam *bparam)
{
  bparam->mdata[0].v_pointer = NULL;
  bparam->mdata[1].v_pointer = NULL;
}

static BstParamBinding bst_value_binding = {
  value_binding_set_value,
  value_binding_get_value,
  value_binding_destroy,
  /* check_writable */
};

BstParamBinding*
bst_param_binding_value (void)
{
  return &bst_value_binding;
}

BstParam*
bst_param_value_create (GParamSpec         *pspec,
                        gboolean            rack_widget,
                        const gchar        *view_name,
                        BstParamValueNotify notify,
                        gpointer            notify_data)
{
  BstParamImpl *impl;
  BstParam *bparam;

  impl = bst_param_lookup_impl (pspec, rack_widget, view_name, &bst_value_binding);
  bparam = bst_param_alloc (impl, pspec);
  bparam->binding = &bst_value_binding;
  bparam->mdata[0].v_pointer = notify;
  bparam->mdata[1].v_pointer = notify_data;
  return bparam;
}


/* --- param and rack widget implementations --- */
#include "bstparam-label.c"
#include "bstparam-toggle.c"
#include "bstparam-spinner.c"
#include "bstparam-entry.c"
#include "bstparam-note-sequence.c"
#include "bstparam-choice.c"
#include "bstparam-strnum.c"
#include "bstparam-note-spinner.c"
#include "bstparam-proxy.c"
#include "bstparam-scale.c"

static BstParamImpl *bst_param_impls[] = {
  &param_pspec,
  &param_check_button,
  &param_spinner_int,
  &param_spinner_num,
  &param_spinner_real,
  &param_entry,
  &param_note_sequence,
  &param_choice,
  &param_note,
  &param_time,
  &param_note_spinner,
  &param_proxy,
};

static BstParamImpl *bst_rack_impls[] = {
  &rack_pspec,
  &rack_toggle_button,
  &rack_check_button,
  &rack_radio_button,
  &rack_spinner_int,
  &rack_spinner_num,
  &rack_spinner_real,
  &rack_entry,
  &rack_note_sequence,
  &rack_choice,
  &rack_note,
  &rack_time,
  &rack_note_spinner,
  &rack_proxy,
  &rack_knob_int,
  &rack_knob_num,
  &rack_knob_real,
  &rack_log_knob_int,
  &rack_log_knob_num,
  &rack_log_knob_real,
  &rack_dial_int,
  &rack_dial_num,
  &rack_dial_real,
  &rack_log_dial_int,
  &rack_log_dial_num,
  &rack_log_dial_real,
  &rack_vscale_int,
  &rack_vscale_num,
  &rack_vscale_real,
  &rack_log_vscale_int,
  &rack_log_vscale_num,
  &rack_log_vscale_real,
  &rack_hscale_int,
  &rack_hscale_num,
  &rack_hscale_real,
  &rack_log_hscale_int,
  &rack_log_hscale_num,
  &rack_log_hscale_real,
};

const gchar**
bst_param_list_names (gboolean rack_widget,
		      guint   *n_p)
{
  static const gchar **pnames = NULL, **rnames = NULL;
  static guint pn = 0, rn = 0;
  const gchar **names = rack_widget ? rnames : pnames;

  if (!names)
    {
      BstParamImpl **impls = rack_widget ? bst_rack_impls : bst_param_impls;
      guint i, j, k = 0, n = rack_widget ? G_N_ELEMENTS (bst_rack_impls) : G_N_ELEMENTS (bst_param_impls);
      names = g_new (const gchar*, n + 1);
      for (i = 0; i < n; i++)
	{
	  for (j = 0; j < k; j++)
	    if (strcmp (names[j], impls[i]->name) == 0)
	      goto skip_duplicate;
	  names[k++] = impls[i]->name;
	skip_duplicate:
	  ;
	}
      names[k] = NULL;
      n = k;
      names = g_renew (const gchar*, names, n + 1);
      if (rack_widget)
	{
	  rnames = names;
	  rn = n;
	}
      else
	{
	  pnames = names;
	  pn = n;
	}
    }
  if (n_p)
    *n_p = rack_widget ? rn : pn;
  return names;
}

static guint
bst_param_rate_impl (BstParamImpl    *impl,
		     GParamSpec      *pspec,
		     BstParamBinding *binding)
{
  gboolean is_editable, does_match, type_specific, type_mismatch, scat_specific = FALSE;
  guint type_distance = 0;
  GType vtype, itype;
  guint rating = 0;

  g_return_val_if_fail (impl != NULL, 0);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), 0);

  vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
  itype = impl->scat ? sfi_category_type (impl->scat) : 0;
  type_specific = itype != 0;

  is_editable = (impl->flags & BST_PARAM_EDITABLE) != 0;
  if (impl->scat)
    {
      if (impl->scat & ~SFI_SCAT_TYPE_MASK)	/* be strict for non-fundamental scats */
	{
	  type_mismatch = impl->scat != sfi_categorize_pspec (pspec);
	  scat_specific = TRUE;
	}
      else
	type_mismatch = FALSE;
      type_mismatch |= !g_type_is_a (vtype, itype);		/* read value */
      is_editable = is_editable && g_type_is_a (itype, vtype);	/* write value */
    }
  else
    type_mismatch = FALSE;
  if (impl->flags & BST_PARAM_PROXY_LIST)
    type_mismatch |= !binding || !binding->list_proxies;

  does_match = !type_mismatch && (!impl->hints || sfi_pspec_require_options (pspec, impl->hints));
  if (!does_match)
    return 0;		/* mismatch */

  if (itype)
    type_distance = g_type_depth (vtype) - g_type_depth (itype);

  rating |= 128 - type_distance;
  rating <<= 1;
  rating |= is_editable;
  rating <<= 1;
  rating |= type_specific;
  rating <<= 1;
  rating |= scat_specific;
  rating <<= 8;
  rating += 128 + impl->rating; /* impl->rating is signed, 8bit */

  return rating;
}

static BstParamImpl*
bst_param_lookup_impl (GParamSpec      *pspec,
		       gboolean         rack_widget,
		       const gchar     *name,
		       BstParamBinding *binding)
{
  BstParamImpl **impls = rack_widget ? bst_rack_impls : bst_param_impls;
  guint i, n = rack_widget ? G_N_ELEMENTS (bst_rack_impls) : G_N_ELEMENTS (bst_param_impls);
  BstParamImpl *best = NULL;
  guint rating = 0; /* threshold for mismatch */

  for (i = 0; i < n; i++)
    if (!name || !strcmp (impls[i]->name, name))
      {
	guint r = bst_param_rate_impl (impls[i], pspec, binding);
	if (r > rating) /* only notice improvements */
	  {
	    best = impls[i];
	    rating = r;
	  }
      }
  /* if !name, best is != NULL */
  if (!best)
    best = bst_param_lookup_impl (pspec, rack_widget, NULL, binding);
  return best;
}

const gchar*
bst_param_lookup_view (GParamSpec      *pspec,
		       gboolean         rack_widget,
		       const gchar     *view_name,
		       BstParamBinding *binding)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), 0);
  g_return_val_if_fail (view_name != NULL, 0);

  return bst_param_lookup_impl (pspec, rack_widget, view_name, binding)->name;
}

guint
bst_param_rate_check (GParamSpec      *pspec,
		      gboolean         rack_widget,
		      const gchar     *view_name,
		      BstParamBinding *binding)
{
  BstParamImpl **impls;
  guint i, n, rating = 0;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), 0);
  g_return_val_if_fail (view_name != NULL, 0);

  impls = rack_widget ? bst_rack_impls : bst_param_impls;
  n = rack_widget ? G_N_ELEMENTS (bst_rack_impls) : G_N_ELEMENTS (bst_param_impls);
  for (i = 0; i < n; i++)
    if (strcmp (impls[i]->name, view_name) == 0)
      {
	guint r = bst_param_rate_impl (impls[i], pspec, binding);
	rating = MAX (r, rating);
      }
  return rating;
}
