// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

/* --- h/v scale editors --- */
enum {
  PARAM_SCALE_HORIZONTAL,
  PARAM_LSCALE_HORIZONTAL,
  PARAM_PSCALE_HORIZONTAL,
  PARAM_SCALE_VERTICAL,
  PARAM_LSCALE_VERTICAL,
  PARAM_PSCALE_VERTICAL,
};

static void
param_scale_hscale_size_request (GtkWidget      *scale,
                                 GtkRequisition *requisition)
{
  gint slider_length = 0, trough_border = 0;
  /* we patch up the scale's minimum size requisition here */
  gtk_widget_style_get (scale, "slider_length", &slider_length, NULL);
  gtk_widget_style_get (scale, "trough_border", &trough_border, NULL);
  requisition->width = slider_length * 1.5;
  requisition->width += 2 * trough_border;
}

static void
param_scale_vscale_size_request (GtkWidget      *scale,
                                 GtkRequisition *requisition)
{
  gint slider_length = 0, trough_border = 0;
  /* we patch up the scale's minimum size requisition here */
  gtk_widget_style_get (scale, "slider_length", &slider_length, NULL);
  gtk_widget_style_get (scale, "trough_border", &trough_border, NULL);
  requisition->height = slider_length * 1.5;
  requisition->height += 2 * trough_border;
}

static GtkWidget*
param_scale_create (GxkParam    *param,
                    const gchar *tooltip,
                    guint        variant)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment = NULL;
  if (variant == PARAM_LSCALE_HORIZONTAL || variant == PARAM_LSCALE_VERTICAL)
    adjustment = gxk_param_get_log_adjustment (param);
  if (variant == PARAM_PSCALE_HORIZONTAL || variant == PARAM_PSCALE_VERTICAL)
    {
      uint exponent;
      g_param_spec_get_poly_scale (param->pspec, &exponent);
      printf ("pscale exp=%d:opts=%s\n", exponent, g_param_spec_get_options (param->pspec));
    }
  if ((variant == PARAM_SCALE_HORIZONTAL || variant == PARAM_SCALE_VERTICAL) &&
      (g_param_spec_check_option (param->pspec, "db-volume") ||
       g_param_spec_check_option (param->pspec, "db-range")) &&
      !g_param_spec_check_option (param->pspec, "db-value"))
    adjustment = gxk_param_get_decibel_adjustment (param);
  if (!adjustment)
    adjustment = gxk_param_get_adjustment (param);
  widget = (GtkWidget*) g_object_new (variant >= PARAM_SCALE_VERTICAL ? GTK_TYPE_VSCALE : GTK_TYPE_HSCALE,
                                      "adjustment", adjustment,
                                      "draw_value", FALSE,
                                      "visible", TRUE,
                                      "can_focus", FALSE,
                                      NULL);
  g_object_connect (widget,
                    "signal_after::size_request",
                    variant >= PARAM_SCALE_VERTICAL ? param_scale_vscale_size_request : param_scale_hscale_size_request,
                    NULL, NULL);
  gxk_widget_set_tooltip (widget, tooltip);
  if (variant >= PARAM_SCALE_VERTICAL)
    gxk_widget_add_option (widget, "vexpand", "+");
  else
    gxk_widget_add_option (widget, "hexpand", "+");
  gxk_param_add_grab_widget (param, widget);
  return widget;
}

static GxkParamEditor param_scale1 = {
  { "hscale-lin",       N_("Horizontal Scale"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { NULL,         +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_HORIZONTAL,
};
static GxkParamEditor param_scale2 = {
  { "vscale-lin",       N_("Vertical Scale"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { NULL,         +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_VERTICAL,
};
static GxkParamEditor param_scale3 = {
  { "hscale-log",       N_("Horizontal Scale (Logarithmic)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "log-scale",  +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_LSCALE_HORIZONTAL,
};
static GxkParamEditor param_scale4 = {
  { "vscale-log",       N_("Vertical Scale (Logarithmic)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "log-scale",  +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_LSCALE_VERTICAL,
};
static GxkParamEditor param_scale5 = {
  { "hscale-poly",       N_("Horizontal Scale (Poly)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "poly-scale",  +10,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_PSCALE_HORIZONTAL,
};
static GxkParamEditor param_scale6 = {
  { "vscale-poly",       N_("Vertical Scale (Poly)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "poly-scale",  +10,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_PSCALE_VERTICAL,
};
static const gchar *param_scale_aliases1[] = {
  "hscale",
  "hscale-lin", "hscale-log", "pscale-poly",
  NULL,
};
static const gchar *param_scale_aliases2[] = {
  "vscale",
  "vscale-lin", "vscale-log", "pscale-poly",
  NULL,
};
