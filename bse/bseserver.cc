// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseserver.hh"
#include "bseproject.hh"
#include "bseengine.hh"
#include "gslcommon.hh"
#include "bseglue.hh"
#include "bsegconfig.hh"
#include "bsemidinotifier.hh"
#include "bsemain.hh"		/* threads enter/leave */
#include "bsepcmwriter.hh"
#include "bsemididevice-null.hh"
#include "bsejanitor.hh"
#include "bsecxxplugin.hh"
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "bsepcmmodule.cc"
using namespace Bse;

/* --- parameters --- */
enum
{
  PROP_0,
  PROP_GCONFIG,
  PROP_WAVE_FILE,
  PROP_LOG_MESSAGES
};


/* --- prototypes --- */
static void	bse_server_class_init		(BseServerClass	   *klass);
static void	bse_server_init			(BseServer	   *server);
static void	bse_server_finalize		(GObject	   *object);
static void	bse_server_set_property		(GObject           *object,
						 guint              param_id,
						 const GValue      *value,
						 GParamSpec        *pspec);
static void	bse_server_get_property		(GObject           *object,
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
static void     bse_server_release_children     (BseContainer      *container);
static gboolean	iowatch_remove			(BseServer	   *server,
						 BseIOWatch	    watch_func,
						 gpointer	    data);
static void	iowatch_add			(BseServer	   *server,
						 gint		    fd,
						 GIOCondition	    events,
						 BseIOWatch	    watch_func,
						 gpointer	    data);
static void	main_thread_source_setup	(BseServer	   *self);
static void	engine_init			(BseServer	   *server,
						 gfloat		    mix_freq);
static void	engine_shutdown			(BseServer	   *server);
/* --- variables --- */
static GTypeClass *parent_class = NULL;
static guint       signal_registration = 0;
static guint       signal_script_start = 0;
static guint       signal_script_error = 0;
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
                                   __FILE__, __LINE__,
                                   &server_info);
}
static void
bse_server_class_init (BseServerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (klass);

  parent_class = (GTypeClass*) g_type_class_peek_parent (klass);

  gobject_class->set_property = bse_server_set_property;
  gobject_class->get_property = bse_server_get_property;
  gobject_class->finalize = bse_server_finalize;

  item_class->set_parent = bse_server_set_parent;

  container_class->add_item = bse_server_add_item;
  container_class->remove_item = bse_server_remove_item;
  container_class->forall_items = bse_server_forall_items;
  container_class->release_children = bse_server_release_children;

  _bse_gconfig_init ();
  bse_object_class_add_param (object_class, "BSE Configuration",
			      PROP_GCONFIG,
			      bse_gconfig_pspec ());	/* "bse-preferences" */
  bse_object_class_add_param (object_class, "PCM Recording",
			      PROP_WAVE_FILE,
			      sfi_pspec_string ("wave_file", _("WAVE File"),
                                                _("Name of the WAVE file used for recording BSE sound output"),
						NULL, SFI_PARAM_GUI ":filename"));
  bse_object_class_add_param (object_class, "Misc",
			      PROP_LOG_MESSAGES,
			      sfi_pspec_bool ("log-messages", "Log Messages", "Log messages through the log system", TRUE, SFI_PARAM_GUI));

  signal_registration = bse_object_class_add_signal (object_class, "registration",
						     G_TYPE_NONE, 3,
						     BSE_TYPE_REGISTRATION_TYPE,
						     G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
						     G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE);
  signal_script_start = bse_object_class_add_signal (object_class, "script-start",
						     G_TYPE_NONE, 1,
						     BSE_TYPE_JANITOR);
  signal_script_error = bse_object_class_add_signal (object_class, "script-error",
						     G_TYPE_NONE, 3,
						     G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
}

static GTokenType
rc_file_try_statement (gpointer   context_data,
		       SfiRStore *rstore,
		       GScanner  *scanner,
		       gpointer   user_data)
{
  BseServer *server = (BseServer*) context_data;
  g_assert (scanner->next_token == G_TOKEN_IDENTIFIER);
  if (strcmp ("bse-preferences", scanner->next_value.v_identifier) == 0)
    {
      GValue *value = sfi_value_rec (NULL);
      GTokenType token;
      SfiRec *rec;
      g_scanner_get_next_token (rstore->scanner);
      token = sfi_rstore_parse_param (rstore, value, bse_gconfig_pspec ());
      rec = sfi_value_get_rec (value);
      if (token == G_TOKEN_NONE && rec)
	bse_item_set (server,
                      "bse-preferences", rec,
                      NULL);
      sfi_value_free (value);
      return token;
    }
  else
    return SFI_TOKEN_UNMATCHED;
}

static void
bse_server_init (BseServer *self)
{
  g_assert (BSE_OBJECT_ID (self) == 1);	/* assert being the first object */
  BSE_OBJECT_SET_FLAGS (self, BSE_ITEM_FLAG_SINGLETON);

  self->engine_source = NULL;
  self->projects = NULL;
  self->dev_use_count = 0;
  self->log_messages = TRUE;
  self->pcm_device = NULL;
  self->pcm_imodule = NULL;
  self->pcm_omodule = NULL;
  self->pcm_writer = NULL;
  self->midi_device = NULL;

  /* keep the server singleton alive */
  bse_item_use (BSE_ITEM (self));

  /* start dispatching main thread stuff */
  main_thread_source_setup (self);

  /* read rc file */
  int fd = -1;
  if (!bse_main_args->stand_alone &&
      bse_main_args->bse_rcfile &&
      bse_main_args->bse_rcfile[0])
    fd = open (bse_main_args->bse_rcfile, O_RDONLY, 0);
  if (fd >= 0)
    {
      SfiRStore *rstore = sfi_rstore_new ();
      sfi_rstore_input_fd (rstore, fd, bse_main_args->bse_rcfile);
      sfi_rstore_parse_all (rstore, self, rc_file_try_statement, NULL);
      sfi_rstore_destroy (rstore);
      close (fd);
    }

  /* integrate argv overides */
  bse_gconfig_merge_args (bse_main_args);

  /* dispatch midi notifiers */
  bse_midi_notifiers_attach_source();
}

static void
bse_server_finalize (GObject *object)
{
  g_error ("Fatal attempt to destroy singleton BseServer");

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
bse_server_set_property (GObject      *object,
			 guint         param_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  BseServer *self = BSE_SERVER (object);
  switch (param_id)
    {
      SfiRec *rec;
    case PROP_GCONFIG:
      rec = sfi_value_get_rec (value);
      if (rec)
	bse_gconfig_apply (rec);
      break;
    case PROP_WAVE_FILE:
      bse_server_start_recording (self, g_value_get_string (value), 0);
      break;
    case PROP_LOG_MESSAGES:
      self->log_messages = sfi_value_get_bool (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_server_get_property (GObject    *object,
			 guint       param_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
  BseServer *self = BSE_SERVER (object);
  switch (param_id)
    {
      SfiRec *rec;
    case PROP_GCONFIG:
      rec = bse_gconfig_to_rec (bse_global_config);
      sfi_value_set_rec (value, rec);
      sfi_rec_unref (rec);
      break;
    case PROP_WAVE_FILE:
      g_value_set_string (value, self->wave_file);
      break;
    case PROP_LOG_MESSAGES:
      sfi_value_set_bool (value, self->log_messages);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

void
bse_server_notify_gconfig (BseServer *server)
{
  g_return_if_fail (BSE_IS_SERVER (server));

  g_object_notify ((GObject*) server, bse_gconfig_pspec ()->name);
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
      BseItem *item = (BseItem*) slist->data;

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

static void
bse_server_release_children (BseContainer *container)
{
  // BseServer *self = BSE_SERVER (container);

  g_warning ("release_children() should never be triggered on BseServer singleton");

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->release_children (container);
}

/**
 * @returns Global BSE Server
 *
 * Retrieve the global BSE server object.
 **/
BseServer*
bse_server_get (void)
{
  static BseServer *server = NULL;

  if (!server)
    {
      server = (BseServer*) g_object_new (BSE_TYPE_SERVER, NULL);
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

  project = (BseProject*) g_object_new (BSE_TYPE_PROJECT, "uname", name, NULL);
  server->projects = g_list_prepend (server->projects, project);
  g_object_connect (project,
		    "signal::release", destroy_project, server,
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
      BseProject *project = (BseProject*) node->data;
      gchar *uname = BSE_OBJECT_UNAME (project);

      if (uname && strcmp (name, uname) == 0)
	return project;
    }
  return NULL;
}

void
bse_server_stop_recording (BseServer *self)
{
  GList *node;
  for (node = self->projects; node; node = node->next)
    {
      BseProject *project = (BseProject*) node->data;
      bse_project_stop_playback (project);
    }
  self->wave_seconds = 0;
  g_free (self->wave_file);
  self->wave_file = NULL;
  g_object_notify ((GObject*) self, "wave-file");
}

void
bse_server_start_recording (BseServer      *self,
                            const char     *wave_file,
                            double          n_seconds)
{
  if (!bse_gconfig_locked ())
    {
      self->wave_seconds = MAX (n_seconds, 0);
      self->wave_file = g_strdup_stripped (wave_file ? wave_file : "");
      if (!self->wave_file[0])
        {
          g_free (self->wave_file);
          self->wave_file = NULL;
        }
      g_object_notify ((GObject*) self, "wave-file");
    }
}

void
bse_server_require_pcm_input (BseServer *server)
{
  if (server->pcm_device && !server->pcm_input_checked)
    {
      server->pcm_input_checked = TRUE;
      if (!BSE_DEVICE_READABLE (server->pcm_device))
        {
          UserMessage umsg;
          umsg.type = Bse::WARNING;
          umsg.title = _("Audio Recording Failed");
          umsg.text1 = _("Failed to start recording from audio device.");
          umsg.text2 = _("An audio project is in use which processes an audio input signal, but the audio device "
                         "has not been opened in recording mode. "
                         "An audio signal of silence will be used instead of a recorded signal, "
                         "so playback operation may produce results not actually intended "
                         "(such as a silent output signal).");
          umsg.text3 = string_format (_("Audio device \"%s\" is not open for input, audio driver: %s=%s"),
                                      BSE_DEVICE (server->pcm_device)->open_device_name,
                                      BSE_DEVICE_GET_CLASS (server->pcm_device)->driver_name,
                                      BSE_DEVICE (server->pcm_device)->open_device_args);
          umsg.label = _("audio input problems");
          ServerImpl::instance().send_user_message (umsg);
        }
    }
}

typedef struct {
  guint      n_channels;
  guint      mix_freq;
  guint      latency;
  guint      block_size;
} PcmRequest;
static void
pcm_request_callback (BseDevice *device,
                      gpointer   data)
{
  PcmRequest *pr = (PcmRequest*) data;
  bse_pcm_device_request (BSE_PCM_DEVICE (device), pr->n_channels, pr->mix_freq, pr->latency, pr->block_size);
}
static BseErrorType
server_open_pcm_device (BseServer *server,
                        guint      mix_freq,
                        guint      latency,
                        guint      block_size)
{
  g_return_val_if_fail (server->pcm_device == NULL, BSE_ERROR_INTERNAL);
  BseErrorType error = BSE_ERROR_UNKNOWN;
  PcmRequest pr;
  pr.n_channels = 2;
  pr.mix_freq = mix_freq;
  pr.latency = latency;
  pr.block_size = block_size;
  server->pcm_device = (BsePcmDevice*) bse_device_open_best (BSE_TYPE_PCM_DEVICE, TRUE, TRUE,
                                                             bse_main_args->pcm_drivers,
                                                             pcm_request_callback, &pr, error ? NULL : &error);
  if (!server->pcm_device)
    server->pcm_device = (BsePcmDevice*) bse_device_open_best (BSE_TYPE_PCM_DEVICE, FALSE, TRUE,
                                                               bse_main_args->pcm_drivers,
                                                               pcm_request_callback, &pr, error ? NULL : &error);
  if (!server->pcm_device)
    {
      UserMessage umsg;
      umsg.type = Bse::ERROR;
      umsg.title = _("Audio I/O Failed");
      umsg.text1 = _("No available audio device was found.");
      umsg.text2 = _("No available audio device could be found and opened successfully. "
                     "Sorry, no fallback selection can be made for audio devices, giving up.");
      umsg.text3 = string_format (_("Failed to open PCM devices: %s"), bse_error_blurb (error));
      umsg.label = _("PCM device selections problems");
      ServerImpl::instance().send_user_message (umsg);
    }
  server->pcm_input_checked = FALSE;
  return server->pcm_device ? BSE_ERROR_NONE : error;
}
static BseErrorType
server_open_midi_device (BseServer *server)
{
  g_return_val_if_fail (server->midi_device == NULL, BSE_ERROR_INTERNAL);
  BseErrorType error;
  server->midi_device = (BseMidiDevice*) bse_device_open_best (BSE_TYPE_MIDI_DEVICE, TRUE, FALSE, bse_main_args->midi_drivers, NULL, NULL, &error);
  if (!server->midi_device)
    {
      SfiRing *ring = sfi_ring_prepend (NULL, (void*) "null");
      server->midi_device = (BseMidiDevice*) bse_device_open_best (BSE_TYPE_MIDI_DEVICE_NULL, TRUE, FALSE, ring, NULL, NULL, NULL);
      sfi_ring_free (ring);

      if (server->midi_device)
        {
          UserMessage umsg;
          umsg.type = Bse::WARNING;
          umsg.title = _("MIDI I/O Failed");
          umsg.text1 = _("MIDI input or output is not available.");
          umsg.text2 = _("No available MIDI device could be found and opened successfully. "
                         "Reverting to null device, no MIDI events will be received or sent.");
          umsg.text3 = string_format (_("Failed to open MIDI devices: %s"), bse_error_blurb (error));
          umsg.label = _("MIDI device selections problems");
          ServerImpl::instance().send_user_message (umsg);
        }
    }
  return server->midi_device ? BSE_ERROR_NONE : error;
}
BseErrorType
bse_server_open_devices (BseServer *self)
{
  BseErrorType error = BSE_ERROR_NONE;
  g_return_val_if_fail (BSE_IS_SERVER (self), BSE_ERROR_INTERNAL);
  /* check whether devices are already opened */
  if (self->dev_use_count)
    {
      self->dev_use_count++;
      return BSE_ERROR_NONE;
    }
  /* lock playback/capture/latency settings */
  bse_gconfig_lock ();
  /* calculate block_size for pcm setup */
  guint block_size, latency = BSE_GCONFIG (synth_latency), mix_freq = BSE_GCONFIG (synth_mixing_freq);
  bse_engine_constrain (latency, mix_freq, BSE_GCONFIG (synth_control_freq), &block_size, NULL);
  /* try opening devices */
  if (!error)
    error = server_open_pcm_device (self, mix_freq, latency, block_size);
  guint aligned_freq = bse_pcm_device_frequency_align (mix_freq);
  if (error && aligned_freq != mix_freq)
    {
      mix_freq = aligned_freq;
      bse_engine_constrain (latency, mix_freq, BSE_GCONFIG (synth_control_freq), &block_size, NULL);
      BseErrorType new_error = server_open_pcm_device (self, mix_freq, latency, block_size);
      error = new_error ? error : BSE_ERROR_NONE;
    }
  if (!error)
    error = server_open_midi_device (self);
  if (!error)
    {
      BseTrans *trans = bse_trans_open ();
      engine_init (self, bse_pcm_device_get_mix_freq (self->pcm_device));
      BsePcmHandle *pcm_handle = bse_pcm_device_get_handle (self->pcm_device, bse_engine_block_size());
      self->pcm_imodule = bse_pcm_imodule_insert (pcm_handle, trans);
      if (self->wave_file)
	{
	  BseErrorType error;
	  self->pcm_writer = (BsePcmWriter*) g_object_new (BSE_TYPE_PCM_WRITER, NULL);
          const uint n_channels = 2;
	  error = bse_pcm_writer_open (self->pcm_writer, self->wave_file,
                                       n_channels, bse_engine_sample_freq (),
                                       n_channels * bse_engine_sample_freq() * self->wave_seconds);
	  if (error)
	    {
              UserMessage umsg;
              umsg.type = Bse::ERROR;
              umsg.title = _("Disk Recording Failed");
              umsg.text1 = _("Failed to start PCM recording to disk.");
              umsg.text2 = _("An error occoured while opening the recording file, selecting a different "
                             "file might fix this situation.");
              umsg.text3 = string_format (_("Failed to open file \"%s\" for recording: %s"), self->wave_file, bse_error_blurb (error));
              umsg.label = _("PCM recording errors");
              ServerImpl::instance().send_user_message (umsg);
	      g_object_unref (self->pcm_writer);
	      self->pcm_writer = NULL;
	    }
	}
      self->pcm_omodule = bse_pcm_omodule_insert (pcm_handle, self->pcm_writer, trans);
      bse_trans_commit (trans);
      self->dev_use_count++;
    }
  else
    {
      if (self->midi_device)
	{
	  bse_device_close (BSE_DEVICE (self->midi_device));
	  g_object_unref (self->midi_device);
	  self->midi_device = NULL;
	}
      if (self->pcm_device)
	{
	  bse_device_close (BSE_DEVICE (self->pcm_device));
	  g_object_unref (self->pcm_device);
	  self->pcm_device = NULL;
	}
    }
  bse_gconfig_unlock ();        /* engine_init() holds another lock count on success */
  return error;
}


void
bse_server_close_devices (BseServer *self)
{
  g_return_if_fail (BSE_IS_SERVER (self));
  g_return_if_fail (self->dev_use_count > 0);

  self->dev_use_count--;
  if (!self->dev_use_count)
    {
      BseTrans *trans = bse_trans_open ();
      bse_pcm_imodule_remove (self->pcm_imodule, trans);
      self->pcm_imodule = NULL;
      bse_pcm_omodule_remove (self->pcm_omodule, trans);
      self->pcm_omodule = NULL;
      bse_trans_commit (trans);
      /* wait until transaction has been processed */
      bse_engine_wait_on_trans ();
      if (self->pcm_writer)
	{
	  if (self->pcm_writer->open)
	    bse_pcm_writer_close (self->pcm_writer);
	  g_object_unref (self->pcm_writer);
	  self->pcm_writer = NULL;
	}
      bse_device_close (BSE_DEVICE (self->pcm_device));
      bse_device_close (BSE_DEVICE (self->midi_device));
      engine_shutdown (self);
      g_object_unref (self->pcm_device);
      self->pcm_device = NULL;
      g_object_unref (self->midi_device);
      self->midi_device = NULL;
    }
}

BseModule*
bse_server_retrieve_pcm_output_module (BseServer   *self,
				       BseSource   *source,
				       const gchar *uplink_name)
{
  g_return_val_if_fail (BSE_IS_SERVER (self), NULL);
  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);
  g_return_val_if_fail (uplink_name != NULL, NULL);
  g_return_val_if_fail (self->dev_use_count > 0, NULL);

  self->dev_use_count += 1;

  return self->pcm_omodule;
}

void
bse_server_discard_pcm_output_module (BseServer *self,
				      BseModule *module)
{
  g_return_if_fail (BSE_IS_SERVER (self));
  g_return_if_fail (module != NULL);
  g_return_if_fail (self->dev_use_count > 0);

  /* decrement dev_use_count */
  bse_server_close_devices (self);
}

BseModule*
bse_server_retrieve_pcm_input_module (BseServer   *self,
				      BseSource   *source,
				      const gchar *uplink_name)
{
  g_return_val_if_fail (BSE_IS_SERVER (self), NULL);
  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);
  g_return_val_if_fail (uplink_name != NULL, NULL);
  g_return_val_if_fail (self->dev_use_count > 0, NULL);

  self->dev_use_count += 1;

  return self->pcm_imodule;
}

void
bse_server_discard_pcm_input_module (BseServer *self,
				     BseModule *module)
{
  g_return_if_fail (BSE_IS_SERVER (self));
  g_return_if_fail (module != NULL);
  g_return_if_fail (self->dev_use_count > 0);

  /* decrement dev_use_count */
  bse_server_close_devices (self);
}

/**
 * @param script_control associated script control object
 *
 * Signal script invocation start.
 */
void
bse_server_script_start (BseServer  *server,
			 BseJanitor *janitor)
{
  g_return_if_fail (BSE_IS_SERVER (server));
  g_return_if_fail (BSE_IS_JANITOR (janitor));

  g_signal_emit (server, signal_script_start, 0, janitor);
}

void
bse_server_registration (BseServer          *server,
			 BseRegistrationType rtype,
			 const gchar	    *what,
			 const gchar	    *error)
{
  g_return_if_fail (BSE_IS_SERVER (server));

  g_signal_emit (server, signal_registration, 0, rtype, what, error);
}

/**
 * @param script_name name of the executed script
 * @param proc_name   procedure name to execute
 * @param reason      error condition
 *
 * Signal script invocation error.
 */
void
bse_server_script_error (BseServer   *server,
			 const gchar *script_name,
			 const gchar *proc_name,
			 const gchar *reason)
{
  g_return_if_fail (BSE_IS_SERVER (server));
  g_return_if_fail (script_name != NULL);
  g_return_if_fail (proc_name != NULL);
  g_return_if_fail (reason != NULL);
  g_signal_emit (server, signal_script_error, 0,
		 script_name, proc_name, reason);
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

BseErrorType
bse_server_run_remote (BseServer         *server,
		       const gchar       *process_name,
		       SfiRing           *params,
		       const gchar       *script_name,
		       const gchar       *proc_name,
		       BseJanitor       **janitor_p)
{
  gint child_pid, command_input, command_output;
  BseJanitor *janitor = NULL;

  g_return_val_if_fail (BSE_IS_SERVER (server), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (process_name != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (script_name != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (proc_name != NULL, BSE_ERROR_INTERNAL);

  child_pid = command_input = command_output = -1;
  const char *reason = sfi_com_spawn_async (process_name,
                                            &child_pid,
                                            NULL, /* &standard_input, */
                                            NULL, /* &standard_output, */
                                            NULL, /* &standard_error, */
                                            "--bse-pipe",
                                            &command_input,
                                            &command_output,
                                            params);
  char *freeme = NULL;
  if (!reason)
    {
      gchar *ident = g_strdup_printf ("%s::%s", script_name, proc_name);
      SfiComPort *port = sfi_com_port_from_child (ident,
						  command_output,
						  command_input,
						  child_pid);
      g_free (ident);
      if (!port->connected)	/* bad, bad */
	{
	  sfi_com_port_unref (port);
	  reason = freeme = g_strdup ("failed to establish connection");
	}
      else
	{
	  janitor = bse_janitor_new (port);
	  bse_janitor_set_procedure (janitor, script_name, proc_name);
	  sfi_com_port_unref (port);
	  /* already owned by server */
	  g_object_unref (janitor);
	}
    }
  if (janitor_p)
    *janitor_p = janitor;
  if (reason)
    {
      bse_server_script_error (server, script_name, proc_name, reason);
      g_free (freeme);
      return BSE_ERROR_SPAWN;
    }
  g_free (freeme);
  bse_server_script_start (server, janitor);
  return BSE_ERROR_NONE;
}


/* --- GSL Main Thread Source --- */
typedef struct {
  GSource         source;
  BseServer	 *server;
  GPollFD	  pfd;
} MainSource;

static gboolean
main_source_prepare (GSource *source,
		     gint    *timeout_p)
{
  // MainSource *xsource = (MainSource*) source;
  gboolean need_dispatch;

  BSE_THREADS_ENTER ();
  need_dispatch = FALSE;
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
  BSE_THREADS_LEAVE ();

  return need_dispatch;
}

static gboolean
main_source_dispatch (GSource    *source,
		      GSourceFunc callback,
		      gpointer    user_data)
{
  // MainSource *xsource = (MainSource*) source;

  BSE_THREADS_ENTER ();
  BSE_THREADS_LEAVE ();

  return TRUE;
}

static void
main_thread_source_setup (BseServer *self)
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

  xsource->server = self;
  g_source_set_priority (source, BSE_PRIORITY_NORMAL);
  g_source_attach (source, bse_main_context);
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
  wsource->watch_func (wsource->data, 1, &wsource->pfd);
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
  g_source_set_priority (source, BSE_PRIORITY_HIGH);
  g_source_add_poll (source, &wsource->pfd);
  g_source_attach (source, bse_main_context);
}

static gboolean
iowatch_remove (BseServer *server,
		BseIOWatch watch_func,
		gpointer   data)
{
  GSList *slist;

  for (slist = server->watch_list; slist; slist = slist->next)
    {
      WSource *wsource = (WSource*) slist->data;

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
  GPollFD       fds[BSE_ENGINE_MAX_POLLFDS];
  BseEngineLoop loop;
} PSource;

static gboolean
engine_prepare (GSource *source,
		gint    *timeout_p)
{
  PSource *psource = (PSource*) source;
  gboolean need_dispatch;

  BSE_THREADS_ENTER ();
  need_dispatch = bse_engine_prepare (&psource->loop);
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
  need_dispatch = bse_engine_check (&psource->loop);
  BSE_THREADS_LEAVE ();

  return need_dispatch;
}

static gboolean
engine_dispatch (GSource    *source,
		 GSourceFunc callback,
		 gpointer    user_data)
{
  BSE_THREADS_ENTER ();
  bse_engine_dispatch ();
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

  bse_gconfig_lock ();
  server->engine_source = g_source_new (&engine_gsource_funcs, sizeof (PSource));
  g_source_set_priority (server->engine_source, BSE_PRIORITY_HIGH);

  if (!engine_is_initialized)
    {
      engine_is_initialized = true;
      bse_engine_init (true);
      // lower priority compared to engine if our priority range permits
      const int mypid = Rapicorn::ThisThread::thread_pid();
      int current_priority = getpriority (PRIO_PROCESS, mypid);
      if (current_priority <= -2 && mypid)
        setpriority (PRIO_PROCESS, mypid, current_priority + 1);
    }
  bse_engine_configure (BSE_GCONFIG (synth_latency), mix_freq, BSE_GCONFIG (synth_control_freq));

  g_source_attach (server->engine_source, bse_main_context);
}

static void
engine_shutdown (BseServer *server)
{
  g_return_if_fail (server->engine_source != NULL);

  g_source_destroy (server->engine_source);
  server->engine_source = NULL;
  bse_engine_user_thread_collect ();
  // FIXME: need to be able to completely unintialize engine here
  bse_gconfig_unlock ();
}


namespace Bse {

ServerImpl::ServerImpl ()
{}

ServerImpl::~ServerImpl ()
{}

TestObjectIface*
ServerImpl::get_test_object ()
{
  if (!test_object_)
    test_object_ = std::make_shared<TestObjectImpl>();
  return &*test_object_;
}

ServerImpl&
ServerImpl::instance()
{
  static Rapicorn::Atomic<ServerImpl*> instance_ = NULL;
  if (LIKELY (instance_ != NULL))
    return *instance_;
  static Mutex instance_mutex;
  ScopedLock<Mutex> locker (instance_mutex);
  if (!instance_)
    {
      static uint64 instance_space[sizeof (*instance_) / sizeof (uint64)];
      instance_ = new (instance_space) ServerImpl();
    }
  return *instance_;
}

void
ServerImpl::send_user_message (const UserMessage &umsg)
{
  assert_return (umsg.text1.empty() == false);
  sig_user_message.emit (umsg);
}

} // Bse
