// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include        "bsewaverepo.hh"
#include        "bsewave.hh"


/* --- parameters --- */
enum
{
  PARAM_0,
};


/* --- prototypes --- */
static void	bse_wave_repo_class_init	(BseWaveRepoClass	*klass);
static void	bse_wave_repo_init		(BseWaveRepo		*wrepo);
static void	bse_wave_repo_dispose		(GObject		*object);
static void     bse_wave_repo_release_children  (BseContainer		*container);
static void	bse_wave_repo_set_property	(GObject                *object,
						 guint			 param_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	bse_wave_repo_get_property	(GObject                *object,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void     bse_wave_repo_add_item          (BseContainer		*container,
						 BseItem		*item);
static void     bse_wave_repo_forall_items	(BseContainer		*container,
						 BseForallItemsFunc	 func,
						 gpointer		 data);
static void     bse_wave_repo_remove_item	(BseContainer		*container,
						 BseItem		*item);


/* --- variables --- */
static GTypeClass     *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseWaveRepo)
{
  GType wave_repo_type;

  static const GTypeInfo snet_info = {
    sizeof (BseWaveRepoClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_wave_repo_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseWaveRepo),
    0,
    (GInstanceInitFunc) bse_wave_repo_init,
  };

  wave_repo_type = bse_type_register_static (BSE_TYPE_SUPER,
					     "BseWaveRepo",
					     "BSE Wave Repository",
                                             __FILE__, __LINE__,
                                             &snet_info);
  return wave_repo_type;
}

static void
bse_wave_repo_class_init (BseWaveRepoClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (klass);

  parent_class = (GTypeClass*) g_type_class_peek_parent (klass);

  gobject_class->set_property = bse_wave_repo_set_property;
  gobject_class->get_property = bse_wave_repo_get_property;
  gobject_class->dispose = bse_wave_repo_dispose;

  container_class->add_item = bse_wave_repo_add_item;
  container_class->remove_item = bse_wave_repo_remove_item;
  container_class->forall_items = bse_wave_repo_forall_items;
  container_class->release_children = bse_wave_repo_release_children;
}

static void
bse_wave_repo_init (BseWaveRepo *wrepo)
{
  wrepo->waves = NULL;
}

static void
bse_wave_repo_release_children (BseContainer *container)
{
  BseWaveRepo *wrepo = BSE_WAVE_REPO (container);

  while (wrepo->waves)
    bse_container_remove_item (container, (BseItem*) wrepo->waves->data);

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->release_children (container);
}

static void
bse_wave_repo_dispose (GObject *object)
{
  // BseWaveRepo *wrepo = BSE_WAVE_REPO (object);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_wave_repo_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  BseWaveRepo *wrepo = BSE_WAVE_REPO (object);
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wrepo, param_id, pspec);
      break;
    }
}

static void
bse_wave_repo_get_property (GObject      *object,
			    guint         param_id,
			    GValue       *value,
			    GParamSpec   *pspec)
{
  BseWaveRepo *wrepo = BSE_WAVE_REPO (object);
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wrepo, param_id, pspec);
      break;
    }
}

static void
bse_wave_repo_add_item (BseContainer *container,
			BseItem      *item)
{
  BseWaveRepo *wrepo = BSE_WAVE_REPO (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_WAVE))
    wrepo->waves = g_list_append (wrepo->waves, item);
  else
    Bse::warning ("BseWaveRepo: cannot hold non-wave item type `%s'", BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_wave_repo_forall_items (BseContainer      *container,
			    BseForallItemsFunc func,
			    gpointer           data)
{
  BseWaveRepo *wrepo = BSE_WAVE_REPO (container);
  GList *list;

  list = wrepo->waves;
  while (list)
    {
      BseItem *item;
      item = (BseItem*) list->data;
      list = list->next;
      if (!func (item, data))
	return;
    }
}

static void
bse_wave_repo_remove_item (BseContainer *container,
			   BseItem      *item)
{
  BseWaveRepo *wrepo = BSE_WAVE_REPO (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_WAVE))
    wrepo->waves = g_list_remove (wrepo->waves, item);
  else
    Bse::warning ("BseWaveRepo: cannot hold non-wave item type `%s'", BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

namespace Bse {

WaveRepoImpl::WaveRepoImpl (BseObject *bobj) :
  SuperImpl (bobj)
{}

WaveRepoImpl::~WaveRepoImpl ()
{}

static Error
repo_load_file (BseWaveRepo *wrepo, const String &file_name, BseWave **wave_p)
{
  String fname = Path::basename (file_name);
  BseWave *wave = (BseWave*) bse_object_new (BSE_TYPE_WAVE, "uname", fname.c_str(), NULL);
  Error error = bse_wave_load_wave_file (wave, file_name.c_str(), NULL, NULL, NULL, TRUE);
  if (wave->n_wchunks)
    {
      bse_container_add_item (BSE_CONTAINER (wrepo), BSE_ITEM (wave));
      *wave_p = wave;
      error = Error::NONE;
    }
  else
    {
      if (error == 0)
        error = Error::WAVE_NOT_FOUND;
      *wave_p = NULL;
    }
  g_object_unref (wave);
  return error;
}

Error
WaveRepoImpl::load_file (const String &file_name)
{
  BseWaveRepo *self = as<BseWaveRepo*>();
  BseWave *wave = NULL;
  Bse::Error error = repo_load_file (self, file_name, &wave);
  if (wave)
    {
      UndoDescriptor<WaveImpl> wave_descriptor = undo_descriptor (*wave->as<WaveImpl*>());
      auto remove_wave_lambda = [wave_descriptor] (WaveRepoImpl &self, BseUndoStack *ustack) -> Error {
        WaveImpl &wave = self.undo_resolve (wave_descriptor);
        self.remove_wave (wave);
        return Error::NONE;
      };
      push_undo (__func__, *this, remove_wave_lambda);
    }
  return error;
}

void
WaveRepoImpl::remove_wave (WaveIface &wave_iface)
{
  BseWaveRepo *self = as<BseWaveRepo*>();
  BseWave *wave = wave_iface.as<BseWave*>();
  assert_return (wave->parent == self);
  BseUndoStack *ustack = bse_item_undo_open (self, __func__);
  bse_container_uncross_undoable (self, wave);          // removes object references
  if (wave)                                             // push undo for 'remove_backedup'
    {
      UndoDescriptor<WaveImpl> wave_descriptor = undo_descriptor (*wave->as<WaveImpl*>());
      auto remove_wave_lambda = [wave_descriptor] (WaveRepoImpl &self, BseUndoStack *ustack) -> Error {
        WaveImpl &wave = self.undo_resolve (wave_descriptor);
        self.remove_wave (wave);
        return Error::NONE;
      };
      push_undo_to_redo (__func__, *this, remove_wave_lambda);
    }
  bse_container_remove_backedup (self, wave, ustack);   // removes without undo
  bse_item_undo_close (ustack);
}

} // Bse
