/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsescriptcontrol.h"

#include "bsemain.h"
#include "bsecomwire.h"
#include "bseserver.h"
#include "bsemarshal.h"
#include "bsecontainer.h"
#include "bseprocedure.h"

enum
{
  PROP_0,
  PROP_USER_MSG_TYPE,
  PROP_USER_MSG,
  PROP_RUNNING,
  PROP_IDENT,
};

typedef struct {
  GSource           source;
  BseScriptControl *sctrl;
  BseComWire       *wire;
  guint             n_pfds;
  GPollFD          *pfds;
} WSource;


/* --- prototypes --- */
static void	bse_script_control_class_init	(BseScriptControlClass	*class);
static void	bse_script_control_init		(BseScriptControl	*script_control);
static void	bse_script_control_finalize	(GObject	        *object);
static void     bse_script_control_set_property (GObject		*script_control,
						 guint          	 param_id,
						 const GValue         	*value,
						 GParamSpec     	*pspec);
static void     bse_script_control_get_property	(GObject	     	*script_control,
						 guint          	 param_id,
						 GValue         	*value,
						 GParamSpec     	*pspec);
static void     bse_script_control_set_parent   (BseItem                *item,
						 BseItem                *parent);
static void	script_control_add_wsource	(BseScriptControl	*self);
static gboolean	script_control_kill_wire	(gpointer		 data);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static GSList     *sctrl_stack = NULL;
static guint       signal_action = 0;
static guint       signal_action_changed = 0;
static guint       signal_killed = 0;
static guint       signal_progress = 0;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseScriptControl)
{
  static const GTypeInfo script_control_info = {
    sizeof (BseScriptControlClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_script_control_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_control */,
    
    sizeof (BseScriptControl),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_script_control_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseScriptControl",
				   "BSE scrip control interface",
				   &script_control_info);
}

static void
bse_script_control_class_init (BseScriptControlClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_script_control_set_property;
  gobject_class->get_property = bse_script_control_get_property;
  gobject_class->finalize = bse_script_control_finalize;

  item_class->set_parent = bse_script_control_set_parent;

  bse_object_class_add_param (object_class, NULL,
			      PROP_USER_MSG_TYPE,
			      g_param_spec_enum ("user-msg-type", "User Message Type", NULL,
						 BSE_TYPE_USER_MSG_TYPE, BSE_USER_MSG_INFO,
						 BSE_PARAM_GUI));
  bse_object_class_add_param (object_class, NULL,
			      PROP_USER_MSG,
			      g_param_spec_string ("user-msg", "User Message", NULL,
						   NULL,
						   BSE_PARAM_GUI));
  bse_object_class_add_param (object_class, NULL,
			      PROP_RUNNING,
			      g_param_spec_boolean ("running", "Running", NULL,
						    FALSE, G_PARAM_READABLE));
  bse_object_class_add_param (object_class, NULL,
			      PROP_IDENT,
			      g_param_spec_string ("ident", "Script Identifier", NULL,
						   NULL,
						   BSE_PARAM_GUI));

  signal_progress = bse_object_class_add_signal (object_class, "progress",
						 bse_marshal_VOID__FLOAT, NULL,
						 G_TYPE_NONE, 1, G_TYPE_FLOAT);
  signal_action_changed = bse_object_class_add_dsignal (object_class, "action-changed",
							bse_marshal_VOID__STRING_UINT, NULL,
							G_TYPE_NONE, 2,
							G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE, G_TYPE_UINT);
  signal_action = bse_object_class_add_dsignal (object_class, "action",
						bse_marshal_VOID__STRING_UINT, NULL,
						G_TYPE_NONE, 2,
						G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE, G_TYPE_UINT);
  signal_killed = bse_object_class_add_signal (object_class, "killed",
					       bse_marshal_VOID__NONE, NULL,
					       G_TYPE_NONE, 0);
}

static void
bse_script_control_init (BseScriptControl *self)
{
  self->user_msg_type = BSE_USER_MSG_INFO;
  self->user_msg = NULL;
  self->wire = NULL;
  self->script_name = NULL;
  self->proc_name = NULL;
  self->source = NULL;
  self->file_name = NULL;
  self->actions = NULL;
}

static void
bse_script_control_set_property (GObject      *object,
				 guint         param_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  BseScriptControl *self = BSE_SCRIPT_CONTROL (object);

  switch (param_id)
    {
    case PROP_USER_MSG_TYPE:
      self->user_msg_type = g_value_get_enum (value);
      break;
    case PROP_USER_MSG:
      g_free (self->user_msg);
      self->user_msg = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_script_control_get_property (GObject     *object,
				 guint        param_id,
				 GValue      *value,
				 GParamSpec  *pspec)
{
  BseScriptControl *self = BSE_SCRIPT_CONTROL (object);

  switch (param_id)
    {
    case PROP_USER_MSG_TYPE:
      g_value_set_enum (value, self->user_msg_type);
      break;
    case PROP_USER_MSG:
      g_value_set_string (value, self->user_msg);
      break;
    case PROP_RUNNING:
      g_value_set_boolean (value, self->wire && self->wire->connected);
      break;
    case PROP_IDENT:
      g_value_set_string (value, bse_script_control_get_ident (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_script_control_finalize (GObject *object)
{
  BseScriptControl *self = BSE_SCRIPT_CONTROL (object);

  g_return_if_fail (self->wire == NULL);
  g_return_if_fail (self->source == NULL);

  while (self->actions)
    {
      BseScriptControlAction *a = self->actions->data;
      bse_script_control_remove_action (self, g_quark_to_string (a->action));
    }

  g_free (self->script_name);
  g_free (self->proc_name);
  g_free (self->file_name);
  g_free (self->user_msg);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

BseScriptControl*
bse_script_control_new (BseComWire  *wire,
			const gchar *script_name,
			const gchar *proc_name)
{
  BseScriptControl *self;
  
  g_return_val_if_fail (wire != NULL, NULL);
  g_return_val_if_fail (wire->owner == NULL, NULL);
  g_return_val_if_fail (wire->connected == TRUE, NULL);
  
  self = g_object_new (BSE_TYPE_SCRIPT_CONTROL, NULL);
  self->wire = wire;
  self->script_name = g_strdup (script_name);
  self->proc_name = g_strdup (proc_name);
  wire->owner = self;
  script_control_add_wsource (self);

  return self;
}

void
bse_script_control_set_file_name (BseScriptControl *self,
				  const gchar      *file_name)
{
  g_return_if_fail (BSE_IS_SCRIPT_CONTROL (self));

  g_free (self->file_name);
  self->file_name = g_strdup (file_name);
  if (!self->user_msg && file_name)
    {
      self->user_msg = g_strdup (file_name);
      g_object_notify (self, "user-msg");
    }
}

const gchar*
bse_script_control_get_file_name (BseScriptControl *self)
{
  g_return_val_if_fail (BSE_IS_SCRIPT_CONTROL (self), NULL);

  return self->file_name;
}

const gchar*
bse_script_control_get_ident (BseScriptControl *self)
{
  g_return_val_if_fail (BSE_IS_SCRIPT_CONTROL (self), NULL);

  return self->wire ? self->wire->ident : NULL;
}

/* bse_script_control_progress
 * @self:     script control object
 * @progress: progress value
 *
 * Signal progress, @progress is either a value between 0 and 1
 * to indicate completion status or is -1 to indicate progress
 * of unknown amount.
 */
void
bse_script_control_progress (BseScriptControl *self,
			     gfloat            progress)
{
  g_return_if_fail (BSE_IS_SCRIPT_CONTROL (self));

  if (progress < 0)
    progress = -1;
  else
    progress = CLAMP (progress, 0, 1.0);
  g_signal_emit (self, signal_progress, 0, progress);
}

static BseScriptControlAction*
find_action (BseScriptControl *self,
	     GQuark            aquark)
{
  GSList *slist;
  for (slist = self->actions; slist; slist = slist->next)
    {
      BseScriptControlAction *a = slist->data;
      if (a->action == aquark)
	return a;
    }
  return NULL;
}

void
bse_script_control_add_action (BseScriptControl *self,
			       const gchar      *action,
			       const gchar      *name,
			       const gchar      *blurb)
{
  BseScriptControlAction *a;

  g_return_if_fail (BSE_IS_SCRIPT_CONTROL (self));
  g_return_if_fail (action != NULL);
  g_return_if_fail (name != NULL);
  g_return_if_fail (!BSE_OBJECT_DISPOSED (self));

  a = find_action (self, g_quark_try_string (action));
  if (!a)
    {
      a = g_new0 (BseScriptControlAction, 1);
      a->action = g_quark_from_string (action);
      self->actions = g_slist_append (self->actions, a);
    }
  a->name = g_strdup (name);
  a->blurb = g_strdup (blurb);
  g_signal_emit (self, signal_action_changed, a->action, g_quark_to_string (a->action), g_slist_index (self->actions, a));
}

void
bse_script_control_remove_action (BseScriptControl *self,
				  const gchar      *action)
{
  BseScriptControlAction *a;

  g_return_if_fail (BSE_IS_SCRIPT_CONTROL (self));
  g_return_if_fail (action != NULL);

  a = find_action (self, g_quark_try_string (action));
  if (a)
    {
      GQuark aquark;

      self->actions = g_slist_remove (self->actions, a);
      aquark = a->action;
      g_free (a->name);
      g_free (a->blurb);
      g_free (a);
      if (!BSE_OBJECT_DISPOSED (self))
	g_signal_emit (self, signal_action_changed, aquark, g_quark_to_string (aquark), g_slist_length (self->actions));
    }
}

void
bse_script_control_trigger_action (BseScriptControl *self,
				   const gchar      *action)
{
  BseScriptControlAction *a;

  g_return_if_fail (BSE_IS_SCRIPT_CONTROL (self));
  g_return_if_fail (action != NULL);

  a = find_action (self, g_quark_try_string (action));
  if (a && !BSE_OBJECT_DISPOSED (self))
    g_signal_emit (self, signal_action, a->action, g_quark_to_string (a->action), g_slist_index (self->actions, a));
}

void
bse_script_control_push_current (BseScriptControl *self)
{
  g_return_if_fail (BSE_IS_SCRIPT_CONTROL (self));
  g_return_if_fail (self->wire);
  g_return_if_fail (g_slist_find (sctrl_stack, self) == NULL);

  sctrl_stack = g_slist_prepend (sctrl_stack, self);
}

BseScriptControl*
bse_script_control_peek_current (void)
{
  return sctrl_stack ? sctrl_stack->data : NULL;
}

void
bse_script_control_pop_current (void)
{
  BseScriptControl *self;

  g_return_if_fail (sctrl_stack != NULL);

  self = sctrl_stack->data;
  sctrl_stack = g_slist_remove (sctrl_stack, self);
}

static void
queue_kill (BseScriptControl *self)
{
  bse_com_wire_close_remote (self->wire, TRUE);
  self->wire = NULL;
  bse_idle_now (script_control_kill_wire, g_object_ref (self));
  g_signal_emit (self, signal_killed, 0);
  g_object_notify (self, "running");
}

void
bse_script_control_queue_kill (BseScriptControl *self)
{
  g_return_if_fail (BSE_IS_SCRIPT_CONTROL (self));
  g_return_if_fail (self->wire != NULL);

  if (BSE_ITEM (self)->parent)
    bse_container_remove_item (BSE_CONTAINER (BSE_ITEM (self)->parent), BSE_ITEM (self));
  else
    queue_kill (self);
}

static void
bse_script_control_set_parent (BseItem *item,
			       BseItem *parent)
{
  BseScriptControl *self = BSE_SCRIPT_CONTROL (item);

  if (!parent)	/* removal */
    {
      if (self->wire)
	queue_kill (self);
    }

  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);
}


/* --- main loop intergration --- */
static gboolean
script_wsource_prepare (GSource *source,
			gint    *timeout_p)
{
  WSource *wsource = (WSource*) source;
  BseComWire *wire = wsource->wire;
  gboolean need_dispatch, fds_changed = FALSE;
  GPollFD *pfds;
  guint n_pfds, i;

  if (!wire)
    return FALSE;
  BSE_THREADS_ENTER ();
  pfds = bse_com_wire_get_poll_fds (wire, &n_pfds);
  fds_changed |= n_pfds != wsource->n_pfds;
  for (i = 0; i < n_pfds && !fds_changed; i++)
    fds_changed |= wsource->pfds[i].fd != pfds[i].fd || wsource->pfds[i].events != pfds[i].events;
  if (fds_changed)
    {
      for (i = 0; i < wsource->n_pfds; i++)
	g_source_remove_poll (source, wsource->pfds + i);
      g_free (wsource->pfds);
      wsource->pfds = pfds;
      wsource->n_pfds = n_pfds;
      for (i = 0; i < wsource->n_pfds; i++)
	{
	  wsource->pfds[i].revents = 0;
	  g_source_add_poll (source, wsource->pfds + i);
	}
    }
  else
    g_free (pfds);
  need_dispatch = bse_com_wire_need_dispatch (wire);
  BSE_THREADS_LEAVE ();

  return need_dispatch;
}

static gboolean
script_wsource_check (GSource *source)
{
  WSource *wsource = (WSource*) source;
  BseComWire *wire = wsource->wire;
  gboolean need_dispatch;
  guint i;

  if (!wire)
    return FALSE;
  BSE_THREADS_ENTER ();
  need_dispatch = bse_com_wire_need_dispatch (wire);
  for (i = 0; i < wsource->n_pfds; i++)
    need_dispatch |= wsource->pfds[i].revents & wsource->pfds[i].events;
  BSE_THREADS_LEAVE ();

  return need_dispatch;
}

static gboolean
script_wsource_dispatch (GSource    *source,
			 GSourceFunc callback,
			 gpointer    user_data)
{
  WSource *wsource = (WSource*) source;
  BseComWire *wire = wsource->wire;
  guint request;

  if (!wire)
    return TRUE;        /* keep source alive */
  BSE_THREADS_ENTER ();
  bse_com_wire_process_io (wire);
  if (wire->gstring_stdout->len)
    {
      g_printerr ("%s:StdOut: %s", wire->ident, wire->gstring_stdout->str);
      g_string_truncate (wire->gstring_stdout, 0);
    }
  if (wire->gstring_stderr->len)
    {
      g_printerr ("%s:StdErr: %s", wire->ident, wire->gstring_stderr->str);
      g_string_truncate (wire->gstring_stderr, 0);
    }
  bse_com_wire_receive_dispatch (wire);
  request = bse_com_wire_peek_first_result (wire);
  if (request)
    {
      gchar *result = bse_com_wire_receive_result (wire, request);

      g_message ("ignoring iresult from \"%s\": %s\n", wire->ident, result);
      g_free (result);
    }
  if (!wire->connected && wsource->sctrl->wire)
    bse_script_control_queue_kill (wsource->sctrl);
  BSE_THREADS_LEAVE ();

  return TRUE;
}

static void
script_wsource_finalize (GSource *source)
{
  WSource *wsource = (WSource*) source;

  g_free (wsource->pfds);

  /* in this finalize handler, the BSE_THREADS_* mutex may or may not be
   * acquired, thus we destroy wires from an idle handler. because of that
   * the wire should already be gone at this point, so we may well check that.
   */
  g_return_if_fail (wsource->wire == NULL);
}

static void
script_control_add_wsource (BseScriptControl *self)
{
  static GSourceFuncs script_wsource_funcs = {
    script_wsource_prepare,
    script_wsource_check,
    script_wsource_dispatch,
    script_wsource_finalize,
  };
  GSource *source = g_source_new (&script_wsource_funcs, sizeof (WSource));
  WSource *wsource = (WSource*) source;

  g_return_if_fail (self->source == NULL);

  wsource->sctrl = self;
  wsource->wire = self->wire;
  wsource->n_pfds = 0;
  g_source_set_priority (source, BSE_PRIORITY_PROG_IFACE);
  g_source_attach (source, g_main_context_default ()); // bse_server_get ()->main_context);
  self->source = source;
}

static gboolean
script_control_kill_wire (gpointer data)
{
  BseScriptControl *self = data;
  WSource *wsource = (WSource*) self->source;

  g_return_val_if_fail (wsource != NULL, FALSE);

  BSE_THREADS_ENTER ();
  self->source = NULL;
  bse_com_wire_destroy (wsource->wire);
  wsource->wire = NULL;
  g_source_destroy (&wsource->source);
  g_object_unref (self);
  BSE_THREADS_LEAVE ();

  return FALSE;
}
