// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseeditablesample.hh"
#include "bsemain.hh"
#include "bsestorage.hh"
#include "gsldatahandle.hh"
#include "bse/internal.hh"

/* --- structures --- */
typedef struct _Notify Notify;
struct _Notify
{
  Notify            *next;
  BseEditableSample *esample;
};


/* --- prototypes --- */
static void	    bse_editable_sample_init		(BseEditableSample	*self);
static void	    bse_editable_sample_class_init	(BseEditableSampleClass	*klass);
static void	    bse_editable_sample_dispose		(GObject		*object);
static void	    bse_editable_sample_finalize	(GObject		*object);


/* --- variables --- */
static void          *parent_class = NULL;
static Notify        *changed_notify_list = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseEditableSample)
{
  static const GTypeInfo editable_sample_info = {
    sizeof (BseEditableSampleClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_editable_sample_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_editable */,

    sizeof (BseEditableSample),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_editable_sample_init,
  };

  assert_return (BSE_EDITABLE_SAMPLE_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT, 0);

  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseEditableSample",
				   "Editable sample type",
                                   __FILE__, __LINE__,
                                   &editable_sample_info);
}

static void
bse_editable_sample_class_init (BseEditableSampleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->dispose = bse_editable_sample_dispose;
  gobject_class->finalize = bse_editable_sample_finalize;
}

static void
bse_editable_sample_init (BseEditableSample *self)
{
  self->wchunk = NULL;
}

static void
bse_editable_sample_dispose (GObject *object)
{
  BseEditableSample *self = BSE_EDITABLE_SAMPLE (object);

  bse_editable_sample_set_wchunk (self, NULL);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_editable_sample_finalize (GObject *object)
{
  BseEditableSample *self = BSE_EDITABLE_SAMPLE (object);
  Notify *notify, *last = NULL;

  for (notify = changed_notify_list; notify; )
    {
      if (notify->esample == self)
	{
	  Notify *tmp;

	  if (last)
	    last->next = notify->next;
	  else
	    changed_notify_list = notify->next;
	  tmp = notify;
	  notify = notify->next;
	  g_free (tmp);
	}
      else
	{
	  last = notify;
	  notify = last->next;
	}
    }

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);

  assert_return (self->wchunk == NULL);
}

static gboolean
changed_notify_handler (void *editable)
{
  BSE_THREADS_ENTER ();

  while (changed_notify_list)
    {
      Notify *notify = changed_notify_list;

      changed_notify_list = notify->next;
      if (!BSE_OBJECT_DISPOSING (notify->esample))
	; // notify ("changed");
      g_free (notify);
    }

  BSE_THREADS_LEAVE ();

  return FALSE;
}

static void
changed_notify_add (BseEditableSample *self)
{
  Notify *notify;

  if (!changed_notify_list)
    bse_idle_notify (changed_notify_handler, NULL);
  for (notify = changed_notify_list; notify; notify = notify->next)
    if (notify->esample == self)
      return;
  notify = g_new (Notify, 1);
  notify->esample = self;
  notify->next = changed_notify_list;
  changed_notify_list = notify;
}

void
bse_editable_sample_set_wchunk (BseEditableSample *self,
				GslWaveChunk      *wchunk)
{
  assert_return (BSE_IS_EDITABLE_SAMPLE (self));

  if (self->wchunk)
    {
      if (self->open_count)
	gsl_wave_chunk_close (self->wchunk);
      self->open_count = 0;
      gsl_wave_chunk_unref (self->wchunk);
    }
  self->wchunk = wchunk ? gsl_wave_chunk_ref (wchunk) : NULL;
  changed_notify_add (self);
}

namespace Bse {

EditableSampleImpl::EditableSampleImpl (BseObject *bobj) :
  ItemImpl (bobj)
{}

EditableSampleImpl::~EditableSampleImpl ()
{}

void
EditableSampleImpl::close ()
{
  BseEditableSample *self = as<BseEditableSample*>();
  if (self->open_count)
    {
      self->open_count--;
      if (!self->open_count)
        gsl_wave_chunk_close (self->wchunk);
    }
}

int64
EditableSampleImpl::get_length ()
{
  BseEditableSample *self = as<BseEditableSample*>();
  GslDataCache *dcache = NULL;
  if (BSE_EDITABLE_SAMPLE_OPENED (self) && self->wchunk)
    dcache = self->wchunk->dcache;
  return dcache ? gsl_data_handle_length (dcache->dhandle) : 0;
}

int64
EditableSampleImpl::get_n_channels ()
{
  BseEditableSample *self = as<BseEditableSample*>();
  return self->wchunk ? self->wchunk->n_channels : 1;
}

double
EditableSampleImpl::get_osc_freq ()
{
  BseEditableSample *self = as<BseEditableSample*>();
  return self->wchunk ? self->wchunk->osc_freq : BSE_KAMMER_FREQUENCY;
}

Error
EditableSampleImpl::open ()
{
  BseEditableSample *self = as<BseEditableSample*>();
  Error error;
  if (!self->wchunk)
    error = Error::WAVE_NOT_FOUND;
  else if (self->open_count)
    {
      self->open_count++;
      error = Error::NONE;
    }
  else
    {
      error = gsl_wave_chunk_open (self->wchunk);
      if (error == 0)
	self->open_count++;
    }
  return error;
}

FloatSeq
EditableSampleImpl::collect_stats (int64 voffset, double offset_scale, int64 block_size, int64 stepping, int64 max_pairs)
{
  BseEditableSample *self = as<BseEditableSample*>();
  GslDataCache *dcache = NULL;
  if (BSE_EDITABLE_SAMPLE_OPENED (self) && self->wchunk)
    dcache = self->wchunk->dcache;
  FloatSeq floats;
  floats.resize (max_pairs * 2, 0);
  const ssize_t dhandle_length = gsl_data_handle_length (dcache->dhandle);
  if (stepping < 1 || !dcache || voffset + block_size > dhandle_length)
    return floats;
  GslDataCacheNode *dnode = gsl_data_cache_ref_node (dcache, voffset, GSL_DATA_CACHE_DEMAND_LOAD);
  ssize_t j, dnode_length = dcache->node_size;
  for (j = 0; j < max_pairs; j++)
    {
      ssize_t i, cur_offset = j * offset_scale;
      float min = +1, max = -1;
      // keep alignment across offset scaling
      cur_offset /= stepping;
      cur_offset = voffset + cur_offset * stepping;
      // collect stats for one block
      for (i = cur_offset; i < cur_offset + block_size; i += stepping)
        {
          size_t pos;
          if (i < dnode->offset || i >= dnode->offset + dnode_length)
            {
              gsl_data_cache_unref_node (dcache, dnode);
              if (i >= dhandle_length)
                dnode = NULL;
              else
                dnode = gsl_data_cache_ref_node (dcache, i, // demand_load the first block (j==0)
                                                 j == 0 ? GSL_DATA_CACHE_DEMAND_LOAD : GSL_DATA_CACHE_PEEK);
              if (!dnode)
                goto break_loops;
            }
          pos = i - dnode->offset;
          min = MIN (min, dnode->data[pos]);
          max = MAX (max, dnode->data[pos]);
        }
      floats[j * 2] = min;
      floats[j * 2 + 1] = max;
    }
  gsl_data_cache_unref_node (dcache, dnode);
 break_loops:
  floats.resize (j * 2);
  return floats;
}

} // Bse
