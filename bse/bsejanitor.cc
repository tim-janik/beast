/* BSE - Better Sound Engine
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bsejanitor.h"
#include "bsemain.h"
#include "bseglue.h"
#include "bseserver.h"
#include "bsecontainer.h"
#include "bseprocedure.h"
#include "bsescripthelper.h"


enum
{
  PROP_0,
  PROP_IDENT,
  PROP_CONNECTED,
  PROP_STATUS_MESSAGE,
  PROP_EXIT_CODE,
  PROP_EXIT_REASON,
};


/* --- prototypes --- */
static void	bse_janitor_class_init		(BseJanitorClass	*klass);
static void	bse_janitor_init		(BseJanitor		*janitor);
static void	bse_janitor_finalize		(GObject	        *object);
static void     bse_janitor_set_property	(GObject		*janitor,
						 uint          	         param_id,
						 const GValue         	*value,
						 GParamSpec     	*pspec);
static void     bse_janitor_get_property	(GObject	     	*janitor,
						 uint          	         param_id,
						 GValue         	*value,
						 GParamSpec     	*pspec);
static void     bse_janitor_set_parent		(BseItem                *item,
						 BseItem                *parent);
static void	janitor_install_jsource		(BseJanitor		*self);
static gboolean	janitor_idle_clean_jsource	(void                   *data);
static void	janitor_port_closed		(SfiComPort		*port,
						 void                   *close_data);
static GValue*	janitor_client_msg		(SfiGlueDecoder		*decoder,
						 void                   *user_data,
						 const char		*message,
						 const GValue		*value);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static GSList     *janitor_stack = NULL;
static uint        signal_action = 0;
static uint        signal_action_changed = 0;
static uint        signal_shutdown = 0;
static uint        signal_progress = 0;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseJanitor)
{
  static const GTypeInfo janitor_info = {
    sizeof (BseJanitorClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_janitor_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseJanitor),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_janitor_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseJanitor",
				   "BSE connection interface object",
                                   __FILE__, __LINE__,
                                   &janitor_info);
}

static void
bse_janitor_class_init (BseJanitorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  
  parent_class = (GTypeClass*) g_type_class_peek_parent (klass);
  
  gobject_class->set_property = bse_janitor_set_property;
  gobject_class->get_property = bse_janitor_get_property;
  gobject_class->finalize = bse_janitor_finalize;
  
  item_class->set_parent = bse_janitor_set_parent;
  
  bse_object_class_add_param (object_class, NULL, PROP_IDENT,
			      sfi_pspec_string ("ident", "Script Identifier", NULL, NULL, SFI_PARAM_GUI));
  bse_object_class_add_param (object_class, NULL, PROP_CONNECTED,
			      sfi_pspec_bool ("connected", "Connected", NULL, FALSE, "G:r"));
  bse_object_class_add_param (object_class, NULL, PROP_STATUS_MESSAGE,
			      sfi_pspec_string ("status-message", "Status Message", NULL, "", SFI_PARAM_GUI));
  bse_object_class_add_param (object_class, NULL, PROP_EXIT_CODE,
			      sfi_pspec_int ("exit-code", "Exit Code", NULL, 0, -256, 256, 0, "G:r"));
  bse_object_class_add_param (object_class, NULL, PROP_EXIT_REASON,
			      sfi_pspec_string ("exit-reason", "Exit Reason", NULL, NULL, "G:r"));
  
  signal_progress = bse_object_class_add_signal (object_class, "progress",
						 G_TYPE_NONE, 1, G_TYPE_FLOAT);
  signal_action_changed = bse_object_class_add_dsignal (object_class, "action-changed",
							G_TYPE_NONE, 2,
							G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE, G_TYPE_INT);
  signal_action = bse_object_class_add_dsignal (object_class, "action",
						G_TYPE_NONE, 2,
						G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE, G_TYPE_INT);
  signal_shutdown = bse_object_class_add_signal (object_class, "shutdown", G_TYPE_NONE, 0);
}

static void
bse_janitor_init (BseJanitor *self)
{
  self->port_closed = FALSE;
  self->force_kill = FALSE;
  self->force_normal_exit = FALSE;
  self->port = NULL;
  self->context = NULL;
  self->decoder = NULL;
  self->source = NULL;
  self->status_message = g_strdup ("");
  self->script_name = NULL;
  self->proc_name = NULL;
  self->actions = NULL;
  self->exit_code = 0;
  self->exit_reason = NULL;
}

static void
bse_janitor_set_property (GObject      *object,
			  uint          param_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  BseJanitor *self = BSE_JANITOR (object);
  
  switch (param_id)
    {
    case PROP_STATUS_MESSAGE:
      g_free (self->status_message);
      self->status_message = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_janitor_get_property (GObject    *object,
			  uint        param_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
  BseJanitor *self = BSE_JANITOR (object);
  
  switch (param_id)
    {
    case PROP_IDENT:
      sfi_value_set_string (value, bse_janitor_get_ident (self));
      break;
    case PROP_STATUS_MESSAGE:
      sfi_value_set_string (value, self->status_message);
      break;
    case PROP_CONNECTED:
      sfi_value_set_bool (value, self->port != NULL);
      break;
    case PROP_EXIT_CODE:
      sfi_value_set_int (value, self->exit_code);
      break;
    case PROP_EXIT_REASON:
      sfi_value_set_string (value, self->exit_reason);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_janitor_finalize (GObject *object)
{
  BseJanitor *self = BSE_JANITOR (object);
  
  g_return_if_fail (self->port == NULL);
  g_return_if_fail (self->source == NULL);
  
  while (self->actions)
    {
      BseJanitorAction *a = (BseJanitorAction*) self->actions->data;
      bse_janitor_remove_action (self, g_quark_to_string (a->action));
    }
  
  g_free (self->status_message);
  g_free (self->script_name);
  g_free (self->proc_name);
  g_free (self->exit_reason);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

BseJanitor*
bse_janitor_new (SfiComPort *port)
{
  BseJanitor *self;
  
  g_return_val_if_fail (port != NULL, NULL);
  
  self = (BseJanitor*) bse_container_new_child ((BseContainer*) bse_server_get (), BSE_TYPE_JANITOR, NULL);
  g_object_ref (self);

  /* store the port */
  self->port = sfi_com_port_ref (port);
  sfi_com_port_set_close_func (self->port, janitor_port_closed, self);
  /* create server-side glue context */
  self->context = bse_glue_context_intern (port->ident);
  /* create server-side decoder */
  self->decoder = sfi_glue_context_decoder (port, self->context);
  sfi_glue_decoder_add_handler (self->decoder, janitor_client_msg, self);
  /* main loop integration */
  janitor_install_jsource (self);
  
  return self;
}

void
bse_janitor_set_procedure (BseJanitor *self,
                           const char *script_name,
                           const char *proc_name)
{
  g_return_if_fail (BSE_IS_JANITOR (self));
  
  g_free (self->proc_name);
  self->proc_name = g_strdup (proc_name);
  g_free (self->script_name);
  self->script_name = g_strdup (script_name);
  g_object_notify (G_OBJECT (self), "status-message");
}

const char*
bse_janitor_get_ident (BseJanitor *self)
{
  g_return_val_if_fail (BSE_IS_JANITOR (self), NULL);
  
  return self->port ? self->port->ident : NULL;
}

/**
 * @param self     janitor object
 * @param progress progress value
 *
 * Signal progress, @a progress is either a value between 0 and 1
 * to indicate completion status or is -1 to indicate progress
 * of unknown amount.
 */
void
bse_janitor_progress (BseJanitor *self,
		      float       progress)
{
  g_return_if_fail (BSE_IS_JANITOR (self));
  
  if (progress < 0)
    progress = -1;
  else
    progress = CLAMP (progress, 0, 1.0);
  g_signal_emit (self, signal_progress, 0, progress);
}

static BseJanitorAction*
find_action (BseJanitor *self,
	     GQuark      aquark)
{
  GSList *slist;
  for (slist = self->actions; slist; slist = slist->next)
    {
      BseJanitorAction *a = (BseJanitorAction*) slist->data;
      if (a->action == aquark)
	return a;
    }
  return NULL;
}

void
bse_janitor_add_action (BseJanitor *self,
			const char *action,
			const char *name,
			const char *blurb)
{
  BseJanitorAction *a;
  
  g_return_if_fail (BSE_IS_JANITOR (self));
  g_return_if_fail (action != NULL);
  g_return_if_fail (name != NULL);
  g_return_if_fail (!BSE_OBJECT_DISPOSING (self));
  
  a = find_action (self, g_quark_try_string (action));
  if (!a)
    {
      a = g_new0 (BseJanitorAction, 1);
      a->action = g_quark_from_string (action);
      self->actions = g_slist_append (self->actions, a);
    }
  a->name = g_strdup (name);
  a->blurb = g_strdup (blurb);
  g_signal_emit (self, signal_action_changed, a->action, g_quark_to_string (a->action), g_slist_index (self->actions, a));
}

void
bse_janitor_remove_action (BseJanitor *self,
			   const char *action)
{
  BseJanitorAction *a;
  
  g_return_if_fail (BSE_IS_JANITOR (self));
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
      if (!BSE_OBJECT_DISPOSING (self))
	g_signal_emit (self, signal_action_changed, aquark, g_quark_to_string (aquark), g_slist_length (self->actions));
    }
}

void
bse_janitor_trigger_action (BseJanitor *self,
			    const char *action)
{
  BseJanitorAction *a;
  
  g_return_if_fail (BSE_IS_JANITOR (self));
  g_return_if_fail (action != NULL);
  
  a = find_action (self, g_quark_try_string (action));
  if (a && !BSE_OBJECT_DISPOSING (self))
    g_signal_emit (self, signal_action, a->action, g_quark_to_string (a->action), g_slist_index (self->actions, a));
}

BseJanitor*
bse_janitor_get_current (void)
{
  return janitor_stack ? (BseJanitor*) janitor_stack->data : NULL;
}

static void
janitor_shutdown (BseJanitor *self)
{
  float n_seconds = 1;
  self->port_closed = TRUE; /* protects further (recursive) janitor_shutdown() calls */
  sfi_com_port_close_remote (self->port, self->force_kill);
  if (sfi_com_port_test_reap_child (self->port))
    n_seconds = 0;
  bse_idle_timed (n_seconds * SFI_USEC_FACTOR, janitor_idle_clean_jsource, g_object_ref (self));
  g_signal_emit (self, signal_shutdown, 0);
}

void
bse_janitor_close (BseJanitor *self)
{
  g_return_if_fail (BSE_IS_JANITOR (self));
  if (self->port && !self->port_closed)
    janitor_shutdown (self);
}

void
bse_janitor_kill (BseJanitor *self)
{
  g_return_if_fail (BSE_IS_JANITOR (self));

  if (!self->port_closed)
    {
      self->force_kill = TRUE;
      bse_janitor_close (self);
    }
}

static void
bse_janitor_set_parent (BseItem *item,
			BseItem *parent)
{
  BseJanitor *self = BSE_JANITOR (item);
  
  if (!parent &&	/* removal */
      !self->port_closed)
    janitor_shutdown (self);

  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);
}

static GValue*
janitor_client_msg (SfiGlueDecoder *decoder,
		    void           *user_data,
		    const char     *message,
		    const GValue   *value)
{
  BseJanitor *self = BSE_JANITOR (user_data);
  GValue *rvalue;
  rvalue = bse_script_check_client_msg (decoder, self, message, value);
  if (rvalue)
    return rvalue;
  return NULL;
}


/* --- main loop intergration --- */
typedef struct {
  GSource     source;
  BseJanitor *janitor;
} JSource;

static gboolean
janitor_prepare (GSource *source,
		 int     *timeout_p)
{
  BseJanitor *self = ((JSource*) source)->janitor;
  return sfi_glue_decoder_pending (self->decoder);
}

static gboolean
janitor_check (GSource *source)
{
  BseJanitor *self = ((JSource*) source)->janitor;
  return sfi_glue_decoder_pending (self->decoder);
}

static gboolean
janitor_dispatch (GSource    *source,
		  GSourceFunc callback,
		  void       *user_data)
{
  BseJanitor *self = ((JSource*) source)->janitor;
  SfiComPort *port = self->port;

  if (!port)
    return TRUE;        /* keep source alive */

  janitor_stack = g_slist_prepend (janitor_stack, self);
  sfi_glue_decoder_dispatch (self->decoder);
  janitor_stack = g_slist_remove (janitor_stack, self);

#if 0
  if (port->gstring_stdout->len)
    {
      g_printerr ("%s:O: %s", port->ident, port->gstring_stdout->str);
      g_string_truncate (port->gstring_stdout, 0);
    }
  if (port->gstring_stderr->len)
    {
      g_printerr ("%s:E: %s", port->ident, port->gstring_stderr->str);
      g_string_truncate (port->gstring_stderr, 0);
    }
#endif
  if (!port->connected && !self->port_closed)
    bse_janitor_close (self);
  return TRUE;
}

static void
janitor_install_jsource (BseJanitor *self)
{
  static GSourceFuncs jsource_funcs = {
    janitor_prepare,
    janitor_check,
    janitor_dispatch,
  };
  GSource *source = g_source_new (&jsource_funcs, sizeof (JSource));
  JSource *jsource = (JSource*) source;
  SfiRing *ring;
  GPollFD *pfd;

  g_return_if_fail (self->source == NULL);

  jsource->janitor = self;
  self->source = source;
  g_source_set_priority (source, BSE_PRIORITY_GLUE);
  ring = sfi_glue_decoder_list_poll_fds (self->decoder);
  pfd = (GPollFD*) sfi_ring_pop_head (&ring);
  while (pfd)
    {
      g_source_add_poll (source, pfd);
      pfd = (GPollFD*) sfi_ring_pop_head (&ring);
    }
  g_source_attach (source, bse_main_context);
}

static gboolean
janitor_idle_clean_jsource (void *data)
{
  BseJanitor *self = BSE_JANITOR (data);
  SfiComPort *port = self->port;

  g_return_val_if_fail (self->source != NULL, FALSE);

  g_source_destroy (self->source);
  self->source = NULL;
  sfi_glue_decoder_destroy (self->decoder);
  self->decoder = NULL;
  sfi_glue_context_destroy (self->context);
  self->context = NULL;
  sfi_com_port_set_close_func (port, NULL, NULL);
  sfi_com_port_reap_child (port, TRUE);
  if (port->remote_pid)
    {
      self->exit_code = 256; /* exit code used for signals */
      if (port->exit_signal_sent && port->sigkill_sent)
        self->exit_reason = g_strdup_printf (_("killed by janitor"));
      else if (port->exit_signal_sent && port->sigterm_sent)
        self->exit_reason = g_strdup_printf (_("connection terminated"));
      else if (port->exit_signal && port->dumped_core)
        self->exit_reason = g_strdup_printf (_("%s (core dumped)"), g_strsignal (port->exit_signal));
      else if (port->exit_signal)
        self->exit_reason = g_strdup_printf ("%s", g_strsignal (port->exit_signal));
      else
        {
          self->exit_code = port->exit_code;
          if (port->exit_code || self->force_kill)
            self->exit_reason = g_strdup_printf ("Exit status (%d)", port->exit_code);
          else
            self->exit_reason = NULL; /* all OK */
        }
      if (self->force_normal_exit)
        {
          self->exit_code = 0;
          g_free (self->exit_reason);
          self->exit_reason = NULL;
        }
      if (self->exit_reason)
        sfi_diag ("%s: %s", port->ident, self->exit_reason);
    }
  else
    {
      /* not a janitor for a remote process */
      self->exit_code = -256;
      self->exit_reason = g_strdup_printf ("unknown intern termination");
    }
  sfi_com_port_unref (port);
  self->port = NULL;
  g_object_notify (G_OBJECT (self), "connected");
  if (BSE_ITEM (self)->parent)
    bse_container_remove_item (BSE_CONTAINER (BSE_ITEM (self)->parent), BSE_ITEM (self));
  g_object_unref (self);
  return FALSE;
}

static void
janitor_port_closed (SfiComPort *port,
		     void       *close_data)
{
  BseJanitor *self = BSE_JANITOR (close_data);
  /* this function is called by the SfiComPort */
  if (!self->port_closed)
    bse_janitor_close (self);
}
