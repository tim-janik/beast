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
#include "bsepcmmodule.cc"
#include "gsldatahandle-mad.hh"
#include "gslvorbis-enc.hh"
#include "bsescripthelper.hh"
#include "bseladspa.hh"
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
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
  assert (scanner->next_token == G_TOKEN_IDENTIFIER);
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
  assert (BSE_OBJECT_ID (self) == 1);	/* assert being the first object */
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
  BseServer *self = BSE_SERVER_CAST (object);
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
  BseServer *self = BSE_SERVER_CAST (object);
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
  assert_return (BSE_IS_SERVER (server));

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
  BseServer *self = BSE_SERVER_CAST (container);

  self->children = g_slist_prepend (self->children, item);

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_server_forall_items (BseContainer      *container,
			 BseForallItemsFunc func,
			 gpointer           data)
{
  BseServer *self = BSE_SERVER_CAST (container);
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
  BseServer *self = BSE_SERVER_CAST (container);

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
      server = (BseServer*) bse_object_new (BSE_TYPE_SERVER, "uname", "ServerImpl", NULL);
      g_object_ref (server);
      assert (server);
      assert (server->cxxobject_);
      assert (dynamic_cast<Bse::ObjectImpl*> (server->cxxobject_));
      assert (dynamic_cast<Bse::ServerImpl*> (server->cxxobject_));
    }

  return server;
}

BseProject*
bse_server_find_project (BseServer   *server,
			 const gchar *name)
{
  GList *node;

  assert_return (BSE_IS_SERVER (server), NULL);
  assert_return (name != NULL, NULL);

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
static Bse::ErrorType
server_open_pcm_device (BseServer *server,
                        guint      mix_freq,
                        guint      latency,
                        guint      block_size)
{
  assert_return (server->pcm_device == NULL, Bse::ERROR_INTERNAL);
  Bse::ErrorType error = Bse::ERROR_UNKNOWN;
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
  return server->pcm_device ? Bse::ERROR_NONE : error;
}
static Bse::ErrorType
server_open_midi_device (BseServer *server)
{
  assert_return (server->midi_device == NULL, Bse::ERROR_INTERNAL);
  Bse::ErrorType error;
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
  return server->midi_device ? Bse::ERROR_NONE : error;
}
Bse::ErrorType
bse_server_open_devices (BseServer *self)
{
  Bse::ErrorType error = Bse::ERROR_NONE;
  assert_return (BSE_IS_SERVER (self), Bse::ERROR_INTERNAL);
  /* check whether devices are already opened */
  if (self->dev_use_count)
    {
      self->dev_use_count++;
      return Bse::ERROR_NONE;
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
      Bse::ErrorType new_error = server_open_pcm_device (self, mix_freq, latency, block_size);
      error = new_error ? error : Bse::ERROR_NONE;
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
	  Bse::ErrorType error;
	  self->pcm_writer = (BsePcmWriter*) bse_object_new (BSE_TYPE_PCM_WRITER, NULL);
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
  assert_return (BSE_IS_SERVER (self));
  assert_return (self->dev_use_count > 0);

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
  assert_return (BSE_IS_SERVER (self), NULL);
  assert_return (BSE_IS_SOURCE (source), NULL);
  assert_return (uplink_name != NULL, NULL);
  assert_return (self->dev_use_count > 0, NULL);

  self->dev_use_count += 1;

  return self->pcm_omodule;
}

void
bse_server_discard_pcm_output_module (BseServer *self,
				      BseModule *module)
{
  assert_return (BSE_IS_SERVER (self));
  assert_return (module != NULL);
  assert_return (self->dev_use_count > 0);

  /* decrement dev_use_count */
  bse_server_close_devices (self);
}

BseModule*
bse_server_retrieve_pcm_input_module (BseServer   *self,
				      BseSource   *source,
				      const gchar *uplink_name)
{
  assert_return (BSE_IS_SERVER (self), NULL);
  assert_return (BSE_IS_SOURCE (source), NULL);
  assert_return (uplink_name != NULL, NULL);
  assert_return (self->dev_use_count > 0, NULL);

  self->dev_use_count += 1;

  return self->pcm_imodule;
}

void
bse_server_discard_pcm_input_module (BseServer *self,
				     BseModule *module)
{
  assert_return (BSE_IS_SERVER (self));
  assert_return (module != NULL);
  assert_return (self->dev_use_count > 0);

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
  assert_return (BSE_IS_SERVER (server));
  assert_return (BSE_IS_JANITOR (janitor));

  g_signal_emit (server, signal_script_start, 0, janitor);
}

void
bse_server_registration (BseServer          *server,
			 BseRegistrationType rtype,
			 const gchar	    *what,
			 const gchar	    *error)
{
  assert_return (BSE_IS_SERVER (server));

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
  assert_return (BSE_IS_SERVER (server));
  assert_return (script_name != NULL);
  assert_return (proc_name != NULL);
  assert_return (reason != NULL);
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
  assert_return (BSE_IS_SERVER (server));
  assert_return (watch_func != NULL);
  assert_return (fd >= 0);
  iowatch_add (server, fd, events, watch_func, data);
}

void
bse_server_remove_io_watch (BseServer *server,
			    BseIOWatch watch_func,
			    gpointer   data)
{
  assert_return (BSE_IS_SERVER (server));
  assert_return (watch_func != NULL);

  if (!iowatch_remove (server, watch_func, data))
    g_warning (G_STRLOC ": no such io watch installed %p(%p)", watch_func, data);
}

Bse::ErrorType
bse_server_run_remote (BseServer         *server,
		       const gchar       *process_name,
		       SfiRing           *params,
		       const gchar       *script_name,
		       const gchar       *proc_name,
		       BseJanitor       **janitor_p)
{
  gint child_pid, command_input, command_output;
  BseJanitor *janitor = NULL;

  assert_return (BSE_IS_SERVER (server), Bse::ERROR_INTERNAL);
  assert_return (process_name != NULL, Bse::ERROR_INTERNAL);
  assert_return (script_name != NULL, Bse::ERROR_INTERNAL);
  assert_return (proc_name != NULL, Bse::ERROR_INTERNAL);

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
      gchar *ident = g_strdup_format ("%s::%s", script_name, proc_name);
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
      return Bse::ERROR_SPAWN;
    }
  g_free (freeme);
  bse_server_script_start (server, janitor);
  return Bse::ERROR_NONE;
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

  assert (single_call++ == 0);

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

  assert_return (server->engine_source == NULL);

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
  assert_return (server->engine_source != NULL);

  g_source_destroy (server->engine_source);
  server->engine_source = NULL;
  bse_engine_user_thread_collect ();
  // FIXME: need to be able to completely unintialize engine here
  bse_gconfig_unlock ();
}


namespace Bse {

ServerImpl::ServerImpl (BseObject *bobj) :
  ObjectImpl (bobj)
{}

ServerImpl::~ServerImpl ()
{}

TestObjectIfaceP
ServerImpl::get_test_object ()
{
  if (!test_object_)
    test_object_ = FriendAllocator<TestObjectImpl>::make_shared();
  return test_object_;
}

ObjectIfaceP
ServerImpl::from_proxy (int64_t proxyid)
{
  BseObject *bo = bse_object_from_id (proxyid);
  if (!bo)
    return ObjectIfaceP();
  return bo->as<ObjectIfaceP>();
}

ServerImpl&
ServerImpl::instance()
{
  return *bse_server_get()->as<ServerImpl*>();
}

void
ServerImpl::send_user_message (const UserMessage &umsg)
{
  assert_return (umsg.text1.empty() == false);
  sig_user_message.emit (umsg);
}

String
ServerImpl::get_mp3_version ()
{
  return String ("MAD ") + gsl_data_handle_mad_version ();
}

String
ServerImpl::get_vorbis_version ()
{
  return "Ogg/Vorbis " + gsl_vorbis_encoder_version();
}

String
ServerImpl::get_ladspa_path ()
{
  return Path::searchpath_join (bse_installpath (BSE_INSTALLPATH_LADSPA), BSE_GCONFIG (ladspa_path));
}

String
ServerImpl::get_plugin_path ()
{
  return Path::searchpath_join (bse_installpath (BSE_INSTALLPATH_BSELIBDIR_PLUGINS), BSE_GCONFIG (plugin_path));
}

String
ServerImpl::get_script_path ()
{
  return Path::searchpath_join (bse_installpath (BSE_INSTALLPATH_DATADIR_SCRIPTS), BSE_GCONFIG (script_path));
}

String
ServerImpl::get_instrument_path ()
{
  return Path::searchpath_join (bse_installpath (BSE_INSTALLPATH_DATADIR_INSTRUMENTS), BSE_GCONFIG (instrument_path));
}

String
ServerImpl::get_sample_path ()
{
  return Path::searchpath_join (bse_installpath (BSE_INSTALLPATH_DATADIR_SAMPLES), BSE_GCONFIG (sample_path));
}

String
ServerImpl::get_effect_path ()
{
  return Path::searchpath_join (bse_installpath (BSE_INSTALLPATH_DATADIR_EFFECTS), BSE_GCONFIG (effect_path));
}

String
ServerImpl::get_demo_path ()
{
  return bse_installpath (BSE_INSTALLPATH_DATADIR_DEMO);
}

String
ServerImpl::get_version ()
{
  return bse_version();
}

String
ServerImpl::get_custom_effect_dir ()
{
  StringVector strings = string_split (BSE_GCONFIG (effect_path), G_SEARCHPATH_SEPARATOR_S);
  return strings.size() ? strings[0] : "";
}

String
ServerImpl::get_custom_instrument_dir ()
{
  StringVector strings = string_split (BSE_GCONFIG (instrument_path), G_SEARCHPATH_SEPARATOR_S);
  return strings.size() ? strings[0] : "";
}

void
ServerImpl::save_preferences ()
{
  gchar *file_name = g_strconcat (g_get_home_dir (), "/.bserc", NULL);
  int fd = open (file_name, O_WRONLY | O_CREAT | O_TRUNC /* | O_EXCL */, 0666);
  g_free (file_name);
  if (fd < 0)
    return;

  SfiWStore *wstore = sfi_wstore_new ();
  sfi_wstore_printf (wstore, "; rc-file for BSE v%s\n", bse_version());

  /* store BseGConfig */
  sfi_wstore_puts (wstore, "\n; BseGConfig Dump\n");
  SfiRec *rec = bse_gconfig_to_rec (bse_global_config);
  GValue *value = sfi_value_rec (rec);
  sfi_wstore_put_param (wstore, value, bse_gconfig_pspec ());
  sfi_value_free (value);
  sfi_rec_unref (rec);
  sfi_wstore_puts (wstore, "\n");

  /* flush stuff to rc file */
  sfi_wstore_flush_fd (wstore, fd);
  sfi_wstore_destroy (wstore);
  close (fd);
}

static gboolean
register_ladspa_plugins_handler (gpointer data)
{
  BseServer *server = (BseServer*) data;
  SfiRing *lplugins = (SfiRing*) g_object_get_data ((GObject*) server, "ladspa-registration-queue");
  const gchar *error;

  if (g_object_get_data ((GObject*) server, "plugin-registration-queue"))
    {
      /* give precedence to core plugins until they're done registering */
      return TRUE;
    }

  if (lplugins)
    {
      char *name = (char*) sfi_ring_pop_head (&lplugins);
      g_object_set_data ((GObject*) server, "ladspa-registration-queue", lplugins);
      error = bse_ladspa_plugin_check_load (name);
      bse_server_registration (server, BSE_REGISTER_PLUGIN, name, error);
      g_free (name);
    }
  else
    {
      bse_server_registration (server, BSE_REGISTER_DONE, NULL, NULL);
      return FALSE;
    }
  return TRUE;
}

void
ServerImpl::register_ladspa_plugins ()
{
  static bool registration_done = false;
  if (registration_done)
    {
      // always honor register_ladspa_plugins() with register_done signal
      bse_server_registration (as<BseServer*>(), BSE_REGISTER_DONE, NULL, NULL);
      return;
    }
  SfiRing *ring = bse_ladspa_plugin_path_list_files ();
  BseServer *server = as<BseServer*>();
  g_object_set_data (server, "ladspa-registration-queue", ring);
  bse_idle_normal (register_ladspa_plugins_handler, server);
  registration_done = true;
}

static gboolean
register_core_plugins_handler (gpointer data)
{
  BseServer *server = (BseServer*) data;
  SfiRing *plugins = (SfiRing*) g_object_get_data ((GObject*) server, "plugin-registration-queue");
  const gchar *error;

  if (plugins)
    {
      char *name = (char*) sfi_ring_pop_head (&plugins);
      g_object_set_data ((GObject*) server, "plugin-registration-queue", plugins);
      error = bse_plugin_check_load (name);
      bse_server_registration (server, BSE_REGISTER_PLUGIN, name, error);
      g_free (name);
    }
  else
    {
      bse_server_registration (server, BSE_REGISTER_DONE, NULL, NULL);
      return FALSE;
    }
  return TRUE;
}

void
ServerImpl::register_core_plugins ()
{
  static gboolean registration_done = false;
  BseServer *server = as<BseServer*>();
  if (registration_done)
    {
      bse_server_registration (server, BSE_REGISTER_DONE, NULL, NULL);
      return;
    }
  SfiRing *ring = bse_plugin_path_list_files (!bse_main_args->load_drivers_early, TRUE);
  g_object_set_data (server, "plugin-registration-queue", ring);
  bse_idle_normal (register_core_plugins_handler, server);
  registration_done = true;
}

void
ServerImpl::start_recording (const String &wave_file, double n_seconds)
{
  BseServer *server = as<BseServer*>();
  bse_server_start_recording (server, wave_file.c_str(), n_seconds);
}

struct ScriptRegistration
{
  gchar         *script;
  Bse::ErrorType (*register_func) (const gchar *script, BseJanitor **janitor_p);
  ScriptRegistration *next;
};

static gboolean	register_scripts_handler (gpointer data);

static void
script_janitor_closed (BseJanitor *janitor,
		       BseServer  *server)
{
  bse_server_registration (server, BSE_REGISTER_SCRIPT, janitor->script_name, NULL);
  bse_idle_normal (register_scripts_handler, server);
}

static gboolean
register_scripts_handler (gpointer data)
{
  BseServer *server = (BseServer*) data;
  ScriptRegistration *scr = (ScriptRegistration*) g_object_get_data ((GObject*) server, "script-registration-queue");
  BseJanitor *janitor = NULL;
  Bse::ErrorType error;

  if (!scr)
    {
      bse_server_registration (server, BSE_REGISTER_DONE, NULL, NULL);
      return FALSE;
    }
  g_object_set_data ((GObject*) server, "script-registration-queue", scr->next);

  error = scr->register_func (scr->script, &janitor);
  if (!janitor)
    bse_server_registration (server, BSE_REGISTER_SCRIPT, scr->script, bse_error_blurb (error));
  else
    g_object_connect (janitor, "signal::shutdown", script_janitor_closed, server, NULL);
  g_free (scr->script);
  g_free (scr);
  return !janitor;
}

void
ServerImpl::register_scripts ()
{
  static gboolean registration_done = false;
  BseServer *server = as<BseServer*>();

  if (registration_done)
    {
      bse_server_registration (server, BSE_REGISTER_DONE, NULL, NULL);
      return;
    }
  registration_done = true;

  SfiRing *ring = bse_script_path_list_files ();
  ScriptRegistration *scr_list = NULL;
  while (ring)
    {
      ScriptRegistration *scr = g_new0 (ScriptRegistration, 1);
      scr->script = (char*) sfi_ring_pop_head (&ring);
      scr->register_func = bse_script_file_register;
      scr->next = scr_list;
      scr_list = scr;
    }

  g_object_set_data (server, "script-registration-queue", scr_list);
  bse_idle_normal (register_scripts_handler, server);
}

bool
ServerImpl::preferences_locked ()
{
  return bse_gconfig_locked();
}

int
ServerImpl::n_scripts()
{
  BseServer *server = as<BseServer*>();
  // count script controls
  uint n_scripts = 0;
  for (GSList *slist = server->children; slist; slist = slist->next)
    if (BSE_IS_JANITOR (slist->data))
      n_scripts++;
  return n_scripts;
}

bool
ServerImpl::can_load (const String &file_name)
{
  // find a loader
  BseWaveFileInfo *finfo = bse_wave_file_info_load (file_name.c_str(), NULL);
  if (finfo)
    bse_wave_file_info_unref (finfo);
  return finfo != NULL;
}

static void
release_project (BseProject *project, BseServer *server)
{
  server->projects = g_list_remove (server->projects, project);
  bse_item_unuse (project);
}

ProjectIfaceP
ServerImpl::create_project (const String &project_name)
{
  BseServer *server = as<BseServer*>();
  /* enforce unique name */
  guint num = 1;
  gchar *uname = g_strdup (project_name.c_str());
  while (bse_server_find_project (server, uname))
    {
      g_free (uname);
      uname = g_strdup_format ("%s-%u", project_name.c_str(), num++);
    }
  /* create project */
  BseProject *project = (BseProject*) bse_object_new (BSE_TYPE_PROJECT, "uname", uname, NULL);
  bse_item_use (project);
  server->projects = g_list_prepend (server->projects, project);
  g_object_unref (project);
  g_free (uname);
  g_object_connect (project,
		    "signal::release", release_project, server,
		    NULL);
  return project->as<ProjectIfaceP>();
}

void
ServerImpl::destroy_project (ProjectIface &project_iface)
{
  BseServer *server = as<BseServer*>();
  BseProject *project = project_iface.as<BseProject*>();
  if (g_list_find (server->projects, project))
    g_object_run_dispose (project);
  else
    critical ("%s: project not found", __func__);
}

struct AuxDataAndIcon : AuxData {
  Icon icon;
};

static std::vector<AuxDataAndIcon> registered_module_types;

/// Register a synthesis module type at program startup.
void
ServerImpl::register_source_module (const String &type, const String &title, const String &tags, const uint8 *pixstream)
{
  assert_return (type.empty() == false);
  registered_module_types.push_back (AuxDataAndIcon());
  AuxDataAndIcon &ad = registered_module_types.back();
  ad.entity = type;
  if (!title.empty())
    ad.attributes.push_back ("title=" + title);
  if (!tags.empty())
    ad.attributes.push_back ("tags=" + tags);
  if (pixstream)
    ad.icon = icon_from_pixstream (pixstream);
  GType gtypeid = g_type_from_name (type.c_str());
  if (gtypeid)
    {
      const char *txt;
      txt = bse_type_get_blurb (gtypeid);
      if (txt)
        ad.attributes.push_back ("blurb=" + String (txt));
      txt = bse_type_get_authors (gtypeid);
      if (txt)
        ad.attributes.push_back ("authors=" + String (txt));
      txt = bse_type_get_license (gtypeid);
      if (txt)
        ad.attributes.push_back ("license=" + String (txt));
      txt = bse_type_get_options (gtypeid);
      if (txt)
        ad.attributes.push_back ("hints=" + String (txt));
    }
  if (0)
    printerr ("%s\n  %s\n  tags=%s\n  icon=...%s\n",
              type,
              title,
              Rapicorn::string_join (", ", Rapicorn::string_split_any (tags, ":;")),
              ad.icon.width && ad.icon.height ? string_format ("%ux%u", ad.icon.width, ad.icon.height) : "0");
}

AuxDataSeq
ServerImpl::list_module_types ()
{
  AuxDataSeq ads;
  ads.insert (ads.end(), registered_module_types.begin(), registered_module_types.end());
  return ads;
}

AuxData
ServerImpl::find_module_type (const String &module_type)
{
  for (const auto &ad : registered_module_types)
    if (ad.entity == module_type)
      return ad;
  return AuxData();
}

Icon
ServerImpl::module_type_icon (const String &module_type)
{
  for (const AuxDataAndIcon &ad : registered_module_types)
    if (ad.entity == module_type)
      return ad.icon;
  return Icon();
}

SampleFileInfo
ServerImpl::sample_file_info (const String &filename)
{
  SampleFileInfo info;
  info.file = filename;
  struct stat sbuf = { 0, };
  if (stat (filename.c_str(), &sbuf) < 0)
    info.error = bse_error_from_errno (errno, Bse::ERROR_FILE_OPEN_FAILED);
  else
    {
      info.size = sbuf.st_size;
      info.mtime = sbuf.st_mtime * SFI_USEC_FACTOR;
      BseWaveFileInfo *wfi = bse_wave_file_info_load (filename.c_str(), &info.error);
      if (wfi)
	{
	  for (size_t i = 0; i < wfi->n_waves; i++)
	    info.waves.push_back (wfi->waves[i].name);
	  info.loader = bse_wave_file_info_loader (wfi);
          bse_wave_file_info_unref (wfi);
        }
    }
  return info;
}

NoteDescription
ServerImpl::note_describe_from_freq (MusicalTuningType musical_tuning, double freq)
{
  const int note = bse_note_from_freq (musical_tuning, freq);
  return bse_note_description (musical_tuning, note, 0);
}

NoteDescription
ServerImpl::note_describe (MusicalTuningType musical_tuning, int note, int fine_tune)
{
  return bse_note_description (musical_tuning, note, fine_tune);
}

NoteDescription
ServerImpl::note_construct (MusicalTuningType musical_tuning, int semitone, int octave, int fine_tune)
{
  const int note = BSE_NOTE_GENERIC (octave, semitone);
  return bse_note_description (musical_tuning, note, fine_tune);
}

NoteDescription
ServerImpl::note_from_string (MusicalTuningType musical_tuning, const String &name)
{
  const int note = bse_note_from_string (name);
  return bse_note_description (musical_tuning, note, 0);
}

int
ServerImpl::note_from_freq (MusicalTuningType musical_tuning, double frequency)
{
  return bse_note_from_freq (musical_tuning, frequency);
}

double
ServerImpl::note_to_freq (MusicalTuningType musical_tuning, int note, int fine_tune)
{
  const Bse::NoteDescription info = bse_note_description (musical_tuning, note, fine_tune);
  return info.name.empty() ? 0 : info.freq;
}

} // Bse
