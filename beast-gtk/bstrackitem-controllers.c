#include	<gtk/gtklabel.h>
#include	"bstdial.h"
#include	"bstlogadjustment.h"


/* --- simple label --- */
static GtkWidget*
controller_label_create (GParamSpec  *pspec,
			 GCallback    notify_func,
			 gpointer     notify_data,
			 const gchar *name)
{
  GtkWidget *widget;
  
  widget = g_object_new (GTK_TYPE_LABEL,
			 "visible", TRUE,
			 NULL);
  
  return widget;
}

static void
controller_label_update (GtkWidget    *widget,
			 GParamSpec   *pspec,
			 const GValue *value)
{	
  gtk_label_set_text (GTK_LABEL (widget), g_value_get_string (value));
}

static BstControllerInfo controller_label = {
  "Label",
  G_TYPE_STRING,
  controller_label_create,
  controller_label_update,
  NULL,
};


/* --- dial widget --- */
static GtkWidget*
controller_dial_create (GParamSpec  *pspec,
			GCallback    notify_func,
			gpointer     notify_data,
			const gchar *name)
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
  g_object_ref (adjustment);
  gtk_object_sink (adjustment);
  g_object_set_data_full (G_OBJECT (dial), "adjustment", adjustment, (GDestroyNotify) g_object_unref);

  bse_param_spec_get_log_scale (pspec, &lscale);
  if (lscale.n_steps)
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
			 const GValue *value)
{
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  adjustment->value = g_value_get_float (value);
  gtk_adjustment_value_changed (adjustment);
}

static void
controller_dial_fetch (GtkWidget  *widget,
		       GParamSpec *pspec,
		       GValue     *value)
{
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  g_value_set_float (value, adjustment->value);
}

static BstControllerInfo controller_dial = {
  "Dial",
  G_TYPE_FLOAT,
  controller_dial_create,
  controller_dial_update,
  controller_dial_fetch,
};

static BstControllerInfo controller_log_dial = {
  "Logarithmic Dial",
  G_TYPE_FLOAT,
  controller_dial_create,
  controller_dial_update,
  controller_dial_fetch,
};


/* --- scale widget --- */
static GtkWidget*
controller_scale_create (GParamSpec  *pspec,
			GCallback    notify_func,
			gpointer     notify_data,
			const gchar *name)
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
  g_object_ref (adjustment);
  gtk_object_sink (adjustment);
  g_object_set_data_full (G_OBJECT (scale), "adjustment", adjustment, (GDestroyNotify) g_object_unref);

  bse_param_spec_get_log_scale (pspec, &lscale);
  if (lscale.n_steps)
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
controller_scale_update (GtkWidget    *widget,
			 GParamSpec   *pspec,
			 const GValue *value)
{
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  adjustment->value = g_value_get_float (value);
  gtk_adjustment_value_changed (adjustment);
}

static void
controller_scale_fetch (GtkWidget  *widget,
		       GParamSpec *pspec,
		       GValue     *value)
{
  GtkAdjustment *adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  g_value_set_float (value, adjustment->value);
}

static BstControllerInfo controller_scale = {
  "Scale",
  G_TYPE_FLOAT,
  controller_scale_create,
  controller_scale_update,
  controller_scale_fetch,
};

static BstControllerInfo controller_log_scale = {
  "Logarithmic Scale",
  G_TYPE_FLOAT,
  controller_scale_create,
  controller_scale_update,
  controller_scale_fetch,
};


/* --- controller list --- */
static BstControllerInfo *controllers[] = {
  &controller_log_scale,
  &controller_scale,
  &controller_log_dial,
  &controller_dial,
  &controller_label,
};
