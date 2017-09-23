// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsestorage.hh"
#include "bseitem.hh"
#include "gsldatahandle.hh"
#include "gsldatahandle-vorbis.hh"
#include "bsedatahandle-flac.hh"
#include "gsldatautils.hh"
#include "gslcommon.hh"
#include "bseproject.hh"
#include "bsecxxplugin.hh"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>

using Bse::Flac1Handle;

/* --- macros --- */
#define parse_or_return sfi_scanner_parse_or_return
#define peek_or_return  sfi_scanner_peek_or_return

/* --- typedefs --- */
struct _BseStorageDBlock
{
  gulong         id;
  GslDataHandle *dhandle;
  guint          n_channels : 16;
  guint          needs_close : 1;
  gfloat         mix_freq;
  gfloat         osc_freq;
};
struct _BseStorageItemLink
{
  BseItem              *from_item;
  BseStorageRestoreLink restore_link;
  gpointer              data;
  guint                 pbackup;
  gchar                *upath;
  BseItem              *to_item;
  gchar                *error;
};

/* --- prototypes --- */
static void       bse_storage_init                 (BseStorage       *self);
static void       bse_storage_class_init           (BseStorageClass  *klass);
static void       bse_storage_finalize             (GObject          *object);
static void       storage_path_table_insert        (BseStorage       *self,
                                                    BseContainer     *container,
                                                    const gchar      *uname,
                                                    BseItem          *item);
static BseItem*   storage_path_table_resolve_upath (BseStorage       *self,
                                                    BseContainer     *container,
                                                    gchar            *upath);
static guint      uname_child_hash                 (gconstpointer     uc);
static gint       uname_child_equals               (gconstpointer     uc1,
                                                    gconstpointer     uc2);
static void       uname_child_free                 (gpointer          uc);
static GTokenType compat_parse_data_handle         (BseStorage       *self,
                                                    GslDataHandle   **data_handle_p,
                                                    guint            *n_channels_p,
                                                    gfloat           *mix_freq_p,
                                                    gfloat           *osc_freq_p);


/* --- variables --- */
static gpointer parent_class = NULL;
static GQuark   quark_raw_data_handle = 0;
static GQuark   quark_blob = 0;
static GQuark   quark_blob_id = 0;
static GQuark   quark_vorbis_data_handle = 0;
static GQuark   quark_flac_data_handle = 0;
static GQuark   quark_dblock_data_handle = 0;
static GQuark   quark_bse_storage_binary_v0 = 0;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseStorage)
{
  static const GTypeInfo storage_info = {
    sizeof (BseStorageClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_storage_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BseStorage),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_storage_init,
  };

  assert_return (BSE_STORAGE_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT, 0);

  return bse_type_register_static (BSE_TYPE_OBJECT, "BseStorage",
                                   "Storage object for item serialization",
                                   __FILE__, __LINE__,
                                   &storage_info);
}

static void
bse_storage_class_init (BseStorageClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  quark_raw_data_handle = g_quark_from_static_string ("raw-data-handle");
  quark_vorbis_data_handle = g_quark_from_static_string ("vorbis-data-handle");
  quark_flac_data_handle = g_quark_from_static_string ("flac-data-handle");
  quark_dblock_data_handle = g_quark_from_static_string ("dblock-data-handle");
  quark_bse_storage_binary_v0 = g_quark_from_static_string ("BseStorageBinaryV0");
  quark_blob = g_quark_from_string ("blob");
  quark_blob_id = g_quark_from_string ("blob-id");

  bse_storage_blob_clean_files(); /* FIXME: maybe better placed in bsemain.c */

  gobject_class->finalize = bse_storage_finalize;
}

static void
bse_storage_init (BseStorage *self)
{
  /* writing */
  self->wstore = NULL;
  self->stored_items = NULL;
  self->referenced_items = NULL;
  /* reading */
  self->rstore = NULL;
  self->path_table = NULL;
  self->item_links = NULL;
  self->restorable_objects = NULL;
  /* misc */
  self->dblocks = NULL;
  self->n_dblocks = 0;
  self->free_me = NULL;

  new (&self->data) BseStorage::Data();

  bse_storage_reset (self);
}

static void
bse_storage_finalize (GObject *object)
{
  BseStorage *self = BSE_STORAGE (object);

  bse_storage_reset (self);

  self->data.~Data();

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
bse_storage_turn_readable (BseStorage  *self,
                           const gchar *storage_name)
{
  BseStorageDBlock *dblocks;
  const gchar *cmem;
  gchar *text;
  guint n_dblocks, l;

  assert_return (BSE_IS_STORAGE (self));
  assert_return (BSE_STORAGE_DBLOCK_CONTAINED (self));
  assert_return (self->wstore);
  assert_return (self->wstore->flushed == FALSE);
  assert_return (self->wstore->bblocks == NULL);
  assert_return (self->free_me == NULL);

  bse_storage_break (self);

  cmem = sfi_wstore_peek_text (self->wstore, &l);
  text = (char*) g_memdup (cmem, l + 1);
  dblocks = self->dblocks;
  n_dblocks = self->n_dblocks;
  self->dblocks = NULL;
  self->n_dblocks = 0;
  auto blobs = self->data.blobs;
  self->data.blobs.clear();

  bse_storage_input_text (self, text, storage_name);
  self->free_me = text;
  self->dblocks = dblocks;
  self->n_dblocks = n_dblocks;
  self->data.blobs = blobs;
  BSE_OBJECT_SET_FLAGS (self, BSE_STORAGE_DBLOCK_CONTAINED);
}

void
bse_storage_reset (BseStorage *self)
{
  guint i;

  assert_return (BSE_IS_STORAGE (self));

  if (self->rstore)
    {
      bse_storage_finish_parsing (self);
      g_hash_table_destroy (self->path_table);
      self->path_table = NULL;
      sfi_rstore_destroy (self->rstore);
      self->rstore = NULL;
      if (self->restorable_objects)
        sfi_ppool_destroy (self->restorable_objects);
      self->restorable_objects = NULL;
    }

  if (self->wstore)
    sfi_wstore_destroy (self->wstore);
  self->wstore = NULL;
  if (self->stored_items)
    sfi_ppool_destroy (self->stored_items);
  self->stored_items = NULL;
  if (self->referenced_items)
    sfi_ppool_destroy (self->referenced_items);
  self->referenced_items = NULL;

  self->major_version = BST_MAJOR_VERSION;
  self->minor_version = BST_MINOR_VERSION;
  self->micro_version = BST_MICRO_VERSION;

  for (i = 0; i < self->n_dblocks; i++)
    {
      bse_id_free (self->dblocks[i].id);
      if (self->dblocks[i].needs_close)
        gsl_data_handle_close (self->dblocks[i].dhandle);
      gsl_data_handle_unref (self->dblocks[i].dhandle);
    }
  g_free (self->dblocks);
  self->dblocks = NULL;
  self->n_dblocks = 0;

  self->data.blobs.clear();

  g_free (self->free_me);
  self->free_me = NULL;

  BSE_OBJECT_UNSET_FLAGS (self, BSE_STORAGE_MODE_MASK);
}

static gulong
bse_storage_add_dblock (BseStorage    *self,
                        GslDataHandle *dhandle)
{
  guint i = self->n_dblocks++;
  self->dblocks = g_renew (BseStorageDBlock, self->dblocks, self->n_dblocks);
  self->dblocks[i].id = bse_id_alloc ();
  self->dblocks[i].dhandle = gsl_data_handle_ref (dhandle);
  if (GSL_DATA_HANDLE_OPENED (dhandle))
    {
      /* keep data handles opened to protect against rewrites */
      gsl_data_handle_open (dhandle);
      self->dblocks[i].needs_close = TRUE;
    }
  else
    self->dblocks[i].needs_close = FALSE;
  self->dblocks[i].n_channels = gsl_data_handle_n_channels (dhandle);
  self->dblocks[i].mix_freq = gsl_data_handle_mix_freq (dhandle);
  self->dblocks[i].osc_freq = gsl_data_handle_osc_freq (dhandle);
  return self->dblocks[i].id;
}

static gulong
bse_storage_add_blob (BseStorage       *self,
                      BseStorage::BlobP blob)
{
  self->data.blobs.push_back (blob);
  return self->data.blobs.back()->id();
}

static BseStorageDBlock*
bse_storage_get_dblock (BseStorage    *self,
                        gulong         id)
{
  guint i;
  for (i = 0; i < self->n_dblocks; i++)
    if (id == self->dblocks[i].id)
      return self->dblocks + i;
  return NULL;
}

void
bse_storage_prepare_write (BseStorage    *self,
                           BseStorageMode mode)
{
  assert_return (BSE_IS_STORAGE (self));

  bse_storage_reset (self);
  self->wstore = sfi_wstore_new ();
  self->stored_items = sfi_ppool_new ();
  self->referenced_items = sfi_ppool_new ();
  mode = BseStorageMode (mode & BSE_STORAGE_MODE_MASK);
  if (mode & BSE_STORAGE_DBLOCK_CONTAINED)
    mode = BseStorageMode (mode | BSE_STORAGE_SELF_CONTAINED);
  BSE_OBJECT_SET_FLAGS (self, mode);
  bse_storage_break (self);
  bse_storage_printf (self, "(bse-version \"%u.%u.%u\")\n\n", BST_MAJOR_VERSION, BST_MINOR_VERSION, BST_MICRO_VERSION);
}

void
bse_storage_input_text (BseStorage  *self,
                        const gchar *text,
                        const gchar *text_name)
{
  assert_return (BSE_IS_STORAGE (self));

  if (!text)
    text = "";

  bse_storage_reset (self);
  self->rstore = sfi_rstore_new ();
  self->rstore->parser_this = self;
  sfi_rstore_input_text (self->rstore, text, text_name);
  self->path_table = g_hash_table_new_full (uname_child_hash, uname_child_equals, NULL, uname_child_free);
  self->restorable_objects = sfi_ppool_new ();
}

Bse::Error
bse_storage_input_file (BseStorage  *self,
                        const gchar *file_name)
{
  assert_return (BSE_IS_STORAGE (self), Bse::Error::INTERNAL);
  assert_return (file_name != NULL, Bse::Error::INTERNAL);

  bse_storage_reset (self);
  self->rstore = sfi_rstore_new_open (file_name);
  if (!self->rstore)
    return bse_error_from_errno (errno, Bse::Error::FILE_OPEN_FAILED);
  self->rstore->parser_this = self;
  self->path_table = g_hash_table_new_full (uname_child_hash, uname_child_equals, NULL, uname_child_free);
  self->restorable_objects = sfi_ppool_new ();

  return Bse::Error::NONE;
}

static GTokenType
storage_parse_bse_version (BseStorage *self)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  gchar *vstring, *pminor, *pmicro, *ep = NULL;
  gboolean parsed_version = FALSE;
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);        /* eat bse-version */
  parse_or_return (scanner, G_TOKEN_STRING);            /* fetch "version" */
  peek_or_return (scanner, ')');                        /* check for closing paren */
  vstring = g_strdup (scanner->value.v_string);
  pminor = strchr (vstring, '.');
  pmicro = !pminor ? NULL : strchr (pminor + 1, '.');
  if (pmicro)
    {
      glong vmajor, vminor = -1, vmicro = -1;
      *pminor++ = 0;
      *pmicro++ = 0;
      vmajor = strtol (vstring, &ep, 10);
      if (!ep || *ep == 0)
        vminor = strtol (pminor, &ep, 10);
      if (!ep || *ep == 0)
        vmicro = strtol (pmicro, &ep, 10);
      if ((!ep || *ep == 0 || ep > pmicro) && vmajor >= 0 && vminor >= 0 && vmicro >= 0 &&
          BSE_VERSION_CMP (vmajor, vminor, vmicro, 0, 0, 0) > 0)
        {
          parsed_version = TRUE;
          if (BSE_VERSION_CMP (vmajor, vminor, vmicro, 0, 5, 0) >= 0)
            {
              self->major_version = vmajor;
              self->minor_version = vminor;
              self->micro_version = vmicro;
            }
        }
    }
  g_free (vstring);
  if (!parsed_version)
    bse_storage_warn (self, "ignoring invalid version string: %s", scanner->value.v_string);
  parse_or_return (scanner, ')');               /* eat closing paren */
  return G_TOKEN_NONE;
}

static BseStorageItemLink*
storage_add_item_link (BseStorage           *self,
                       BseItem              *from_item,
                       BseStorageRestoreLink restore_link,
                       gpointer              data,
                       gchar                *error)
{
  BseStorageItemLink *ilink = g_new0 (BseStorageItemLink, 1);
  self->item_links = sfi_ring_append (self->item_links, ilink);
  ilink->from_item = (BseItem*) g_object_ref (from_item);
  ilink->restore_link = restore_link;
  ilink->data = data;
  ilink->error = error;

  return ilink;
}

void
bse_storage_add_restorable (BseStorage             *self,
                            BseObject              *object)
{
  assert_return (BSE_IS_STORAGE (self));
  assert_return (self->rstore);
  assert_return (self->restorable_objects);
  assert_return (BSE_IS_OBJECT (object));
  assert_return (BSE_OBJECT_IN_RESTORE (object));

  sfi_ppool_set (self->restorable_objects, object);
}

static gboolean
storage_restorable_objects_foreach (gpointer        data,
                                    gpointer        pointer)
{
  BseStorage *self = BSE_STORAGE (data);
  BseObject *object = BSE_OBJECT (pointer);
  bse_object_restore_finish (object, self->major_version, self->minor_version, self->micro_version);
  return TRUE;
}

void
bse_storage_finish_parsing (BseStorage *self)
{
  assert_return (BSE_IS_STORAGE (self));
  assert_return (self->rstore != NULL);

  while (self->item_links)
    {
      BseStorageItemLink *ilink = (BseStorageItemLink*) sfi_ring_pop_head (&self->item_links);

      if (ilink->error)
        {
          gchar *error = g_strdup_format ("unable to resolve link path for item `%s': %s",
                                          BSE_OBJECT_UNAME (ilink->from_item),
                                          ilink->error);
          ilink->restore_link (ilink->data, self, ilink->from_item, NULL, error);
          g_free (error);
          if (ilink->to_item)
            g_object_unref (ilink->to_item);
          g_free (ilink->error);
        }
      else if (ilink->to_item)
        {
          ilink->restore_link (ilink->data, self, ilink->from_item, ilink->to_item, NULL);
          g_object_unref (ilink->to_item);
        }
      else if (!ilink->upath)
        {
          ilink->restore_link (ilink->data, self, ilink->from_item, NULL, NULL);
        }
      else
        {
          BseItem *child = NULL, *parent = ilink->from_item;
          guint pbackup = ilink->pbackup;
          gchar *error = NULL;

          while (pbackup && parent)
            {
              pbackup--;
              parent = parent->parent;
            }
          if (!parent)
            error = g_strdup_format ("failed to find ancestor of item `%s' (branch depth: -%u, "
                                     "number of parents: %u) while resolving link path \"%s\"",
                                     BSE_OBJECT_UNAME (ilink->from_item),
                                     ilink->pbackup,
                                     ilink->pbackup - pbackup + 1,
                                     ilink->upath);
          else
            {
              child = storage_path_table_resolve_upath (self, BSE_CONTAINER (parent), ilink->upath);
              if (!child)
                error = g_strdup_format ("failed to find object for item `%s' while resolving link path \"%s\" from ancestor `%s'",
                                         BSE_OBJECT_UNAME (ilink->from_item),
                                         ilink->upath, BSE_OBJECT_UNAME (parent));
            }
          ilink->restore_link (ilink->data, self, ilink->from_item, child, error);
          g_free (error);
        }
      g_object_unref (ilink->from_item);
      g_free (ilink->upath);
      g_free (ilink);
    }

  /* finish restorables */
  sfi_ppool_foreach (self->restorable_objects, storage_restorable_objects_foreach, self);
  /* clear pool */
  sfi_ppool_destroy (self->restorable_objects);
  self->restorable_objects = sfi_ppool_new();
}

const gchar*
bse_storage_item_get_compat_type (BseItem *item)
{
  const gchar *type = (const char*) g_object_get_data ((GObject*) item, "BseStorage-compat-type");
  if (!type)
    type = G_OBJECT_TYPE_NAME (item);
  return type;
}

typedef struct {
  BseContainer *container;
  gchar        *uname;
  BseItem      *item;
} UNameChild;

static guint
uname_child_hash (gconstpointer uc)
{
  const UNameChild *uchild = (const UNameChild*) uc;
  guint h = g_str_hash (uchild->uname);
  h ^= G_HASH_LONG ((long) uchild->container);
  return h;
}

static gint
uname_child_equals (gconstpointer uc1,
                    gconstpointer uc2)
{
  const UNameChild *uchild1 = (const UNameChild*) uc1;
  const UNameChild *uchild2 = (const UNameChild*) uc2;
  return (bse_string_equals (uchild1->uname, uchild2->uname) &&
          uchild1->container == uchild2->container);
}

static void
uname_child_free (gpointer uc)
{
  UNameChild *uchild = (UNameChild*) uc;
  g_object_unref (uchild->container);
  g_free (uchild->uname);
  g_object_unref (uchild->item);
  g_free (uchild);
}

static void
storage_path_table_insert (BseStorage   *self,
                           BseContainer *container,
                           const gchar  *uname,
                           BseItem      *item)
{
  UNameChild key;
  key.container = container;
  key.uname = (char*) uname;
  UNameChild *uchild = (UNameChild*) g_hash_table_lookup (self->path_table, &key);
  if (!uchild)
    {
      uchild = g_new (UNameChild, 1);
      uchild->container = (BseContainer*) g_object_ref (container);
      uchild->uname = g_strdup (uname);
      uchild->item = NULL;
      g_hash_table_insert (self->path_table, uchild, uchild);
    }
  if (uchild->item)
    g_object_unref (uchild->item);
  uchild->item = (BseItem*) g_object_ref (item);
  // DEBUG ("INSERT: (%p,%s) => %p", container, uname, item);
}

static inline BseItem*
storage_path_table_lookup (BseStorage   *self,
                           BseContainer *container,
                           const gchar  *uname)
{
  UNameChild key, *uchild;
  key.container = container;
  key.uname = (gchar*) uname;
  uchild = (UNameChild*) g_hash_table_lookup (self->path_table, &key);
  // DEBUG ("LOOKUP: (%p,%s) => %p", container, uname, uchild ? uchild->item : NULL);
  if (uchild)
    return uchild->item;
  /* we resort to container lookups in case
   * object links refer across external
   * containers.
   */
  return bse_container_lookup_item (container, uname);
}

static BseItem*
storage_path_table_resolve_upath (BseStorage   *self,
                                  BseContainer *container,
                                  gchar        *upath)
{
  char *next_upath = strchr (upath, ':');
  /* upaths consist of colon seperated unames from the item's ancestry */
  if (next_upath) /* A:B[:...] */
    {
      char *next_next_upath = strchr (next_upath + 1, ':');
      BseItem *item;
      next_upath[0] = 0;
      if (next_next_upath)
	next_next_upath[0] = 0;
      /* lookup A */
      item = storage_path_table_resolve_upath (self, container, upath);
      next_upath[0] = ':';
      if (next_next_upath)
	next_next_upath[0] = ':';
      /* lookup B[:...] in A */
      if (BSE_IS_CONTAINER (item))
	return storage_path_table_resolve_upath (self, BSE_CONTAINER (item), next_upath + 1);
      else
	return NULL;
    }
  return storage_path_table_lookup (self, container, upath);
}

static void
item_link_resolved (gpointer     data,
                    BseStorage  *self,
                    BseItem     *item,
                    BseItem     *dest_item,
                    const gchar *error)
{
  if (error)
    bse_storage_warn (self, "%s", error);
  else
    {
      GParamSpec *pspec = (GParamSpec*) data;
      GValue value = { 0, };
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_value_set_object (&value, dest_item);
      g_object_set_property (G_OBJECT (item), /* no undo */
                             pspec->name, &value);
      g_value_unset (&value);
    }
}

/// Retrive the value of @a field.key through aux_vector_find() and convert the value to @a ValueType.
template<typename ValueType = std::string> ValueType
aux_vector_get (const std::vector<std::string> &auxvector, const std::string &field, const std::string &key, const std::string &fallback = "")
{
  return Bse::string_to_type<ValueType> (Aida::aux_vector_find (auxvector, field, key, fallback));
}

static bool
any_set_from_string (BseStorage *self, Bse::Any &any, const std::string &string)
{
  switch (any.kind())
    {
    case Aida::BOOL:
      if (string.size() == 2 && string.data()[0] == '#')
        any.set (bool (string.data()[1] == 't' || string.data()[1] == 'T'));
      else
        any.set (Bse::string_to_bool (string));
      break;
    case Aida::INT64:           any.set (Bse::string_to_int (string));        break;
    case Aida::FLOAT64:         any.set (Bse::string_to_double (string));     break;
    case Aida::STRING:          any.set (Bse::string_from_cquote (string));   break;
    case Aida::ENUM:
      {
        const Aida::EnumInfo &einfo = any.get_enum_info();
        const int64 v = einfo.value_from_string (string);
        if (!einfo.find_value (v).ident) // 'v' is no valid enum value
          {
            const Aida::EnumValueVector evv = einfo.value_vector();
            if (evv.size())
              any.set_enum (einfo, evv[0].value);
            bse_storage_warn (self, "fixing invalid enum value: %s (%d) -> %s", einfo.name(), v, evv[0].ident);
          }
        else
          any.set_enum (einfo, v);
        break;
      }
    default:
      Bse::warn ("unhandled Any: %s; string=%s", any.repr(), string);
      return false;
    }
  return true;
}

static GTokenType
scanner_parse_paren_rest (GScanner *scanner, std::string *result)
{
  // configure scanner to pass through most characters, so we can delay parsing
  const GScannerConfig saved_config = *scanner->config;
  scanner->config->cset_skip_characters = (char*) " \t\r\n";
  scanner->config->scan_identifier = true;
  scanner->config->scan_identifier_1char = false;
  scanner->config->identifier_2_string = false;
  scanner->config->scan_symbols = false;
  scanner->config->scan_binary = false;
  scanner->config->scan_octal = false;
  scanner->config->scan_float = false;
  scanner->config->scan_hex = false;
  scanner->config->scan_hex_dollar = false;
  scanner->config->char_2_token = false;
  GTokenType expected_token = G_TOKEN_NONE, token = g_scanner_get_next_token (scanner);
  uint level = 1; // need one ')' to terminate
  std::string rest;
  while (token && expected_token == G_TOKEN_NONE)
    {
      if (token == G_TOKEN_CHAR)
        {
          if (scanner->value.v_char == '(')
            level += 1;
          else if (scanner->value.v_char == ')')
            {
              level -= 1;
              if (level == 0)
                break;
            }
          else
            rest += scanner->value.v_char;
        }
      else if (token == G_TOKEN_STRING)
        {
          if (!rest.empty())
            rest += " ";
          rest += Bse::string_to_cquote (scanner->value.v_string);
        }
      else if (token == G_TOKEN_IDENTIFIER)
        {
          if (!rest.empty())
            rest += " ";
          rest += scanner->value.v_string;
        }
      else if (token == G_TOKEN_INT)
        {
          if (!rest.empty())
            rest += " ";
          rest += Bse::string_from_int (scanner->value.v_int);
        }
      else
        {
          expected_token = G_TOKEN_RIGHT_PAREN; // was expecting a char, got something unknown
          break;
        }
      token = g_scanner_get_next_token (scanner);
    }
  *scanner->config = saved_config;
  if (result && expected_token == G_TOKEN_NONE)
    *result = rest;
  return expected_token; // G_TOKEN_NONE on success
}

static GTokenType
storage_parse_property_value (BseStorage *self, const std::string &name, Bse::Any &any, const std::vector<std::string> &aux_data)
{
  assert_return (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  GScanner *scanner = bse_storage_get_scanner (self);
  std::string rest;
  GTokenType expected_token = scanner_parse_paren_rest (scanner, &rest);
  if (expected_token != G_TOKEN_NONE)
    return expected_token;
  const bool any_from_string_conversion = any_set_from_string (self, any, rest);
  if (any_from_string_conversion)
    return G_TOKEN_NONE;
  else
    {
      bse_storage_error (self, "failed to parse Any from: \"%s\"", rest.c_str());
      return G_TOKEN_ERROR;
    }
}

static GTokenType
restore_cxx_item_property (BseItem *bitem, BseStorage *self)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  Bse::ItemImpl *item = bitem->as<Bse::ItemImpl*>();
  // need identifier
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return SFI_TOKEN_UNMATCHED;
  std::string identifier = scanner->next_value.v_identifier;
  for (size_t i = 0; i < identifier.size(); i++)
    if (identifier.data()[i] == '-')
      identifier[i] = '_';
  // find identifier in item, we could search __aida_dir__, but *getting* is simpler
  Bse::Any any = item->__aida_get__ (identifier);
  if (any.kind())
    {
      const std::vector<std::string> auxvector = item->__aida_aux_data__();
      // FIXME: need special casing of object references, see bse_storage_parse_item_link
      parse_or_return (scanner, G_TOKEN_IDENTIFIER);    // eat pspec name
      // parse Any value, including the closing ')'
      GTokenType expected_token = storage_parse_property_value (self, identifier, any, auxvector);
      if (expected_token != G_TOKEN_NONE)
        return expected_token;
      if (Aida::aux_vector_check_options (auxvector, identifier, "hints", "r:w:S")) // readable, writable, storage
        {
          item->__aida_set__ (identifier, any);
        }
      else
        bse_storage_warn (self, "ignoring non-writable object property \"%s\" of type '%s'",
                          identifier, Aida::type_kind_name (any.kind()));
      return G_TOKEN_NONE;
    }
  return SFI_TOKEN_UNMATCHED;
}

static GTokenType item_restore_try_statement (gpointer item, BseStorage *self, GScanner *scanner, gpointer user_data);

static GTokenType
restore_item_property (BseItem *item, BseStorage *self)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  GTokenType expected_token;
  GValue value = { 0, };
  /* check identifier */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return SFI_TOKEN_UNMATCHED;
  /* in theory, we should only find SFI_PARAM_SERVE_STORAGE
   * properties here, but due to version changes or even
   * users editing their files, we will simply parse all
   * kinds of properties (we might want to at least restrict
   * them to SFI_PARAM_SERVE_STORAGE and SFI_PARAM_SERVE_GUI
   * at some point...)
   */
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (item), scanner->next_value.v_identifier);
  if (!pspec)
    return restore_cxx_item_property (item, self);
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);        /* eat pspec name */
  /* parse value, special casing object references */
  if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), BSE_TYPE_ITEM))
    {
      expected_token = bse_storage_parse_item_link (self, item, item_link_resolved, pspec);
      if (expected_token != G_TOKEN_NONE)
        return expected_token;
      parse_or_return (scanner, ')');
      /* we cannot provide the object value at this time */
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_value_set_object (&value, NULL);
    }
  else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), G_TYPE_OBJECT))
    return bse_storage_warn_skip (self, "unable to restore object property \"%s\" of type `%s'",
                                  pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
  else
    {
      /* parse the value for this pspec, including the closing ')' */
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      expected_token = bse_storage_parse_param_value (self, &value, pspec);
      if (expected_token != G_TOKEN_NONE)
        {
          g_value_unset (&value);
          return expected_token;
        }
    }
  /* set property value while preserving the object uname */
  if ((pspec->flags & G_PARAM_WRITABLE) && !(pspec->flags & G_PARAM_CONSTRUCT_ONLY))
    g_object_set_property (G_OBJECT (item), /* no undo */
                           pspec->name, &value);
  else
    bse_storage_warn (self, "ignoring non-writable object property \"%s\" of type '%s'",
                      pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
  g_value_unset (&value);
  return G_TOKEN_NONE;
}

static GTokenType
restore_source_automation (BseItem    *item,
                           BseStorage *self)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  /* check identifier */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER ||
      !bse_string_equals ("source-automate", scanner->next_value.v_identifier))
    return SFI_TOKEN_UNMATCHED;
  /* check object type */
  if (!BSE_IS_SOURCE (item))
    return SFI_TOKEN_UNMATCHED;
  /* eat source-automate */
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  /* read pspec name */
  parse_or_return (scanner, G_TOKEN_STRING);
  /* find pspec */
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (item), scanner->value.v_string);
  if (!pspec || !sfi_pspec_check_option (pspec, "automate"))
    return bse_storage_warn_skip (self, "not an automatable property: \"%s\"", pspec->name);
  /* parse midi channel */
  parse_or_return (scanner, G_TOKEN_INT);
  gint midi_channel = scanner->value.v_int64;
  /* parse control type */
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  Bse::MidiControl control_type = Aida::enum_value_from_string<Bse::MidiControl> (scanner->value.v_identifier);
  /* close statement */
  parse_or_return (scanner, ')');
  Bse::Error error = bse_source_set_automation_property (BSE_SOURCE (item), pspec->name, midi_channel, Bse::MidiSignal (control_type));
  if (error != 0)
    bse_storage_warn (self, "failed to automate property \"%s\": %s", pspec->name, bse_error_blurb (error));
  return G_TOKEN_NONE;
}

static GTokenType
restore_container_child (BseContainer *container,
                         BseStorage   *self)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  GTokenType expected_token;
  BseItem *item;
  const gchar *uname;
  gchar *type_name, *tmp, *compat_type = NULL;
  /* check identifier */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER ||
      !bse_string_equals ("container-child", scanner->next_value.v_identifier))
    return SFI_TOKEN_UNMATCHED;
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);        /* eat identifier */
  /* parse and validate type::uname argument */
  parse_or_return (scanner, G_TOKEN_STRING);
  uname = strchr (scanner->value.v_string, ':');
  if (!uname || uname[1] != ':')
    {
      bse_storage_error (self, "invalid object handle: \"%s\"", scanner->value.v_string);
      return G_TOKEN_ERROR;
    }
  type_name = g_strndup (scanner->value.v_string, uname - scanner->value.v_string);
  uname += 2;
  /* handle different versions */
  tmp = bse_compat_rewrite_type_name (self, type_name);
  if (tmp)
    {
      compat_type = type_name;
      type_name = tmp;
    }
  /* check container's storage filter */
  if (!bse_container_check_restore (container, type_name))
    {
      g_free (type_name);
      g_free (compat_type);
      return bse_storage_warn_skip (self, "ignoring child: \"%s\"", scanner->value.v_string);
    }
  /* create container child */
  tmp = g_strconcat (type_name, "::", uname, NULL);
  g_free (type_name);
  item = bse_container_retrieve_child (container, tmp);
  if (item)
    g_object_set_data_full ((GObject*) item, "BseStorage-compat-type", compat_type, g_free);
  else
    g_free (compat_type);
  g_free (tmp);
  if (!item)
    return bse_storage_warn_skip (self, "failed to create object from (invalid?) handle: \"%s\"",
                                  scanner->value.v_string);
  /* provide compatibility setup (e.g. property defaults) */
  bse_item_compat_setup (item, self->major_version, self->minor_version, self->micro_version);
  storage_path_table_insert (self, container, uname, item);
  /* restore_item reads out closing parenthesis */
  g_object_ref (item);
  expected_token = bse_storage_parse_rest (self, item, item_restore_try_statement, NULL);
  g_object_unref (item);
  return expected_token;
}

static GTokenType
item_restore_try_statement (gpointer    _item,
                            BseStorage *self,
                            GScanner   *scanner,
                            gpointer    user_data)
{
  BseItem *item = BSE_ITEM (_item);
  GTokenType expected_token = SFI_TOKEN_UNMATCHED;
  /* ensure that the statement starts out with an identifier */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    {
      g_scanner_get_next_token (scanner);
      return G_TOKEN_IDENTIFIER;
    }
  /* this is pretty much the *only* place where something else than
   * G_TOKEN_NONE may be returned without erroring out. return values:
   * G_TOKEN_NONE        - statement got parsed, advance to next statement
   * SFI_TOKEN_UNMATCHED - statement not recognized, try further
   * anything else       - encountered (syntax/semantic) error during parsing
   */
  bse_object_restore_start (BSE_OBJECT (item), self);
  if (expected_token == SFI_TOKEN_UNMATCHED)
    expected_token = restore_item_property (item, self);
  if (expected_token == SFI_TOKEN_UNMATCHED)
    expected_token = restore_source_automation (item, self);
  if (expected_token == SFI_TOKEN_UNMATCHED)
    expected_token = BSE_OBJECT_GET_CLASS (item)->restore_private ((BseObject*) item, self, scanner);
  if (expected_token == SFI_TOKEN_UNMATCHED && BSE_IS_CONTAINER (item))
    expected_token = restore_container_child ((BseContainer*) item, self);
  if (expected_token == SFI_TOKEN_UNMATCHED && strcmp (scanner->next_value.v_identifier, "bse-version") == 0)
    expected_token = storage_parse_bse_version (self);
  return expected_token;
}

GTokenType
bse_storage_restore_item (BseStorage *self,
                          gpointer    item)
{
  GTokenType expected_token;
  assert_return (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  assert_return (BSE_IS_ITEM (item), G_TOKEN_ERROR);
  g_object_ref (self);
  g_object_ref (item);
  expected_token = sfi_rstore_parse_until (self->rstore, G_TOKEN_EOF, item,
                                           (SfiStoreParser) item_restore_try_statement, NULL);
  g_object_unref (item);
  g_object_unref (self);
  return expected_token;
}

GTokenType
bse_storage_parse_rest (BseStorage     *self,
                        gpointer        context_data,
                        BseTryStatement try_statement,
                        gpointer        user_data)
{
  assert_return (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  assert_return (self->rstore != NULL, G_TOKEN_ERROR);
  return sfi_rstore_parse_until (self->rstore, GTokenType (')'), context_data, (SfiStoreParser) try_statement, user_data);
}

gboolean
bse_storage_check_parse_negate (BseStorage *self)
{
  assert_return (BSE_IS_STORAGE (self), FALSE);
  if (g_scanner_peek_next_token (bse_storage_get_scanner (self)) == '-')
    {
      g_scanner_get_next_token (bse_storage_get_scanner (self));
      return TRUE;
    }
  else
    return FALSE;
}

void
bse_storage_put_param (BseStorage   *self,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  assert_return (BSE_IS_STORAGE (self));
  assert_return (self->wstore);
  assert_return (G_IS_VALUE (value));
  assert_return (G_IS_PARAM_SPEC (pspec));
  sfi_wstore_put_param (self->wstore, value, pspec);
}

GTokenType
bse_storage_parse_param_value (BseStorage *self,
                               GValue     *value,
                               GParamSpec *pspec)
{
  assert_return (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  assert_return (self->rstore, G_TOKEN_ERROR);
  return sfi_rstore_parse_param (self->rstore, value, pspec);
}

void
bse_storage_put_item_link (BseStorage *self,
                           BseItem    *from_item,
                           BseItem    *to_item)
{
  assert_return (BSE_IS_STORAGE (self));
  assert_return (self->wstore);
  assert_return (BSE_IS_ITEM (from_item));
  assert_return (BSE_IS_ITEM (to_item));
  if (!to_item)                                         /* special case (1) */
    {
      bse_storage_puts (self, SFI_SERIAL_NULL_TOKEN);
    }
  else          /* ordinary object link within a project or other container */
    {
      BseItem *tmp, *common_ancestor;
      guint pbackup = 0;
      gchar *upath, *epath;
      assert_return (BSE_IS_ITEM (to_item));
      common_ancestor = bse_item_common_ancestor (from_item, to_item);
      assert_return (BSE_IS_CONTAINER (common_ancestor));
      sfi_ppool_set (self->referenced_items, to_item);
      /* figure number of parent backup levels to reach common ancestor */
      for (tmp = from_item; tmp != common_ancestor; tmp = tmp->parent)
        pbackup++;
      /* path to reach to_item */
      upath = bse_container_make_upath (BSE_CONTAINER (common_ancestor), to_item);
      /* store path reference */
      epath = g_strescape (upath, NULL);
      bse_storage_printf (self, "(link %u \"%s\")", pbackup, epath);
      g_free (epath);
      g_free (upath);
    }
}

#ifdef DOXER
/**
 * @param data	        user data
 * @param storage	BseStorage instance
 * @param from_item	link owner
 * @param to_item	link target or NULL
 * @param error	error string describing failing link lookups
 *
 * BseStorageRestoreLink() is a user supplied handler to be called
 * at the end of a parsing stage, once object references could be
 * resolved. Failing resolutions are indicated by non NULL @a error
 * strings.
 */
typedef void (*BseStorageRestoreLink)   (gpointer        data,
                                         BseStorage     *storage,
                                         BseItem        *from_item,
                                         BseItem        *to_item,
                                         const gchar    *error);
#endif

/**
 * @param self  	valid BseStorage
 * @param from_item	link owner
 * @param restore_link	BseStorageRestoreLink handler to be called once the link was resolved
 * @param data	        user data passed into @a restore_link()
 * @return		expected token in case of a parsing error (G_TOKEN_NONE on success)
 *
 * Parse an item link statement and return the expected token if a parsing
 * error occours. Item links are resolved at the end of the parsing stage
 * by calling the user supplied handler @a restore_link() with the link target
 * amongst its arguments (see BseStorageRestoreLink()).
 */
GTokenType
bse_storage_parse_item_link (BseStorage           *self,
                             BseItem              *from_item,
                             BseStorageRestoreLink restore_link,
                             gpointer              data)
{
  GScanner *scanner;
  BseStorageItemLink *ilink;
  GTokenType expected_token;
  assert_return (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  assert_return (self->rstore, G_TOKEN_ERROR);
  assert_return (BSE_IS_ITEM (from_item), G_TOKEN_ERROR);
  assert_return (restore_link != NULL, G_TOKEN_ERROR);
  scanner = bse_storage_get_scanner (self);
#define parse_or_goto(etoken,label) \
  { expected_token = (etoken); if (g_scanner_get_next_token (scanner) != expected_token) goto label; }
#define peek_or_goto(etoken,label)  \
  { expected_token = (etoken); if (g_scanner_peek_next_token (scanner) != expected_token) \
    { g_scanner_get_next_token (scanner); goto label; } }
  g_scanner_get_next_token (scanner);
  bse_object_restore_start (BSE_OBJECT (from_item), self);
  if (sfi_serial_check_parse_null_token (scanner))
    {
      ilink = storage_add_item_link (self, from_item, restore_link, data, NULL);
    }
  else if (scanner->token == '(')
    {
      parse_or_goto (G_TOKEN_IDENTIFIER, error_parse_link);
      if (strcmp (scanner->value.v_identifier, "link") == 0)
        {
          guint pbackup = 0;
          if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
            {
              g_scanner_get_next_token (scanner);       /* eat int */
              pbackup = scanner->value.v_int64;
            }
          parse_or_goto (G_TOKEN_STRING, error_parse_link);
          peek_or_goto (GTokenType (')'), error_parse_link);
          ilink = storage_add_item_link (self, from_item, restore_link, data, NULL);
          ilink->upath = g_strdup (scanner->value.v_string);
          ilink->pbackup = pbackup;
        }
      else
        {
          expected_token = G_TOKEN_IDENTIFIER;
          goto error_parse_link;
        }
      parse_or_goto (GTokenType (')'), error_parse_link);
    }
  else
    {
      expected_token = GTokenType ('(');
      goto error_parse_link;
    }
  return G_TOKEN_NONE;
#undef  parse_or_goto
#undef  peek_or_goto
 error_parse_link:
  ilink = storage_add_item_link (self, from_item, restore_link, data, g_strdup ("failed to parse link path"));
  return expected_token;
}

void
bse_storage_warn_str (BseStorage *self, const std::string &string)
{
  assert_return (BSE_IS_STORAGE (self));
  if (self->rstore)
    sfi_rstore_warn (self->rstore, string);
  else
    Bse::printerr ("BseStorage: while storing: %s", string.c_str());
}

GTokenType
bse_storage_skip (BseStorage *self, const std::string &string)
{
  GTokenType token;
  assert_return (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  assert_return (self->rstore != NULL, G_TOKEN_ERROR);
  token = sfi_rstore_warn_skip (self->rstore, string);
  return token;
}

void
bse_storage_error_str (BseStorage *self, const std::string &string)
{
  assert_return (BSE_IS_STORAGE (self));
  if (self->rstore)
    sfi_rstore_error (self->rstore, string);
  else
    Bse::printerr ("BseStorage: ERROR: while storing: %s\n", string.c_str());
}

static void
bse_item_store_property (BseItem    *item,
                         BseStorage *storage,
                         GValue     *value,
                         GParamSpec *pspec)
{
  if (g_type_is_a (G_VALUE_TYPE (value), BSE_TYPE_ITEM))
    {
      bse_storage_break (storage);
      bse_storage_putc (storage, '(');
      bse_storage_puts (storage, pspec->name);
      bse_storage_putc (storage, ' ');
      bse_storage_put_item_link (storage, item, (BseItem*) g_value_get_object (value));
      bse_storage_putc (storage, ')');
    }
  else if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_OBJECT))
    Bse::warning ("%s: unable to store object property \"%s\" of type `%s'", G_STRLOC, pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
  else
    bse_storage_put_param (storage, value, pspec);
}

static void
bse_source_store_automation (BseSource  *source,
                             BseStorage *storage,
                             GParamSpec *pspec)
{
  guint midi_channel = 0;
  Bse::MidiSignal signal_type = Bse::MidiSignal (0);
  bse_source_get_automation_property (source, pspec->name, &midi_channel, &signal_type);
  Bse::MidiControl control_type = Bse::MidiControl (signal_type);
  if (control_type != 0)
    {
      bse_storage_break (storage);
      bse_storage_printf (storage, "(source-automate \"%s\" %u %s)", pspec->name, midi_channel,
                          Aida::enum_value_to_string (control_type));
    }
}

static void
store_item_properties (BseItem    *item,
                       BseStorage *storage)
{
  GParamSpec **pspecs;
  guint n;
  /* dump the object properties, starting out at the base class */
  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (item), &n);
  while (n--)
    {
      GParamSpec *pspec = pspecs[n];
      if (sfi_pspec_check_option (pspec, "S")) /* check serializable */
        {
          GValue value = { 0, };
          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          g_object_get_property (G_OBJECT (item), pspec->name, &value);
          if (!g_param_value_defaults (pspec, &value) ||
              !sfi_pspec_check_option (pspec, "skip-default"))
            bse_item_store_property (item, storage, &value, pspec);
          g_value_unset (&value);
          if (sfi_pspec_check_option (pspec, "automate") && BSE_IS_SOURCE (item))
            bse_source_store_automation (BSE_SOURCE (item), storage, pspec);
        }
    }
  g_free (pspecs);
}

static void
storage_store_property_value (BseStorage *self, const std::string &property_name, Bse::Any any, const std::vector<std::string> &aux_data)
{
  if (Aida::aux_vector_check_options (aux_data, property_name, "hints", "skip-default"))
    {
      const char *const invalid = "\377\377\376\376\1\2 invalid \3"; // no-value marker, (invalid UTF-8)
      const std::string dflt_val = aux_vector_get (aux_data, property_name, "default", invalid);
      if (dflt_val != invalid)
        {
          Bse::Any dflt = any; // copy type
          const bool any_from_string_conversion = any_set_from_string (self, dflt, dflt_val);
          if (any_from_string_conversion && dflt == any)
            return;     // skip storing default value
        }
    }
  std::string target;
  switch (any.kind())
    {
    case Aida::BOOL:            target = Bse::string_from_bool (any.get<bool>());            break;
    case Aida::INT64:           target = Bse::string_from_int (any.get<int64>());            break;
    case Aida::FLOAT64:         target = Bse::string_from_double (any.get<double>());        break;
    case Aida::STRING:          target = Bse::string_to_cquote (any.get<std::string>());     break;
    case Aida::ENUM:
      {
        const Aida::EnumInfo &einfo = any.get_enum_info();
        target = einfo.value_to_string (any.as_int64());
        break;
      }
    default:                    assert_return_unreached();
    }
  assert_return (!target.empty());
  bse_storage_break (self);
  bse_storage_putc (self, '(');
  bse_storage_puts (self, property_name.c_str());
  bse_storage_putc (self, ' ');
  bse_storage_puts (self, target.c_str());
  bse_storage_putc (self, ')');
}

static void
store_cxx_item_properties (BseItem *bitem, BseStorage *self)
{
  Bse::ItemImpl *item = bitem->as<Bse::ItemImpl*>();
  const std::vector<std::string> auxvector = item->__aida_aux_data__();
  for (const std::string &pname : item->__aida_dir__())
    if (Aida::aux_vector_check_options (auxvector, pname, "hints", "r:w:S")) // readable, writable, storage
      storage_store_property_value (self, pname, item->__aida_get__ (pname), auxvector);
}

void
bse_storage_store_item (BseStorage *self, BseItem *item)
{
  assert_return (BSE_IS_STORAGE (self));
  assert_return (self->wstore);
  assert_return (BSE_IS_ITEM (item));
  g_object_ref (self);
  g_object_ref (item);
  sfi_ppool_set (self->stored_items, item);
  store_cxx_item_properties (item, self);
  store_item_properties (item, self);
  BSE_OBJECT_GET_CLASS (item)->store_private (BSE_OBJECT (item), self);
  if (BSE_IS_CONTAINER (item))
    bse_container_store_children ((BseContainer*) item, self);
  g_object_unref (item);
  g_object_unref (self);
}

void
bse_storage_store_child (BseStorage *self, BseItem *item)
{
  gchar *uname;
  assert_return (BSE_IS_STORAGE (self));
  assert_return (self->wstore);
  assert_return (BSE_IS_ITEM (item));
  uname = g_strescape (BSE_OBJECT_UNAME (item), NULL);
  bse_storage_break (self);
  bse_storage_printf (self, "(%s \"%s::%s\"", "container-child", G_OBJECT_TYPE_NAME (item), uname);
  g_free (uname);
  bse_storage_push_level (self);
  bse_storage_store_item (self, item);
  bse_storage_pop_level (self);
  bse_storage_putc (self, ')');
}

void
bse_storage_putf (BseStorage *self,
                  gfloat      vfloat)
{
  assert_return (BSE_IS_STORAGE (self));
  assert_return (self->wstore);
  sfi_wstore_putf (self->wstore, vfloat);
}

void
bse_storage_putd (BseStorage *self,
                  gdouble     vdouble)
{
  assert_return (BSE_IS_STORAGE (self));
  assert_return (self->wstore);
  sfi_wstore_putd (self->wstore, vdouble);
}

void
bse_storage_putr (BseStorage     *self,
                  SfiReal         vreal,
                  const gchar    *hints)
{
  assert_return (BSE_IS_STORAGE (self));
  assert_return (self->wstore);
  if (hints && g_option_check (hints, "f"))     /* check float option */
    bse_storage_putf (self, vreal);
  else
    bse_storage_putd (self, vreal);
}

void
bse_storage_put_xinfos (BseStorage *self,
                        gchar     **xinfos)
{
  xinfos = bse_xinfos_dup_consolidated (xinfos, FALSE);
  if (xinfos && xinfos[0])
    {
      bse_storage_break (self);
      gchar *str = g_strescape (xinfos[0], NULL);
      bse_storage_printf (self, " (\"%s\"", str);
      g_free (str);
      guint i;
      bse_storage_push_level (self);
      for (i = 1; xinfos[i]; i++)
        {
          bse_storage_break (self);
          str = g_strescape (xinfos[i], NULL);
          bse_storage_printf (self, "\"%s\"", str);
          g_free (str);
        }
      bse_storage_pop_level (self);
      bse_storage_puts (self, ")");
    }
  else
    bse_storage_printf (self, "#f");
  g_strfreev (xinfos);
}

GTokenType
bse_storage_parse_xinfos (BseStorage *self,
                          gchar    ***xinfosp)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  g_scanner_get_next_token (scanner);
  if (scanner->token == '#')    /* parse "#f" => NULL */
    {
      g_scanner_get_next_token (scanner);
      if (scanner->token == 'f' || scanner->token == 'F')
        {
          *xinfosp = NULL;
          return G_TOKEN_NONE;
        }
      /* everything else, even #t is bogus */
      return GTokenType ('f');
    }
  else if (scanner->token == '(')
    {
      gchar **xinfos = NULL;
      while (g_scanner_get_next_token (scanner) != ')')
        {
          if (scanner->token == G_TOKEN_STRING)
            xinfos = bse_xinfos_parse_assignment (xinfos, scanner->value.v_string);
          else
            return G_TOKEN_STRING;
        }
      *xinfosp = bse_xinfos_dup_consolidated (xinfos, FALSE);
      g_strfreev (xinfos);
      return G_TOKEN_NONE;
    }
  else
    return GTokenType ('(');
}

static GTokenType
parse_dblock_data_handle (BseStorage     *self,
                          GslDataHandle **data_handle_p,
                          guint          *n_channels_p,
                          gfloat         *mix_freq_p,
                          gfloat         *osc_freq_p)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  BseStorageDBlock *dblock;
  gulong id;
  parse_or_return (scanner, G_TOKEN_INT);
  id = scanner->value.v_int64;
  parse_or_return (scanner, ')');
  dblock = bse_storage_get_dblock (self, id);
  if (!dblock)
    {
      bse_storage_error (self, "failed to lookup internal data handle with id: %lu", id);
      return G_TOKEN_ERROR;
    }
  *data_handle_p = gsl_data_handle_ref (dblock->dhandle);       /* fake "creating" a data handle */
  if (n_channels_p)
    *n_channels_p = dblock->n_channels;
  if (mix_freq_p)
    *mix_freq_p = dblock->mix_freq;
  if (osc_freq_p)
    *osc_freq_p = dblock->osc_freq;
  return G_TOKEN_NONE;
}

typedef struct {
  GslDataHandle *dhandle;
  guint          opened : 1;
  guint          bpv, format, byte_order;
  BseStorage    *storage;
  guint          length;
} WStoreDHandle;

static void
wstore_data_handle_destroy (gpointer data)
{
  WStoreDHandle *wh = (WStoreDHandle*) data;
  if (wh->opened)
    gsl_data_handle_close (wh->dhandle);
  gsl_data_handle_unref (wh->dhandle);
  g_free (wh);
}

static gint /* -errno || length */
wstore_data_handle_reader (gpointer data,
                           void    *buffer,
                           guint    blength)
{
  WStoreDHandle *wh = (WStoreDHandle*) data;
  GslLong n;
  if (!wh->opened)
    {
      Bse::Error error = gsl_data_handle_open (wh->dhandle);
      if (error != 0)
        {
          bse_storage_error (wh->storage, "failed to open data handle: %s", bse_error_blurb (error));
          return -ENOENT;
        }
      wh->opened = TRUE;
    }
  /* catch end */
  if (wh->length >= gsl_data_handle_length (wh->dhandle))
    return 0;
  do
    n = gsl_data_handle_read (wh->dhandle, wh->length, blength / sizeof (gfloat), (float*) buffer);
  while (n < 0 && errno == EINTR);
  if (n < 0)    /* bail out */
    {
      bse_storage_error (wh->storage, "failed to read from data handle");
      return -EIO;
    }
  wh->length += n;
  return gsl_conv_from_float_clip (GslWaveFormatType (wh->format), wh->byte_order, (const float*) buffer, buffer, n);
}

void
bse_storage_put_data_handle (BseStorage    *self,
                             guint          significant_bits,
                             GslDataHandle *dhandle)
{
  assert_return (BSE_IS_STORAGE (self));
  assert_return (self->wstore);
  assert_return (dhandle != NULL);
  assert_return (GSL_DATA_HANDLE_OPENED (dhandle));
  if (BSE_STORAGE_DBLOCK_CONTAINED (self))
    {
      /* stored as binary data block in memory for undo storage */
      gulong id = bse_storage_add_dblock (self, dhandle);
      bse_storage_break (self);
      bse_storage_printf (self, "(%s %lu)", g_quark_to_string (quark_dblock_data_handle), id);
      return;
    }
  GslDataHandle *test_handle, *tmp_handle = dhandle;
  do    /* skip comment or cache handles */
    {
      test_handle = tmp_handle;
      tmp_handle = gsl_data_handle_get_source (test_handle);
    }
  while (tmp_handle);   /* skip comment or cache handles */
  GslVorbis1Handle *vhandle = gsl_vorbis1_handle_new (test_handle, gsl_vorbis_make_serialno());
  Flac1Handle      *flac_handle = Flac1Handle::create (test_handle);
  if (vhandle)  /* save already compressed Ogg/Vorbis data */
    {
      bse_storage_break (self);
      bse_storage_printf (self, "(%s ", g_quark_to_string (quark_vorbis_data_handle));
      bse_storage_putf (self, gsl_data_handle_osc_freq (dhandle));
      bse_storage_push_level (self);
      bse_storage_break (self);
      gsl_vorbis1_handle_put_wstore (vhandle, self->wstore);
      bse_storage_pop_level (self);
      bse_storage_putc (self, ')');
    }
  else if (flac_handle) /* save flac compressed handle */
    {
      bse_storage_break (self);
      bse_storage_printf (self, "(%s ", g_quark_to_string (quark_flac_data_handle));
      bse_storage_putf (self, gsl_data_handle_osc_freq (dhandle));
      bse_storage_push_level (self);
      bse_storage_break (self);
      flac_handle->put_wstore (self->wstore);
      bse_storage_pop_level (self);
      bse_storage_putc (self, ')');
    }
  else          /* save raw data handle */
    {
      if (significant_bits < 1)
        significant_bits = 32;
      const uint bitdepth = gsl_data_handle_bit_depth (dhandle);
      significant_bits = MIN (bitdepth, significant_bits);
      GslWaveFormatType format;
      if (significant_bits > 16)
        format = GSL_WAVE_FORMAT_FLOAT;
      else if (significant_bits <= 8)
        format = GSL_WAVE_FORMAT_SIGNED_8;
      else
        format = GSL_WAVE_FORMAT_SIGNED_16;
      bse_storage_break (self);
      bse_storage_printf (self,
                          "(%s %u %s %s",
                          g_quark_to_string (quark_raw_data_handle),
                          gsl_data_handle_n_channels (dhandle),
                          gsl_wave_format_to_string (format),
                          gsl_byte_order_to_string (G_LITTLE_ENDIAN));
      bse_storage_puts (self, " ");
      bse_storage_putf (self, gsl_data_handle_mix_freq (dhandle));
      bse_storage_puts (self, " ");
      bse_storage_putf (self, gsl_data_handle_osc_freq (dhandle));
      bse_storage_push_level (self);
      bse_storage_break (self);
      WStoreDHandle *wh = g_new0 (WStoreDHandle, 1);
      wh->dhandle = gsl_data_handle_ref (dhandle);
      wh->format = format;
      wh->byte_order = G_LITTLE_ENDIAN;
      wh->bpv = gsl_wave_format_byte_width (format);
      wh->storage = self;
      sfi_wstore_put_binary (self->wstore, wstore_data_handle_reader, wh, wstore_data_handle_destroy);
      bse_storage_pop_level (self);
      bse_storage_putc (self, ')');
    }
}

static GTokenType
parse_raw_data_handle (BseStorage     *self,
                       GslDataHandle **data_handle_p,
                       guint          *n_channels_p,
                       gfloat         *mix_freq_p,
                       gfloat         *osc_freq_p)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  guint n_channels, byte_order;
  gfloat mix_freq, osc_freq;
  SfiNum offset, length;
  GTokenType token;
  parse_or_return (scanner, G_TOKEN_INT);
  n_channels = scanner->value.v_int64;
  if (n_channels <= 0 || n_channels > 256)
    return bse_storage_warn_skip (self, "invalid number of channels: %u", n_channels);
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  GslWaveFormatType format = gsl_wave_format_from_string (scanner->value.v_identifier);
  if (format == GSL_WAVE_FORMAT_NONE)
    return bse_storage_warn_skip (self, "unknown format for data handle: %s", scanner->value.v_identifier);
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  byte_order = gsl_byte_order_from_string (scanner->value.v_identifier);
  if (!byte_order)
    return bse_storage_warn_skip (self, "unknown byte-order for data handle: %s", scanner->value.v_identifier);
  g_scanner_get_next_token (scanner);
  if (scanner->token == G_TOKEN_INT)
    mix_freq = scanner->value.v_int64;
  else if (scanner->token == G_TOKEN_FLOAT)
    mix_freq = scanner->value.v_float;
  else
    return G_TOKEN_FLOAT;
  g_scanner_get_next_token (scanner);
  if (scanner->token == G_TOKEN_INT)
    osc_freq = scanner->value.v_int64;
  else if (scanner->token == G_TOKEN_FLOAT)
    osc_freq = scanner->value.v_float;
  else
    return G_TOKEN_FLOAT;
  if (osc_freq <= 0 || mix_freq < 4000 || osc_freq >= mix_freq / 2)
    return bse_storage_warn_skip (self, "invalid oscillating/mixing frequencies: %.7g/%.7g", osc_freq, mix_freq);
  token = sfi_rstore_parse_binary (self->rstore, &offset, &length);
  if (token != G_TOKEN_NONE)
    return token;
  length /= gsl_wave_format_byte_width (format);
  parse_or_return (scanner, ')');
  if (length < 1)
    {
      bse_storage_warn (self, "encountered empty data handle");
      *data_handle_p = NULL;
    }
  else
    *data_handle_p = gsl_wave_handle_new (self->rstore->fname,
                                          n_channels, format, byte_order,
                                          mix_freq, osc_freq,
                                          offset, length, NULL);
  if (n_channels_p)
    *n_channels_p = n_channels;
  if (mix_freq_p)
    *mix_freq_p = mix_freq;
  if (osc_freq_p)
    *osc_freq_p = osc_freq;
  return G_TOKEN_NONE;
}

static GTokenType
parse_vorbis_or_flac_data_handle (BseStorage     *self,
                                  GQuark          quark,
                                  GslDataHandle **data_handle_p,
                                  guint          *n_channels_p,
                                  gfloat         *mix_freq_p,
                                  gfloat         *osc_freq_p)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  GTokenType token;
  gfloat osc_freq;
  g_scanner_get_next_token (scanner);
  if (scanner->token == G_TOKEN_INT)
    osc_freq = scanner->value.v_int64;
  else if (scanner->token == G_TOKEN_FLOAT)
    osc_freq = scanner->value.v_float;
  else
    return G_TOKEN_FLOAT;
  if (osc_freq <= 0)
    return bse_storage_warn_skip (self, "invalid oscillating frequency: %.7g", osc_freq);
  if (osc_freq_p)
    *osc_freq_p = osc_freq;
  SfiNum offset, length;
  token = sfi_rstore_parse_zbinary (self->rstore, &offset, &length);
  if (token != G_TOKEN_NONE)
    return token;
  parse_or_return (scanner, ')');
  if (length < 1)
    {
      bse_storage_warn (self, "encountered empty data handle");
      *data_handle_p = NULL;
    }
  else
    {
      gfloat mix_freq;
      if (quark == quark_vorbis_data_handle)
        {
          *data_handle_p = gsl_data_handle_new_ogg_vorbis_zoffset (self->rstore->fname, osc_freq,
                                                                   offset, length, n_channels_p, &mix_freq);
        }
      else if (quark == quark_flac_data_handle)
        {
          *data_handle_p = bse_data_handle_new_flac_zoffset (self->rstore->fname, osc_freq,
                                                             offset, length, n_channels_p, &mix_freq);
        }
      else
        {
          return bse_storage_warn_skip (self, "unknown compressed data handle type in parse_vorbis_or_flac_data_handle");
        }
      if (osc_freq <= 0 || mix_freq < 4000 || osc_freq >= mix_freq / 2)
        return bse_storage_warn_skip (self, "invalid oscillating/mixing frequencies: %.7g/%.7g", osc_freq, mix_freq);
      if (mix_freq_p)
        *mix_freq_p = mix_freq;
    }
  return G_TOKEN_NONE;
}

gboolean
bse_storage_match_data_handle (BseStorage *self,
                               GQuark      quark)
{
  if (BSE_STORAGE_DBLOCK_CONTAINED (self) &&
      quark == quark_dblock_data_handle)
    return TRUE;
  if (quark == quark_raw_data_handle ||
      quark == quark_vorbis_data_handle ||
      quark == quark_flac_data_handle)
    return TRUE;
  return FALSE;
}

static GTokenType
parse_data_handle_trampoline (BseStorage     *self,
                              gboolean        statement_opened,
                              GslDataHandle **data_handle_p,
                              guint          *n_channels_p,
                              gfloat         *mix_freq_p,
                              gfloat         *osc_freq_p)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  GQuark quark;
  *data_handle_p = NULL;
  if (n_channels_p)
    *n_channels_p = 0;
  if (mix_freq_p)
    *mix_freq_p = 0;
  if (osc_freq_p)
    *osc_freq_p = 0;
  if (!statement_opened)
    parse_or_return (scanner, '(');
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  quark = g_quark_try_string (scanner->value.v_identifier);
  if (BSE_STORAGE_DBLOCK_CONTAINED (self) && quark == quark_dblock_data_handle)
    return parse_dblock_data_handle (self, data_handle_p, n_channels_p, mix_freq_p, osc_freq_p);
  if (quark == quark_raw_data_handle)
    return parse_raw_data_handle (self, data_handle_p, n_channels_p, mix_freq_p, osc_freq_p);
  else if (quark == quark_vorbis_data_handle || quark == quark_flac_data_handle)
    return parse_vorbis_or_flac_data_handle (self, quark, data_handle_p, n_channels_p, mix_freq_p, osc_freq_p);
  if (BSE_STORAGE_COMPAT (self, 0, 5, 1) && quark == quark_bse_storage_binary_v0)
    return compat_parse_data_handle (self, data_handle_p, n_channels_p, mix_freq_p, osc_freq_p);
  bse_storage_error (self, "unknown data handle keyword: %s", scanner->value.v_identifier);
  return G_TOKEN_ERROR;
}

GTokenType
bse_storage_parse_data_handle (BseStorage     *self,
                               GslDataHandle **data_handle_p,
                               guint          *n_channels_p,
                               gfloat         *mix_freq_p,
                               gfloat         *osc_freq_p)
{
  assert_return (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  assert_return (self->rstore, G_TOKEN_ERROR);
  assert_return (data_handle_p != NULL, G_TOKEN_ERROR);
  return parse_data_handle_trampoline (self, FALSE, data_handle_p, n_channels_p, mix_freq_p, osc_freq_p);
}

GTokenType
bse_storage_parse_data_handle_rest (BseStorage     *self,
                                    GslDataHandle **data_handle_p,
                                    guint          *n_channels_p,
                                    gfloat         *mix_freq_p,
                                    gfloat         *osc_freq_p)
{
  assert_return (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  assert_return (self->rstore, G_TOKEN_ERROR);
  assert_return (data_handle_p != NULL, G_TOKEN_ERROR);
  return parse_data_handle_trampoline (self, TRUE, data_handle_p, n_channels_p, mix_freq_p, osc_freq_p);
}

// == blobs ==

BseStorage::Blob::Blob (const std::string& file_name, bool is_temp_file) :
  file_name_ (file_name),
  is_temp_file_ (is_temp_file)
{
  id_ = bse_id_alloc();
}

BseStorage::Blob::~Blob()
{
  if (is_temp_file_)
    {
      unlink (file_name_.c_str());
      /* FIXME: check error code and do what? */
    }
  bse_id_free (id_);
}

static std::string
bse_storage_blob_tmp_dir()
{
  std::string dirname = Bse::Path::join (Bse::Path::cache_home(), "libbse");
  if (!Bse::Path::check (dirname, "d"))
    g_mkdir_with_parents (dirname.c_str(), 0755);
  return dirname;
}

/* search in temp dir for files called "bse-<user>-<pid>*"
 * delete files if the pid does not exist any longer
 */
void
bse_storage_blob_clean_files()
{
  std::string tmp_dir = bse_storage_blob_tmp_dir();

  GError *error;
  GDir *dir = g_dir_open (tmp_dir.c_str(), 0, &error);
  if (dir)
    {
      char *pattern = g_strdup_format ("bse-storage-blob-%s-", g_get_user_name());
      const char *file_name;
      while ((file_name = g_dir_read_name (dir)))
	{
	  if (strncmp (pattern, file_name, strlen (pattern)) == 0)
	    {
	      int pid = atoi (file_name + strlen (pattern));

              if (kill (pid, 0) == -1 && errno == ESRCH)
		{
		  char *path = g_strdup_format ("%s/%s", tmp_dir.c_str(), file_name);
		  unlink (path);
		  g_free (path);
		}
	    }
	}
      g_free (pattern);
      g_dir_close (dir);
    }
}

struct WStoreBlob
{
  BseStorage::BlobP blob;
  BseStorage       *storage;
  int               fd;
};

static WStoreBlob *
wstore_blob_new (BseStorage        *storage,
                 BseStorage::BlobP  blob)
{
  WStoreBlob *wsb = new WStoreBlob();
  wsb->blob = blob;
  wsb->storage = storage;
  wsb->fd = -1;
  return wsb;
}

static gint /* -errno || length */
wstore_blob_reader (gpointer data,
		    void    *buffer,
		    guint    blength)
{
  WStoreBlob *wsb = (WStoreBlob *) data;
  if (wsb->fd == -1)
    {
      do
	wsb->fd = open (wsb->blob->file_name().c_str(), O_RDONLY);
      while (wsb->fd == -1 && errno == EINTR);
      if (wsb->fd == -1)
	{
	  bse_storage_error (wsb->storage, "file %s could not be opened: %s", wsb->blob->file_name().c_str(), strerror (errno));
	  return -errno;
	}
    }
  int n;
  do
    n = read (wsb->fd, buffer, blength);
  while (n == -1 && errno == EINTR);
  if (n < 0)
    return -errno;
  else
    return n;
}

static void
wstore_blob_destroy (gpointer data)
{
  WStoreBlob *wblob = (WStoreBlob *) data;
  if (wblob->fd >= 0)
    close (wblob->fd);
  delete wblob;
}

void
bse_storage_put_blob (BseStorage         *self,
                      BseStorage::BlobP   blob)
{
  if (BSE_STORAGE_DBLOCK_CONTAINED (self))
    {
      gulong id = bse_storage_add_blob (self, blob);
      bse_storage_break (self);
      bse_storage_printf (self, "(%s %lu)", g_quark_to_string (quark_blob_id), id);
    }
  else
    {
      bse_storage_break (self);
      bse_storage_printf (self, "(%s ", g_quark_to_string (quark_blob));
      bse_storage_push_level (self);
      bse_storage_break (self);
      sfi_wstore_put_binary (self->wstore, wstore_blob_reader, wstore_blob_new (self, blob), wstore_blob_destroy);
      bse_storage_pop_level (self);
      bse_storage_putc (self, ')');
    }
}

GTokenType
bse_storage_parse_blob (BseStorage             *self,
                        BseStorage::BlobP      &blob_out)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  int bse_fd = -1;
  int tmp_fd = -1;
  std::string file_name = Bse::string_format ("%s/bse-storage-blob-%s-%u", bse_storage_blob_tmp_dir(), g_get_user_name(), getpid());

  // add enough randomness to ensure that collisions will not happen
  for (int i = 0; i < 5; i++)
    file_name += Bse::string_format ("-%08x", g_random_int());

  blob_out = nullptr; /* on error, the resulting blob should be NULL */

  parse_or_return (scanner, '(');
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  if (g_quark_try_string (scanner->value.v_identifier) == quark_blob)
    {
      SfiNum offset, length;
      GTokenType token = sfi_rstore_parse_binary (self->rstore, &offset, &length);
      if (token != G_TOKEN_NONE)
	return token;

      char buffer[1024];
      bse_fd = open (self->rstore->fname, O_RDONLY);
      if (bse_fd < 0)
	{
	  bse_storage_error (self, "couldn't open file %s for reading: %s\n", self->rstore->fname, strerror (errno));
	  goto return_with_error;
	}
      tmp_fd = open (file_name.c_str(), O_CREAT | O_WRONLY, 0600);
      if (tmp_fd < 0)
	{
	  bse_storage_error (self, "couldn't open file %s for writing: %s\n", file_name, strerror (errno));
	  goto return_with_error;
	}
      int result = lseek (bse_fd, offset, SEEK_SET);
      if (result != offset)
	{
	  bse_storage_error (self, "could not seek to position %lld in bse file %s\n", offset, self->rstore->fname);
	  goto return_with_error;
	}
      int bytes_todo = length;
      while (bytes_todo > 0)
	{
	  int rbytes, wbytes;

          do
            rbytes = read (bse_fd, buffer, MIN (bytes_todo, 1024));
          while (rbytes == -1 && errno == EINTR);

	  if (rbytes == -1)
	    {
	      bse_storage_error (self, "error while reading file %s: %s\n", self->rstore->fname, strerror (errno));
	      goto return_with_error;
	    }
	  if (rbytes == 0)
	    {
	      bse_storage_error (self, "end-of-file occured too early in file %s\n", self->rstore->fname);
	      goto return_with_error;
	    }

	  int bytes_written = 0;
	  while (bytes_written != rbytes)
	    {
	      do
		wbytes = write (tmp_fd, &buffer[bytes_written], rbytes - bytes_written);
	      while (wbytes == -1 && errno == EINTR);
	      if (wbytes == -1)
		{
		  bse_storage_error (self, "error while writing file %s: %s\n", self->rstore->fname, strerror (errno));
		  goto return_with_error;
		}
	      bytes_written += wbytes;
	    }

	  bytes_todo -= rbytes;
	}
      close (bse_fd);
      close (tmp_fd);
      blob_out = std::make_shared<BseStorage::Blob> (file_name, true);
    }
  else if (g_quark_try_string (scanner->value.v_identifier) == quark_blob_id)
    {
      gulong id;
      parse_or_return (scanner, G_TOKEN_INT);
      id = scanner->value.v_int64;
      blob_out = NULL;
      for (auto blob : self->data.blobs)
	{
	  if (blob->id() == id)
	    blob_out = blob;
;
	}
      if (!blob_out)
	{
	  Bse::warning ("failed to lookup storage blob with id=%ld\n", id);
	  goto return_with_error;
	}
     }
  else
    {
      goto return_with_error;
    }
  parse_or_return (scanner, ')');

  return G_TOKEN_NONE;

return_with_error:
  if (bse_fd != -1)
    close (bse_fd);
  if (tmp_fd != -1)
    {
      close (tmp_fd);
      unlink (file_name.c_str());
    }
  return G_TOKEN_ERROR;
}

Bse::Error
bse_storage_flush_fd (BseStorage *self,
                      gint        fd)
{
  assert_return (BSE_IS_STORAGE (self), Bse::Error::INTERNAL);
  assert_return (self->wstore, Bse::Error::INTERNAL);
  assert_return (fd >= 0, Bse::Error::INTERNAL);
  bse_storage_break (self);
  gint nerrno = sfi_wstore_flush_fd (self->wstore, fd);
  return bse_error_from_errno (-nerrno, Bse::Error::FILE_WRITE_FAILED);
}

void
bse_storage_compat_dhreset (BseStorage     *self)
{
  self->n_channels = 1;
  self->mix_freq = 44100;
  self->osc_freq = 440;
}

void
bse_storage_compat_dhmixf (BseStorage     *self,
                           gfloat          mix_freq)
{
  self->mix_freq = mix_freq;
}

void
bse_storage_compat_dhoscf (BseStorage     *self,
                           gfloat          osc_freq)
{
  self->osc_freq = osc_freq;
}

void
bse_storage_compat_dhchannels (BseStorage     *self,
                               guint           n_channels)
{
  self->n_channels = n_channels;
}

static GTokenType
compat_parse_data_handle (BseStorage     *self,
                          GslDataHandle **data_handle_p,
                          guint          *n_channels_p,
                          gfloat         *mix_freq_p,
                          gfloat         *osc_freq_p)
{
  guint offset, bytes_per_value, length, vlength, byte_order = G_LITTLE_ENDIAN;
  GScanner *scanner = bse_storage_get_scanner (self);
  GTokenType token;
  gchar *string;

  parse_or_return (scanner, G_TOKEN_INT);
  offset = scanner->value.v_int64;

  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  string = scanner->value.v_identifier;
  if (string[0] == 'L' || string[0] == 'l')
    byte_order = G_LITTLE_ENDIAN;
  else if (string[0] == 'B' || string[0] == 'b')
    byte_order = G_BIG_ENDIAN;
  else
    string = NULL;
  if (string && string[1] != ':')
    string = NULL;
  if (string)
    {
      gchar *f = NULL;

      bytes_per_value = strtol (string + 2, &f, 10);
      if ((bytes_per_value != 1 && bytes_per_value != 2 && bytes_per_value != 4) ||
          (f && *f != 0))
        string = NULL;
    }
  if (!string)
    return bse_storage_warn_skip (self,
                                  "unknown value type `%s' in binary data definition",
                                  scanner->value.v_identifier);

  parse_or_return (scanner, G_TOKEN_INT);
  length = scanner->value.v_int64;
  if (length < bytes_per_value)
    return G_TOKEN_INT;

  if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
    {
      g_scanner_get_next_token (scanner);
      vlength = scanner->value.v_int64;
      if (vlength < 1 || vlength * bytes_per_value > length)
        return G_TOKEN_INT;
    }
  else
    vlength = length / bytes_per_value;

  parse_or_return (scanner, ')');

  token = sfi_rstore_ensure_bin_offset (self->rstore);
  if (token != G_TOKEN_NONE)
    return token;

  if (n_channels_p)
    *n_channels_p = self->n_channels;
  if (mix_freq_p)
    *mix_freq_p = self->mix_freq;
  if (osc_freq_p)
    *osc_freq_p = self->osc_freq;
  *data_handle_p = gsl_wave_handle_new (self->rstore->fname, self->n_channels,
                                        bytes_per_value == 1 ? GSL_WAVE_FORMAT_SIGNED_8 :
                                        bytes_per_value == 2 ? GSL_WAVE_FORMAT_SIGNED_16 :
                                        GSL_WAVE_FORMAT_FLOAT,
                                        byte_order,
                                        self->mix_freq, self->osc_freq,
                                        sfi_rstore_get_bin_offset (self->rstore) + offset,
                                        vlength, NULL);
  return G_TOKEN_NONE;
}
