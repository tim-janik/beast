// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseengine.hh"
#include "processor.hh"

enum
{
  BSE_PCM_MODULE_JSTREAM_LEFT,
  BSE_PCM_MODULE_JSTREAM_RIGHT,
  BSE_PCM_MODULE_N_JSTREAMS
};
enum
{
  BSE_PCM_MODULE_OSTREAM_LEFT,
  BSE_PCM_MODULE_OSTREAM_RIGHT,
  BSE_PCM_MODULE_N_OSTREAMS
};
static constexpr const auto MAIN_OBUS = Bse::AudioSignal::OBusId (1);

static_assert (Bse::AudioSignal::MAX_RENDER_BLOCK_SIZE == BSE_ENGINE_MAX_BLOCK_SIZE);

// == BsePCMModuleData ==
struct BsePCMModuleData {
  const uint      max_values = 0; // BSE_ENGINE_MAX_BLOCK_SIZE * 2 (stereo)
  float          *const buffer = nullptr;
  float          *const bound = nullptr;
  Bse::PcmDriver *pcm_driver = nullptr;
  BsePcmWriter   *pcm_writer = nullptr;
  bool            pcm_input_checked = false;
  Bse::AudioSignal::Engine *engine = nullptr;
  std::vector<Bse::AudioSignal::ProcessorP> procs;
  explicit BsePCMModuleData (uint nv);
  ~BsePCMModuleData();
};

BsePCMModuleData::BsePCMModuleData (uint nv) :
  max_values (nv), buffer (new float[max_values] ()), bound (buffer + max_values)
{}

BsePCMModuleData::~BsePCMModuleData()
{
  delete[] buffer;
}

static void
bse_pcm_module_set_processor_engine (BseModule *module, Bse::AudioSignal::Engine *engine)
{
  BsePCMModuleData *mdata = (BsePCMModuleData*) module->user_data;
  assert_return (mdata != nullptr);
  assert_return (mdata->procs.size() == 0);
  auto padd = [mdata, engine] () { // keeps ProcessorP alive until lambda destruction in UserThread
    mdata->engine = engine;
  };
  BseTrans *trans = bse_trans_open ();
  bse_trans_add (trans, bse_job_access (module, padd));
  bse_trans_commit (trans);
}

static void
bse_pcm_module_add_proc (BseModule *module, Bse::AudioSignal::ProcessorP procp)
{
  assert_return (procp != nullptr);
  assert_return (procp->n_obuses() >= 1);
  BsePCMModuleData *mdata = (BsePCMModuleData*) module->user_data;
  assert_return (mdata->engine != nullptr);
  auto padd = [procp, mdata] () { // keeps ProcessorP alive until lambda destruction in UserThread
    mdata->engine->add_root (procp);
    mdata->procs.push_back (procp);
  };
  BseTrans *trans = bse_trans_open ();
  bse_trans_add (trans, bse_job_access (module, padd));
  bse_trans_commit (trans);
}

static void
bse_pcm_module_del_proc (BseModule *module, Bse::AudioSignal::ProcessorP procp)
{
  assert_return (procp != nullptr);
  BsePCMModuleData *mdata = (BsePCMModuleData*) module->user_data;
  assert_return (mdata->engine != nullptr);
  auto pdel = [procp, mdata] () { // keeps ProcessorP alive until lambda destruction in UserThread
    const bool audiosignal_processor_deleted = Bse::vector_erase_element (mdata->procs, procp);
    assert_return (audiosignal_processor_deleted == true);
    const bool audiosignal_processor_unrooted = mdata->engine->del_root (procp);
    assert_return (audiosignal_processor_unrooted == true);
  };
  BseTrans *trans = bse_trans_open ();
  bse_trans_add (trans, bse_job_access (module, pdel));
  bse_trans_commit (trans);
}

static gboolean
bse_pcm_module_poll (gpointer       data,
		     guint          n_values,
		     glong         *timeout_p,
		     guint          n_fds,
		     const GPollFD *fds,
		     gboolean       revents_filled)
{
  BsePCMModuleData *mdata = (BsePCMModuleData*) data;
  return mdata->pcm_driver ? mdata->pcm_driver->pcm_check_io (timeout_p) : false;
}

static void
bse_pcm_omodule_process (BseModule *module,
			 guint      n_values)
{
  BsePCMModuleData *mdata = (BsePCMModuleData*) module->user_data;
  gfloat *d = mdata->buffer;
  gfloat *b = mdata->bound;
  const gfloat *src;
  guint i;

  assert_return (n_values <= mdata->max_values / BSE_PCM_MODULE_N_JSTREAMS);

  Bse::AudioSignal::Engine *engine = mdata->engine;
  engine->make_schedule();
  engine->render_block();
  assert_return (n_values == Bse::AudioSignal::MAX_RENDER_BLOCK_SIZE);

  if (BSE_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_LEFT).n_connections)
    src = BSE_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_LEFT, 0);
  else
    src = bse_engine_const_values (0);
  d = mdata->buffer;
  do { *d = *src++; d += 2; } while (d < b);
  for (i = 1; i < BSE_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_LEFT).n_connections; i++)
    {
      src = BSE_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_LEFT, i);
      d = mdata->buffer;
      do { *d += *src++; d += 2; } while (d < b);
    }
  for (auto p : mdata->procs)
    if (p->n_ochannels (MAIN_OBUS) >= 1)
      {
        const float *src = p->ofloats (MAIN_OBUS, 0);
        d = mdata->buffer;
        do { *d += *src++; d += 2; } while (d < b);
      }

  if (BSE_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_RIGHT).n_connections)
    src = BSE_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_RIGHT, 0);
  else
    src = bse_engine_const_values (0);
  d = mdata->buffer + 1;
  do { *d = *src++; d += 2; } while (d < b);
  for (i = 1; i < BSE_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_RIGHT).n_connections; i++)
    {
      src = BSE_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_RIGHT, i);
      d = mdata->buffer + 1;
      do { *d += *src++; d += 2; } while (d < b);
    }
  for (auto p : mdata->procs)
    if (p->n_ochannels (MAIN_OBUS) >= 2)
      {
        const float *src = p->ofloats (MAIN_OBUS, 1);
        d = mdata->buffer + 1;
        do { *d += *src++; d += 2; } while (d < b);
      }

  if (mdata->pcm_driver)
    mdata->pcm_driver->pcm_write (n_values * BSE_PCM_MODULE_N_JSTREAMS, mdata->buffer);
  if (mdata->pcm_writer)
    bse_pcm_writer_write (mdata->pcm_writer, n_values * BSE_PCM_MODULE_N_JSTREAMS, mdata->buffer,
                          bse_module_tick_stamp (module));
}

static void
bse_pcm_module_data_free (gpointer        data,
			  const BseModuleClass *klass)
{
  BsePCMModuleData *mdata = (BsePCMModuleData*) data;
  assert_return (!mdata->pcm_driver && !mdata->pcm_writer);
  delete mdata;
}

static BseJob*
bse_pcm_omodule_change_driver (BseModule *module, Bse::PcmDriver *pcm_driver, BsePcmWriter *writer)
{
  if (pcm_driver)
    assert_return (pcm_driver->writable() && pcm_driver->pcm_frequency(), NULL);
  auto setter = [module, pcm_driver, writer] () {
    BsePCMModuleData *mdata = (BsePCMModuleData*) module->user_data;
    mdata->pcm_driver = pcm_driver;
    mdata->pcm_writer = writer;
  };
  return bse_job_access (module, setter);
}

static BseModule*
bse_pcm_omodule_insert (BseTrans *trans)
{
  static const BseModuleClass pcm_omodule_class = {
    0,				/* n_istreams */
    BSE_PCM_MODULE_N_JSTREAMS,	/* n_jstreams */
    0,				/* n_ostreams */
    bse_pcm_omodule_process,	/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    bse_pcm_module_data_free,	/* free */
    Bse::ModuleFlag::CHEAP,	/* cost */
  };

  assert_return (trans != NULL, NULL);

  BsePCMModuleData *mdata = new BsePCMModuleData (BSE_ENGINE_MAX_BLOCK_SIZE * BSE_PCM_MODULE_N_JSTREAMS);
  BseModule *module = bse_module_new (&pcm_omodule_class, mdata);

  bse_trans_add (trans,
		 bse_job_integrate (module));
  bse_trans_add (trans,
		 bse_job_set_consumer (module, TRUE));
  bse_trans_add (trans,
		 bse_job_add_poll (bse_pcm_module_poll, mdata, NULL, 0, NULL));

  return module;
}

static void
bse_pcm_omodule_remove (BseModule *pcm_module,
			BseTrans  *trans)
{
  assert_return (pcm_module != NULL);
  assert_return (trans != NULL);

  BsePCMModuleData *mdata = (BsePCMModuleData*) pcm_module->user_data;
  bse_trans_add (trans,
		 bse_job_remove_poll (bse_pcm_module_poll, mdata));
  bse_trans_add (trans,
		 bse_job_discard (pcm_module));
}

static gboolean
pcm_imodule_check_input (gpointer data)         /* UserThread */
{
  Bse::ServerImpl::instance().require_pcm_input();
  return false;
}

static void
bse_pcm_imodule_reset (BseModule *module)       /* EngineThread */
{
  BsePCMModuleData *mdata = (BsePCMModuleData*) module->user_data;
  if (!mdata->pcm_input_checked)
    {
      mdata->pcm_input_checked = TRUE;
      /* queue a job into the BSE core for immediate execution */
      bse_idle_now (pcm_imodule_check_input, NULL);
    }
}

static void
bse_pcm_imodule_process (BseModule *module,     /* EngineThread */
			 guint      n_values)
{
  BsePCMModuleData *mdata = (BsePCMModuleData*) module->user_data;
  gfloat *left = BSE_MODULE_OBUFFER (module, BSE_PCM_MODULE_OSTREAM_LEFT);
  gfloat *right = BSE_MODULE_OBUFFER (module, BSE_PCM_MODULE_OSTREAM_RIGHT);
  gsize l;

  assert_return (n_values <= mdata->max_values / BSE_PCM_MODULE_N_OSTREAMS);

  if (mdata->pcm_driver && mdata->pcm_driver->readable())
    {
      l = mdata->pcm_driver->pcm_read (n_values * BSE_PCM_MODULE_N_OSTREAMS, mdata->buffer);
      assert_return (l == n_values * BSE_PCM_MODULE_N_OSTREAMS);
    }
  else
    memset (mdata->buffer, 0, mdata->max_values * sizeof (float));

  /* due to suspend/resume, we may be called with partial read requests */
  const gfloat *s = mdata->buffer + mdata->max_values - (n_values * BSE_PCM_MODULE_N_OSTREAMS);
  const gfloat *b = mdata->bound;
  do
    {
      *left++ = *s++;
      *right++ = *s++;
    }
  while (s < b);
}

static BseJob*
bse_pcm_imodule_change_driver (BseModule *module, Bse::PcmDriver *pcm_driver)
{
  if (pcm_driver)
    assert_return (pcm_driver->pcm_frequency(), NULL);
  auto setter = [module, pcm_driver] () {
    BsePCMModuleData *mdata = (BsePCMModuleData*) module->user_data;
    mdata->pcm_driver = pcm_driver;
  };
  return bse_job_access (module, setter);
}

static BseModule*
bse_pcm_imodule_insert (BseTrans *trans)
{
  static const BseModuleClass pcm_imodule_class = {
    0,				/* n_istreams */
    0,				/* n_jstreams */
    BSE_PCM_MODULE_N_OSTREAMS,	/* n_ostreams */
    bse_pcm_imodule_process,	/* process */
    NULL,                       /* process_defer */
    bse_pcm_imodule_reset,      /* reset */
    bse_pcm_module_data_free,	/* free */
    Bse::ModuleFlag::EXPENSIVE,		/* cost */
  };

  assert_return (trans != NULL, NULL);

  BsePCMModuleData *mdata = new BsePCMModuleData (BSE_ENGINE_MAX_BLOCK_SIZE * BSE_PCM_MODULE_N_OSTREAMS);
  BseModule *module = bse_module_new (&pcm_imodule_class, mdata);

  bse_trans_add (trans,
		 bse_job_integrate (module));

  return module;
}

static void
bse_pcm_imodule_remove (BseModule *pcm_module,
			BseTrans  *trans)
{
  assert_return (pcm_module != NULL);
  assert_return (trans != NULL);

  bse_trans_add (trans,
		 bse_job_discard (pcm_module));
}
