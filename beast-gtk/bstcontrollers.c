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
#include "bstcontrollers.h"


/* --- prototypes --- */
static BstControllerInfo** controller_array (guint *n);


/* --- variables --- */
static GSList *controller_list = NULL;


/* --- functions --- */
guint
bst_controller_check (BstControllerInfo *cinfo,
		      GParamSpec        *pspec)
{
  gboolean can_update = TRUE, can_fetch = TRUE, good_fetch = FALSE;
  gboolean exact_update = FALSE, exact_fetch = FALSE, does_match = FALSE;
  guint rating = 0;

  g_return_val_if_fail (cinfo != NULL, FALSE);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);

  if (cinfo->value_type)
    {
      can_update = g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec), cinfo->value_type) != FALSE;
      good_fetch = g_value_type_transformable (cinfo->value_type, G_PARAM_SPEC_VALUE_TYPE (pspec)) != FALSE;
      can_fetch = good_fetch || !cinfo->fetch;
    }
  else
    good_fetch = cinfo->fetch != NULL;

  if (!cinfo->check)
    does_match = can_update && can_fetch;
  else if (!cinfo->value_type || (can_update && can_fetch))
    does_match = cinfo->check (pspec, cinfo->controller_data) != FALSE;

  if (does_match && cinfo->value_type)
    {
      exact_update = g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), cinfo->value_type) != FALSE;
      exact_fetch = cinfo->fetch && g_type_is_a (cinfo->value_type, G_PARAM_SPEC_VALUE_TYPE (pspec)) != FALSE;
    }

  rating |= (exact_fetch && exact_update);
  rating <<= 1;
  rating |= good_fetch && does_match;
  rating <<= 1;
  rating |= exact_update;
  rating <<= 1;
  rating |= does_match;

  return rating;
}

GtkWidget*
bst_controller_create (BstControllerInfo *cinfo,
		       GParamSpec        *pspec,
		       GCallback          changed_notify,
		       gpointer           notify_data)
{
  GtkWidget *widget;

  g_return_val_if_fail (cinfo != NULL, NULL);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (changed_notify != NULL, NULL);

  widget = cinfo->create (pspec, changed_notify, notify_data, cinfo->controller_data);
  if (widget)
    {
      g_object_set_data_full (G_OBJECT (widget), "BstController-pspec",
			      g_param_spec_ref (pspec), (GDestroyNotify) g_param_spec_unref);
      g_object_set_data (G_OBJECT (widget), "BstController", cinfo);
    }
  return widget;
}

void
bst_controller_update (GtkWidget    *widget,
		       const GValue *value)
{
  BstControllerInfo *cinfo;
  GParamSpec *pspec;
  
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (G_IS_VALUE (value));

  pspec = g_object_get_data (G_OBJECT (widget), "BstController-pspec");
  cinfo = g_object_get_data (G_OBJECT (widget), "BstController");
  g_return_if_fail (pspec != NULL && cinfo != NULL);

  cinfo->update (widget, pspec, value, cinfo->controller_data);
}

void
bst_controller_fetch (GtkWidget *widget,
		      GValue    *value)
{
  BstControllerInfo *cinfo;
  GParamSpec *pspec;
  
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (G_IS_VALUE (value));

  pspec = g_object_get_data (G_OBJECT (widget), "BstController-pspec");
  cinfo = g_object_get_data (G_OBJECT (widget), "BstController");
  g_return_if_fail (pspec != NULL && cinfo != NULL);

  if (cinfo->fetch)
    cinfo->fetch (widget, pspec, value, cinfo->controller_data);
}

GSList*
bst_controller_list (void)
{
  if (!controller_list)
    {
      guint n;
      BstControllerInfo **array = controller_array (&n);
      GSList *slist = NULL;

      while (n--)
	slist = g_slist_prepend (slist, array[n]);
      controller_list = slist;
    }
  return controller_list;
}

BstControllerInfo*
bst_controller_lookup (const gchar *name,
		       GParamSpec  *pspec)
{
  BstControllerInfo *cinfo, *best = NULL;
  GSList *slist;
  guint rating = 0;

  if (pspec)
    g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  
  if (name)
    for (slist = bst_controller_list (); slist; slist = slist->next)
      {
	cinfo = slist->data;
	if (strcmp (name, cinfo->name) == 0)
	  {
	    if (pspec)
	      return bst_controller_check (cinfo, pspec) ? cinfo : NULL;
	    else
	      return cinfo;
	  }
      }
  
  if (pspec)
    for (slist = bst_controller_list (); slist; slist = slist->next)
      {
	guint r;
	
	cinfo = slist->data;
	r = bst_controller_check (cinfo, pspec);
	if (r > rating)	/* only notice improvements */
	  {
	    best = cinfo;
	    rating = r;
	  }
      }
  
  return best;
}


/* --- controller implmenetations --- */
#include	<gtk/gtklabel.h>
#include	"bstdial.h"
#include	"bstknob.h"
#include	"bstlogadjustment.h"


/* --- simple label --- */
static GtkWidget*
controller_label_create (GParamSpec  *pspec,
			 GCallback    notify_func,
			 gpointer     notify_data,
			 gsize        controller_data)
{
  GtkWidget *widget = g_object_new (GTK_TYPE_LABEL,
				    "visible", TRUE,
				    "xalign", 0.0,
				    NULL);
  return widget;
}

static void
controller_label_update (GtkWidget    *widget,
			 GParamSpec   *pspec,
			 const GValue *value,
			 gsize         controller_data)
{	
  gtk_label_set_text (GTK_LABEL (widget), g_value_get_string (value));
}

static BstControllerInfo controller_label = {
  G_TYPE_STRING,	"Label",	0,
  NULL,
  controller_label_create,
  controller_label_update,
  NULL,
};


/* --- pspec name display --- */
static GtkWidget*
controller_pspec_create (GParamSpec  *pspec,
			 GCallback    notify_func,
			 gpointer     notify_data,
			 gsize        controller_data)
{
  GtkWidget *widget = g_object_new (GTK_TYPE_LABEL,
				    "visible", TRUE,
				    "xalign", 0.0,
				    NULL);
  return widget;
}

static void
controller_pspec_update (GtkWidget    *widget,
			 GParamSpec   *pspec,
			 const GValue *value,
			 gsize         controller_data)
{	
  gtk_label_set_text (GTK_LABEL (widget), g_param_spec_get_nick (pspec));
}

static BstControllerInfo controller_pspec = {
  0,	"Property Name",	0,
  NULL,
  controller_pspec_create,
  controller_pspec_update,
  NULL,
};


/* --- scale alike knob --- */
static gboolean
controller_knob_check (GParamSpec *pspec,
		       gsize       controller_data)
{
  if (controller_data)
    {
      BseParamLogScale lscale;

      bse_param_spec_get_log_scale (pspec, &lscale);
      return G_IS_PARAM_SPEC_FLOAT (pspec) && lscale.n_steps;
    }
  else
    return G_IS_PARAM_SPEC_FLOAT (pspec);
}

static GtkWidget*
controller_knob_create (GParamSpec  *pspec,
			GCallback    notify_func,
			gpointer     notify_data,
			gsize	     controller_data)
{
  BseParamLogScale lscale;
  GtkWidget *widget;
  gfloat stepping_rate = BSE_IS_PARAM_SPEC_FLOAT (pspec) ? BSE_PARAM_SPEC_FLOAT (pspec)->stepping_rate : 1;
  gpointer adjustment = gtk_adjustment_new (G_PARAM_SPEC_FLOAT (pspec)->default_value,
					    G_PARAM_SPEC_FLOAT (pspec)->minimum,
					    G_PARAM_SPEC_FLOAT (pspec)->maximum,
					    MIN (0.1, stepping_rate),
					    MAX (0.1, stepping_rate),
					    0);
  
  widget = g_object_new (BST_TYPE_KNOB,
			 "visible", TRUE,
			 NULL);
  g_object_set_data_full (G_OBJECT (widget), "adjustment", g_object_ref (adjustment), (GDestroyNotify) g_object_unref);
  gtk_object_sink (adjustment);
  
  bse_param_spec_get_log_scale (pspec, &lscale);
  if (lscale.n_steps && controller_data)
    {
      adjustment = bst_log_adjustment_from_adj (adjustment);
      bst_log_adjustment_setup (BST_LOG_ADJUSTMENT (adjustment),
				lscale.center,
				lscale.base,
				lscale.n_steps);
    }

  bst_knob_set_adjustment (BST_KNOB (widget), adjustment);
  
  g_object_connect (adjustment,
		    "swapped_signal_after::value-changed", notify_func, notify_data,
		    NULL);

  return widget;
}

static void
controller_knob_update (GtkWidget    *widget,
			GParamSpec   *pspec,
			const GValue *value,
			gsize	      controller_data)
{	
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  adjustment->value = g_value_get_float (value);
  gtk_adjustment_value_changed (adjustment);
}

static void
controller_knob_fetch (GtkWidget  *widget,
		       GParamSpec *pspec,
		       GValue     *value,
		       gsize       controller_data)
{
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  g_value_set_float (value, adjustment->value);
}

static BstControllerInfo controller_knob = {
  G_TYPE_FLOAT,	"Knob",			0,
  controller_knob_check,
  controller_knob_create,
  controller_knob_update,
  controller_knob_fetch,
};

static BstControllerInfo controller_log_knob = {
  G_TYPE_FLOAT,	"Knob (Logarithmic)",		1,
  controller_knob_check,
  controller_knob_create,
  controller_knob_update,
  controller_knob_fetch,
};


/* --- dial widget --- */
static gboolean
controller_dial_check (GParamSpec *pspec,
		       gsize       controller_data)
{
  if (controller_data)
    {
      BseParamLogScale lscale;

      bse_param_spec_get_log_scale (pspec, &lscale);
      return G_IS_PARAM_SPEC_FLOAT (pspec) && lscale.n_steps;
    }
  else
    return G_IS_PARAM_SPEC_FLOAT (pspec);
}

static GtkWidget*
controller_dial_create (GParamSpec  *pspec,
			GCallback    notify_func,
			gpointer     notify_data,
			gsize	     controller_data)
{
  BseParamLogScale lscale;
  GtkWidget *dial;
  gfloat stepping_rate = BSE_IS_PARAM_SPEC_FLOAT (pspec) ? BSE_PARAM_SPEC_FLOAT (pspec)->stepping_rate : 1;
  gpointer adjustment = gtk_adjustment_new (G_PARAM_SPEC_FLOAT (pspec)->default_value,
					    G_PARAM_SPEC_FLOAT (pspec)->minimum,
					    G_PARAM_SPEC_FLOAT (pspec)->maximum,
					    MIN (0.1, stepping_rate),
					    MAX (0.1, stepping_rate),
					    0);
  dial = gtk_widget_new (BST_TYPE_DIAL,
			 "visible", TRUE,
			 "can_focus", FALSE,
			 NULL);
  g_object_set_data_full (G_OBJECT (dial), "adjustment", g_object_ref (adjustment), (GDestroyNotify) g_object_unref);
  gtk_object_sink (adjustment);

  bse_param_spec_get_log_scale (pspec, &lscale);
  if (lscale.n_steps && controller_data)
    {
      adjustment = bst_log_adjustment_from_adj (adjustment);
      bst_log_adjustment_setup (BST_LOG_ADJUSTMENT (adjustment),
				lscale.center,
				lscale.base,
				lscale.n_steps);
    }

  bst_dial_set_adjustment (BST_DIAL (dial), adjustment);

  g_object_connect (adjustment,
		    "swapped_signal_after::value-changed", notify_func, notify_data,
		    NULL);

  return dial;
}

static void
controller_dial_update (GtkWidget    *widget,
			GParamSpec   *pspec,
			const GValue *value,
			gsize	      controller_data)
{
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  adjustment->value = g_value_get_float (value);
  gtk_adjustment_value_changed (adjustment);
}

static void
controller_dial_fetch (GtkWidget  *widget,
		       GParamSpec *pspec,
		       GValue     *value,
		       gsize       controller_data)
{
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  g_value_set_float (value, adjustment->value);
}

static BstControllerInfo controller_dial = {
  G_TYPE_FLOAT,	"Dial",			0,
  controller_dial_check,
  controller_dial_create,
  controller_dial_update,
  controller_dial_fetch,
};

static BstControllerInfo controller_log_dial = {
  G_TYPE_FLOAT,	"Dial (Logarithmic)",	1,
  controller_dial_check,
  controller_dial_create,
  controller_dial_update,
  controller_dial_fetch,
};


/* --- float scale widget --- */
static gboolean
controller_fscale_check (GParamSpec *pspec,
			 gsize       controller_data)
{
  if (controller_data)
    {
      BseParamLogScale lscale;

      bse_param_spec_get_log_scale (pspec, &lscale);
      return G_IS_PARAM_SPEC_FLOAT (pspec) && lscale.n_steps;
    }
  else
    return G_IS_PARAM_SPEC_FLOAT (pspec);
}

static GtkWidget*
controller_fscale_create (GParamSpec  *pspec,
			  GCallback    notify_func,
			  gpointer     notify_data,
			  gsize        controller_data)
{
  BseParamLogScale lscale;
  GtkWidget *scale;
  gfloat stepping_rate = BSE_IS_PARAM_SPEC_FLOAT (pspec) ? BSE_PARAM_SPEC_FLOAT (pspec)->stepping_rate : 1;
  gpointer adjustment = gtk_adjustment_new (G_PARAM_SPEC_FLOAT (pspec)->default_value,
					    G_PARAM_SPEC_FLOAT (pspec)->minimum,
					    G_PARAM_SPEC_FLOAT (pspec)->maximum,
					    MIN (0.1, stepping_rate),
					    MAX (0.1, stepping_rate),
					    0);
  scale = gtk_widget_new (GTK_TYPE_VSCALE,
			  "visible", TRUE,
			  "can_focus", FALSE,
			  "draw_value", FALSE,
			 NULL);
  g_object_set_data_full (G_OBJECT (scale), "adjustment", g_object_ref (adjustment), (GDestroyNotify) g_object_unref);
  gtk_object_sink (adjustment);

  bse_param_spec_get_log_scale (pspec, &lscale);
  if (lscale.n_steps && controller_data)
    {
      adjustment = bst_log_adjustment_from_adj (adjustment);
      bst_log_adjustment_setup (BST_LOG_ADJUSTMENT (adjustment),
				lscale.center,
				lscale.base,
				lscale.n_steps);
    }

  g_object_set (scale,
		"adjustment", adjustment,
		NULL);

  g_object_connect (adjustment,
		    "swapped_signal_after::value-changed", notify_func, notify_data,
		    NULL);

  return scale;
}

static void
controller_fscale_update (GtkWidget    *widget,
			  GParamSpec   *pspec,
			  const GValue *value,
			  gsize         controller_data)
{
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  adjustment->value = g_value_get_float (value);
  gtk_adjustment_value_changed (adjustment);
}

static void
controller_fscale_fetch (GtkWidget  *widget,
			 GParamSpec *pspec,
			 GValue     *value,
			 gsize       controller_data)
{
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  g_value_set_float (value, adjustment->value);
}

static BstControllerInfo controller_scale = {
  G_TYPE_FLOAT,		"Scale",		0,
  controller_fscale_check,
  controller_fscale_create,
  controller_fscale_update,
  controller_fscale_fetch,
};

static BstControllerInfo controller_log_scale = {
  G_TYPE_FLOAT,		"Scale (Logarithmic)",	1,
  controller_fscale_check,
  controller_fscale_create,
  controller_fscale_update,
  controller_fscale_fetch,
};


/* --- integer scale widget --- */
static gboolean
controller_iscale_check (GParamSpec *pspec,
			 gsize       controller_data)
{
  return ((G_IS_PARAM_SPEC_INT (pspec) && controller_data == G_TYPE_INT) ||
	  (G_IS_PARAM_SPEC_UINT (pspec) && controller_data == G_TYPE_UINT));
}

static GtkWidget*
controller_iscale_create (GParamSpec  *pspec,
			  GCallback    notify_func,
			  gpointer     notify_data,
			  gsize        controller_data)
{
  GtkWidget *scale;
  gfloat stepping_rate;
  gpointer adjustment;

  if (G_IS_PARAM_SPEC_INT (pspec))
    {
      stepping_rate = BSE_IS_PARAM_SPEC_INT (pspec) ? BSE_PARAM_SPEC_INT (pspec)->stepping_rate : 10;
      adjustment = gtk_adjustment_new (G_PARAM_SPEC_INT (pspec)->default_value,
				       G_PARAM_SPEC_INT (pspec)->minimum,
				       G_PARAM_SPEC_INT (pspec)->maximum,
				       MIN (1, stepping_rate),
				       MAX (10, stepping_rate),
				       0);
    }
  else
    {
      stepping_rate = BSE_IS_PARAM_SPEC_UINT (pspec) ? BSE_PARAM_SPEC_UINT (pspec)->stepping_rate : 10;
      adjustment = gtk_adjustment_new (G_PARAM_SPEC_UINT (pspec)->default_value,
				       G_PARAM_SPEC_UINT (pspec)->minimum,
				       G_PARAM_SPEC_UINT (pspec)->maximum,
				       MIN (1, stepping_rate),
				       MAX (10, stepping_rate),
				       0);
    }
  scale = gtk_widget_new (GTK_TYPE_VSCALE,
			  "visible", TRUE,
			  "can_focus", FALSE,
			  "draw_value", FALSE,
			  NULL);
  g_object_set_data_full (G_OBJECT (scale), "adjustment", g_object_ref (adjustment), (GDestroyNotify) g_object_unref);
  gtk_object_sink (adjustment);
  g_object_set (scale,
		"adjustment", adjustment,
		NULL);
  g_object_connect (adjustment,
		    "swapped_signal_after::value-changed", notify_func, notify_data,
		    NULL);

  return scale;
}

static void
controller_iscale_update (GtkWidget    *widget,
			  GParamSpec   *pspec,
			  const GValue *value,
			  gsize         controller_data)
{
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  if (G_IS_PARAM_SPEC_INT (pspec))
    adjustment->value = g_value_get_int (value);
  else
    adjustment->value = g_value_get_uint (value);
  gtk_adjustment_value_changed (adjustment);
}

static void
controller_iscale_fetch (GtkWidget  *widget,
			 GParamSpec *pspec,
			 GValue     *value,
			 gsize       controller_data)
{
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  if (G_IS_PARAM_SPEC_INT (pspec))
    g_value_set_int (value, adjustment->value);
  else
    g_value_set_uint (value, adjustment->value);
}

static BstControllerInfo controller_signed_iscale = {
  G_TYPE_INT,		"IntScale (Signed)",		G_TYPE_INT,
  controller_iscale_check,
  controller_iscale_create,
  controller_iscale_update,
  controller_iscale_fetch,
};

static BstControllerInfo controller_unsigned_iscale = {
  G_TYPE_UINT,		"IntScale (Unsigned)",		G_TYPE_UINT,
  controller_iscale_check,
  controller_iscale_create,
  controller_iscale_update,
  controller_iscale_fetch,
};


/* --- controller list --- */
static BstControllerInfo**
controller_array (guint *n)
{
  static BstControllerInfo *controllers[] = {
    &controller_log_knob,
    &controller_knob,
    &controller_signed_iscale,
    &controller_unsigned_iscale,
    &controller_log_scale,
    &controller_scale,
    &controller_log_dial,
    &controller_dial,
    &controller_label,
    &controller_pspec,
  };

  *n = G_N_ELEMENTS (controllers);
  return controllers;
}
