// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseserver.hh"
#include "bseproject.hh"
#include "bseengine.hh"
#include "gslcommon.hh"
#include "bsemain.hh"		/* threads enter/leave */
#include "bsepcmwriter.hh"
#include "bsecxxplugin.hh"
#include "gsldatahandle-mad.hh"
#include "gslvorbis-enc.hh"
#include "bseladspa.hh"
#include "devicecrawler.hh"
#include "storage.hh"
#include "bse/internal.hh"
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "bsepcmmodule.cc"

using namespace Bse;

/* --- prototypes --- */
static void	bse_server_class_init		(BseServerClass	   *klass);
static void	bse_server_init			(BseServer	   *server);
static void	bse_server_singleton_finalize	(GObject	   *object);
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
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (klass);

  parent_class = (GTypeClass*) g_type_class_peek_parent (klass);

  gobject_class->set_property = bse_server_set_property;
  gobject_class->get_property = bse_server_get_property;
  gobject_class->finalize = bse_server_singleton_finalize;

  item_class->set_parent = bse_server_set_parent;

  container_class->add_item = bse_server_add_item;
  container_class->remove_item = bse_server_remove_item;
  container_class->forall_items = bse_server_forall_items;
  container_class->release_children = bse_server_release_children;
}

static void
bse_server_init (BseServer *self)
{
  assert_return (BSE_OBJECT_ID (self) == 1);	/* assert being the first object */
  self->set_flag (BSE_ITEM_FLAG_SINGLETON);

  self->engine_source = NULL;
  self->dev_use_count = 0;
  self->pcm_imodule = NULL;
  self->pcm_omodule = NULL;
  self->pcm_writer = NULL;

  /* keep the server singleton alive */
  bse_item_use (BSE_ITEM (self));

  /* start dispatching main thread stuff */
  main_thread_source_setup (self);
}

static void
bse_server_singleton_finalize (GObject *object)
{
  assert_return_unreached();

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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_server_set_parent (BseItem *item,
		       BseItem *parent)
{
  Bse::warning ("%s: BseServer is a global singleton that cannot be added to a container", G_STRLOC);
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

  Bse::warning ("release_children() should never be triggered on BseServer singleton");

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
      assert_return (server, NULL);
      assert_return (server->cxxobject_, NULL);
      assert_return (dynamic_cast<Bse::LegacyObjectImpl*> (server->cxxobject_), NULL);
      assert_return (dynamic_cast<Bse::ServerImpl*> (server->cxxobject_), NULL);
    }

  return server;
}

void
bse_server_stop_recording (BseServer *self)
{
  for (auto project : ProjectImpl::project_list())
    project->stop_playback();
  self->wave_seconds = 0;
  g_free (self->wave_file);
  self->wave_file = NULL;
  auto impl = self->as<Bse::ServerImpl*>();
  impl->notify ("wave_file");
}

void
bse_server_start_recording (BseServer      *self,
                            const char     *wave_file,
                            double          n_seconds)
{
  self->wave_seconds = MAX (n_seconds, 0);
  self->wave_file = g_strdup_stripped (wave_file ? wave_file : "");
  if (!self->wave_file[0])
    {
      g_free (self->wave_file);
      self->wave_file = NULL;
    }
  auto impl = self->as<Bse::ServerImpl*>();
  impl->notify ("wave_file");
}

Bse::Error
bse_server_open_devices (BseServer *self)
{
  auto impl = self->as<Bse::ServerImpl*>();
  Bse::Error error = Bse::Error::NONE;
  assert_return (BSE_IS_SERVER (self), Bse::Error::INTERNAL);
  /* check whether devices are already opened */
  if (self->dev_use_count)
    {
      self->dev_use_count++;
      return Bse::Error::NONE;
    }
  /* lock playback/capture/latency settings */
  Bse::global_config->lock();
  /* calculate block_size for pcm setup */
  const uint block_size = bse_engine_block_size();
  const uint latency = Bse::global_config->synth_latency;
  const uint mix_freq = bse_engine_sample_freq();
  /* try opening devices */
  if (error == 0)
    error = impl->open_pcm_driver (mix_freq, latency, block_size);
  if (error == 0)
    error = impl->open_midi_driver();
  if (error == 0)
    {
      BseTrans *trans = bse_trans_open ();
      engine_init (self, impl->pcm_driver()->pcm_frequency());
      self->pcm_imodule = bse_pcm_imodule_insert (impl->pcm_driver().get(), trans);
      if (self->wave_file)
	{
	  Bse::Error error;
	  self->pcm_writer = (BsePcmWriter*) bse_object_new (BSE_TYPE_PCM_WRITER, NULL);
          const uint n_channels = 2;
	  error = bse_pcm_writer_open (self->pcm_writer, self->wave_file,
                                       n_channels, bse_engine_sample_freq (),
                                       n_channels * bse_engine_sample_freq() * self->wave_seconds);
	  if (error != 0)
	    {
              UserMessage umsg;
              umsg.utype = Bse::UserMessageType::ERROR;
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
      self->pcm_omodule = bse_pcm_omodule_insert (impl->pcm_driver().get(), self->pcm_writer, trans);
      bse_trans_commit (trans);
      self->dev_use_count++;
      ServerImpl::instance().enginechange (true);
    }
  else
    {
      impl->close_pcm_driver();
      impl->close_midi_driver();
    }
  Bse::global_config->unlock();
  return error;
}

void
bse_server_shutdown (BseServer *self)
{
  assert_return (BSE_IS_SERVER (self));
  // projects in playback can hold an open device use count
  for (auto project : ProjectImpl::project_list())
    {
      project->stop_playback();
      project->deactivate();
    }
  while (self->dev_use_count)
    bse_server_close_devices (self);
  bse_engine_shutdown();
}

void
bse_server_close_devices (BseServer *self)
{
  assert_return (BSE_IS_SERVER (self));
  assert_return (self->dev_use_count > 0);
  auto impl = self->as<Bse::ServerImpl*>();

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
      impl->close_pcm_driver();
      impl->close_midi_driver();
      engine_shutdown (self);
      ServerImpl::instance().enginechange (false);
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
    Bse::warning (G_STRLOC ": no such io watch installed %p(%p)", watch_func, data);
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

  assert_return (single_call++ == 0);

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

  Bse::global_config->lock();
  server->engine_source = g_source_new (&engine_gsource_funcs, sizeof (PSource));
  g_source_set_priority (server->engine_source, BSE_PRIORITY_HIGH);

  if (!engine_is_initialized)
    {
      engine_is_initialized = true;
      bse_engine_init();
      // lower priority compared to engine if our priority range permits
      const int mytid = Bse::this_thread_gettid();
      int current_priority = getpriority (PRIO_PROCESS, mytid);
      if (current_priority <= -2 && mytid)
        setpriority (PRIO_PROCESS, mytid, current_priority + 1);
    }
  bse_engine_configure (Bse::global_config->synth_latency, mix_freq, Bse::global_config->synth_control_freq);

  g_source_attach (server->engine_source, bse_main_context);
}

static void
engine_shutdown (BseServer *server)
{
  assert_return (server->engine_source != NULL);

  g_source_destroy (server->engine_source);
  server->engine_source = NULL;
  bse_engine_user_thread_collect ();
  Bse::global_config->unlock();
}


namespace Bse {

ServerImpl::ServerImpl (BseObject *bobj) :
  ContainerImpl (bobj)
{}

ServerImpl::~ServerImpl ()
{
  close_pcm_driver();
  close_midi_driver();
}

bool
ServerImpl::log_messages() const
{
  return log_messages_;
}

void
ServerImpl::log_messages (bool val)
{
  APPLY_IDL_PROPERTY (log_messages_, val);
}

String
ServerImpl::wave_file() const
{
  BseServer *self = const_cast<ServerImpl*> (this)->as<BseServer*>();

  return self->wave_file ? self->wave_file : "";
}

void
ServerImpl::wave_file (const String& filename)
{
  BseServer *self = as<BseServer*>();

  bse_server_start_recording (self, filename.c_str(), 0);
}

void
ServerImpl::enginechange (bool active)
{
  using namespace Aida::KeyValueArgs;
  emit_event ("enginechange", "active"_v = active);
}

bool
ServerImpl::engine_active ()
{
  BseServer *self = as<BseServer*>();
  return self->dev_use_count;
}

LegacyObjectIfaceP
ServerImpl::from_proxy (int64_t proxyid)
{
  BseObject *bo = bse_object_from_id (proxyid);
  if (!bo)
    return LegacyObjectIfaceP();
  return bo->as<LegacyObjectIfaceP>();
}

ServerImpl&
ServerImpl::instance()
{
  return *bse_server_get()->as<ServerImpl*>();
}

void
ServerImpl::send_user_message (const UserMessage &umsg)
{
  std::string out;
  out += Aida::enum_value_to_short_string (umsg.utype) + ": ";  // Severity classification for this message.
  if (!umsg.title.empty())
    out += umsg.title + " — ";                                  // Usually GUI window title.
  if (!umsg.text1.empty())
    out += umsg.text1 + "\n";                                   // Primary message to the user, should be limited to 80-100 chars.
  if (!umsg.text2.empty())
    out += "  " + umsg.text2 + "\n";                            // Explanatory (secondary) message no limitations recommended.
  if (!umsg.text3.empty())
    out += "  " + umsg.text3 + "\n";                            // Possibly (technical) details or machine error message.
  if (!umsg.label.empty())
    out += "  (" + umsg.label + ")\n";                          // Message class label, used to enable/disable this type of message.
  if (out.empty() || out[out.size() -1] != '\n')
    out += "\n";
  Bse::printerr ("BSE:UserMessage:%s", out);
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
  return Bse::runpath (Bse::RPath::LADSPADIRS);
}

String
ServerImpl::get_plugin_path ()
{
  return Path::searchpath_join (Bse::runpath (Bse::RPath::PLUGINDIR), Bse::global_config->plugin_path);
}

String
ServerImpl::get_instrument_path ()
{
  return Path::searchpath_join (Bse::runpath (Bse::RPath::INSTRUMENTDIR), Bse::global_config->instrument_path);
}

String
ServerImpl::get_sample_path ()
{
  return Path::searchpath_join (Bse::runpath (Bse::RPath::SAMPLEDIR), Bse::global_config->sample_path);
}

String
ServerImpl::get_effect_path ()
{
  return Path::searchpath_join (Bse::runpath (Bse::RPath::EFFECTDIR), Bse::global_config->effect_path);
}

String
ServerImpl::get_demo_path ()
{
  return Bse::runpath (Bse::RPath::DEMODIR);
}

String
ServerImpl::get_version ()
{
  return Bse::version();
}

String
ServerImpl::get_custom_effect_dir ()
{
  StringVector strings = string_split (Bse::global_config->effect_path, G_SEARCHPATH_SEPARATOR_S);
  return strings.size() ? strings[0] : "";
}

String
ServerImpl::get_custom_instrument_dir ()
{
  StringVector strings = string_split (Bse::global_config->instrument_path, G_SEARCHPATH_SEPARATOR_S);
  return strings.size() ? strings[0] : "";
}

void
ServerImpl::purge_stale_cachedirs ()
{
  beastbse_cachedir_cleanup();
}

void
ServerImpl::load_assets ()
{
  static bool done_once = false;
  return_unless (!done_once);
  done_once = true;
  SfiRing *ring;
  // load Bse drivers & plugins
  ring = bse_plugin_path_list_files (true, true);
  while (ring)
    {
      gchar *name = (char*) sfi_ring_pop_head (&ring);
      const char *error = bse_plugin_check_load (name);
      if (error)
        printerr ("%s: Bse plugin registration failed: %s\n", name, error);
      g_free (name);
    }
}

void
ServerImpl::load_ladspa ()
{
  static bool done_once = false;
  return_unless (!done_once);
  done_once = true;
  // load LADSPA plugins
  for (const std::string &ladspa_so : bse_ladspa_plugin_path_list_files ())
    {
      const char *error = bse_ladspa_plugin_check_load (ladspa_so.c_str());
      if (error)
        printerr ("%s: Bse LADSPA plugin registration failed: %s\n", ladspa_so, error);
    }
}

void
ServerImpl::start_recording (const String &wave_file, double n_seconds)
{
  BseServer *server = as<BseServer*>();
  bse_server_start_recording (server, wave_file.c_str(), n_seconds);
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

ProjectIfaceP
ServerImpl::create_project (const String &project_name)
{
  BseProject *project = (BseProject*) bse_object_new (BSE_TYPE_PROJECT, "uname", project_name.c_str(), NULL);
  return project->as<ProjectIfaceP>();
}

ProjectIfaceP
ServerImpl::last_project ()
{
  auto projectlist = ProjectImpl::project_list();
  return !projectlist.empty() ? projectlist.back()->as<ProjectIfaceP>() : nullptr;
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
              Bse::string_join (", ", Bse::string_split_any (tags, ":;")),
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
    info.error = bse_error_from_errno (errno, Bse::Error::FILE_OPEN_FAILED);
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

Configuration
ServerImpl::get_config_defaults ()
{
  return global_config->defaults();
}

Configuration
ServerImpl::get_config ()
{
  return *global_config;
}

void
ServerImpl::set_config (const Configuration &configuration)
{
  global_config->assign (configuration);
}

bool
ServerImpl::locked_config()
{
  return global_config->locked();
}

NoteDescription
ServerImpl::note_describe_from_freq (MusicalTuning musical_tuning, double freq)
{
  const int note = bse_note_from_freq (musical_tuning, freq);
  return bse_note_description (musical_tuning, note, 0);
}

NoteDescription
ServerImpl::note_describe (MusicalTuning musical_tuning, int note, int fine_tune)
{
  return bse_note_description (musical_tuning, note, fine_tune);
}

NoteDescription
ServerImpl::note_construct (MusicalTuning musical_tuning, int semitone, int octave, int fine_tune)
{
  const int note = BSE_NOTE_GENERIC (octave, semitone);
  return bse_note_description (musical_tuning, note, fine_tune);
}

NoteDescription
ServerImpl::note_from_string (MusicalTuning musical_tuning, const String &name)
{
  const int note = bse_note_from_string (name);
  return bse_note_description (musical_tuning, note, 0);
}

int
ServerImpl::note_from_freq (MusicalTuning musical_tuning, double frequency)
{
  return bse_note_from_freq (musical_tuning, frequency);
}

double
ServerImpl::note_to_freq (MusicalTuning musical_tuning, int note, int fine_tune)
{
  const Bse::NoteDescription info = bse_note_description (musical_tuning, note, fine_tune);
  return info.name.empty() ? 0 : info.freq;
}

CategorySeq
ServerImpl::category_match_typed (const String &pattern, const String &type_name)
{
  return bse_categories_match_typed (pattern, g_type_from_name (type_name.c_str()));
}

CategorySeq
ServerImpl::category_match (const String &pattern)
{
  return bse_categories_match_typed (pattern, 0);
}

int64
ServerImpl::tick_stamp_from_systime (int64 systime_usecs)
{
  return bse_engine_tick_stamp_from_systime (systime_usecs);
}

struct FragmentBroadcaster {
  using BinarySender = IpcHandler::BinarySender;
  BinarySender   binary_sender;
  size_t         binary_size = 0;
  ptrdiff_t      conid = 0;
  ShmFragmentSeq plan;
  SharedMemory   smem;
  uint           timerid = 0;
};
static std::vector<FragmentBroadcaster> fbroadcasters;

static void
broadcast_timer (ptrdiff_t conid)
{
  size_t i;
  for (i = 0; i < fbroadcasters.size(); ++i)    // size() is usually 1
    if (conid == fbroadcasters[i].conid)
      break;
  if (i >= fbroadcasters.size())
    return;                                     // should not happen
  FragmentBroadcaster &broad = fbroadcasters[i];
  const SharedMemory &smem = broad.smem;
  const char *shm_start = (char*) smem.shm_start;
  std::string binary;
  binary.resize (broad.binary_size);
  char *data = &binary[0];
  for (const auto &frag : broad.plan)           // offsets and lengths were validated earlier
    memcpy (data + frag.bpos, shm_start + frag.shmoffset, frag.blength);
  const bool keep_alive = broad.binary_sender (binary);
  if (!keep_alive)                              // clear slot, disable timer
    {
      exec_handler_clear (broad.timerid);
      broad.timerid = 0;
      broad.conid = 0;
      broad.binary_sender = nullptr;
      broad.plan.clear();
      broad.smem = SharedMemory();
    }
}

static bool
validate_shm_fragments (const SharedMemory &smem, const ShmFragmentSeq &plan, size_t *lengthp)
{
  size_t length = 0;
  for (const auto &frag : plan)
    if (frag.shmoffset < 0 || frag.blength < 0 || frag.bpos < 0 ||
        size_t (frag.shmoffset) + frag.blength > smem.shm_length)
      return false;
    else
      length = std::max (length, frag.bpos + size_t (frag.blength));
  // the target buffer length does not strictly need to be smaller than the shared memory
  // area, but if it is indeed bigger, something fishy or very inefficient is going on
  if (length > smem.shm_length)
    return false;       // heuristic for overflow or out-of-bounds somewhere
  *lengthp = length;
  return true;
}

void
ServerImpl::broadcast_shm_fragments (const ShmFragmentSeq &plan, int interval_ms)
{
  SharedMemory sm = get_shared_memory();
  IpcHandler *ipch = get_ipc_handler();
  const ptrdiff_t conid = ipch ? ipch->current_connection_id() : 0;
  assert_return (conid != 0);
  size_t maxlen = 0;
  const bool valid_shm_plan = validate_shm_fragments (sm, plan, &maxlen);
  assert_return (valid_shm_plan);
  size_t i;
  for (i = 0; i < fbroadcasters.size(); ++i)    // find candidate for re-assigment
    if (fbroadcasters[i].conid == conid)
      break;
  if (i == fbroadcasters.size())                // none found, new connection plan
    {
      if (plan.empty())
        return;                                 // reset of unknown plan
      for (i = 0; i < fbroadcasters.size(); ++i)
        if (fbroadcasters[i].conid == 0)        // found empty slot
          break;
      if (i == fbroadcasters.size())
        fbroadcasters.resize (i + 1);           // or allocate new slot
      fbroadcasters[i].conid = conid;
      fbroadcasters[i].binary_sender = ipch->create_binary_sender();
    }
  FragmentBroadcaster &broad = fbroadcasters[i];   // change existing plan
  if (broad.timerid)
    exec_handler_clear (broad.timerid);
  broad.timerid = 0;
  broad.plan = plan;
  broad.binary_size = maxlen;
  if (plan.empty())                             // reset of old plan
    {
      broad.conid = 0;
      broad.binary_sender = nullptr;
      broad.plan.clear();
      broad.smem = SharedMemory();
      return;                                   // clear slot to keep indices stable
    }
  broad.smem = sm;
  std::function<bool()> broadcast_timer_func = [conid] () {
    broadcast_timer (conid);
    return true;                                // keep interval timer alive
  };
  broad.timerid = exec_timeout (broadcast_timer_func, std::max (interval_ms, 16));
}

static constexpr ssize_t SHARED_MEMORY_AREA_SIZE = 8 * 1024 * 1024;
static size_t current_shared_memory_area_size = SHARED_MEMORY_AREA_SIZE;

static FastMemory::Arena&
server_shared_memory_area()
{
  static FastMemory::Arena &shm_area = *new FastMemory::Arena { uint32 (current_shared_memory_area_size), FastMemory::cache_line_size };
  return shm_area;
}

SharedMemory
ServerImpl::get_shared_memory()
{
  const FastMemory::Arena &arena = server_shared_memory_area();
  SharedMemory sm;
  sm.shm_start = arena.location();
  sm.shm_length = arena.reserved();
  sm.shm_creator = this_thread_getpid();
  return sm;
}

size_t
ServerImpl::shared_block_offset (const void *mem) const
{
  assert_return (mem != nullptr, -1);
  FastMemory::Arena &arena = server_shared_memory_area();
  const uintptr_t addr = uintptr_t (mem);
  const uintptr_t start = arena.location();
  if (addr >= start)
    {
      const uintptr_t offset = addr - start;
      if (offset < arena.reserved() && offset == int32 (offset))
        return offset;
    }
  return -1;
}

SharedBlock
ServerImpl::allocate_shared_block (int64 length)
{
  SharedBlock sb;
  assert_return (length <= SHARED_MEMORY_AREA_SIZE, sb);
  return_unless (length > 0, sb);
  FastMemory::Arena &arena = server_shared_memory_area();
  FastMemory::Block ab = arena.allocate (length);
  if (!ab.block_start)
    {
      // Generally, this should not happen, if it does, we need to increase the area size
      // and possibly deal with fragmentation better.
      fatal_error ("Failed to allocate from shared memory (reserved=%d, used=%d, needed=%d), please report to upstream",
                   SHARED_MEMORY_AREA_SIZE, SHARED_MEMORY_AREA_SIZE - current_shared_memory_area_size, length);
    }
  const size_t prev_shared_memory_area_size = current_shared_memory_area_size;
  current_shared_memory_area_size -= ab.block_length;
  if (prev_shared_memory_area_size >= SHARED_MEMORY_AREA_SIZE / 2 &&
      current_shared_memory_area_size < SHARED_MEMORY_AREA_SIZE / 2)
    {
      // There should always be plenty of space, we need to increase the area if not
      warning ("Available shared memory dropped below 50%% (reserved=%d, used=%d, needed=%d), please report to upstream",
               SHARED_MEMORY_AREA_SIZE, SHARED_MEMORY_AREA_SIZE - current_shared_memory_area_size, length);
    }
  sb.mem_length = ab.block_length;
  sb.mem_start = ab.block_start;
  sb.mem_offset = uint64 (sb.mem_start) - arena.location();
  return sb;
}

void
ServerImpl::release_shared_block (const SharedBlock &sb)
{
  assert_return (sb.mem_length >= 0);
  FastMemory::Arena &arena = server_shared_memory_area();
  FastMemory::Block ab { sb.mem_start, uint32 (sb.mem_length) };
  arena.release (ab);
  assert_return (current_shared_memory_area_size + ab.block_length <= SHARED_MEMORY_AREA_SIZE);
  current_shared_memory_area_size += ab.block_length;
}

IpcHandler::~IpcHandler()
{}

static IpcHandler *server_ipc_handler = nullptr;

void
ServerImpl::set_ipc_handler (IpcHandler *ipch)
{
  server_ipc_handler = ipch;
}

IpcHandler*
ServerImpl::get_ipc_handler ()
{
  return server_ipc_handler;
}

void
ServerImpl::close_midi_driver()
{
  if (midi_driver_)
    {
      midi_driver_->close();
      midi_driver_ = nullptr;
    }
}

Bse::Error
ServerImpl::open_midi_driver()
{
  assert_return (midi_driver_ == nullptr, Error::INTERNAL);
  Error error = Error::UNKNOWN;
  midi_driver_ = MidiDriver::open (get_config().midi_driver, Driver::READONLY, &error);
  if (!midi_driver_)
    {
      UserMessage umsg;
      umsg.utype = Bse::UserMessageType::WARNING;
      umsg.title = _("MIDI I/O Failed");
      umsg.text1 = _("No MIDI input is available.");
      umsg.text2 = _("No available MIDI device could be found and opened successfully.");
      umsg.text3 = string_format (_("Failed to open MIDI devices: %s"), bse_error_blurb (error));
      umsg.label = _("MIDI device selections problems");
      send_user_message (umsg);
      midi_driver_ = MidiDriver::open ("null", Driver::READONLY, &error);
    }
  return midi_driver_ ? Error::NONE : error;
}

void
ServerImpl::close_pcm_driver()
{
  if (pcm_driver_)
    {
      pcm_driver_->close();
      pcm_driver_ = nullptr;
    }
}

Bse::Error
ServerImpl::open_pcm_driver (uint mix_freq, uint latency, uint block_size)
{
  assert_return (pcm_driver_ == nullptr, Error::INTERNAL);
  Error error = Error::UNKNOWN;
  PcmDriverConfig config;
  config.n_channels = 2;
  config.mix_freq = mix_freq;
  config.latency_ms = latency;
  config.block_length = block_size;
  pcm_driver_ = PcmDriver::open (get_config().pcm_driver, Driver::READWRITE, Driver::WRITEONLY, config, &error);
  if (!pcm_driver_)
    {
      UserMessage umsg;
      umsg.utype = Bse::UserMessageType::ERROR;
      umsg.title = _("Audio I/O Failed");
      umsg.text1 = _("No available audio device was found.");
      umsg.text2 = _("No available audio device could be found and opened successfully. "
                     "Sorry, no fallback selection can be made for audio devices, giving up.");
      umsg.text3 = string_format (_("Failed to open PCM devices: %s"), bse_error_blurb (error));
      umsg.label = _("PCM device selections problems");
      send_user_message (umsg);
    }
  pcm_input_checked_ = false;
  return pcm_driver_ ? Error::NONE : error;
}

void
ServerImpl::require_pcm_input()
{
  if (pcm_driver_ && !pcm_input_checked_)
    {
      pcm_input_checked_ = true;
      if (!pcm_driver_->readable())
        {
          UserMessage umsg;
          umsg.utype = Bse::UserMessageType::WARNING;
          umsg.title = _("Audio Recording Failed");
          umsg.text1 = _("Failed to start recording from audio device.");
          umsg.text2 = _("An audio project is in use which processes an audio input signal, but the audio device "
                         "has not been opened in recording mode. "
                         "An audio signal of silence will be used instead of a recorded signal, "
                         "so playback operation may produce results not actually intended "
                         "(such as a silent output signal).");
          umsg.text3 = string_format (_("PCM audio device is not open for input: %s"), pcm_driver_->devid());
          umsg.label = _("audio input problems");
          send_user_message (umsg);
        }
    }
}

DriverEntrySeq
ServerImpl::list_pcm_drivers ()
{
  return PcmDriver::list_drivers();
}

DriverEntrySeq
ServerImpl::list_midi_drivers ()
{
  return MidiDriver::list_drivers();
}

ResourceCrawlerIfaceP
ServerImpl::resource_crawler()
{
  return ResourceCrawlerImpl::instance_p();
}

int
ServerImpl::test_counter_inc_fetch ()
{
  return ++tc_;
}

int
ServerImpl::test_counter_get ()
{
  return tc_;
}

void
ServerImpl::test_counter_set (int v)
{
  tc_ = v;
}

} // Bse

// == Allocator Tests ==
namespace { // Anon
using namespace Bse;

BSE_INTEGRITY_TEST (bse_server_test_allocator);
static void
bse_server_test_allocator()
{
  const ssize_t block = 512 * 1024;
  SharedBlock sb1 = BSE_SERVER.allocate_shared_block (block);
  assert_return (sb1.mem_start);
  SharedBlock sb2 = BSE_SERVER.allocate_shared_block (block);
  assert_return (sb2.mem_start);
  SharedBlock sb3 = BSE_SERVER.allocate_shared_block (block * 2);
  assert_return (sb3.mem_start);
  // assert_return (sb3.shm_id == sb1.shm_id);     // 1mb + 1mb + 2mb barely fit into 4mb area
  BSE_SERVER.release_shared_block (sb3);
  BSE_SERVER.release_shared_block (sb2);
  sb3 = BSE_SERVER.allocate_shared_block (block * 3);
  assert_return (sb3.mem_start);
  if (0) // this triggers the 50% warning
    {
      sb2 = BSE_SERVER.allocate_shared_block (1);
      assert_return (sb2.mem_start);
      BSE_SERVER.release_shared_block (sb2);
    }
  // assert_return (sb2.shm_id != sb3.shm_id);     // but nothing else
  // assert_return (BSE_SERVER.get_shared_memory (sb2.shm_id).shm_id == sb2.shm_id); // is shared?
  // assert_return (BSE_SERVER.get_shared_memory (sb3.shm_id).shm_id == sb3.shm_id); // is shared?
  BSE_SERVER.release_shared_block (sb3);
  BSE_SERVER.release_shared_block (sb1);
  sb2 = BSE_SERVER.allocate_shared_block (4 * block);   // 50% of maximum
  assert_return (sb2.mem_start);
  BSE_SERVER.release_shared_block (sb2);
}

} // Anon
