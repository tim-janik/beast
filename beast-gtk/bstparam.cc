// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstparam.hh"
#include "bstxframe.hh"


/* --- prototypes --- */
static gboolean bst_param_xframe_check_button (GxkParam *param,
                                               guint     button);


/* --- variable --- */
static GQuark quark_null_group = 0;
static GQuark quark_param_choice_values = 0;
static guint  param_size_group = 0;


/* --- gmask parameters --- */
static GtkWidget*
param_get_gmask_container (GtkWidget *parent,
                           GQuark     quark_group)
{
  GtkWidget *container = bst_container_get_named_child (parent, quark_group ? quark_group : quark_null_group);
  if (!container || !GTK_IS_CONTAINER (container))
    {
      GtkWidget *any;
      container = bst_gmask_container_create (quark_group ? 5 : 0, FALSE);
      if (quark_group)
        any = (GtkWidget*) g_object_new (GTK_TYPE_FRAME,
                                         "visible", TRUE,
                                         "child", container,
                                         "label-widget", g_object_new (GXK_TYPE_SIMPLE_LABEL,
                                                                       "label", g_quark_to_string (quark_group),
                                                                       NULL),
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

static const GxkParamEditorSizes param_editor_homogeneous_sizes = {
  FALSE,        /* may_resize */
  FALSE,        /* request_fractions */
  2, 8,         /* char */
  2, 8,         /* uchar */
  2, 8,         /* int */
  2, 8,         /* uint */
  2, 8,         /* long */
  2, 8,         /* ulong */
  2, 8,         /* int64 */
  2, 8,         /* uint64 */
  2, 8,         /* float */
  2, 8,         /* double */
  9, 3,         /* string */
};

static BstGMask*
bst_param_create_gmask_intern (GxkParam    *param,
                               const gchar *editor_name,
                               GtkWidget   *parent,
                               guint        column,
                               gboolean     multi_span)
{
  SfiProxy proxy = bst_param_get_proxy (param);
  const gchar *group;
  GtkWidget *xframe, *action, *prompt = NULL;
  BstGMask *gmask;
  gboolean expand_action;
  gchar *tooltip;

  assert_return (GXK_IS_PARAM (param), NULL);
  assert_return (GTK_IS_CONTAINER (parent), NULL);

  gxk_param_set_sizes (param_size_group, BST_GCONFIG (size_group_input_fields) ? &param_editor_homogeneous_sizes : NULL);
  group = sfi_pspec_get_group (param->pspec);
  parent = param_get_gmask_container (parent, group ? g_quark_from_string (group) : 0);

  action = gxk_param_create_editor (param, editor_name);

  xframe = (GtkWidget*) g_object_new (BST_TYPE_XFRAME, "cover", action, NULL);
  g_object_connect (xframe,
                    "swapped_signal::button_check", bst_param_xframe_check_button, param,
                    NULL);

  if (GTK_IS_TOGGLE_BUTTON (action))
    {
      /* if there's a prompt widget inside the button already, sneak in xframe */
      if (GTK_BIN (action)->child)
        {
          gtk_widget_reparent (GTK_BIN (action)->child, xframe);
          g_object_set (xframe, "parent", action, "steal_button", TRUE, NULL);
        }
    }
  else
    {
      prompt = (GtkWidget*) g_object_new (GTK_TYPE_LABEL,
                                          "visible", TRUE,
                                          "label", g_param_spec_get_nick (param->pspec),
                                          "xalign", 0.0,
                                          "parent", xframe,
                                          NULL);
      gxk_param_add_object (param, GTK_OBJECT (prompt));
    }

  expand_action = !prompt || gxk_widget_check_option (action, "hexpand");
  gmask = bst_gmask_form (parent, action, multi_span ? BST_GMASK_MULTI_SPAN : expand_action ? BST_GMASK_BIG : BST_GMASK_INTERLEAVE);
  bst_gmask_set_column (gmask, column);
  if (BSE_IS_SOURCE (proxy) && sfi_pspec_check_option (param->pspec, "automate"))
    {
      GtkWidget *automation = gxk_param_create_editor (param, "automation");
      if (prompt)
        {
          GtkBox *hbox = (GtkBox*) g_object_new (GTK_TYPE_HBOX, "visible", TRUE, NULL);
          gtk_box_pack_start (hbox, gtk_widget_get_toplevel (prompt), FALSE, TRUE, 0);
          gtk_box_pack_end (hbox, automation, FALSE, TRUE, 0);
        }
      else
        prompt = automation;
    }
  if (prompt)
    bst_gmask_set_prompt (gmask, prompt);
  if (sfi_pspec_check_option (param->pspec, "dial"))
    {
      GtkWidget *dial = gxk_param_create_editor (param, "dial");
      bst_gmask_set_aux1 (gmask, dial);
    }
  if (sfi_pspec_check_option (param->pspec, "scale") ||
      sfi_pspec_check_option (param->pspec, "dial") ||
      sfi_pspec_check_option (param->pspec, "note"))
    {
      GtkWidget *scale = gxk_param_create_editor (param, "hscale");
      if (scale)
        bst_gmask_set_aux2 (gmask, scale);
      else
        g_message ("failed to create scale/dial widget for parameter \"%s\" of type `%s'",
                   param->pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (param->pspec)));
    }

  tooltip = gxk_param_dup_tooltip (param);
  bst_gmask_set_tip (gmask, tooltip);
  g_free (tooltip);
  bst_gmask_pack (gmask); /* this alters tooltips of some editors, e.g. GxkMenuButton choice */
  gxk_param_update (param);
  return gmask;
}

BstGMask*
bst_param_create_gmask (GxkParam    *param,
                        const gchar *editor_name,
                        GtkWidget   *parent)
{
  return bst_param_create_gmask_intern (param, editor_name, parent, 0, FALSE);
}

BstGMask*
bst_param_create_col_gmask (GxkParam    *param,
                            const gchar *editor_name,
                            GtkWidget   *parent,
                            guint        column)
{
  return bst_param_create_gmask_intern (param, editor_name, parent, column, FALSE);
}

BstGMask*
bst_param_create_span_gmask (GxkParam    *param,
                             const gchar *editor_name,
                             GtkWidget   *parent,
                             guint        column)
{
  return bst_param_create_gmask_intern (param, editor_name, parent, column, TRUE);
}

/* --- value binding --- */
GxkParam*
bst_param_new_value (GParamSpec          *pspec,
                     GxkParamValueNotify  notify,
                     gpointer             notify_data)
{
  GxkParam *param = gxk_param_new_value (pspec, notify, notify_data);
  if (param)
    gxk_param_set_size_group (param, param_size_group);
  return param;
}

/* --- GObject binding --- */
GxkParam*
bst_param_new_object (GParamSpec  *pspec,
                      GObject     *object)
{
  GxkParam *param = gxk_param_new_object (pspec, object);
  if (param)
    gxk_param_set_size_group (param, param_size_group);
  return param;
}

/* --- proxy binding --- */
static void
proxy_binding_set_value (GxkParam     *param,
                         const GValue *value)
{
  SfiProxy proxy = param->bdata[0].v_long;
  if (proxy)
    sfi_glue_proxy_set_property (proxy, param->pspec->name, value);
}

static void
proxy_binding_get_value (GxkParam *param,
                         GValue   *value)
{
  SfiProxy proxy = param->bdata[0].v_long;
  if (proxy)
    {
      const GValue *cvalue = sfi_glue_proxy_get_property (proxy, param->pspec->name);
      if (cvalue)
	g_value_transform (cvalue, value);
      else
	g_value_reset (value);
    }
  else
    g_value_reset (value);
}

static void
proxy_binding_weakref (gpointer data,
                       SfiProxy junk)
{
  GxkParam *param = (GxkParam*) data;
  param->bdata[0].v_long = 0;
  param->bdata[1].v_long = 0;	/* already disconnected */
}

static void
proxy_binding_destroy (GxkParam *param)
{
  SfiProxy proxy = param->bdata[0].v_long;
  if (proxy)
    {
      sfi_glue_signal_disconnect (proxy, param->bdata[1].v_long);
      sfi_glue_proxy_weak_unref (proxy, proxy_binding_weakref, param);
      param->bdata[0].v_long = 0;
      param->bdata[1].v_long = 0;
    }
}

static void
proxy_binding_start_grouping (GxkParam *param)
{
  SfiProxy proxy = param->bdata[0].v_long;
  Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (proxy));
  if (item)
    item.group_undo (string_format ("Modify %s", g_param_spec_get_nick (param->pspec)));
}

static void
proxy_binding_stop_grouping (GxkParam *param)
{
  SfiProxy proxy = param->bdata[0].v_long;
  Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (proxy));
  if (proxy)
    item.ungroup_undo();
}

static gboolean
proxy_binding_check_writable (GxkParam *param)
{
  SfiProxy proxy = param->bdata[0].v_long;
  if (proxy)
    return Bse::ItemH::down_cast (bse_server.from_proxy (proxy)).editable_property (param->pspec->name);
  else
    return FALSE;
}

static GxkParamBinding proxy_binding = {
  2,    // n_data_fields
  NULL, // setup
  proxy_binding_set_value,
  proxy_binding_get_value,
  proxy_binding_destroy,
  proxy_binding_check_writable,
  proxy_binding_start_grouping,
  proxy_binding_stop_grouping,
};

GxkParam*
bst_param_new_proxy (GParamSpec *pspec,
                     SfiProxy    proxy)
{
  GxkParam *param = gxk_param_new (pspec, &proxy_binding, (gpointer) FALSE);
  bst_param_set_proxy (param, proxy);
  gxk_param_set_size_group (param, param_size_group);
  return param;
}

void
bst_param_set_proxy (GxkParam *param,
                     SfiProxy  proxy)
{
  assert_return (GXK_IS_PARAM (param));
  assert_return (param->binding == &proxy_binding);

  proxy_binding_destroy (param);
  param->bdata[0].v_long = proxy;
  if (proxy)
    {
      gchar *sig = g_strconcat ("property-notify::", param->pspec->name, NULL);
      param->bdata[1].v_long = sfi_glue_signal_connect_swapped (proxy, sig, (void*) gxk_param_update, param);
      g_free (sig);
      sfi_glue_proxy_weak_ref (proxy, proxy_binding_weakref, param);
    }
}

SfiProxy
bst_param_get_proxy (GxkParam *param)
{
  assert_return (GXK_IS_PARAM (param), 0);

  if (param->binding == &proxy_binding)
    return param->bdata[0].v_long;
  return 0;
}

bool
bst_param_is_proxy (GxkParam *param)
{
  assert_return (GXK_IS_PARAM (param), 0);
  return param->binding == &proxy_binding;
}


/* --- record binding --- */
static void
record_binding_set_value (GxkParam     *param,
			  const GValue *value)
{
  sfi_rec_set ((SfiRec*) param->bdata[0].v_pointer, param->pspec->name, value);
}

static void
record_binding_get_value (GxkParam *param,
			  GValue   *value)
{
  const GValue *cvalue = sfi_rec_get ((SfiRec*) param->bdata[0].v_pointer, param->pspec->name);
  if (cvalue)
    g_value_transform (cvalue, value);
  else
    g_value_reset (value);
}

static void
record_binding_destroy (GxkParam *param)
{
  sfi_rec_unref ((SfiRec*) param->bdata[0].v_pointer);
  param->bdata[0].v_pointer = NULL;
}

static GxkParamBinding record_binding = {
  1, NULL,
  record_binding_set_value,
  record_binding_get_value,
  record_binding_destroy,
  NULL,	/* check_writable */
};

GxkParam*
bst_param_new_rec (GParamSpec *pspec,
                   SfiRec     *rec)
{
  GxkParam *param = gxk_param_new (pspec, &record_binding, (gpointer) FALSE);
  assert_return (rec != NULL, NULL);
  param->bdata[0].v_pointer = sfi_rec_ref (rec);
  gxk_param_set_size_group (param, param_size_group);
  return param;
}


// == Aida::Parameter binding ==
static void
aida_parameter_binding_set_value (GxkParam *param, const GValue *value)
{
  Aida::Parameter *const apa = (Aida::Parameter*) param->bdata[0].v_pointer;
  Any any;
  switch (G_TYPE_FUNDAMENTAL (G_VALUE_TYPE (value)))
    {
    case G_TYPE_BOOLEAN:        // sfi_pspec_bool
      any.set<bool> (g_value_get_boolean (value));
      break;
    case G_TYPE_INT64:          // sfi_pspec_num
      any.set (g_value_get_int64 (value));
      break;
    case G_TYPE_DOUBLE:         // sfi_pspec_real
      any.set (g_value_get_double (value));
      break;
    case G_TYPE_STRING:
      if (G_VALUE_TYPE (value) == SFI_TYPE_CHOICE)      // sfi_pspec_choice
        {
          const Aida::EnumInfo &enum_info = *(const Aida::EnumInfo*) param->bdata[1].v_pointer;
          assert_return (NULL != &enum_info);
          any.set_enum (enum_info, enum_info.value_from_string (sfi_value_get_choice (value)));
        }
      else                      // sfi_pspec_string
        any.set (g_value_get_string (value));
      break;
    default:
      Bse::warning ("%s: unsupported type: %s", __func__, g_type_name (G_PARAM_SPEC_VALUE_TYPE (param->pspec)));
      return;
    }
  apa->set (any);
}

static void
aida_parameter_binding_get_value (GxkParam *param, GValue *param_value)
{
  Aida::Parameter *const apa = (Aida::Parameter*) param->bdata[0].v_pointer;
  Any any = apa->get();
  GValue value = { 0, };
  switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (param->pspec)))
    {
    case G_TYPE_BOOLEAN:        // sfi_pspec_bool
      g_value_init (&value, G_TYPE_BOOLEAN);
      g_value_set_boolean (&value, any.get<bool>());
      break;
    case G_TYPE_INT64:          // sfi_pspec_num
      g_value_init (&value, G_TYPE_INT64);
      g_value_set_int64 (&value, any.get<int64>());
      break;
    case G_TYPE_DOUBLE:         // sfi_pspec_real
      g_value_init (&value, G_TYPE_DOUBLE);
      g_value_set_double (&value, any.get<double>());
      break;
    case G_TYPE_STRING:
      if (G_PARAM_SPEC_VALUE_TYPE (param->pspec) == SFI_TYPE_CHOICE)    // sfi_pspec_choice
        {
          const Aida::EnumInfo &enum_info = any.get_enum_info();
          g_value_init (&value, SFI_TYPE_CHOICE);
          sfi_value_set_choice (&value, enum_info.value_to_string (any.as_int64()).c_str());
          param->bdata[1].v_pointer = (void*) &enum_info;
        }
      else                      // sfi_pspec_string
        {
          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, any.get<String>().c_str());
        }
      break;
    default:
      Bse::warning ("%s: unsupported type: %s", __func__, g_type_name (G_PARAM_SPEC_VALUE_TYPE (param->pspec)));
      return;
    }
  if (G_VALUE_TYPE (&value))
    {
      g_value_transform (&value, param_value);
      g_value_unset (&value);
    }
}

static void
aida_parameter_binding_destroy (GxkParam *param)
{
  Aida::Parameter *const cxxparam = (Aida::Parameter*) param->bdata[0].v_pointer;
  param->bdata[0].v_pointer = NULL;
  delete cxxparam;
}

static gboolean
aida_parameter_binding_check_writable (GxkParam *param)
{
  // Aida::Parameter *const cxxparam = (Aida::Parameter*) param->bdata[0].v_pointer;
  // assert (cxxparam);
  return true;
}

static GxkParamBinding aida_parameter_binding = {
  2, // Aida::Parameter*, const Aida::EnumInfo*
  NULL,
  aida_parameter_binding_set_value,
  aida_parameter_binding_get_value,
  aida_parameter_binding_destroy,
  aida_parameter_binding_check_writable,
};

GxkParam*
bst_param_new_aida_parameter (GParamSpec *pspec, const Aida::Parameter &aparameter)
{
  GxkParam *param = gxk_param_new (pspec, &aida_parameter_binding, NULL);
  Aida::Parameter *cxxparam = new Aida::Parameter (aparameter);
  param->bdata[0].v_pointer = cxxparam;
  param->bdata[1].v_pointer = NULL;
  auto handler = [param] (const String &what) {
    bool match = what == param->pspec->name;
    if (!match && what.size() == strlen (param->pspec->name))
      {
        const String pname = Bse::string_canonify (Bse::string_tolower (param->pspec->name), "abcdefghijklmnopqrstuvwxyz0123456789", "_");
        match = what == pname;
      }
    if (match)
      gxk_param_update (param);
  };
  cxxparam->sig_changed() += handler; // disconnected by delete cxxparam
  gxk_param_set_size_group (param, param_size_group);
  return param;
}


/* --- param implementation utils --- */
static gboolean
bst_param_xframe_check_button (GxkParam *param,
                               guint     button)
{
  assert_return (GXK_IS_PARAM (param), FALSE);
#if 0
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
#endif
  return FALSE;
}


/* --- param editor registration --- */
#include "bstparam-choice.cc"
#include "bstparam-color-spinner.cc"
#include "bstparam-note-sequence.cc"
#include "bstparam-note-spinner.cc"
#include "bstparam-proxy.cc"
#include "bstparam-item-seq.cc"
#include "bstparam-scale.cc"
#include "bstparam-searchpath.cc"
#include "bstparam-time.cc"
#include "bstparam-automation.cc"
void
_bst_init_params (void)
{
  assert_return (quark_null_group == 0);

  quark_null_group = g_quark_from_static_string ("bst-param-null-group");
  quark_param_choice_values = g_quark_from_static_string ("bst-param-choice-values");
  param_size_group = gxk_param_create_size_group ();
  gxk_param_register_editor (&param_choice1, NULL);
  gxk_param_register_editor (&param_choice2, NULL);
  gxk_param_register_editor (&param_choice3, NULL);
  gxk_param_register_editor (&param_choice4, NULL);
  gxk_param_register_aliases (param_choice_aliases1);
  gxk_param_register_editor (&param_color_spinner_int, NULL);
  gxk_param_register_editor (&param_color_spinner_num, NULL);
  gxk_param_register_editor (&param_note_sequence, NULL);
  gxk_param_register_editor (&param_note_spinner_int, NULL);
  gxk_param_register_editor (&param_note_spinner_num, NULL);
  gxk_param_register_editor (&param_proxy, NULL);
  gxk_param_register_editor (&param_it3m_seq, NULL);
  gxk_param_register_editor (&param_automation, NULL);
  gxk_param_register_editor (&param_scale1, NULL);
  gxk_param_register_editor (&param_scale2, NULL);
  gxk_param_register_editor (&param_scale3, NULL);
  gxk_param_register_editor (&param_scale4, NULL);
  gxk_param_register_aliases (param_scale_aliases1);
  gxk_param_register_aliases (param_scale_aliases2);
  gxk_param_register_editor (&param_searchpath, NULL);
  gxk_param_register_editor (&param_filename, NULL);
  gxk_param_register_editor (&param_time, NULL);
}
