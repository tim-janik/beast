/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bseserver.h"


#include "bseproject.h"
#include "gslengine.h"
#include "gslcommon.h"
#include "bsemarshal.h"
#include "bseglue.h"
#include "bsecomwire.h"
#include "bsemidinotifier.h"
#include "bsemain.h"		/* threads enter/leave */
#include "bsecomwire.h"
#include "bsemidireceiver.h"
#include "bsescriptcontrol.h"


/* --- PCM GslModule implementations ---*/
#include "bsepcmmodule.c"


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_PCM_LATENCY,
};


/* --- prototypes --- */
static void	bse_server_class_init		(BseServerClass	   *class);
static void	bse_server_init			(BseServer	   *server);
static void	bse_server_destroy		(BseObject	   *object);
static void	bse_server_set_property		(BseServer	   *server,
						 guint              param_id,
						 GValue            *value,
						 GParamSpec        *pspec);
static void	bse_server_get_property		(BseServer	   *server,
						 guint              param_id,
						 GValue            *value,
						 GParamSpec        *pspec);
static void	bse_server_set_parent		(BseItem	   *item,
						 BseItem	   *parent);
static void     bse_server_add_item             (BseContainer      *container,
						 BseItem           *item);
static void     bse_server_forall_items         (BseContainer      *container,
						 BseForallItemsFunc func,
						 gpointer           data);
static void     bse_server_remove_item          (BseContainer      *container,
						 BseItem           *item);
static gboolean	iowatch_remove			(BseServer	   *server,
						 BseIOWatch	    watch_func,
						 gpointer	    data);
static void	iowatch_add			(BseServer	   *server,
						 gint		    fd,
						 GIOCondition	    events,
						 BseIOWatch	    watch_func,
						 gpointer	    data);
static void	main_thread_source_setup	(BseServer	   *self,
						 gint               priority,
						 GslGlueContext    *context);
static void	engine_init			(BseServer	   *server,
						 gfloat		    mix_freq);
static void	engine_shutdown			(BseServer	   *server);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static guint       signal_user_message = 0;
static guint       signal_exec_status = 0;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseServer)
{
  static const GTypeInfo server_info = {
    sizeof (BseServerClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_server_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseServer),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_server_init,
  };
  
  return bse_type_register_static (BSE_TYPE_CONTAINER,
				   "BseServer",
				   "BSE Server type",
				   &server_info);
}

static void
bse_server_class_init (BseServerClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_server_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_server_get_property;

  object_class->destroy = bse_server_destroy;

  item_class->set_parent = bse_server_set_parent;

  container_class->add_item = bse_server_add_item;
  container_class->remove_item = bse_server_remove_item;
  container_class->forall_items = bse_server_forall_items;

  bse_object_class_add_param (object_class, "PCM Settings",
			      PARAM_PCM_LATENCY,
			      bse_param_spec_uint ("latency", "Latency [ms]", NULL,
						   1, 2000,
						   50, 5,
						   BSE_PARAM_DEFAULT | G_PARAM_CONSTRUCT));

  signal_user_message = bse_object_class_add_signal (object_class, "user-message",
						     bse_marshal_VOID__ENUM_STRING, NULL,
						     G_TYPE_NONE, 2,
						     BSE_TYPE_USER_MSG_TYPE,
						     G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE);
  signal_exec_status = bse_object_class_add_signal (object_class, "exec-status",
						    bse_marshal_VOID__ENUM_STRING_FLOAT_ENUM_OBJECT,
						    bse_marshal_VOID__ENUM_STRING_FLOAT_ENUM_POINTER,
						    G_TYPE_NONE, 5,
						    BSE_TYPE_EXEC_STATUS,
						    G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
						    G_TYPE_FLOAT, BSE_TYPE_ERROR_TYPE, BSE_TYPE_SCRIPT_CONTROL);
}

static void
bse_server_init (BseServer *server)
{
  server->engine_source = NULL;
  server->projects = NULL;
  server->dev_use_count = 0;
  server->pcm_device = NULL;
  server->pcm_imodule = NULL;
  server->pcm_omodule = NULL;
  server->pcm_ref_count = 0;
  server->midi_device = NULL;
  server->main_context = g_main_context_default ();
  g_main_context_ref (server->main_context);
  BSE_OBJECT_SET_FLAGS (server, BSE_ITEM_FLAG_SINGLETON);

  /* start dispatching main thread stuff */
  main_thread_source_setup (server, BSE_NOTIFY_PRIORITY, bse_glue_context ());
}

static void
bse_server_destroy (BseObject *object)
{
  // BseServer *server = BSE_SERVER (object);

  g_error ("BseServer got unreferenced, though persistent-singleton");
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}


static void
bse_server_set_property (BseServer  *server,
			 guint       param_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
  switch (param_id)
    {
      BsePcmHandle *handle;
    case PARAM_PCM_LATENCY:
      server->pcm_latency = g_value_get_uint (value);
      handle = server->pcm_device ? bse_pcm_device_get_handle (server->pcm_device) : NULL;
      if (handle)
	bse_pcm_handle_set_watermark (handle, server->pcm_latency);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (server, param_id, pspec);
      break;
    }
}

static void
bse_server_get_property (BseServer  *server,
			 guint       param_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
  switch (param_id)
    {
    case PARAM_PCM_LATENCY:
      g_value_set_uint (value, server->pcm_latency);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (server, param_id, pspec);
      break;
    }
}

static void
bse_server_set_parent (BseItem *item,
		       BseItem *parent)
{
  g_warning ("%s: BseServer is a global singleton that cannot be added to a container", G_STRLOC);
}

static void
bse_server_add_item (BseContainer *container,
		     BseItem      *item)
{
  BseServer *self = BSE_SERVER (container);

  self->children = g_slist_prepend (self->children, item);

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_server_forall_items (BseContainer      *container,
			 BseForallItemsFunc func,
			 gpointer           data)
{
  BseServer *self = BSE_SERVER (container);
  GSList *slist = self->children;

  while (slist)
    {
      BseItem *item = slist->data;

      slist = slist->next;
      if (!func (item, data))
	return;
    }
}

static void
bse_server_remove_item (BseContainer *container,
			BseItem      *item)
{
  BseServer *self = BSE_SERVER (container);
  
  self->children = g_slist_remove (self->children, item);

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

/**
 * bse_server_get
 * @Returns: Global BSE Server
 *
 * Retrive the global BSE server object.
 **/
BseServer*
bse_server_get (void)
{
  static BseServer *server = NULL;

  if (!server)
    {
      server = bse_object_new (BSE_TYPE_SERVER, NULL);
      g_object_ref (server);
    }

  return server;
}

static void
destroy_project (BseProject *project,
		 BseServer  *server)
{
  server->projects = g_list_remove (server->projects, project);
}

BseProject*
bse_server_create_project (BseServer   *server,
			   const gchar *name)
{
  BseProject *project;

  g_return_val_if_fail (BSE_IS_SERVER (server), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (bse_server_find_project (server, name) == NULL, NULL);

  project = g_object_new (BSE_TYPE_PROJECT,
			  "uname", name,
			  NULL);
  server->projects = g_list_prepend (server->projects, project);
  g_object_connect (project,
		    "signal::destroy", destroy_project, server,
		    NULL);

  return project;
}

BseProject*
bse_server_find_project (BseServer   *server,
			 const gchar *name)
{
  GList *node;

  g_return_val_if_fail (BSE_IS_SERVER (server), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (node = server->projects; node; node = node->next)
    {
      BseProject *project = node->data;
      gchar *uname = BSE_OBJECT_UNAME (project);

      if (uname && strcmp (name, uname) == 0)
	return project;
    }
  return NULL;
}

void
bse_server_pick_default_devices (BseServer *server)
{
  GType *children, choice = 0;
  guint n, i, rating;

  g_return_if_fail (BSE_IS_SERVER (server));
  g_return_if_fail (server->pcm_device == NULL);
  g_return_if_fail (server->midi_device == NULL);

  /* pcm device driver implementations all derive from BsePcmDevice */
  children = g_type_children (BSE_TYPE_PCM_DEVICE, &n);
  /* pick class with highest rating */
  rating = 0;
  for (i = 0; i < n; i++)
    {
      BsePcmDeviceClass *class = g_type_class_ref (children[i]);

      if (class->driver_rating > rating)
	{
	  rating = class->driver_rating;
	  choice = children[i];
	}
      g_type_class_unref (class);
    }
  g_free (children);
  if (rating)
    server->pcm_device = g_object_new (choice, NULL);

  /* midi device driver implementations all derive from BseMidiDevice */
  children = g_type_children (BSE_TYPE_MIDI_DEVICE, &n);
  /* pick class with highest rating */
  rating = 0;
  for (i = 0; i < n; i++)
    {
      BseMidiDeviceClass *class = g_type_class_ref (children[i]);

      if (class->driver_rating > rating)
	{
	  rating = class->driver_rating;
	  choice = children[i];
	}
      g_type_class_unref (class);
    }
  g_free (children);
  if (rating)
    {
      server->midi_device = g_object_new (choice,
					  "midi_receiver", bse_server_get_midi_receiver (server, "default"),
					  NULL);
    }
}

BseErrorType
bse_server_activate_devices (BseServer *server)
{
  BseErrorType error = BSE_ERROR_NONE;

  g_return_val_if_fail (BSE_IS_SERVER (server), BSE_ERROR_INTERNAL);

  if (!server->pcm_device || !server->midi_device)
    bse_server_pick_default_devices (server);
  if (!server->pcm_device || !server->midi_device)
    return BSE_ERROR_INTERNAL;	/* shouldn't happen */
  
  if (!error)
    error = bse_pcm_device_open (server->pcm_device);
  if (!error)
    {
      error = bse_midi_device_open (server->midi_device);
      if (error)
	bse_pcm_device_suspend (server->pcm_device);
    }
  if (!error)
    {
      GslTrans *trans;

      bse_pcm_handle_set_watermark (bse_pcm_device_get_handle (server->pcm_device),
				    server->pcm_latency);
      engine_init (server, bse_pcm_device_get_handle (server->pcm_device)->mix_freq);

      trans = gsl_trans_open ();
      server->pcm_imodule = bse_pcm_imodule_insert (bse_pcm_device_get_handle (server->pcm_device), trans);
      server->pcm_omodule = bse_pcm_omodule_insert (bse_pcm_device_get_handle (server->pcm_device), trans);
      gsl_trans_commit (trans);
    }
  
  return error;
}

void
bse_server_suspend_devices (BseServer *server)
{
  GslTrans *trans;

  g_return_if_fail (BSE_IS_SERVER (server));
  g_return_if_fail (server->pcm_ref_count == 0);

  trans = gsl_trans_open ();
  if (server->pcm_omodule)
    {
      bse_pcm_imodule_remove (server->pcm_imodule, trans);
      server->pcm_imodule = NULL;
      bse_pcm_omodule_remove (server->pcm_omodule, trans);
      server->pcm_omodule = NULL;
    }
  /* we don't need to discard the midi_receiver */
  // FIXME: discard midi_receiver modules
  gsl_trans_commit (trans);
  
  /* wait until transaction has been processed */
  gsl_engine_wait_on_trans ();
  
  bse_pcm_device_suspend (server->pcm_device);
  bse_midi_device_suspend (server->midi_device);

  engine_shutdown (server);
}

GslModule*
bse_server_retrive_pcm_output_module (BseServer   *server,
				      BseSource   *source,
				      const gchar *uplink_name)
{
  g_return_val_if_fail (BSE_IS_SERVER (server), NULL);
  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);
  g_return_val_if_fail (uplink_name != NULL, NULL);
  g_return_val_if_fail (server->pcm_omodule != NULL, NULL); // FIXME server->pcm_devices_open

  server->pcm_ref_count += 1;

  return server->pcm_omodule;
}

void
bse_server_discard_pcm_output_module (BseServer *server,
				      GslModule *module)
{
  g_return_if_fail (BSE_IS_SERVER (server));
  g_return_if_fail (module != NULL);
  g_return_if_fail (server->pcm_ref_count > 0);

  g_return_if_fail (server->pcm_omodule == module); // FIXME

  server->pcm_ref_count -= 1;
}

GslModule*
bse_server_retrive_pcm_input_module (BseServer   *server,
				     BseSource   *source,
				     const gchar *uplink_name)
{
  g_return_val_if_fail (BSE_IS_SERVER (server), NULL);
  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);
  g_return_val_if_fail (uplink_name != NULL, NULL);
  g_return_val_if_fail (server->pcm_imodule != NULL, NULL); // FIXME server->pcm_devices_open

  server->pcm_ref_count += 1;

  return server->pcm_imodule;
}

void
bse_server_discard_pcm_input_module (BseServer *server,
				     GslModule *module)
{
  g_return_if_fail (BSE_IS_SERVER (server));
  g_return_if_fail (module != NULL);
  g_return_if_fail (server->pcm_ref_count > 0);

  g_return_if_fail (server->pcm_imodule == module); // FIXME

  server->pcm_ref_count -= 1;
}

BseMidiReceiver*
bse_server_get_midi_receiver (BseServer   *self,
			      const gchar *midi_name)
{
  g_return_val_if_fail (BSE_IS_SERVER (self), NULL);
  g_return_val_if_fail (midi_name != NULL, NULL);

  if (!self->midi_receiver)
    self->midi_receiver = bse_midi_receiver_new ("default");
  // FIXME: we don't actually check the midi_receiver name

  return self->midi_receiver;
}

/* bse_server_exec_status
 * @status:    execution status (start, progress or done)
 * @exec_name: name of procedure or script
 * @progress:  progress value for status=BSE_EXEC_STATUS_PROGRESS
 * error:      error condition for BSE_EXEC_STATUS_DONE
 *
 * Signal procedure or script execution status, The @progress value
 * is meaningfull for @status=BSE_EXEC_STATUS_PROGRESS, and contains
 * eitehr a value between 0 and 1 to indicate completion status, or
 * is -1 to signal progress of unknown amount.
 */
void
bse_server_exec_status (BseServer    *server,
			BseExecStatus status,
			const gchar  *exec_name,
			gfloat        progress,
			BseErrorType  error)
{
  BseScriptControl *sctrl;

  g_return_if_fail (BSE_IS_SERVER (server));
  g_return_if_fail (exec_name != NULL);

  progress = CLAMP (progress, -1, +1);
  sctrl = bse_script_control_peek_current ();
  if (sctrl && !BSE_ITEM (sctrl)->parent)
    sctrl = NULL;
  g_signal_emit (server, signal_exec_status, 0,
		 status, exec_name, progress, error,
		 sctrl);
}

void
bse_server_user_message (BseServer     *server,
			 BseUserMsgType msg_type,
			 const gchar   *message)
{
  g_return_if_fail (BSE_IS_SERVER (server));
  g_return_if_fail (message != NULL);

  g_signal_emit (server, signal_user_message, 0,
		 msg_type, message);
}

void
bse_server_add_io_watch (BseServer      *server,
			 gint            fd,
			 GIOCondition    events,
			 BseIOWatch      watch_func,
			 gpointer        data)
{
  g_return_if_fail (BSE_IS_SERVER (server));
  g_return_if_fail (watch_func != NULL);
  g_return_if_fail (fd >= 0);

  iowatch_add (server, fd, events, watch_func, data);
}

void
bse_server_remove_io_watch (BseServer *server,
			    BseIOWatch watch_func,
			    gpointer   data)
{
  g_return_if_fail (BSE_IS_SERVER (server));
  g_return_if_fail (watch_func != NULL);

  if (!iowatch_remove (server, watch_func, data))
    g_warning (G_STRLOC ": no such io watch installed %p(%p)", watch_func, data);
}

gchar*
bse_server_run_remote (BseServer     *server,
		       const gchar   *wire_name,
		       const gchar   *process_name,
		       BseComDispatch dispatcher,
		       gpointer       dispatch_data,
		       GDestroyNotify destroy_data,
		       GSList        *params,
		       BseScriptControl **sctrl_p)
{
  gint child_pid, standard_input, standard_output, standard_error, command_input, command_output;
  gchar *error;

  g_return_val_if_fail (BSE_IS_SERVER (server), NULL);
  g_return_val_if_fail (process_name != NULL, NULL);
  g_return_val_if_fail (dispatcher != NULL, NULL);

  if (sctrl_p)
    *sctrl_p = NULL;
  child_pid = standard_input = standard_output = standard_error = command_input = command_output = -1;
  error = bse_com_spawn_async (process_name,
			       &child_pid,
			       NULL, /* &standard_input, */
			       NULL, /* &standard_output, */
			       NULL, /* &standard_error, */
			       "--bse-command-pipe",
			       &command_input,
			       &command_output,
			       params);
  if (!error)
    {
      BseComWire *wire = bse_com_wire_from_child (wire_name,
						  command_output,
						  command_input,
						  standard_input,
						  standard_output,
						  standard_error,
						  child_pid);
      if (!wire->connected)	/* bad, bad */
	{
	  bse_com_wire_destroy (wire);
	  error = g_strdup (bse_error_blurb (BSE_ERROR_SPAWN));
	}
      else
	{
	  BseScriptControl *sctrl;

	  bse_com_wire_set_dispatcher (wire, dispatcher, dispatch_data, destroy_data);
	  sctrl = bse_script_control_new (wire);
	  bse_container_add_item (BSE_CONTAINER (server), BSE_ITEM (sctrl));
	  if (sctrl_p)
	    *sctrl_p = sctrl;
	}
    }
  if (error)
    {
      if (destroy_data)
	destroy_data (dispatch_data);
      bse_server_exec_status (server, BSE_EXEC_STATUS_DONE, wire_name, 0, BSE_ERROR_SPAWN);
    }

  return error;
}


/* --- GSL Main Thread Source --- */
typedef struct {
  GSource         source;
  BseServer	 *server;
  GslGlueContext *context;
  GPollFD	  pfd;
} MainSource;

static gboolean
main_source_prepare (GSource *source,
		     gint    *timeout_p)
{
  MainSource *xsource = (MainSource*) source;
  gboolean need_dispatch;

  BSE_THREADS_ENTER ();
  need_dispatch = gsl_glue_context_pending (xsource->context);
  if (xsource->server->midi_receiver)
    need_dispatch |= bse_midi_receiver_has_notify_events (xsource->server->midi_receiver);
  BSE_THREADS_LEAVE ();

  return need_dispatch;
}

static gboolean
main_source_check (GSource *source)
{
  MainSource *xsource = (MainSource*) source;
  gboolean need_dispatch;

  BSE_THREADS_ENTER ();
  need_dispatch = xsource->pfd.events & xsource->pfd.revents;
  need_dispatch |= gsl_glue_context_pending (xsource->context);
  if (xsource->server->midi_receiver)
    need_dispatch |= bse_midi_receiver_has_notify_events (xsource->server->midi_receiver);
  BSE_THREADS_LEAVE ();
  
  return need_dispatch;
}

static gboolean
main_source_dispatch (GSource    *source,
		      GSourceFunc callback,
		      gpointer    user_data)
{
  MainSource *xsource = (MainSource*) source;

  BSE_THREADS_ENTER ();
  gsl_glue_context_dispatch (xsource->context);
  if (xsource->server->midi_receiver && xsource->server->midi_receiver->notifier)
    bse_midi_notifier_dispatch (xsource->server->midi_receiver->notifier, xsource->server->midi_receiver);
  gsl_thread_sleep (0);	/* process poll fd data */
  BSE_THREADS_LEAVE ();

  return TRUE;
}

static void
main_thread_source_setup (BseServer      *self,
			  gint            priority,
			  GslGlueContext *context)
{
  static GSourceFuncs main_source_funcs = {
    main_source_prepare,
    main_source_check,
    main_source_dispatch,
  };
  GSource *source = g_source_new (&main_source_funcs, sizeof (MainSource));
  MainSource *xsource = (MainSource*) source;
  static gboolean single_call = 0;

  g_assert (single_call++ == 0);
  
  xsource->context = context;
  xsource->server = self;
  gsl_thread_get_pollfd (&xsource->pfd);
  g_source_set_priority (source, priority);
  g_source_add_poll (source, &xsource->pfd);
  g_source_attach (source, g_main_context_default ());
}


/* --- GPollFD IO watch source --- */
typedef struct {
  GSource    source;
  GPollFD    pfd;
  BseIOWatch watch_func;
  gpointer   data;
} WSource;

static gboolean
iowatch_prepare (GSource *source,
		 gint    *timeout_p)
{
  /* WSource *wsource = (WSource*) source; */
  gboolean need_dispatch;
  
  /* BSE_THREADS_ENTER (); */
  need_dispatch = FALSE;
  /* BSE_THREADS_LEAVE (); */

  return need_dispatch;
}

static gboolean
iowatch_check (GSource *source)
{
  WSource *wsource = (WSource*) source;
  guint need_dispatch;

  /* BSE_THREADS_ENTER (); */
  need_dispatch = wsource->pfd.events & wsource->pfd.revents;
  /* BSE_THREADS_LEAVE (); */

  return need_dispatch > 0;
}

static gboolean
iowatch_dispatch (GSource    *source,
		  GSourceFunc callback,
		  gpointer    user_data)
{
  WSource *wsource = (WSource*) source;

  BSE_THREADS_ENTER ();
  wsource->watch_func (wsource->data, &wsource->pfd);
  BSE_THREADS_LEAVE ();

  return TRUE;
}

static void
iowatch_add (BseServer   *server,
	     gint         fd,
	     GIOCondition events,
	     BseIOWatch   watch_func,
	     gpointer     data)
{
  static GSourceFuncs iowatch_gsource_funcs = {
    iowatch_prepare,
    iowatch_check,
    iowatch_dispatch,
    NULL
  };
  GSource *source = g_source_new (&iowatch_gsource_funcs, sizeof (WSource));
  WSource *wsource = (WSource*) source;

  server->watch_list = g_slist_prepend (server->watch_list, wsource);
  wsource->pfd.fd = fd;
  wsource->pfd.events = events;
  wsource->watch_func = watch_func;
  wsource->data = data;
  g_source_set_priority (source, G_PRIORITY_HIGH);
  g_source_add_poll (source, &wsource->pfd);
  g_source_attach (source, g_main_context_default ());
}

static gboolean
iowatch_remove (BseServer *server,
		BseIOWatch watch_func,
		gpointer   data)
{
  GSList *slist;

  for (slist = server->watch_list; slist; slist = slist->next)
    {
      WSource *wsource = slist->data;

      if (wsource->watch_func == watch_func && wsource->data == data)
	{
	  g_source_destroy (&wsource->source);
	  server->watch_list = g_slist_remove (server->watch_list, wsource);
	  return TRUE;
	}
    }
  return FALSE;
}


/* --- GSL engine main loop --- */
typedef struct {
  GSource       source;
  guint         n_fds;
  GPollFD       fds[GSL_ENGINE_MAX_POLLFDS];
  GslEngineLoop loop;
} PSource;

static gboolean
engine_prepare (GSource *source,
		gint    *timeout_p)
{
  PSource *psource = (PSource*) source;
  gboolean need_dispatch;
  
  BSE_THREADS_ENTER ();
  need_dispatch = gsl_engine_prepare (&psource->loop);
  if (psource->loop.fds_changed)
    {
      guint i;

      for (i = 0; i < psource->n_fds; i++)
	g_source_remove_poll (source, psource->fds + i);
      psource->n_fds = psource->loop.n_fds;
      for (i = 0; i < psource->n_fds; i++)
	{
	  GPollFD *pfd = psource->fds + i;

	  pfd->fd = psource->loop.fds[i].fd;
	  pfd->events = psource->loop.fds[i].events;
	  g_source_add_poll (source, pfd);
	}
    }
  *timeout_p = psource->loop.timeout;
  BSE_THREADS_LEAVE ();

  return need_dispatch;
}

static gboolean
engine_check (GSource *source)
{
  PSource *psource = (PSource*) source;
  gboolean need_dispatch;
  guint i;

  BSE_THREADS_ENTER ();
  for (i = 0; i < psource->n_fds; i++)
    psource->loop.fds[i].revents = psource->fds[i].revents;
  psource->loop.revents_filled = TRUE;
  need_dispatch = gsl_engine_check (&psource->loop);
  BSE_THREADS_LEAVE ();

  return need_dispatch;
}

static gboolean
engine_dispatch (GSource    *source,
		 GSourceFunc callback,
		 gpointer    user_data)
{
  BSE_THREADS_ENTER ();
  gsl_engine_dispatch ();
  BSE_THREADS_LEAVE ();

  return TRUE;
}

static void
engine_init (BseServer *server,
	     gfloat	mix_freq)
{
  static GSourceFuncs engine_gsource_funcs = {
    engine_prepare,
    engine_check,
    engine_dispatch,
    NULL
  };
  static gboolean engine_is_initialized = FALSE;

  g_return_if_fail (server->engine_source == NULL);

  bse_globals_lock ();		// FIXME: globals mix_freq
  server->engine_source = g_source_new (&engine_gsource_funcs, sizeof (PSource));
  g_source_set_priority (server->engine_source, G_PRIORITY_DEFAULT); 	// FIXME: prio settings

  if (!engine_is_initialized)	// FIXME: hack because we can't deinitialize the engine
    {
      engine_is_initialized = TRUE;
      gsl_engine_init (1, BSE_BLOCK_N_VALUES, mix_freq, 63);
    }
  else
    g_assert (mix_freq == gsl_engine_sample_freq () && BSE_BLOCK_N_VALUES == gsl_engine_block_size ());

  g_source_attach (server->engine_source, g_main_context_default ());
}

static void
engine_shutdown (BseServer *server)
{
  g_return_if_fail (server->engine_source != NULL);

  g_source_destroy (server->engine_source);
  server->engine_source = NULL;
  gsl_engine_garbage_collect ();
  // FIXME: need to be able to completely unintialize engine here
  bse_globals_unlock ();
}
