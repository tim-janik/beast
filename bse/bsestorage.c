/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2003 Tim Janik
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
#include "bsestorage.h"
#include "bseitem.h"
#include "gsldatahandle.h"
#include "gsldatahandle-vorbis.h"
#include "gsldatautils.h"
#include "gslcommon.h"
#include "bseproject.h"
#include "bseparasite.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


/* --- macros --- */
#define DEBUG           sfi_nodebug
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
static void       bse_storage_class_init           (BseStorageClass  *class);
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
static GQuark   quark_vorbis_data_handle = 0;
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

  g_assert (BSE_STORAGE_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT);

  return bse_type_register_static (BSE_TYPE_OBJECT, "BseStorage",
                                   "Storage object for item serialization",
                                   &storage_info);
}

static void
bse_storage_class_init (BseStorageClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  quark_raw_data_handle = g_quark_from_static_string ("raw-data-handle");
  quark_vorbis_data_handle = g_quark_from_static_string ("vorbis-data-handle");
  quark_dblock_data_handle = g_quark_from_static_string ("dblock-data-handle");
  quark_bse_storage_binary_v0 = g_quark_from_static_string ("BseStorageBinaryV0");

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
  self->item_links = NULL;
  /* misc */
  self->dblocks = NULL;
  self->n_dblocks = 0;
  self->free_me = NULL;

  bse_storage_reset (self);
}

static void
bse_storage_finalize (GObject *object)
{
  BseStorage *self = BSE_STORAGE (object);

  bse_storage_reset (self);

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

  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (BSE_STORAGE_DBLOCK_CONTAINED (self));
  g_return_if_fail (self->wstore);
  g_return_if_fail (self->wstore->flushed == FALSE);
  g_return_if_fail (self->wstore->bblocks == NULL);
  g_return_if_fail (self->free_me == NULL);

  bse_storage_break (self);

  cmem = sfi_wstore_peek_text (self->wstore, &l);
  text = g_memdup (cmem, l + 1);
  dblocks = self->dblocks;
  n_dblocks = self->n_dblocks;
  self->dblocks = NULL;
  self->n_dblocks = 0;

  bse_storage_input_text (self, text, storage_name);
  self->free_me = text;
  self->dblocks = dblocks;
  self->n_dblocks = n_dblocks;
  BSE_OBJECT_SET_FLAGS (self, BSE_STORAGE_DBLOCK_CONTAINED);
}

void
bse_storage_reset (BseStorage *self)
{
  guint i;

  g_return_if_fail (BSE_IS_STORAGE (self));

  if (self->rstore)
    {
      bse_storage_resolve_item_links (self);
      g_hash_table_destroy (self->path_table);
      self->path_table = NULL;
      sfi_rstore_destroy (self->rstore);
      self->rstore = NULL;
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

  self->major_version = BSE_MAJOR_VERSION;
  self->minor_version = BSE_MINOR_VERSION;
  self->micro_version = BSE_MICRO_VERSION;

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
  g_return_if_fail (BSE_IS_STORAGE (self));

  bse_storage_reset (self);
  self->wstore = sfi_wstore_new ();
  self->stored_items = sfi_ppool_new ();
  self->referenced_items = sfi_ppool_new ();
  mode &= BSE_STORAGE_MODE_MASK;
  if (mode & BSE_STORAGE_DBLOCK_CONTAINED)
    mode |= BSE_STORAGE_SELF_CONTAINED;
  BSE_OBJECT_SET_FLAGS (self, mode);
  bse_storage_break (self);
  bse_storage_printf (self, "(bse-version \"%u.%u.%u\")\n\n", BSE_MAJOR_VERSION, BSE_MINOR_VERSION, BSE_MICRO_VERSION);
}

void
bse_storage_input_text (BseStorage  *self,
                        const gchar *text,
                        const gchar *text_name)
{
  g_return_if_fail (BSE_IS_STORAGE (self));

  if (!text)
    text = "";

  bse_storage_reset (self);
  self->rstore = sfi_rstore_new ();
  self->rstore->parser_this = self;
  sfi_rstore_input_text (self->rstore, text, text_name);
  self->path_table = g_hash_table_new_full (uname_child_hash, uname_child_equals, NULL, uname_child_free);
}

BseErrorType
bse_storage_input_file (BseStorage  *self,
                        const gchar *file_name)
{
  g_return_val_if_fail (BSE_IS_STORAGE (self), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  bse_storage_reset (self);
  self->rstore = sfi_rstore_new_open (file_name);
  if (!self->rstore)
    return bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
  self->rstore->parser_this = self;
  self->path_table = g_hash_table_new_full (uname_child_hash, uname_child_equals, NULL, uname_child_free);

  return BSE_ERROR_NONE;
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
  if (0)
    g_printerr ("bse-version: code: %u.%u.%u file: %u.%u.%u feature(current):%d compat(current):%d compat(-1):%d\n",
                BSE_MAJOR_VERSION, BSE_MINOR_VERSION, BSE_MICRO_VERSION,
                self->major_version, self->minor_version, self->micro_version,
                BSE_STORAGE_VERSION (self, BSE_MAJOR_VERSION, BSE_MINOR_VERSION, BSE_MICRO_VERSION),
                BSE_STORAGE_COMPAT (self, BSE_MAJOR_VERSION, BSE_MINOR_VERSION, BSE_MICRO_VERSION),
                BSE_STORAGE_COMPAT (self, BSE_MAJOR_VERSION, BSE_MINOR_VERSION, BSE_MICRO_VERSION - 1));
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
  ilink->from_item = g_object_ref (from_item);
  ilink->restore_link = restore_link;
  ilink->data = data;
  ilink->error = error;

  return ilink;
}

void
bse_storage_resolve_item_links (BseStorage *self)
{
  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (self->rstore != NULL);

  while (self->item_links)
    {
      BseStorageItemLink *ilink = sfi_ring_pop_head (&self->item_links);

      if (ilink->error)
        {
          gchar *error = g_strdup_printf ("unable to resolve link path for item `%s': %s",
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
            error = g_strdup_printf ("failed to find ancestor of item `%s' (branch depth: -%u, "
                                     "number of parents: %u) while resolving link path \"%s\"",
                                     BSE_OBJECT_UNAME (ilink->from_item),
                                     ilink->pbackup,
                                     ilink->pbackup - pbackup + 1,
                                     ilink->upath);
          else
            {
              child = storage_path_table_resolve_upath (self, BSE_CONTAINER (parent), ilink->upath);
              if (!child)
                error = g_strdup_printf ("failed to find object for item `%s' while resolving link path \"%s\" from ancestor `%s'",
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
}

const gchar*
bse_storage_item_get_compat_type (BseItem *item)
{
  const gchar *type = g_object_get_data (item, "BseStorage-compat-type");
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
  const UNameChild *uchild = uc;
  guint h = g_str_hash (uchild->uname);
  h ^= G_HASH_LONG ((long) uchild->container);
  return h;
}

static gint
uname_child_equals (gconstpointer uc1,
                    gconstpointer uc2)
{
  const UNameChild *uchild1 = uc1;
  const UNameChild *uchild2 = uc2;
  return (bse_string_equals (uchild1->uname, uchild2->uname) &&
          uchild1->container == uchild2->container);
}

static void
uname_child_free (gpointer uc)
{
  UNameChild *uchild = uc;
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
  UNameChild key, *uchild;
  key.container = container;
  key.uname = (gchar*) uname;
  uchild = g_hash_table_lookup (self->path_table, &key);
  if (!uchild)
    {
      uchild = g_new (UNameChild, 1);
      uchild->container = g_object_ref (container);
      uchild->uname = g_strdup (uname);
      uchild->item = NULL;
      g_hash_table_insert (self->path_table, uchild, uchild);
    }
  if (uchild->item)
    g_object_unref (uchild->item);
  uchild->item = g_object_ref (item);
  DEBUG ("INSERT: (%p,%s) => %p", container, uname, item);
}

static inline BseItem*
storage_path_table_lookup (BseStorage   *self,
                           BseContainer *container,
                           const gchar  *uname)
{
  UNameChild key, *uchild;
  key.container = container;
  key.uname = (gchar*) uname;
  uchild = g_hash_table_lookup (self->path_table, &key);
  DEBUG ("LOOKUP: (%p,%s) => %p", container, uname, uchild ? uchild->item : NULL);
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
  gchar *next_uname = strchr (upath, ':');
  /* upaths consist of colon seperated unames from the item's ancestry */
  if (next_uname)
    {
      BseItem *item;
      next_uname[0] = 0;
      item = storage_path_table_lookup (self, container, upath);
      next_uname[0] = ':';
      if (BSE_IS_CONTAINER (item))
        return storage_path_table_lookup (self, BSE_CONTAINER (item), next_uname + 1);
      else
        return NULL;
    }
  else
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
    bse_storage_warn (self, error);
  else
    {
      GParamSpec *pspec = data;
      GValue value = { 0, };

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_value_set_object (&value, dest_item);
      g_object_set_property (G_OBJECT (item), /* no undo */
                             pspec->name, &value);
      g_value_unset (&value);
    }
}

static SfiTokenType item_restore_try_statement (gpointer    item,
                                                BseStorage *self,
                                                GScanner   *scanner,
                                                gpointer    user_data);

static GTokenType
restore_item_property (BseItem    *item,
                       BseStorage *self)
{
  GScanner *scanner = bse_storage_get_scanner (self);
  GTokenType expected_token;
  GParamSpec *pspec;
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
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (item), scanner->next_value.v_identifier);
  if (!pspec)
    return SFI_TOKEN_UNMATCHED;
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
    bse_storage_warn (self, "ignoring non-writable object property \"%s\" of type `%s'",
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
  BseMidiControlType control_type = sfi_choice2enum (scanner->value.v_identifier, BSE_TYPE_MIDI_CONTROL_TYPE);
  /* close statement */
  parse_or_return (scanner, ')');
  BseErrorType error = bse_source_set_automation_property (BSE_SOURCE (item), pspec->name, midi_channel, control_type);
  if (error)
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
    g_object_set_data_full (item, "BseStorage-compat-type", compat_type, g_free);
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

static SfiTokenType
item_restore_try_statement (gpointer    item,
                            BseStorage *self,
                            GScanner   *scanner,
                            gpointer    user_data)
{
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

  if (expected_token == SFI_TOKEN_UNMATCHED)
    expected_token = restore_item_property (item, self);

  if (expected_token == SFI_TOKEN_UNMATCHED)
    expected_token = restore_source_automation (item, self);

  if (expected_token == SFI_TOKEN_UNMATCHED)
    expected_token = BSE_OBJECT_GET_CLASS (item)->restore_private (item, self, scanner);

  if (expected_token == SFI_TOKEN_UNMATCHED)
    expected_token = bse_parasite_restore (item, self);

  if (expected_token == SFI_TOKEN_UNMATCHED && BSE_IS_CONTAINER (item))
    expected_token = restore_container_child (item, self);

  if (expected_token == SFI_TOKEN_UNMATCHED && strcmp (scanner->next_value.v_identifier, "bse-version") == 0)
    expected_token = storage_parse_bse_version (self);

  return expected_token;
}

GTokenType
bse_storage_restore_item (BseStorage *self,
                          gpointer    item)
{
  GTokenType expected_token;

  g_return_val_if_fail (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_IS_ITEM (item), G_TOKEN_ERROR);

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
  g_return_val_if_fail (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  g_return_val_if_fail (self->rstore != NULL, G_TOKEN_ERROR);

  return sfi_rstore_parse_until (self->rstore, ')', context_data, (SfiStoreParser) try_statement, user_data);
}

gboolean
bse_storage_check_parse_negate (BseStorage *self)
{
  g_return_val_if_fail (BSE_IS_STORAGE (self), FALSE);

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
  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (self->wstore);
  g_return_if_fail (G_IS_VALUE (value));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  sfi_wstore_put_param (self->wstore, value, pspec);
}

GTokenType
bse_storage_parse_param_value (BseStorage *self,
                               GValue     *value,
                               GParamSpec *pspec)
{
  g_return_val_if_fail (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  g_return_val_if_fail (self->rstore, G_TOKEN_ERROR);

  return sfi_rstore_parse_param (self->rstore, value, pspec);
}

void
bse_storage_put_item_link (BseStorage *self,
                           BseItem    *from_item,
                           BseItem    *to_item)
{
  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (self->wstore);
  g_return_if_fail (BSE_IS_ITEM (from_item));
  g_return_if_fail (BSE_IS_ITEM (to_item));

  if (!to_item)                                         /* special case (1) */
    {
      bse_storage_puts (self, SFI_SERIAL_NULL_TOKEN);
    }
  else          /* ordinary object link within a project or other container */
    {
      BseItem *tmp, *common_ancestor;
      guint pbackup = 0;
      gchar *upath, *epath;

      g_return_if_fail (BSE_IS_ITEM (to_item));
      common_ancestor = bse_item_common_ancestor (from_item, to_item);
      g_return_if_fail (BSE_IS_CONTAINER (common_ancestor));

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

/**
 * BseStorageRestoreLink
 * @data:      user data
 * @storage:   #BseStorage instance
 * @from_item: link owner
 * @to_item:   link target or NULL
 * @error:     error string describing failing link lookups
 *
 * BseStorageRestoreLink() is a user supplied handler to be called
 * at the end of a parsing stage, once object references could be
 * resolved. Failing resolutions are indicated by non %NULL @error
 * strings.
 */

/**
 * bse_storage_parse_item_link
 * @storage:      valid #BseStorage
 * @from_item:    link owner
 * @restore_link: BseStorageRestoreLink handler to be called once the link was resolved
 * @data:         user data passed into @restore_link()
 * @RETURNS:      expected token in case of a parsing error (%G_TOKEN_NONE on success)
 *
 * Parse an item link statement and return the expected token if a parsing
 * error occours. Item links are resolved at the end of the parsing stage
 * by calling the user supplied handler @restore_link() with the link target
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

  g_return_val_if_fail (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  g_return_val_if_fail (self->rstore, G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_IS_ITEM (from_item), G_TOKEN_ERROR);
  g_return_val_if_fail (restore_link != NULL, G_TOKEN_ERROR);

  scanner = bse_storage_get_scanner (self);

#define parse_or_goto(etoken,label) \
  { expected_token = (etoken); if (g_scanner_get_next_token (scanner) != expected_token) goto label; }
#define peek_or_goto(etoken,label)  \
  { expected_token = (etoken); if (g_scanner_peek_next_token (scanner) != expected_token) \
    { g_scanner_get_next_token (scanner); goto label; } }

  g_scanner_get_next_token (scanner);

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
          peek_or_goto (')', error_parse_link);

          ilink = storage_add_item_link (self, from_item, restore_link, data, NULL);
          ilink->upath = g_strdup (scanner->value.v_string);
          ilink->pbackup = pbackup;
        }
      else
        {
          expected_token = G_TOKEN_IDENTIFIER;
          goto error_parse_link;
        }
      parse_or_goto (')', error_parse_link);
    }
  else
    {
      expected_token = '(';
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
bse_storage_warn (BseStorage  *self,
                  const gchar *format,
                  ...)
{
  va_list args;
  gchar *string;
  
  g_return_if_fail (BSE_IS_STORAGE (self));
  
  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  
  if (self->rstore)
    sfi_rstore_warn (self->rstore, "%s", string);
  else
    g_printerr ("BseStorage: while storing: %s", string);
  
  g_free (string);
}

GTokenType
bse_storage_warn_skip (BseStorage  *self,
                       const gchar *format,
                       ...)
{
  va_list args;
  gchar *string;
  GTokenType token;

  g_return_val_if_fail (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  g_return_val_if_fail (self->rstore != NULL, G_TOKEN_ERROR);

  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  token = sfi_rstore_warn_skip (self->rstore, "%s", string);
  g_free (string);
  return token;
}

void
bse_storage_error (BseStorage  *self,
                   const gchar *format,
                   ...)
{
  va_list args;
  gchar *string;
  
  g_return_if_fail (BSE_IS_STORAGE (self));
  
  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  if (self->rstore)
    sfi_rstore_error (self->rstore, "%s", string);
  else
    g_printerr ("BseStorage: ERROR: while storing: %s\n", string);
  g_free (string);
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
      bse_storage_put_item_link (storage, item, g_value_get_object (value));
      bse_storage_putc (storage, ')');
    }
  else if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_OBJECT))
    g_warning ("%s: unable to store object property \"%s\" of type `%s'",
               G_STRLOC, pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
  else
    bse_storage_put_param (storage, value, pspec);
}

static void
bse_source_store_automation (BseSource  *source,
                             BseStorage *storage,
                             GParamSpec *pspec)
{
  guint midi_channel = 0;
  BseMidiControlType control_type = 0;
  bse_source_get_automation_property (source, pspec->name, &midi_channel, &control_type);
  if (control_type)
    {
      bse_storage_break (storage);
      bse_storage_printf (storage, "(source-automate \"%s\" %u %s)", pspec->name,
                          midi_channel, sfi_enum2choice (control_type, BSE_TYPE_MIDI_CONTROL_TYPE));
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

void
bse_storage_store_item (BseStorage *self,
                        gpointer    item)
{
  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (self->wstore);
  g_return_if_fail (BSE_IS_ITEM (item));

  g_object_ref (self);
  g_object_ref (item);

  sfi_ppool_set (self->stored_items, item);

  store_item_properties (item, self);

  BSE_OBJECT_GET_CLASS (item)->store_private (BSE_OBJECT (item), self);

  bse_parasite_store (BSE_OBJECT (item), self);

  if (BSE_IS_CONTAINER (item))
    bse_container_store_children (item, self);

  g_object_unref (item);
  g_object_unref (self);
}

void
bse_storage_store_child (BseStorage *self,
                         gpointer    item)
{
  gchar *uname;

  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (self->wstore);
  g_return_if_fail (BSE_IS_ITEM (item));

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
  gchar numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = "";

  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (self->wstore);

  g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.7g", vfloat);

  bse_storage_puts (self, numbuf);
}

void
bse_storage_putd (BseStorage *self,
                  gdouble     vdouble)
{
  gchar numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = "";

  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (self->wstore);

  g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.17g", vdouble);

  bse_storage_puts (self, numbuf);
}

void
bse_storage_putr (BseStorage     *self,
                  SfiReal         vreal,
                  const gchar    *hints)
{
  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (self->wstore);

  if (hints && g_option_check (hints, "f"))     /* check float option */
    bse_storage_putf (self, vreal);
  else
    bse_storage_putd (self, vreal);
}

void
bse_storage_printf (BseStorage  *self,
                    const gchar *format,
                    ...)
{
  gchar *buffer;
  va_list args;
  
  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (self->wstore);
  g_return_if_fail (format != NULL);
  
  va_start (args, format);
  buffer = g_strdup_vprintf (format, args);
  va_end (args);
  
  bse_storage_puts (self, buffer);
  
  g_free (buffer);
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
      return 'f';
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
    return '(';
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
} WStoreDHandle;

static void
wstore_data_handle_destroy (gpointer data)
{
  WStoreDHandle *wh = data;
  if (wh->opened)
    gsl_data_handle_close (wh->dhandle);
  gsl_data_handle_unref (wh->dhandle);
  g_free (wh);
}

static gint /* -errno || length */
wstore_data_handle_reader (gpointer data,
                           SfiNum   pos,
                           void    *buffer,
                           guint    blength)
{
  WStoreDHandle *wh = data;
  GslLong n;
  
  if (!wh->opened)
    {
      BseErrorType error = gsl_data_handle_open (wh->dhandle);
      if (error)
        {
          bse_storage_error (wh->storage, "failed to open data handle: %s", bse_error_blurb (error));
          return -ENOENT;
        }
      wh->opened = TRUE;
    }

  /* shouldn't need to seek, check alignment */
  g_return_val_if_fail (pos % wh->bpv == 0, -EIO);

  /* catch end */
  if (pos / wh->bpv >= gsl_data_handle_length (wh->dhandle))
    return 0;

  do
    n = gsl_data_handle_read (wh->dhandle, pos / wh->bpv, blength / sizeof (gfloat), buffer);
  while (n < 0 && errno == EINTR);
  if (n < 0)    /* bail out */
    {
      bse_storage_error (wh->storage, "failed to read from data handle");
      return -EIO;
    }

  return gsl_conv_from_float_clip (wh->format, wh->byte_order, buffer, buffer, n);
}

void
bse_storage_put_data_handle (BseStorage    *self,
                             guint          significant_bits,
                             GslDataHandle *dhandle)
{
  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (self->wstore);
  g_return_if_fail (dhandle != NULL);
  g_return_if_fail (GSL_DATA_HANDLE_OPENED (dhandle));

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
  if (vhandle)  /* save already compressed Ogg/Vorbis data */
    {
      bse_storage_break (self);
      bse_storage_printf (self,
                          "(%s %.7g",
                          g_quark_to_string (quark_vorbis_data_handle),
                          gsl_data_handle_osc_freq (dhandle));
      bse_storage_push_level (self);
      bse_storage_break (self);
      gsl_vorbis1_handle_put_wstore (vhandle, self->wstore);
      bse_storage_pop_level (self);
      bse_storage_putc (self, ')');
    }
  else          /* save raw data handle */
    {
      if (significant_bits < 1)
        significant_bits = 32;
      guint format = gsl_data_handle_bit_depth (dhandle);
      significant_bits = MIN (format, significant_bits);
      if (significant_bits > 16)
        format = GSL_WAVE_FORMAT_FLOAT;
      else if (significant_bits <= 8)
        format = GSL_WAVE_FORMAT_SIGNED_8;
      else
        format = GSL_WAVE_FORMAT_SIGNED_16;
      
      bse_storage_break (self);
      bse_storage_printf (self,
                          "(%s %u %s %s %.7g %.7g",
                          g_quark_to_string (quark_raw_data_handle),
                          gsl_data_handle_n_channels (dhandle),
                          gsl_wave_format_to_string (format),
                          gsl_byte_order_to_string (G_LITTLE_ENDIAN),
                          gsl_data_handle_mix_freq (dhandle),
                          gsl_data_handle_osc_freq (dhandle));
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
  guint n_channels, format, byte_order;
  gfloat mix_freq, osc_freq;
  SfiNum offset, length;
  GTokenType token;

  parse_or_return (scanner, G_TOKEN_INT);
  n_channels = scanner->value.v_int64;
  if (n_channels <= 0 || n_channels > 256)
    return bse_storage_warn_skip (self, "invalid number of channels: %u", n_channels);

  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  format = gsl_wave_format_from_string (scanner->value.v_identifier);
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
parse_vorbis_data_handle (BseStorage     *self,
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
      *data_handle_p = gsl_data_handle_new_ogg_vorbis_zoffset (self->rstore->fname, osc_freq,
                                                               offset, length, n_channels_p, &mix_freq);
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
      quark == quark_vorbis_data_handle)
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
  else if (quark == quark_vorbis_data_handle)
    return parse_vorbis_data_handle (self, data_handle_p, n_channels_p, mix_freq_p, osc_freq_p);

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
  g_return_val_if_fail (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  g_return_val_if_fail (self->rstore, G_TOKEN_ERROR);
  g_return_val_if_fail (data_handle_p != NULL, G_TOKEN_ERROR);

  return parse_data_handle_trampoline (self, FALSE, data_handle_p, n_channels_p, mix_freq_p, osc_freq_p);
}

GTokenType
bse_storage_parse_data_handle_rest (BseStorage     *self,
                                    GslDataHandle **data_handle_p,
                                    guint          *n_channels_p,
                                    gfloat         *mix_freq_p,
                                    gfloat         *osc_freq_p)
{
  g_return_val_if_fail (BSE_IS_STORAGE (self), G_TOKEN_ERROR);
  g_return_val_if_fail (self->rstore, G_TOKEN_ERROR);
  g_return_val_if_fail (data_handle_p != NULL, G_TOKEN_ERROR);

  return parse_data_handle_trampoline (self, TRUE, data_handle_p, n_channels_p, mix_freq_p, osc_freq_p);
}

void
bse_storage_flush_fd (BseStorage *self,
                      gint        fd)
{
  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (self->wstore);
  g_return_if_fail (fd >= 0);

  bse_storage_break (self);

  sfi_wstore_flush_fd (self->wstore, fd);
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
