/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bstparam.h"

#include	"bstsamplerepo.h"
#include	<stdlib.h>
#include	<math.h>


#undef	DEBUG_ADJUSTMENT


/* --- structures --- */
typedef struct _DotAreaData DotAreaData;
struct _DotAreaData
{
  gint cdot;
  gboolean entered;
};

/* --- prototypes --- */
static gint dots_area_configure_event	(GtkWidget          *widget,
					 GdkEventConfigure  *event,
					 BstParam           *bparam);
static gint dots_area_expose_event	(GtkWidget          *widget,
					 GdkEventExpose     *event,
					 BstParam	    *bparam);
static gint dots_area_cross_event	(GtkWidget          *widget,
					 GdkEventCrossing   *event,
					 BstParam	    *bparam);
static gint dots_area_button_event	(GtkWidget          *widget,
					 GdkEventButton     *event,
					 BstParam	    *bparam);
static gint dots_area_motion_event	(GtkWidget          *widget,
					 GdkEventMotion     *event,
					 BstParam	    *bparam);


/* --- variables --- */
static GQuark quark_evalues = 0;
static GQuark quark_fvalues = 0;


/* --- widget group --- */
#define _GROUP_GET_NAMED_WIDGET(g,n) ((GtkWidget*) gtk_object_get_data (GTK_OBJECT (g), n))
static void
_GROUP_ADD_NAMED_OBJECT (GtkWidget   *group,
			 const gchar *name,
			 gpointer     object)
{
  g_return_if_fail (GTK_IS_WIDGET (group));
  g_return_if_fail (GTK_IS_OBJECT (object));
  
  if (group != object)
    {
      gtk_object_ref (object);
      gtk_object_sink (object);
    }
  gtk_object_set_data_full (GTK_OBJECT (group), name, object,
			    (GtkDestroyNotify) (group != object ? gtk_object_unref : NULL));
}
static void
_GROUP_CALL (GtkWidget *group,
	     gpointer   f,
	     gpointer   d)
{
  static const gchar *names[] = {
    "group_prompt",
    "group_dial",
    "group_scale",
    "group_pre_action",
    "group_post_action",
    "group_action",
  };
  guint i;

  g_return_if_fail (GTK_IS_WIDGET (group));

  gtk_widget_ref (group);
  for (i = 0; i < sizeof (names) / sizeof (names[0]); i++)
    {
      GtkWidget *callee = _GROUP_GET_NAMED_WIDGET (group, names[i]);

      if (callee)
	((GtkCallback) f) (callee, d);
    }
  gtk_widget_unref (group);
}
static GtkWidget*
GROUP_FORM (GtkWidget *parent,
	    GtkWidget *action,
	    gboolean   expandable)
{
  g_return_val_if_fail (GTK_IS_CONTAINER (parent), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (action), NULL);

  _GROUP_ADD_NAMED_OBJECT (action, "group_action", action);
  _GROUP_ADD_NAMED_OBJECT (action, "group_parent", parent);
  gtk_object_set_data (GTK_OBJECT (action), "group_flags", GUINT_TO_POINTER (expandable ? 2 : 0));

  return action;
}
static GtkWidget*
GROUP_FORM_BIG (GtkWidget *parent,
		GtkWidget *action)
{
  g_return_val_if_fail (GTK_IS_CONTAINER (parent), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (action), NULL);

  _GROUP_ADD_NAMED_OBJECT (action, "group_action", action);
  _GROUP_ADD_NAMED_OBJECT (action, "group_parent", parent);
  gtk_object_set_data (GTK_OBJECT (action), "group_flags", GUINT_TO_POINTER (2 | 1));

  return action;
}
#define GROUP_ADD_POST_ACTION(g, w)       (_GROUP_ADD_NAMED_OBJECT ((g), "group_post_action", (w)))
#define GROUP_ADD_PRE_ACTION(g, w)        (_GROUP_ADD_NAMED_OBJECT ((g), "group_pre_action", (w)))
#define GROUP_ADD_PROMPT(g, w)            (_GROUP_ADD_NAMED_OBJECT ((g), "group_prompt", (w)))
#define GROUP_ADD_DIAL(g, w)              (_GROUP_ADD_NAMED_OBJECT ((g), "group_dial", (w)))
#define GROUP_ADD_SCALE(g, w)             (_GROUP_ADD_NAMED_OBJECT ((g), "group_scale", (w)))
#define GROUP_GET_ACTION(g)	          (GTK_WIDGET (g))
#define GROUP_GET_PRE_ACTION(g)	          (_GROUP_GET_NAMED_WIDGET ((g), "group_pre_action"))
#define GROUP_GET_POST_ACTION(g)	  (_GROUP_GET_NAMED_WIDGET ((g), "group_post_action"))
#define GROUP_GET_PROMPT(g)	          (_GROUP_GET_NAMED_WIDGET ((g), "group_prompt"))
static void
GROUP_SET_TIP (GtkWidget   *group,
	       const gchar *tip_text,
	       const gchar *tip_private)
{
  g_return_if_fail (GTK_IS_WIDGET (group));

  if (tip_text)
    gtk_object_set_data_full (GTK_OBJECT (group), "group_tiptext", g_strdup (tip_text), g_free);
  if (tip_private)
    gtk_object_set_data_full (GTK_OBJECT (group), "group_tipref", g_strdup (tip_private), g_free);
}
static GtkWidget*
GROUP_PARENT_CREATE (gpointer tooltips,
		     guint    border_width)
{
  GtkWidget *container = gtk_widget_new (GTK_TYPE_TABLE,
					 "visible", TRUE,
					 "homogeneous", FALSE,
					 "n_columns", 2,
					 "border_width", border_width,
					 NULL);
  if (tooltips)
    _GROUP_ADD_NAMED_OBJECT (container, "group_tooltips", tooltips);

  return container;
}
static GtkWidget*
GROUP_DONE (GtkWidget *group)
{
  GtkWidget *parent, *any;
  GtkWidget *post_action, *action, *pre_action, *prompt, *dial, *scale;
  GtkTooltips *tt = NULL;
  gchar *tip_text, *tip_ref = NULL;
  GtkTable *table;
  gboolean big, expandable;
  guint row, n, c;

  g_return_val_if_fail (GTK_IS_WIDGET (group), NULL);

  action = group;
  parent = _GROUP_GET_NAMED_WIDGET (group, "group_parent");
  prompt = _GROUP_GET_NAMED_WIDGET (group, "group_prompt");
  dial = _GROUP_GET_NAMED_WIDGET (group, "group_dial");
  scale = _GROUP_GET_NAMED_WIDGET (group, "group_scale");
  pre_action = _GROUP_GET_NAMED_WIDGET (group, "group_pre_action");
  big = GPOINTER_TO_UINT (gtk_object_get_data (GTK_OBJECT (group), "group_flags"));
  expandable = (big & 2) != 0;
  big &= 1;
  post_action = _GROUP_GET_NAMED_WIDGET (group, "group_post_action");
  tip_text = gtk_object_get_data (GTK_OBJECT (group), "group_tiptext");
  if (tip_text)
    {
      tip_ref = gtk_object_get_data (GTK_OBJECT (group), "group_tipref");
      tt = gtk_object_get_data (GTK_OBJECT (parent), "group_tooltips");
    }
  
  /* ensure we have a tooltips sensitive field if required */
  if (tt && GTK_WIDGET_NO_WINDOW (action))
    {
      GtkWidget *at = gtk_widget_get_toplevel (action);

      if (GTK_WIDGET_NO_WINDOW (at))
	action = gtk_widget_new (GTK_TYPE_EVENT_BOX,
				 "visible", TRUE,
				 "child", at,
				 NULL);
      else
	action = at;
    }

  /* pack stuff, options: GTK_EXPAND, GTK_SHRINK, GTK_FILL */
  table = GTK_TABLE (parent);
  row = !table->children ? table->nrows - 1 : table->nrows;
  c = 0;
  if (prompt)
    gtk_table_attach (table, gtk_widget_get_toplevel (prompt),
		      c, c + 1, row, row + 1,
		      GTK_FILL, GTK_FILL,
		      0, 0);
  c++;
  if (dial)
    gtk_table_attach (table, gtk_widget_get_toplevel (dial),
		      c, c + 1, row, row + 1,
		      GTK_FILL, GTK_FILL,
		      5, 0);
  c++;
  gtk_table_attach (table,
		    scale ? gtk_widget_get_toplevel (scale) : gtk_widget_new (GTK_TYPE_ALIGNMENT,
									      "visible", TRUE,
									      NULL),
		    dial ? c : c - 1, c + 1, row, row + 1,
		    GTK_EXPAND | GTK_FILL, 0,
		    5, 0);
  c++;
  /* stuff action with pre- and post- widgets closely together */
  any = gtk_widget_get_toplevel (action);
  if (pre_action || post_action)
    {
      any = gtk_widget_new (GTK_TYPE_HBOX,
			    "visible", TRUE,
			    "child", any,
			    NULL);
      if (pre_action)
	gtk_container_add_with_args (GTK_CONTAINER (any), gtk_widget_get_toplevel (pre_action),
				     "position", 0,
				     "expand", FALSE,
				     NULL);
      if (post_action)
	gtk_container_add_with_args (GTK_CONTAINER (any), gtk_widget_get_toplevel (post_action),
				     "position", -1,
				     "expand", FALSE,
				     NULL);
    }
  n = c;
  if (big && !scale) /* expand action to the left when possible for big forms */
    {
      n--;
      if (!dial)
	{
	  n--;
	  if (!prompt)
	    n--;
	}
    }
  if (!expandable) /* align to right without expansion if desired */
    any = gtk_widget_new (GTK_TYPE_ALIGNMENT,
			  "visible", TRUE,
			  "child", any,
			  "xalign", 1.0,
			  "xscale", 0.0,
			  "yscale", 0.0,
			  NULL);
  gtk_table_attach (table, any,
		    n, c + 1, row, row + 1,
		    GTK_FILL, GTK_FILL,
		    0, 0);
  
  /* set tooltips */
  if (tt)
    {
      if (prompt && !GTK_WIDGET_NO_WINDOW (prompt))
	gtk_tooltips_set_tip (tt, prompt, tip_text, tip_ref);
      if (dial && !GTK_WIDGET_NO_WINDOW (dial))
	gtk_tooltips_set_tip (tt, dial, tip_text, tip_ref);
      if (scale && !GTK_WIDGET_NO_WINDOW (scale))
	gtk_tooltips_set_tip (tt, scale, tip_text, tip_ref);
      if (post_action && !GTK_WIDGET_NO_WINDOW (post_action))
	gtk_tooltips_set_tip (tt, post_action, tip_text, tip_ref);
      if (action && !GTK_WIDGET_NO_WINDOW (action))
	gtk_tooltips_set_tip (tt, action, tip_text, tip_ref);
      if (pre_action && !GTK_WIDGET_NO_WINDOW (pre_action))
	gtk_tooltips_set_tip (tt, pre_action, tip_text, tip_ref);
    }

  return group;
}
#define GROUP_DESTROY(g)          (_GROUP_CALL ((g), gtk_widget_destroy, NULL))
#define GROUP_ENSURE_STYLES(g)    (_GROUP_CALL ((g), gtk_widget_ensure_style, NULL))
#define GROUP_SET_SENSITIVE(g, s) (_GROUP_CALL ((g), (s) ? gtk_widget_make_sensitive : gtk_widget_make_insensitive, NULL))


/* --- functions --- */
static void
bst_string_toggle (GtkToggleButton *tb,
		   GtkWidget       *widget)
{
  gtk_editable_set_editable (GTK_EDITABLE (widget), tb->active);
  if (tb->active)
    gtk_widget_grab_focus (widget);
}

static void
bst_param_gtk_changed (BstParam *bparam)
{
  if (!bparam->locked)
    bst_param_set (bparam);
}

static void
bst_param_gtk_update (BstParam *bparam)
{
  if (!bparam->locked)
    bst_param_get (bparam);
}

static gboolean
bst_entry_key_press (GtkWidget   *entry,
		     GdkEventKey *event,
		     BstParam    *bparam)
{
  gboolean is_tab = FALSE;

  if (!gtk_type_is_a (GTK_OBJECT_TYPE (entry), GTK_TYPE_SPIN_BUTTON))
    switch (event->keyval)
      {
      case GDK_Tab:
      case GDK_ISO_Left_Tab:
	is_tab = TRUE;
	/* fall through */
      case GDK_Up:
      case GDK_KP_Up:
      case GDK_Down:
      case GDK_KP_Down:
	/* special case clue hunter entries */
	if (gtk_clue_hunter_from_entry (entry))
	  {
	    if (!is_tab && !gtk_grab_get_current ())
	      bst_param_gtk_changed (bparam);
	  }
	else
	  bst_param_gtk_changed (bparam);
	break;
      default:
	break;
      }

  if (event->keyval == 'f' && (event->state & GDK_MOD1_MASK))
    gtk_signal_emit_stop_by_name (GTK_OBJECT (entry), "key_press_event");

  return FALSE;
}

static void
bst_param_update_clue_hunter (BstParam *bparam)
{
  GtkWidget *group = bparam->group;
  BseParam *param = &bparam->param;
  BseParamSpec *pspec = param->pspec;
  GtkWidget *action = GROUP_GET_ACTION (group);
  GtkClueHunter *ch = gtk_object_get_user_data (GTK_OBJECT (action));

  g_return_if_fail (pspec->type == BSE_TYPE_PARAM_ITEM && GTK_IS_CLUE_HUNTER (ch));
  
  if (bse_type_is_a (pspec->s_item.item_type, BSE_TYPE_SAMPLE))
    {
      GList *free_list = bst_sample_repo_list_sample_locs (), *list;
      
      gtk_clue_hunter_remove_matches (ch, "*");
      
      for (list = free_list; list; list = list->next)
	{
	  BstSampleLoc *loc = list->data;
	  
	  /* FIXME: showing the repo as well would be nice */
	  gtk_clue_hunter_add_string (ch, loc->name);
	}
      g_list_free (free_list);
    }
  else if (bparam->is_object && BSE_IS_ITEM (bparam->owner) &&
	   bse_type_is_a (pspec->s_item.item_type, BSE_TYPE_ITEM))
    {
      BseItem *item = BSE_ITEM (bparam->owner);
      BseProject *project = bse_item_get_project (item);
      GList *nick_list, *list;
      
      gtk_clue_hunter_remove_matches (ch, "*");

      nick_list = bse_project_list_nick_paths (project, pspec->s_item.item_type);
      
      for (list = nick_list; list; list = list->next)
	{
	  gtk_clue_hunter_add_string (ch, list->data);
	  g_free (list->data);
	}
      g_list_free (nick_list);
    }
}

static void
bst_bparam_bse_changed (BstParam     *bparam,
			BseParamSpec *pspec)
{
  if (bparam->param.pspec == pspec && !bparam->locked)
    bst_param_get (bparam);
}

#ifdef DEBUG_ADJUSTMENT
static void
debug_adjustment (GtkAdjustment *adjustment,
		  BstParam      *bparam)
{
  g_print ("adjustment changed: %f (%u)\n", adjustment->value, bparam->locked);
}
#endif

void
bst_param_destroy (BstParam *bparam)
{
  g_return_if_fail (BSE_IS_PARAM (bparam));
  g_return_if_fail (bparam->locked == 0);

  GROUP_DESTROY (bparam->group);
}

static void
bst_param_free (BstParam *bparam)
{
  if (bparam->is_object)
    bst_param_set_object (bparam, NULL);
  else if (bparam->is_procedure)
    bst_param_set_procedure (bparam, NULL);

  bse_param_free_value (&bparam->param);
  
  g_free (bparam);
}

void
bst_param_set_procedure (BstParam          *bparam,
			 BseProcedureClass *proc)
{
  g_return_if_fail (BSE_IS_PARAM (bparam));
  g_return_if_fail (!bparam->locked);
  g_return_if_fail (bparam->is_procedure);
  if (proc)
    g_return_if_fail (BSE_IS_PROCEDURE_CLASS (proc));

  if (bparam->owner)
    bse_procedure_unref (bparam->owner);

  bparam->owner = proc;
  if (bparam->owner)
    {
      /* we SHOULD make sure here that bparam->param->pspec is a valid pspec
       * for proc, but actually i don't feel like writing the extra code just
       * to issue a warning
       */
      bse_procedure_ref (bparam->owner);
      if (bparam->group)
	bst_param_get (bparam);
    }
}

static void
bparam_reset_object (BstParam *bparam)
{
  bst_param_set_object (bparam, NULL);
}

void
bst_param_set_object (BstParam  *bparam,
		      BseObject *object)
{
  g_return_if_fail (BSE_IS_PARAM (bparam));
  g_return_if_fail (!bparam->locked);
  g_return_if_fail (bparam->is_object);
  if (object)
    g_return_if_fail (BSE_IS_OBJECT (object));

  if (bparam->owner)
    {
      bse_object_remove_notifier (bparam->owner, bparam->param_set_id);
      bse_object_remove_notifiers_by_func (bparam->owner,
					   bparam_reset_object,
					   bparam);
    }

  bparam->owner = object;
  if (bparam->owner)
    {
      /* we SHOULD make sure here that bparam->param->pspec is a valid pspec
       * for object->class (or its anchestors), but actually i don't feel
       * like writing the extra code just to issue a warning
       */
      bparam->param_set_id = bse_object_add_data_notifier (bparam->owner,
							   "param_changed",
							   bst_bparam_bse_changed,
							   bparam);
      bse_object_add_data_notifier (bparam->owner,
				    "destroy",
				    bparam_reset_object,
				    bparam);
      if (bparam->group)
	bst_param_get (bparam);
    }
}

BstParam*
bst_param_create (gpointer      owner,
		  BseType	owner_type,
		  BseParamSpec *pspec,
		  GQuark        param_group,
		  GtkWidget    *parent,
		  GtkTooltips  *tooltips)
{
  GtkWidget *parent_container;
  gpointer widget_group;
  static GQuark null_group = 0;
  BstParam *bparam;
  GtkAdjustment *adjustment = NULL;
  guint digits = 0;
  GtkWidget *spinner = NULL;
  GtkWidget *scale = NULL;
  GtkWidget *dial = NULL;
  gboolean read_only, string_toggle, radio, expandable;
  gchar *name, *tooltip;

  if (BSE_TYPE_IS_PROCEDURE (owner_type))
    g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (owner), NULL);
  else
    g_return_val_if_fail (BSE_IS_OBJECT (owner), NULL);
  g_return_val_if_fail (BSE_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (GTK_IS_TOOLTIPS (tooltips), NULL);
  
  if (!null_group)
    {
      null_group = g_quark_from_static_string ("Bst-null-group");
      quark_evalues = g_quark_from_static_string ("Bst-enum-values");
      quark_fvalues = g_quark_from_static_string ("Bst-flags-values");
    }

  bparam = g_new0 (BstParam, 1);
  bse_param_init_default (&bparam->param, pspec);
  bparam->owner = NULL;
  if (BSE_TYPE_IS_PROCEDURE (owner_type))
    {
      bparam->is_procedure = TRUE;
      bst_param_set_procedure (bparam, owner);
    }
  else
    {
      bparam->is_object = TRUE;
      bst_param_set_object (bparam, owner);
    }
  bparam->locked = 1;
  bse_type_class_ref (owner_type);
  
  parent_container = gtk_object_get_data_by_id (GTK_OBJECT (parent), param_group ? param_group : null_group);
  if (!parent_container ||
      GTK_OBJECT_DESTROYED (parent_container) ||
      !GTK_IS_CONTAINER (parent_container))
    {
      GtkWidget *any;
      
      any = GROUP_PARENT_CREATE (tooltips, param_group ? 5 : 0);
      parent_container = any;
      gtk_widget_ref (any);
      gtk_object_set_data_by_id_full (GTK_OBJECT (parent),
				      param_group ? param_group : null_group,
				      any,
				      (GtkDestroyNotify) gtk_widget_unref);
      if (param_group)
	any = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "label", g_quark_to_string (param_group),
			      "child", any,
			      NULL);
      if (GTK_IS_BOX (parent))
	gtk_box_pack_start (GTK_BOX (parent), any, FALSE, TRUE, 0);
      else
	gtk_container_add (GTK_CONTAINER (parent), any);
    }
  parent = NULL;
  
  /* feature param hints and integral values
   */
  read_only = (pspec->any.flags & BSE_PARAM_HINT_RDONLY) != 0;
  radio = (pspec->any.flags & BSE_PARAM_HINT_RADIO) != 0;
  string_toggle = (pspec->any.flags & BSE_PARAM_HINT_CHECK_NULL) != 0;
  switch (pspec->type)
    {
    case BSE_TYPE_PARAM_INT:
      if (pspec->s_int.stepping_rate != 0)
	adjustment = (GtkAdjustment*) gtk_adjustment_new (pspec->s_int.default_value,
							  pspec->s_int.minimum,
							  pspec->s_int.maximum,
							  1,
							  pspec->s_int.stepping_rate,
							  0);
      digits = 0;
      break;
    case BSE_TYPE_PARAM_UINT:
      if (pspec->s_uint.stepping_rate != 0)
	adjustment = (GtkAdjustment*) gtk_adjustment_new (pspec->s_uint.default_value,
							  pspec->s_uint.minimum,
							  pspec->s_uint.maximum,
							  1,
							  pspec->s_uint.stepping_rate,
							  0);
      digits = 0;
      break;
    case BSE_TYPE_PARAM_FLOAT:
      if (BSE_EPSILON_CMP (pspec->s_float.stepping_rate, 0) != 0)
	adjustment = (GtkAdjustment*) gtk_adjustment_new (pspec->s_float.default_value,
							  pspec->s_float.minimum,
							  pspec->s_float.maximum,
							  MIN (0.1, pspec->s_float.stepping_rate),
							  MAX (0.1, pspec->s_float.stepping_rate),
							  0);
      digits = 3;
      break;
    case BSE_TYPE_PARAM_DOUBLE:
      if (BSE_EPSILON_CMP (pspec->s_double.stepping_rate, 0) != 0)
	adjustment = (GtkAdjustment*) gtk_adjustment_new (pspec->s_double.default_value,
							  pspec->s_double.minimum,
							  pspec->s_double.maximum,
							  MIN (0.1, pspec->s_double.stepping_rate),
							  MAX (0.1, pspec->s_double.stepping_rate),
							  0);
      digits = 5;
      break;
    }
  if (adjustment)
    {
      gtk_object_ref (GTK_OBJECT (adjustment));
      gtk_object_sink (GTK_OBJECT (adjustment));
      
      /* we need to connect *after* the spinner so the spinner's value is
       * already updated
       */
      gtk_signal_connect_object_after (GTK_OBJECT (adjustment),
				       "value-changed",
				       bst_param_gtk_changed,
				       (GtkObject*) bparam);
#ifdef DEBUG_ADJUSTMENT
      gtk_signal_connect (GTK_OBJECT (adjustment), "value-changed", debug_adjustment, bparam);
#endif
      
      spinner = gtk_spin_button_new (adjustment, 0, digits);
      if (pspec->any.flags & BSE_PARAM_HINT_DIAL)
	{
	  dial = gtk_widget_new (GTK_TYPE_LABEL,
				 "visible", TRUE,
				 "label", "|<DIAL>|", /* FIXME: we need a real dial */
				 NULL);
	}
      if (pspec->any.flags & BSE_PARAM_HINT_SCALE
	  || pspec->any.flags & BSE_PARAM_HINT_DIAL) /* FIXME: we need a real dial */
	{
	  scale = gtk_hscale_new (adjustment);
	  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
	}
      
      gtk_object_unref (GTK_OBJECT (adjustment));
    }
  
  name = pspec->any.nick;
  tooltip = pspec->any.blurb;

  expandable = FALSE;
  switch (pspec->type)
    {
      GtkWidget *action, *prompt, *pre_action, *post_action, *frame, *any, *group;
      DotAreaData *dot_data;
      BseEnumValue *ev;
      guint width;

    case BSE_TYPE_PARAM_BOOL:
      action = gtk_widget_new (radio ? BST_TYPE_FREE_RADIO_BUTTON : GTK_TYPE_CHECK_BUTTON,
			       "visible", TRUE,
			       "label", name,
			       "object_signal::clicked", bst_param_gtk_changed, bparam,
			       NULL);
      gtk_misc_set_alignment (GTK_MISC (GTK_BIN (action)->child), 0, 0.5);
      group = GROUP_FORM_BIG (parent_container, action);
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    case BSE_TYPE_PARAM_INT:
    case BSE_TYPE_PARAM_UINT:
    case BSE_TYPE_PARAM_FLOAT:
    case BSE_TYPE_PARAM_DOUBLE:
    case BSE_TYPE_PARAM_TIME:
    case BSE_TYPE_PARAM_NOTE:
      switch (pspec->type)
	{
	case BSE_TYPE_PARAM_INT:
	case BSE_TYPE_PARAM_UINT:
	  width = 70;
	  break;
	case BSE_TYPE_PARAM_FLOAT:
	case BSE_TYPE_PARAM_DOUBLE:
	  expandable = TRUE;
	  width = 80;
	  break;
	case BSE_TYPE_PARAM_TIME:
	  expandable = TRUE;
	  width = 140;
	  break;
	case BSE_TYPE_PARAM_NOTE:
	  width = 50;
	  break;
	default:
	  width = 3;
	  break;
	}
      if (spinner)
	width += 10;
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       NULL);
      action = spinner ? spinner : gtk_entry_new ();
      gtk_widget_set (action,
		      "visible", TRUE,
		      "width", width,
		      "object_signal::activate", bst_param_gtk_changed, bparam,
		      "signal::key_press_event", bst_entry_key_press, bparam,
		      NULL);
      if (!spinner)
	gtk_widget_set (action,
			"object_signal::focus_out_event", bst_param_gtk_update, bparam,
			NULL);
      group = GROUP_FORM (parent_container, action, expandable);
      GROUP_ADD_PROMPT (group, prompt);
      if (scale)
	{
	  gtk_widget_show (scale);
	  GROUP_ADD_SCALE (group, scale);
	}
      if (dial)
	{
	  gtk_widget_show (dial);
	  GROUP_ADD_DIAL (group, dial);
	}
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    case BSE_TYPE_PARAM_ENUM:
      ev = pspec->s_enum.enum_class->values;
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       "sensitive", !read_only && ev,
			       NULL);
      action = gtk_option_menu_new ();
      gtk_widget_set (action,
		      "visible", TRUE,
		      NULL);
      if (ev)
	{
	  GtkWidget *menu;

	  menu = gtk_widget_new (GTK_TYPE_MENU,
				 "object_signal::selection_done", bst_param_gtk_changed, bparam,
				 NULL);
	  while (ev->value_nick)
	    {
	      GtkWidget *item;

	      item = gtk_menu_item_new_with_label (ev->value_nick);
	      gtk_widget_lock_accelerators (item);
	      gtk_widget_show (item);
	      gtk_object_set_data_by_id (GTK_OBJECT (item), quark_evalues, ev);
	      gtk_container_add (GTK_CONTAINER (menu), item);

	      ev++;
	    }

	  gtk_option_menu_set_menu (GTK_OPTION_MENU (action), menu);
	}
      group = GROUP_FORM (parent_container, action, FALSE);
      GROUP_ADD_PROMPT (group, prompt);
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    case BSE_TYPE_PARAM_FLAGS:
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "sensitive", FALSE,
			       "label", pspec->any.name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       NULL);
      action = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", pspec->any.blurb,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       NULL);
      group = GROUP_FORM (parent_container, action, FALSE);
      GROUP_ADD_PROMPT (group, prompt);
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    case BSE_TYPE_PARAM_STRING:
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       NULL);
      action = gtk_widget_new (GTK_TYPE_ENTRY,
			       "visible", TRUE,
			       "object_signal::focus_out_event", bst_param_gtk_update, bparam,
			       "object_signal::activate", bst_param_gtk_changed, bparam,
			       "signal::key_press_event", bst_entry_key_press, bparam,
			       NULL);
      group = GROUP_FORM (parent_container, action, TRUE);
      GROUP_ADD_PROMPT (group, prompt);
      if (string_toggle)
	{
	  pre_action = gtk_widget_new (GTK_TYPE_TOGGLE_BUTTON,
				       "visible", TRUE,
				       "can_focus", FALSE,
				       "width", 10,
				       "height", 10,
				       "parent", gtk_widget_new (GTK_TYPE_ALIGNMENT,
								 "visible", TRUE,
								 "xscale", 0.0,
								 "yscale", 0.0,
								 "xalign", 0.0,
								 "width", 13,
								 NULL),
				       "object_signal::clicked", bst_param_gtk_changed, bparam,
				       "signal::clicked", bst_string_toggle, action,
				       NULL);
	  GROUP_ADD_PRE_ACTION (group, pre_action);
	  bst_string_toggle (GTK_TOGGLE_BUTTON (pre_action), action);
	}
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    case BSE_TYPE_PARAM_DOTS:
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       NULL);
      frame = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "label", NULL,
			      "shadow", GTK_SHADOW_OUT,
			      "border_width", 5,
			      NULL);
      dot_data = g_new0 (DotAreaData, 1);
      action = gtk_widget_new (GTK_TYPE_DRAWING_AREA,
			       "visible", TRUE,
			       "height", 50,
			       "parent", frame,
			       "object_signal::destroy", g_free, dot_data,
			       "signal::configure_event", dots_area_configure_event, bparam,
			       "signal::expose_event", dots_area_expose_event, bparam,
			       "signal::enter_notify_event", dots_area_cross_event, bparam,
			       "signal::leave_notify_event", dots_area_cross_event, bparam,
			       "signal::button_press_event", dots_area_button_event, bparam,
			       "signal::button_release_event", dots_area_button_event, bparam,
			       "signal::motion_notify_event", dots_area_motion_event, bparam,
			       "events", (GDK_EXPOSURE_MASK |
					  GDK_ENTER_NOTIFY_MASK |
					  GDK_LEAVE_NOTIFY_MASK |
					  GDK_BUTTON_PRESS_MASK |
					  GDK_BUTTON_RELEASE_MASK |
					  GDK_BUTTON1_MOTION_MASK),
			       NULL);
      dot_data->cdot = -1;
      GTK_DRAWING_AREA (action)->draw_data = dot_data;
      group = GROUP_FORM_BIG (parent_container, action);
      GROUP_ADD_PROMPT (group, prompt);
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    case BSE_TYPE_PARAM_ITEM:
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       NULL);
      action = gtk_widget_new (GTK_TYPE_ENTRY,
			       "visible", TRUE,
			       "width", 160,
			       "object_signal::focus_out_event", bst_param_gtk_update, bparam,
			       "object_signal::activate", bst_param_gtk_changed, bparam,
			       "signal::key_press_event", bst_entry_key_press, bparam,
			       "object_signal::grab_focus", bst_param_update_clue_hunter, bparam,
			       NULL);
      group = GROUP_FORM (parent_container, action, TRUE);
      GROUP_ADD_PROMPT (group, prompt);
      any = gtk_widget_new (GTK_TYPE_CLUE_HUNTER,
			    "keep_history", FALSE,
			    "entry", action,
			    "user_data", bparam,
			    NULL);
      gtk_object_set_user_data (GTK_OBJECT (action), any);
      post_action = gtk_clue_hunter_create_arrow (GTK_CLUE_HUNTER (any));
      GROUP_ADD_POST_ACTION (group, post_action);
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    default:
      g_warning ("unknown param type: `%s'", pspec->any.name);
      widget_group = NULL;
      break;
    }
  
  bparam->group = widget_group;
  if (bparam->group)
    gtk_widget_set (bparam->group,
		    "signal::destroy", gtk_widget_destroyed, &bparam->group,
		    "object_signal::destroy", bst_param_free, bparam,
		    "object_signal::destroy", bse_type_class_unref, bse_type_class_peek (owner_type),
		    NULL);
  else
    g_return_val_if_fail (bparam->group != NULL, bparam);
  
  GROUP_ENSURE_STYLES (bparam->group);
  
  return bparam;
}

static void
bst_param_update (BstParam *bparam)
{
  GtkWidget *group = bparam->group;
  BseParam *param = &bparam->param;
  BseParamSpec *pspec = param->pspec;
  gboolean read_only = (pspec->any.flags & BSE_PARAM_HINT_RDONLY) != 0;

  GROUP_SET_SENSITIVE (group, !read_only);
  
  switch (pspec->type)
    {
      GtkWidget *action, *prompt, *pre_action, *any;
      gchar *string;
      
    case BSE_TYPE_PARAM_BOOL:
      action = GROUP_GET_ACTION (group);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (action), param->value.v_bool);
      break;
    case BSE_TYPE_PARAM_INT:
    case BSE_TYPE_PARAM_UINT:
    case BSE_TYPE_PARAM_FLOAT:
    case BSE_TYPE_PARAM_DOUBLE:
    case BSE_TYPE_PARAM_TIME:
    case BSE_TYPE_PARAM_NOTE:
      action = GROUP_GET_ACTION (group);
      string = NULL; /* eek, cure stupid compiler */
      switch (pspec->type)
	{
	case BSE_TYPE_PARAM_INT:    string = g_strdup_printf ("%d", param->value.v_int);    break;
	case BSE_TYPE_PARAM_UINT:   string = g_strdup_printf ("%u", param->value.v_uint);   break;
	case BSE_TYPE_PARAM_FLOAT:  string = g_strdup_printf ("%f", param->value.v_float);  break;
	case BSE_TYPE_PARAM_DOUBLE: string = g_strdup_printf ("%f", param->value.v_double); break;
	case BSE_TYPE_PARAM_TIME:   string = bse_time_to_str (param->value.v_time);         break;
	case BSE_TYPE_PARAM_NOTE:   string = bse_note_to_string (param->value.v_note);      break;
	}
      if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (action)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (action), string);
#if 0
	  GtkRequisition requisition;
	  
	  gtk_widget_size_request (action, &requisition);
	  if (MAX (requisition.width, action->requisition.width) > action->allocation.width)
	    gtk_widget_set_usize (action,
				  MAX (requisition.width, action->requisition.width),
				  -1);
#endif
	  // gtk_entry_set_position (GTK_ENTRY (action), 0);
	  if (GTK_IS_SPIN_BUTTON (action))
	    gtk_spin_button_update (GTK_SPIN_BUTTON (action));
	}
      g_free (string);
      break;
    case BSE_TYPE_PARAM_STRING:
      action = GROUP_GET_ACTION (group);
      string = param->value.v_string;
      if (!string || !g_str_equal (gtk_entry_get_text (GTK_ENTRY (action)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (action), string ? string : "");
	  // gtk_entry_set_position (GTK_ENTRY (action), 0);
	}
      pre_action = GROUP_GET_PRE_ACTION (group);
      if (pre_action)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pre_action), string != NULL);
      break;
    case BSE_TYPE_PARAM_DOTS:
      action = GROUP_GET_ACTION (group);
      gtk_widget_queue_draw (action);
      break;
    case BSE_TYPE_PARAM_ITEM:
      action = GROUP_GET_ACTION (group);
      string = param->value.v_item ? bse_item_make_nick_path (param->value.v_item) : NULL;
      if (!bse_string_equals (gtk_entry_get_text (GTK_ENTRY (action)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (action), string ? string : "");
	  // gtk_entry_set_position (GTK_ENTRY (action), 0);
	}
      any = gtk_object_get_user_data (GTK_OBJECT (action));
      if (any && 0) // FIXME: remove
	bst_param_update_clue_hunter (bparam);
      break;
    case BSE_TYPE_PARAM_ENUM:
      action = GROUP_GET_ACTION (group);
      any = gtk_option_menu_get_menu (GTK_OPTION_MENU (action));
      prompt = GROUP_GET_PROMPT (group);
      gtk_widget_set_sensitive (prompt, GTK_WIDGET_IS_SENSITIVE (prompt) && pspec->s_enum.enum_class->values);
      if (any)
	{
	  GList *list;
	  guint n = 0;
	  
	  for (list = GTK_MENU_SHELL (any)->children; list; list = list->next)
	    {
	      GtkWidget *item = list->data;
	      GtkEnumValue *ev = gtk_object_get_data_by_id (GTK_OBJECT (item), quark_evalues);

	      if (ev->value == param->value.v_enum)
		{
		  gtk_option_menu_set_history (GTK_OPTION_MENU (action), n);
		  break;
		}
	      n++;
	    }
	}
      break;
    case BSE_TYPE_PARAM_FLAGS:
    default:
      g_warning ("unknown param type: `%s'", pspec->any.name);
      break;
    }
}

static gboolean
bst_param_apply (BstParam *bparam,
		 gboolean *changed)
{
  GtkWidget *group = bparam->group;
  BseParam *param = &bparam->param;
  BseParamSpec *pspec = param->pspec;
  BseParam param2 = { 0, };
  gchar *dummy = NULL;
  guint dirty = 0;

  bse_param_init (&param2, pspec);
  bse_param_copy_value (param, &param2);

  *changed = FALSE;

  switch (pspec->type)
    {
      GtkWidget *action, *pre_action, *any;
      gchar *string;
      BseTime time_data;
      guint base;
      gint note_data;

    case BSE_TYPE_PARAM_BOOL:
      action = GROUP_GET_ACTION (group);
      dirty += bse_param_set_bool (param, GTK_TOGGLE_BUTTON (action)->active);
      break;
    case BSE_TYPE_PARAM_INT:
      action = GROUP_GET_ACTION (group);
      string = gtk_entry_get_text (GTK_ENTRY (action));
      if (string && string[0] == '0')
	{
	  base = 8;
	  string++;
	  if (string[0] == 'x' || string[0] == 'X')
	    {
	      base = 16;
	      string++;
	    }
	}
      else
	base = 10;
      dirty += bse_param_set_int (param, strtol (string, &dummy, base));
      dirty += dummy != NULL && (*dummy != 0 || dummy == string);
      break;
    case BSE_TYPE_PARAM_UINT:
      action = GROUP_GET_ACTION (group);
      string = gtk_entry_get_text (GTK_ENTRY (action));
      if (string && string[0] == '0')
	{
	  base = 8;
	  string++;
	  if (string[0] == 'x' || string[0] == 'X')
	    {
	      base = 16;
	      string++;
	    }
	}
      else
	base = 10;
      dirty += bse_param_set_uint (param, strtol (string, &dummy, base));
      dirty += dummy != NULL && (*dummy != 0 || dummy == string);
      break;
    case BSE_TYPE_PARAM_FLOAT:
      action = GROUP_GET_ACTION (group);
      dirty += bse_param_set_float (param, g_strtod (gtk_entry_get_text (GTK_ENTRY (action)), &dummy));
      break;
    case BSE_TYPE_PARAM_DOUBLE:
      action = GROUP_GET_ACTION (group);
      dirty += bse_param_set_double (param, g_strtod (gtk_entry_get_text (GTK_ENTRY (action)), &dummy));
      break;
    case BSE_TYPE_PARAM_TIME:
      action = GROUP_GET_ACTION (group);
      time_data = bse_time_from_string (gtk_entry_get_text (GTK_ENTRY (action)), NULL);
      if (time_data)
	dirty += bse_param_set_time (param, time_data);
      else
	dirty++;
      break;
    case BSE_TYPE_PARAM_NOTE:
      action = GROUP_GET_ACTION (group);
      note_data = bse_note_from_string (gtk_entry_get_text (GTK_ENTRY (action)));
      if (note_data != BSE_NOTE_UNPARSABLE)
	dirty += bse_param_set_note (param, note_data);
      else
	dirty++;
      break;
    case BSE_TYPE_PARAM_STRING:
      action = GROUP_GET_ACTION (group);
      pre_action = GROUP_GET_PRE_ACTION (group);
      if (!pre_action)
	string = gtk_entry_get_text (GTK_ENTRY (action));
      else if (GTK_TOGGLE_BUTTON (pre_action)->active)
	{
	  string = gtk_entry_get_text (GTK_ENTRY (action));
	  if (!string)
	    string = "";
	}
      else
	string = NULL;
      dirty += bse_param_set_string (param, string);
      break;
    case BSE_TYPE_PARAM_DOTS:
      *changed = TRUE;
      break;
    case BSE_TYPE_PARAM_ITEM:
      action = GROUP_GET_ACTION (group);
      string = bse_strdup_stripped (gtk_entry_get_text (GTK_ENTRY (action)));
      if (string && bparam->is_object && BSE_IS_ITEM (bparam->owner))
	{
	  BseProject *project = bse_item_get_project (BSE_ITEM (bparam->owner));
	  BseItem *item = NULL;

	  /* check whether this is a nick path */
	  if (!item && strchr (string, '.'))
	    {
	      item = bse_project_item_from_nick_path (project, string);

	      if (item && !bse_type_is_a (BSE_OBJECT_TYPE (item), pspec->s_item.item_type))
		item = NULL;
	    }
	  else if (!item) /* try generic lookup for pure name (brute force actually) */
	    {
	      GList *list, *free_list = bse_objects_list_by_name (pspec->s_item.item_type, string);
	      
	      for (list = free_list; list; list = list->next)
		if (bse_item_get_project (list->data) == project)
		  {
		    item = BSE_ITEM (list->data);
		    break;
		  }
	      g_list_free (free_list);
	    }

	  /* check whether this refers to an on-disk sample that we can demand load */
	  if (!item && pspec->s_item.item_type == BSE_TYPE_SAMPLE)
	    {
	      BstSampleLoc *loc = bst_sample_repo_find_sample_loc (string);
	      
	      if (loc)
		{
		  g_message ("demand loading: %s (%s)", loc->name, loc->repo->name);
		  
		  item = (BseItem*) bst_sample_repo_load_sample (loc, project);
		}
	    }

	  /* ok, found one or giving up */
	  dirty += bse_param_set_item (param, item);
	  g_free (string);

	  /* enforce redisplay of the entry's string with the correct name */
	  dirty += 1;
	}
      else
	dirty += bse_param_set_item (param, NULL);
      break;
    case BSE_TYPE_PARAM_ENUM:
      action = GROUP_GET_ACTION (group);
      any = GTK_OPTION_MENU (action)->menu_item;
      if (any)
	{
	  GtkEnumValue *ev = gtk_object_get_data_by_id (GTK_OBJECT (any), quark_evalues);

	  dirty += bse_param_set_enum (param, ev->value);
	}
      break;
    case BSE_TYPE_PARAM_FLAGS:
    default:
      g_warning ("unknown param type: `%s'", pspec->any.name);
      break;
    }

  *changed |= bse_param_values_cmp (param, &param2) != 0;
  bse_param_free_value (&param2);

  dirty += *changed;

  return dirty > 0;
}

void
bst_param_get (BstParam *bparam)
{
  g_return_if_fail (BSE_IS_PARAM (bparam));

  bparam->locked++;
  if (bparam->is_object && bparam->owner)
    bse_object_get_param (bparam->owner, &bparam->param);
  else
    {
      /* bse_param_reset_value (&bparam->param); */
    }
  bst_param_update (bparam);
  bparam->locked = 0;
}

void
bst_param_set (BstParam *bparam)
{
  gboolean dirty, changed;

  g_return_if_fail (BSE_IS_PARAM (bparam));

  bparam->locked++;
  dirty = bst_param_apply (bparam, &changed);
  if (changed && bparam->is_object && bparam->owner)
    bse_object_set_param (bparam->owner, &bparam->param);

  if (dirty)
    bst_param_get (bparam);
  else
    bparam->locked = 0;
}

gboolean
bst_param_set_from_other (BstParam *bparam,
			  BseParam *src_param)
{
  gboolean success;

  g_return_val_if_fail (BSE_IS_PARAM (bparam), FALSE);
  g_return_val_if_fail (BSE_IS_PARAM (src_param), FALSE);

  success = bse_param_value_convert (src_param, &bparam->param);

  if (success)
    {
      if (bparam->is_object && bparam->owner)
	bse_object_set_param (bparam->owner, &bparam->param);
      bst_param_get (bparam);
    }
  else
    bparam->locked = 0;

  return success;
}

void
bst_param_set_editable (BstParam *bparam,
			gboolean  editable)
{
  g_return_if_fail (BSE_IS_PARAM (bparam));

  if (bparam->group)
    {
      GtkWidget *prompt = GROUP_GET_PROMPT (bparam->group);
      GtkWidget *action = GROUP_GET_ACTION (bparam->group);
      GtkWidget *pre_action = GROUP_GET_PRE_ACTION (bparam->group);
      GtkWidget *post_action = GROUP_GET_POST_ACTION (bparam->group);
      
      if (GTK_IS_WIDGET (prompt))
	gtk_widget_set_sensitive (prompt, editable);
      /* don't interfere with bst_string_toggle() */
      gtk_widget_set_sensitive (action, editable);
      if (GTK_IS_WIDGET (pre_action))
	gtk_widget_set_sensitive (pre_action, editable);
      if (GTK_IS_WIDGET (post_action))
	gtk_widget_set_sensitive (post_action, editable);
    }
}

static gint
dots_area_configure_event (GtkWidget         *widget,
			   GdkEventConfigure *event,
			   BstParam          *bparam)
{
  gdk_window_set_background (widget->window, &widget->style->base[GTK_WIDGET_STATE (widget)]);

  return TRUE;
}

static gint
dots_area_cross_event (GtkWidget        *widget,
		       GdkEventCrossing *event,
		       BstParam         *bparam)
{
  DotAreaData *data = GTK_DRAWING_AREA (widget)->draw_data;

  if (event->type == GDK_ENTER_NOTIFY)
    data->entered = TRUE;
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (data->entered)
	data->entered = FALSE;
      else
	data->cdot = -1;
    }
  
  gtk_widget_queue_draw (widget);

  return TRUE;
}

static gint
dots_area_expose_event (GtkWidget      *widget,
			GdkEventExpose *event,
			BstParam       *bparam)
{
  guint n_dots = bparam->param.pspec->s_dots.n_dots;
  BseDot *dots = bparam->param.value.v_dots;
  GdkDrawable *drawable = widget->window;
  GdkGC *fg_gc = widget->style->black_gc;
  GdkGC *bg_gc = widget->style->base_gc[GTK_WIDGET_STATE (widget)];
  GdkGC *hl_gc = widget->style->bg_gc[GTK_STATE_SELECTED];
  DotAreaData *data = GTK_DRAWING_AREA (widget)->draw_data;
  gint width, height, maxx, maxy;
  guint i;

  gdk_window_get_size (widget->window, &width, &height);
  maxx = width - 1;
  maxy = height - 1;

  /* clear background
   */
  gdk_draw_rectangle (drawable, bg_gc,
		      TRUE,
		      0,
		      0,
		      width,
		      height);

  /* draw lines
   */
  for (i = 0; i < n_dots - 1; i++)
    gdk_draw_line (drawable, fg_gc,
		   maxx * dots[i].x,
		   maxy * (1.0 - dots[i].y),
		   maxx * dots[i + 1].x,
		   maxy * (1.0 - dots[i + 1].y));
  

  /* draw circles
   */
  if (data->entered || data->cdot >= 0)
    for (i = 0; i < n_dots; i++)
      gdk_draw_arc (drawable, hl_gc, FALSE,
		    maxx * dots[i].x - BST_TAG_DIAMETER / 2,
		    maxy * (1.0 - dots[i].y) - BST_TAG_DIAMETER / 2,
		    BST_TAG_DIAMETER, BST_TAG_DIAMETER,
		    0 * 64, 360 *64);
      
  return TRUE;
}

static gint
dots_area_button_event (GtkWidget      *widget,
			GdkEventButton *event,
			BstParam       *bparam)
{
  guint n_dots = bparam->param.pspec->s_dots.n_dots;
  BseDot *dots = bparam->param.value.v_dots;
  gint maxx, maxy;
  DotAreaData *data = GTK_DRAWING_AREA (widget)->draw_data;

  if (bparam->locked)
    return TRUE;

  data->cdot = -1;

  gdk_window_get_size (widget->window, &maxx, &maxy);
  maxx -= 1; maxy -= 1;

  if (event->button == 1 &&
      event->type == GDK_BUTTON_PRESS)
    {
      guint i;
      gfloat min = BST_TAG_DIAMETER / 2 + 1;
      
      for (i = 0; i < n_dots; i++)
	{
	  gfloat dx = event->x - maxx * dots[i].x;
	  gfloat dy = event->y - maxy * (1.0 - dots[i].y);
	  gfloat dist;
	  
	  dist = sqrt (dx * dx + dy * dy);
	  if (dist < min || (dist == min && i < n_dots / 2))
	    {
	      min = dist;
	      data->cdot = i;
	    }
	}

      if (data->cdot >= 0)
	{
	  gfloat y = event->y ? event->y / maxy : 0;
	  gfloat x = event->x ? event->x / maxx : 0;
	  
	  bse_param_set_dot (&bparam->param, data->cdot,
			     x,
			     1.0 - y);

	  bst_param_set (bparam);
	}
    }

  return TRUE;
}

static gint
dots_area_motion_event (GtkWidget      *widget,
			GdkEventMotion *event,
			BstParam       *bparam)
{
  gint maxx, maxy;
  DotAreaData *data = GTK_DRAWING_AREA (widget)->draw_data;

  gdk_window_get_size (widget->window, &maxx, &maxy);
  maxx -= 1; maxy -= 1;

  if (bparam->locked)
    return TRUE;

  if (data->cdot >= 0 &&
      event->type == GDK_MOTION_NOTIFY &&
      !event->is_hint)
    {
      gfloat y = event->y ? event->y / maxy : 0;
      gfloat x = event->x ? event->x / maxx : 0;
      
      bse_param_set_dot (&bparam->param, data->cdot,
			 x,
			 1.0 - y);
      
      bst_param_set (bparam);
    }
  
  return TRUE;
}
