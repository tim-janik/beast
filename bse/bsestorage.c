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
#include "bsebindata.h"
#include "bseproject.h"
#include "bseparasite.h"
#include <bse/bseconfig.h>
#include "gsldatahandle.h"
#include "gsldatautils.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#define	DEBUG	sfi_nodebug


/* --- macros --- */
#define parse_or_return		bse_storage_scanner_parse_or_return
#define peek_or_return		bse_storage_scanner_peek_or_return


/* --- typedefs --- */
struct _BseStorageBBlock
{
  BseStorageBBlock *next;
  GslLong	    write_voffset;
  GslLong	    vlength;
  GslDataHandle	   *data_handle;
  guint		    bytes_per_value;
  gulong	    storage_offset;	/* in bytes */
  gulong	    storage_length;	/* in bytes */
};
struct _BseStorageItemLink
{
  BseStorageItemLink   *next;
  BseItem	       *from_item;
  BseStorageRestoreLink restore_link;
  gpointer              data;
  guint			pbackup;
  gchar		       *upath;
  BseItem	       *to_item;
  gchar		       *error;
};


/* --- prototypes --- */
static void	bse_storage_init			(BseStorage	 *self);
static void	bse_storage_class_init			(BseStorageClass *class);
static void	bse_storage_finalize			(GObject	 *object);
static void	storage_path_table_insert		(BseStorage	*self,
							 BseContainer	*container,
							 const gchar	*uname,
							 BseItem	*item);
static BseItem*	storage_path_table_resolve_upath	(BseStorage	*self,
							 BseContainer	*container,
							 gchar		*upath);
static guint	uname_child_hash			(gconstpointer	 uc);
static gint	uname_child_equals			(gconstpointer   uc1,
							 gconstpointer   uc2);
static void	uname_child_free			(gpointer	 uc);


/* --- variables --- */
static gpointer parent_class = NULL;


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
				   "Storage object for object serialization",
				   &storage_info);
}

static void
bse_storage_class_init (BseStorageClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = bse_storage_finalize;
}

static void
bse_storage_init (BseStorage *self)
{
  /* reading */
  self->scanner = NULL;
  self->fd = -1;
  self->bin_offset = 0;
  self->item_links = NULL;
  /* writing */
  self->indent = NULL;
  self->wblocks = NULL;
  self->gstring = NULL;

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
bse_storage_reset (BseStorage *self)
{
  g_return_if_fail (BSE_IS_STORAGE (self));
  
  if (BSE_STORAGE_READABLE (self))
    {
      bse_storage_resolve_item_links (self);
      g_hash_table_destroy (self->path_table);
      self->path_table = NULL;
      g_free ((gchar*) self->scanner->input_name);
      g_scanner_destroy (self->scanner);
      if (self->fd >= 0)
        close (self->fd);
      self->scanner = NULL;
      self->fd = -1;
      self->bin_offset = 0;
    }
  self->major_version = 0;
  self->minor_version = 5;
  self->micro_version = 0;
  
  if (BSE_STORAGE_WRITABLE (self))
    {
      GSList *slist;

      for (slist = self->indent; slist; slist = slist->next)
        g_free (slist->data);
      g_slist_free (self->indent);
      self->indent = NULL;
      
      while (self->wblocks)
        {
          BseStorageBBlock *bblock = self->wblocks;
          self->wblocks = bblock->next;
	  gsl_data_handle_unref (bblock->data_handle);
          g_free (bblock);
        }
      
      g_string_free (self->gstring, TRUE);
      self->gstring = NULL;
    }

  BSE_OBJECT_UNSET_FLAGS (self,
			  BSE_STORAGE_FLAG_READABLE |
			  BSE_STORAGE_FLAG_WRITABLE |
			  BSE_STORAGE_FLAG_NEEDS_BREAK |
			  BSE_STORAGE_FLAG_AT_BOL |
			  BSE_STORAGE_FLAG_PUT_DEFAULTS |
			  BSE_STORAGE_FLAG_SELF_CONTAINED);
}

void
bse_storage_prepare_write (BseStorage    *self,
                           BseStorageMode mode)
{
  g_return_if_fail (BSE_IS_STORAGE (self));

  bse_storage_reset (self);
  self->indent = g_slist_prepend (NULL, g_strdup (""));
  self->gstring = g_string_sized_new (8192);
  BSE_OBJECT_SET_FLAGS (self, BSE_STORAGE_FLAG_WRITABLE);
  BSE_OBJECT_SET_FLAGS (self, BSE_STORAGE_FLAG_AT_BOL);
  if (mode & BSE_STORAGE_SKIP_DEFAULTS)
    BSE_OBJECT_UNSET_FLAGS (self, BSE_STORAGE_FLAG_PUT_DEFAULTS);
  else
    BSE_OBJECT_SET_FLAGS (self, BSE_STORAGE_FLAG_PUT_DEFAULTS);
  bse_storage_break (self);
  if (!(mode & BSE_STORAGE_SKIP_COMPAT))
    {
      bse_storage_printf (self, "(bse-version \"%u.%u.%u\")", BSE_MAJOR_VERSION, BSE_MINOR_VERSION, BSE_MICRO_VERSION);
      bse_storage_break (self);
    }
}

BseErrorType
bse_storage_input_file (BseStorage  *self,
                        const gchar *file_name)
{
  gint fd;
  
  g_return_val_if_fail (BSE_IS_STORAGE (self), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  bse_storage_reset (self);
  fd = open (file_name, O_RDONLY, 0);
  if (fd < 0)
    return bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
  self->fd = fd;
  self->scanner = g_scanner_new (sfi_storage_scanner_config);
  g_datalist_set_data (&self->scanner->qdata, "BseStorage", self);
  g_scanner_input_file (self->scanner, fd);
  self->scanner->input_name = g_strdup (file_name);
  self->scanner->max_parse_errors = 1;
  self->scanner->parse_errors = 0;
  self->path_table = g_hash_table_new_full (uname_child_hash, uname_child_equals, NULL, uname_child_free);
  BSE_OBJECT_SET_FLAGS (self, BSE_STORAGE_FLAG_READABLE);
  
  return BSE_ERROR_NONE;
}

BseErrorType
bse_storage_input_text (BseStorage  *self,
                        const gchar *text)
{
  g_return_val_if_fail (BSE_IS_STORAGE (self), BSE_ERROR_INTERNAL);
  if (!text)
    text = "";

  bse_storage_reset (self);
  self->fd = -1;
  self->scanner = g_scanner_new (sfi_storage_scanner_config);
  g_datalist_set_data (&self->scanner->qdata, "BseStorage", self);
  g_scanner_input_text (self->scanner, text, strlen (text));
  self->scanner->input_name = g_strdup ("InternalString");
  self->scanner->max_parse_errors = 1;
  self->scanner->parse_errors = 0;
  self->path_table = g_hash_table_new_full (uname_child_hash, uname_child_equals, NULL, uname_child_free);
  BSE_OBJECT_SET_FLAGS (self, BSE_STORAGE_FLAG_READABLE);
  
  return BSE_ERROR_NONE;
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

      if (sfi_pspec_test_hint (pspec, SFI_PARAM_SERVE_STORAGE))
	{
	  GValue value = { 0, };

	  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  g_object_get_property (G_OBJECT (item), pspec->name, &value);
	  if (!g_param_value_defaults (pspec, &value) || BSE_STORAGE_PUT_DEFAULTS (storage))
	    bse_item_store_property (item, storage, &value, pspec);
	  g_value_unset (&value);
	}
    }
  g_free (pspecs);
}

void
bse_storage_store_item (BseStorage *self,
			gpointer    item)
{
  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (BSE_IS_ITEM (item));

  g_object_ref (self);
  g_object_ref (item);

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
  g_return_if_fail (BSE_IS_ITEM (item));

  uname = g_strescape (BSE_OBJECT_UNAME (item), NULL);
  bse_storage_break (self);
  bse_storage_printf (self, "(%s \"%s::%s\"", "container-child", G_OBJECT_TYPE_NAME (item), uname);
  bse_storage_needs_break (self);
  g_free (uname);

  bse_storage_push_level (self);
  bse_storage_store_item (self, item);
  bse_storage_pop_level (self);
  bse_storage_handle_break (self);
  bse_storage_putc (self, ')');
}

static GTokenType
storage_parse_bse_version (BseStorage *self)
{
  GScanner *scanner = self->scanner;
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
      if ((!ep || *ep == 0) && vmajor >= 0 && vminor >= 0 && vmicro >= 0 &&
          BSE_VERSION_CMP (vmajor, vminor, vmicro, 0, 0, 0) > 0)
        {
          parsed_version = TRUE;
          if (BSE_VERSION_CMP (vmajor, vminor, vmicro,
                               self->major_version,
                               self->minor_version,
                               self->micro_version) > 0)
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

static void
item_link_resolved (gpointer     data,
		    BseStorage  *storage,
		    BseItem     *item,
		    BseItem     *dest_item,
		    const gchar *error)
{
  if (error)
    bse_storage_warn (storage, error);
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

static BseTokenType item_restore_try_statement (gpointer    item,
                                                BseStorage *storage,
                                                gpointer    user_data);

static GTokenType
restore_item_property (BseItem    *item,
		       BseStorage *storage)
{
  GScanner *scanner = storage->scanner;
  GTokenType expected_token;
  GParamSpec *pspec;
  GValue value = { 0, };

  /* check identifier */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return BSE_TOKEN_UNMATCHED;

  /* in theory, we should only find SFI_PARAM_SERVE_STORAGE
   * properties here, but due to version changes or even
   * users editing their files, we will simply parse all
   * kinds of properties (we might want to at least restrict
   * them to SFI_PARAM_SERVE_STORAGE and SFI_PARAM_SERVE_GUI
   * at some point...)
   */
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (item), scanner->next_value.v_identifier);
  if (!pspec)
    return BSE_TOKEN_UNMATCHED;
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);	/* eat pspec name */

  /* parse value, special casing object references */
  if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), BSE_TYPE_ITEM))
    {
      expected_token = bse_storage_parse_item_link (storage, item, item_link_resolved, pspec);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;
      parse_or_return (scanner, ')');
      /* we cannot provide the object value at this time */
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_value_set_object (&value, NULL);
    }
  else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), G_TYPE_OBJECT))
    return bse_storage_warn_skip (storage, "unable to restore object property \"%s\" of type `%s'",
				  pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
  else
    {
      /* parse the value for this pspec, including the closing ')' */
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      expected_token = bse_storage_parse_param_value (storage, &value, pspec);
      if (expected_token != G_TOKEN_NONE)
	{
	  g_value_unset (&value);
	  return expected_token;
	}
    }

  /* set property value while preserving the object uname */
  g_object_set_property (G_OBJECT (item), /* no undo */
                         pspec->name, &value);
  g_value_unset (&value);

  return G_TOKEN_NONE;
}

static GTokenType
restore_container_child (BseContainer *container,
			 BseStorage   *storage)
{
  GScanner *scanner = storage->scanner;
  GTokenType expected_token;
  BseItem *item;
  const gchar *uname;
  gchar *type_name, *tmp;

  /* check identifier */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER ||
      !bse_string_equals ("container-child", scanner->next_value.v_identifier))
    return BSE_TOKEN_UNMATCHED;
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);        /* eat identifier */

  /* parse and validate type::uname argument */
  parse_or_return (scanner, G_TOKEN_STRING);
  uname = strchr (scanner->value.v_string, ':');
  if (!uname || uname[1] != ':')
    {
      bse_storage_error (storage, "invalid object handle: \"%s\"", scanner->value.v_string);
      return G_TOKEN_ERROR;
    }
  type_name = g_strndup (scanner->value.v_string, uname - scanner->value.v_string);
  uname += 2;

  /* handle different versions */
  tmp = bse_compat_rewrite_type_name (storage->major_version, storage->minor_version, storage->micro_version, type_name);
  if (tmp)
    {
      g_free (type_name);
      type_name = tmp;
    }
  
  /* check container's storage filter */
  if (!bse_container_check_restore (container, type_name))
    {
      g_free (type_name);
      return bse_storage_warn_skip (storage, "ignoring child: \"%s\"", scanner->value.v_string);
    }

  /* create container child */
  tmp = g_strconcat (type_name, "::", uname, NULL);
  g_free (type_name);
  item = bse_container_retrieve_child (container, tmp);
  g_free (tmp);
  if (!item)
    return bse_storage_warn_skip (storage, "failed to create object from (invalid?) handle: \"%s\"",
				  scanner->value.v_string);

  storage_path_table_insert (storage, container, uname, item);

  /* restore_item reads out closing parenthesis */
  g_object_ref (item);
  expected_token = bse_storage_parse_rest (storage, ')', item_restore_try_statement, item, NULL);
  g_object_unref (item);
  if (expected_token != G_TOKEN_NONE)
    return expected_token == BSE_TOKEN_UNMATCHED ? G_TOKEN_ERROR : expected_token;

  return G_TOKEN_NONE;
}

static BseTokenType
item_restore_try_statement (gpointer    item,
			    BseStorage *storage,
			    gpointer    user_data)
{
  GScanner *scanner = storage->scanner;
  GTokenType expected_token = BSE_TOKEN_UNMATCHED;

  /* ensure that the statement starts out with an identifier */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    {
      g_scanner_get_next_token (scanner);
      return G_TOKEN_IDENTIFIER;
    }

  /* this is pretty much the *only* place where something else than
   * G_TOKEN_NONE may be returned without erroring out. return values:
   * G_TOKEN_NONE	 - statement got parsed, advance to next statement
   * BSE_TOKEN_UNMATCHED - statement not recognized, try further
   * anything else	 - encountered (syntax/semantic) error during parsing
   */

  if (expected_token == BSE_TOKEN_UNMATCHED)
    expected_token = restore_item_property (item, storage);

  if (expected_token == BSE_TOKEN_UNMATCHED)
    expected_token = BSE_OBJECT_GET_CLASS (item)->restore_private (item, storage);

  if (expected_token == BSE_TOKEN_UNMATCHED)
    expected_token = bse_parasite_restore (item, storage);

  if (expected_token == BSE_TOKEN_UNMATCHED && BSE_IS_CONTAINER (item))
    expected_token = restore_container_child (item, storage);

  if (expected_token == BSE_TOKEN_UNMATCHED && strcmp (scanner->next_value.v_identifier, "bse-version") == 0)
    expected_token = storage_parse_bse_version (storage);

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

  expected_token = bse_storage_parse_rest (self, G_TOKEN_EOF, item_restore_try_statement, item, NULL);

  g_object_unref (item);
  g_object_unref (self);

  return expected_token;
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
			   BseItem	*item)
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

static inline const gchar*
bse_storage_get_indent (BseStorage *self)
{
  return self->indent->data;
}

static inline void
bse_storage_indent (BseStorage *self)
{
  bse_storage_puts (self, bse_storage_get_indent (self));
}

void
bse_storage_push_level (BseStorage *self)
{
  guint indent_width = 2;

  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (BSE_STORAGE_WRITABLE (self));

  self->indent = g_slist_prepend (self->indent,
				  g_strnfill (indent_width +
					      strlen (bse_storage_get_indent (self)),
					      ' '));
}

void
bse_storage_pop_level (BseStorage *self)
{
  GSList *next;

  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (BSE_STORAGE_WRITABLE (self));

  next = self->indent->next;
  if (next)
    {
      g_free (self->indent->data);
      g_slist_free_1 (self->indent);
      self->indent = next;
    }
}

void
bse_storage_putc (BseStorage *storage,
                  gchar       character)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  if (storage->gstring)
    g_string_append_c (storage->gstring, character);
  
  if (character == '\n')
    BSE_OBJECT_SET_FLAGS (storage, BSE_STORAGE_FLAG_AT_BOL);
  else
    BSE_OBJECT_UNSET_FLAGS (storage, BSE_STORAGE_FLAG_AT_BOL);
}

void
bse_storage_puts (BseStorage  *storage,
                  const gchar *string)
{
  guint l;

  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));

  if (!string)
    return;
  l = strlen (string);
  if (!l)
    return;

  if (storage->gstring)
    g_string_append (storage->gstring, string);
  
  if (string[l - 1] == '\n')
    BSE_OBJECT_SET_FLAGS (storage, BSE_STORAGE_FLAG_AT_BOL);
  else
    BSE_OBJECT_UNSET_FLAGS (storage, BSE_STORAGE_FLAG_AT_BOL);
}

void
bse_storage_putf (BseStorage *self,
		  gfloat      vfloat)
{
  gchar numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = "";

  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (BSE_STORAGE_WRITABLE (self));

  g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.7g", vfloat);

  bse_storage_puts (self, numbuf);
}

void
bse_storage_putd (BseStorage *self,
		  gdouble     vdouble)
{
  gchar numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = "";

  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (BSE_STORAGE_WRITABLE (self));

  g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.17g", vdouble);

  bse_storage_puts (self, numbuf);
}

void
bse_storage_putr (BseStorage     *self,
		  SfiReal         vreal,
		  const gchar    *hints)
{
  g_return_if_fail (BSE_IS_STORAGE (self));
  g_return_if_fail (BSE_STORAGE_WRITABLE (self));

  if (hints && strstr (hints, ":"SFI_PARAM_FLOAT))
    bse_storage_putf (self, vreal);
  else
    bse_storage_putd (self, vreal);
}

void
bse_storage_printf (BseStorage  *storage,
                    const gchar *format,
                    ...)
{
  gchar *buffer;
  va_list args;
  
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (format != NULL);
  
  va_start (args, format);
  buffer = g_strdup_vprintf (format, args);
  va_end (args);
  
  bse_storage_puts (storage, buffer);
  
  g_free (buffer);
}

void
bse_storage_break (BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  bse_storage_putc (storage, '\n');
  BSE_OBJECT_UNSET_FLAGS (storage, BSE_STORAGE_FLAG_NEEDS_BREAK);
  bse_storage_indent (storage);
}

void
bse_storage_handle_break (BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  if (BSE_STORAGE_NEEDS_BREAK (storage))
    bse_storage_break (storage);
}

void
bse_storage_needs_break (BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  
  BSE_OBJECT_SET_FLAGS (storage, BSE_STORAGE_FLAG_NEEDS_BREAK);
}

static BseStorageBBlock*
bse_storage_ensure_wblock (BseStorage    *storage,
			   guint          bytes_per_value,
			   GslDataHandle *data_handle,
			   GslLong        voffset,
			   GslLong        vlength)
{
  BseStorageBBlock *bblock, *last = NULL;

  for (bblock = storage->wblocks; bblock; last = bblock, bblock = last->next)
    if (bblock->data_handle == data_handle && bblock->bytes_per_value == bytes_per_value &&
	bblock->write_voffset == voffset && bblock->vlength == vlength)
      return bblock;

  /* create */
  bblock = g_new0 (BseStorageBBlock, 1);
  bblock->write_voffset = voffset;
  bblock->vlength = vlength;
  bblock->data_handle = gsl_data_handle_ref (data_handle);
  bblock->bytes_per_value = bytes_per_value;
  bblock->storage_offset = last ? last->storage_offset + last->storage_length : 0;
  bblock->storage_length = vlength * bblock->bytes_per_value;
  /* align to 4 bytes */
  bblock->storage_length += 4 - 1;
  bblock->storage_length /= 4;
  bblock->storage_length *= 4;
  if (last)
    last->next = bblock;
  else
    storage->wblocks = bblock;
  
  return bblock;
}

void
bse_storage_put_data_handle (BseStorage    *storage,
			     guint	    significant_bits,
			     GslDataHandle *data_handle,
			     GslLong        vlength)
{
  BseStorageBBlock *bblock;
  guint bytes_per_value;

  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (data_handle != NULL);
  g_return_if_fail (vlength > 0);

  if (significant_bits <= 8)
    bytes_per_value = 1;
  else if (significant_bits <= 16)
    bytes_per_value = 2;
  else
    bytes_per_value = 4;

  bblock = bse_storage_ensure_wblock (storage, bytes_per_value, data_handle, 0, vlength);

  bse_storage_handle_break (storage);
  bse_storage_printf (storage,
                      "(BseStorageBinaryV0 %lu %c:%u %lu %lu)",
                      bblock->storage_offset,
                      G_BYTE_ORDER == G_LITTLE_ENDIAN ? 'L' : 'B',
                      bblock->bytes_per_value,
                      bblock->storage_length,
		      bblock->vlength);
}

const gchar*
bse_storage_peek_text (BseStorage *storage,
		       guint      *length)
{
  g_return_val_if_fail (BSE_IS_STORAGE (storage), NULL);
  g_return_val_if_fail (BSE_STORAGE_WRITABLE (storage), NULL);

  bse_storage_handle_break (storage);

  if (length)
    *length = storage->gstring->len;
  return storage->gstring->str;
}

gchar*
bse_storage_mem_flush (BseStorage *storage)
{
  gchar *mem;

  g_return_val_if_fail (BSE_IS_STORAGE (storage), NULL);
  g_return_val_if_fail (BSE_STORAGE_WRITABLE (storage), NULL);

  bse_storage_handle_break (storage);

  mem = g_new (gchar, storage->gstring->len + 1);
  memcpy (mem, storage->gstring->str, storage->gstring->len + 1);

  // FIXME: we're not handling binary appendices here
  return mem;
}

void
bse_storage_flush_fd (BseStorage *storage,
                      gint        fd)
{
  gint l;
  
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (fd >= 0);

  bse_storage_handle_break (storage);
  
  /* dump text storage
   */
  do
    l = write (fd, storage->gstring->str, storage->gstring->len);
  while (l < 0 && errno == EINTR);
  
  do
    l = write (fd, "\n", 1);
  while (l < 0 && errno == EINTR);
  
  /* dump binary data
   */
  if (storage->wblocks)
    {
      BseStorageBBlock *bblock;
      gchar term[] = "\n; binary appendix:\n";
      guint n = strlen (term) + 1;
      
      do
        l = write (fd, term, n);
      while (l < 0 && errno == EINTR);
      
      for (bblock = storage->wblocks; bblock; bblock = bblock->next)
	{
	  GslErrorType error = gsl_data_handle_open (bblock->data_handle);
	  GslLong vlength = bblock->vlength;
	  GslLong voffset = bblock->write_voffset, pad;
	  
	  if (error)
	    bse_storage_warn (storage, "failed to open data handle (%s) for reading: %s",
			      gsl_data_handle_name (bblock->data_handle),
			      gsl_strerror (error));
	  while (vlength > 0)
	    {
	      gfloat fbuffer[8192];
	      GslLong l = MIN (8192, vlength);
	      guint n_retries = 4;
	      gssize s;
	      
	      if (error)
		memset (fbuffer, 0, l * sizeof (fbuffer[0]));
	      else
		do
		  l = gsl_data_handle_read (bblock->data_handle, voffset, l, fbuffer);
		while (l < 1 && n_retries--);
	      if (l < 1)
		{
		  bse_storage_warn (storage, "failed to read from data handle (%s): %s",
				    gsl_data_handle_name (bblock->data_handle),
				    gsl_strerror (error));
		  memset (fbuffer, 0, l * sizeof (fbuffer[0]));
		}
	      voffset += l;
	      vlength -= l;
	      if (bblock->bytes_per_value == 1)
		{
		  gsl_conv_from_float_clip (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER,
					    fbuffer, fbuffer, l);
		}
	      else if (bblock->bytes_per_value == 2)
		{
                  gsl_conv_from_float_clip (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER,
                                            fbuffer, fbuffer, l);
		  l *= 2;
		}
	      else
		l *= 4;
	      do
		s = write (fd, fbuffer, l);
	      while (s < 0 && errno == EINTR);
	    }
	  if (!error)
	    gsl_data_handle_close (bblock->data_handle);
	  pad = bblock->storage_length - bblock->vlength * bblock->bytes_per_value;
	  while (pad > 0)
            {
              guint8 buffer[1024] = { 0, };
              guint n = MIN (pad, 1024);
              
              do
                l = write (fd, buffer, n);
              while (l < 0 && errno == EINTR);
              pad -= n;
            }
        }
    }
}

static BseStorageItemLink*
storage_add_item_link (BseStorage           *storage,
		       BseItem              *from_item,
		       BseStorageRestoreLink restore_link,
		       gpointer              data,
		       gchar		    *error)
{
  BseStorageItemLink *ilink = g_new0 (BseStorageItemLink, 1);

  ilink->next = storage->item_links;
  storage->item_links = ilink;
  ilink->from_item = g_object_ref (from_item);
  ilink->restore_link = restore_link;
  ilink->data = data;
  ilink->error = error;

  return ilink;
}

void
bse_storage_resolve_item_links (BseStorage *storage)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_READABLE (storage));

  while (storage->item_links)
    {
      BseStorageItemLink *ilink = storage->item_links;

      storage->item_links = ilink->next;

      if (ilink->error)
	{
	  gchar *error = g_strdup_printf ("unable to resolve link path for item `%s': %s",
					  BSE_OBJECT_UNAME (ilink->from_item),
					  ilink->error);
	  ilink->restore_link (ilink->data, storage, ilink->from_item, NULL, error);
	  g_free (error);
	  if (ilink->to_item)
	    g_object_unref (ilink->to_item);
	  g_free (ilink->error);
	}
      else if (ilink->to_item)
	{
	  ilink->restore_link (ilink->data, storage, ilink->from_item, ilink->to_item, NULL);
	  g_object_unref (ilink->to_item);
	}
      else if (!ilink->upath)
	{
	  ilink->restore_link (ilink->data, storage, ilink->from_item, NULL, NULL);
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
	      child = storage_path_table_resolve_upath (storage, BSE_CONTAINER (parent), ilink->upath);
	      if (!child)
		error = g_strdup_printf ("failed to find object for item `%s' while resolving link path \"%s\" from ancestor `%s'",
					 BSE_OBJECT_UNAME (ilink->from_item),
					 ilink->upath, BSE_OBJECT_UNAME (parent));
	    }
	  ilink->restore_link (ilink->data, storage, ilink->from_item, child, error);
	  g_free (error);
	}
      g_object_unref (ilink->from_item);
      g_free (ilink->upath);
      g_free (ilink);
    }
}

static GTokenType
storage_skipc_statement (BseStorage *storage,
			 guint       level)
{
  GScanner *scanner = storage->scanner;

  g_return_val_if_fail (level > 0, G_TOKEN_ERROR);

 loop:
  switch (scanner->token)
    {
    case G_TOKEN_EOF:
    case G_TOKEN_ERROR:
      return ')';
    case '(':
      level++;
      break;
    case ')':
      level--;
      break;
    default:
      break;
    }
  
  if (level)
    {
      g_scanner_get_next_token (scanner);
      goto loop;
    }
  
  return G_TOKEN_NONE;
}

GTokenType
bse_storage_parse_rest (BseStorage     *storage,
                        GTokenType      closing_token,
                        BseTryStatement try_statement,
                        gpointer        func_data,
                        gpointer        user_data)
{
  GScanner *scanner;
  
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  
  scanner = storage->scanner;
  
  /* we catch any BSE_TOKEN_UNMATCHED at this level, this is merely
   * a "magic" token value to implement the try_statement() semantics
   */
  while (!bse_storage_input_eof (storage) &&
         g_scanner_get_next_token (scanner) == '(')
    {
      GTokenType expected_token;
      
      /* it is only usefull to feature statements that start
       * out with an identifier (syntactically)
       */
      if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
        {
          /* eat token and bail out */
          g_scanner_get_next_token (scanner);
          return G_TOKEN_IDENTIFIER;
        }
      
      /* parse a statement */
      if (try_statement)
        expected_token = try_statement (func_data, storage, user_data);
      else
        expected_token = BSE_TOKEN_UNMATCHED;
      
      /* if there are no matches, skip statement */
      if (expected_token == BSE_TOKEN_UNMATCHED)
        {
          if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
            {
              g_warning (G_STRLOC ": try_statement() implementation <%p> is broken", try_statement);
              return G_TOKEN_ERROR;
            }
          expected_token = bse_storage_warn_skip (storage,
                                                  "unknown identifier: %s",
                                                  scanner->value.v_identifier);
        }
      
      /* bail out on errors */
      if (expected_token != G_TOKEN_NONE)
        return expected_token;
    }

  return scanner->token == closing_token ? G_TOKEN_NONE : closing_token;
}

gboolean
bse_storage_input_eof (BseStorage *storage)
{
  g_return_val_if_fail (BSE_IS_STORAGE (storage), FALSE);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), FALSE);

  return (g_scanner_eof (storage->scanner) ||
          storage->scanner->parse_errors >= storage->scanner->max_parse_errors);
}

void
bse_storage_warn (BseStorage  *storage,
                  const gchar *format,
                  ...)
{
  va_list args;
  gchar *string;
  
  g_return_if_fail (BSE_IS_STORAGE (storage));
  
  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  
  if (BSE_STORAGE_READABLE (storage))
    g_scanner_warn (storage->scanner, "%s", string);
  else
    g_printerr ("during storage: %s", string);
  
  g_free (string);
}

static GTokenType
bse_storage_skip_statement (BseStorage *storage)
{
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);

  g_scanner_get_next_token (storage->scanner);
  
  return storage_skipc_statement (storage, 1);
}

GTokenType
bse_storage_warn_skip (BseStorage  *storage,
                       const gchar *format,
                       ...)
{
  va_list args;
  gchar *string;
  
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);

  /* construct warning *before* modifying scanner state */
  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  
  if (storage->scanner->parse_errors < storage->scanner->max_parse_errors)
    g_scanner_warn (storage->scanner, "%s - skipping...", string);
  
  g_free (string);

  return bse_storage_skip_statement (storage);
}

GTokenType
bse_storage_warn_skipc (BseStorage  *storage,
			const gchar *format,
			...)
{
  va_list args;
  gchar *string;
  
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  
  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  
  if (storage->scanner->parse_errors < storage->scanner->max_parse_errors)
    g_scanner_warn (storage->scanner, "%s - skipping...", string);
  
  g_free (string);
  
  return storage_skipc_statement (storage, 1);
}

void
bse_storage_error (BseStorage  *storage,
                   const gchar *format,
                   ...)
{
  va_list args;
  gchar *string;
  
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_READABLE (storage));
  
  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  
  if (storage->scanner->parse_errors < storage->scanner->max_parse_errors)
    g_scanner_error (storage->scanner, "%s", string);
  
  g_free (string);
}

void
bse_storage_unexp_token (BseStorage *storage,
                         GTokenType  expected_token)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_READABLE (storage));
  
  if (storage->scanner->parse_errors < storage->scanner->max_parse_errors)
    {
      gchar *message;
      
      if (storage->scanner->parse_errors + 1 >= storage->scanner->max_parse_errors)
        message = "aborting...";
      else
        message = NULL;
      g_scanner_unexp_token (storage->scanner, expected_token, NULL, NULL, NULL, message, TRUE);
    }
}

static BseErrorType
bse_storage_ensure_bin_offset (BseStorage *storage,
                               glong      *cur_offs_p)
{
  gint fd = storage->fd;
  glong coffs;
  
  if (fd < 0)
    return BSE_ERROR_FILE_NOT_FOUND;
  
  coffs = lseek (fd, 0, SEEK_CUR);
  if (coffs < 0)
    return BSE_ERROR_FILE_IO;
  
  if (!storage->bin_offset)
    {
      glong bin_offset, l;
      gboolean seen_zero = FALSE;
      
      bin_offset = lseek (fd, 0, SEEK_SET);
      if (bin_offset != 0)
        return BSE_ERROR_FILE_IO;
      
      do
        {
          guint8 data[4096];
          guint i;
          
          do
            l = read (fd, data, 4096);
          while (l < 0 && errno == EINTR);
          
          if (l < 0)
            return BSE_ERROR_FILE_IO;
          
          for (i = 0; i < l; i++)
            if (data[i] == 0)
              {
                seen_zero = TRUE;
                break;
              }
          bin_offset += seen_zero ? i : l;
        }
      while (!seen_zero && l);
      
      if (seen_zero)
        storage->bin_offset = bin_offset;
      else
        return BSE_ERROR_FILE_IO;
    }
  
  if (lseek (fd, coffs, SEEK_SET) != coffs)
    return BSE_ERROR_FILE_IO;
  
  if (cur_offs_p)
    *cur_offs_p = coffs;
  
  return BSE_ERROR_NONE;
}

GTokenType
bse_storage_parse_data_handle (BseStorage     *storage,
			       guint           n_channels,
			       gfloat          osc_freq,
			       gfloat          mix_freq,
			       GslDataHandle **data_handle_p)
{
  GScanner *scanner;
  BseStorageBBlock bblock = { 0, };
  BseEndianType byte_order = BSE_LITTLE_ENDIAN;
  BseErrorType error;
  gchar *string;

  g_return_val_if_fail (data_handle_p != NULL, G_TOKEN_ERROR);
  *data_handle_p = NULL;
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (n_channels > 0, G_TOKEN_ERROR);

  scanner = storage->scanner;

  parse_or_return (scanner, '(');
  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  if (!bse_string_equals ("BseStorageBinaryV0", scanner->value.v_identifier))
    return G_TOKEN_IDENTIFIER;
  parse_or_return (scanner, G_TOKEN_INT);
  bblock.storage_offset = scanner->value.v_int;

  parse_or_return (scanner, G_TOKEN_IDENTIFIER);
  string = scanner->value.v_identifier;
  if (string[0] == 'L' || string[0] == 'l')
    byte_order = BSE_LITTLE_ENDIAN;
  else if (string[0] == 'B' || string[0] == 'b')
    byte_order = BSE_BIG_ENDIAN;
  else
    string = NULL;
  if (string && string[1] != ':')
    string = NULL;
  if (string)
    {
      gchar *f = NULL;
      
      bblock.bytes_per_value = strtol (string + 2, &f, 10);
      if ((bblock.bytes_per_value != 1 && bblock.bytes_per_value != 2 &&
           bblock.bytes_per_value != 4) ||
          (f && *f != 0))
        string = NULL;
    }
  if (!string)
    return bse_storage_warn_skip (storage,
                                  "unknown value type `%s' in binary data definition",
                                  scanner->value.v_identifier);
  
  parse_or_return (scanner, G_TOKEN_INT);
  bblock.storage_length = scanner->value.v_int;
  if (bblock.storage_length < bblock.bytes_per_value)
    return G_TOKEN_INT;
  
  if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
    {
      g_scanner_get_next_token (scanner);
      bblock.vlength = scanner->value.v_int;
      if (bblock.vlength < 1 || bblock.vlength * bblock.bytes_per_value > bblock.storage_length)
	return G_TOKEN_INT;
    }
  else
    bblock.vlength = bblock.storage_length / bblock.bytes_per_value;
  
  parse_or_return (scanner, ')');
  
  error = bse_storage_ensure_bin_offset (storage, NULL);
  if (error)
    {
      /* except for BSE_ERROR_FILE_NOT_FOUND, all errors are fatal ones,
       * we can't guarantee that further parsing is possible.
       */
      if (error == BSE_ERROR_FILE_NOT_FOUND)
	bse_storage_warn (storage, "no device to retrieve binary data from");
      else
	bse_storage_error (storage, "failed to retrieve binary data: %s", bse_error_blurb (error));
      return G_TOKEN_ERROR;
    }

  *data_handle_p = gsl_wave_handle_new (storage->scanner->input_name, n_channels,
					bblock.bytes_per_value == 1 ? GSL_WAVE_FORMAT_SIGNED_8 :
					bblock.bytes_per_value == 2 ? GSL_WAVE_FORMAT_SIGNED_16 :
					GSL_WAVE_FORMAT_FLOAT,
					byte_order,
					storage->bin_offset + 1 + bblock.storage_offset,
					bblock.vlength);
  
  return G_TOKEN_NONE;
}

GTokenType
bse_storage_parse_note (BseStorage *storage,
                        gint       *note_p,
                        gchar       bbuffer[BSE_BBUFFER_SIZE])
{
  GScanner *scanner;
  gint note;
  gchar ibuffer[BSE_BBUFFER_SIZE];
  
  if (note_p)
    *note_p = BSE_NOTE_UNPARSABLE;
  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  
  if (!bbuffer)
    bbuffer = ibuffer;
  
  scanner = storage->scanner;
  
  g_scanner_get_next_token (scanner);
  if (scanner->token == G_TOKEN_STRING)
    bse_bbuffer_puts (bbuffer, scanner->value.v_string);
  else
    return G_TOKEN_STRING;

  note = bse_note_from_string (bbuffer);
  
  if (note_p)
    *note_p = note;
  
  return G_TOKEN_NONE;
}

void
bse_storage_put_param (BseStorage   *storage,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (G_IS_VALUE (value));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  
  // bse_storage_handle_break (storage);
  bse_storage_put_value (storage, value, pspec);
}

void
bse_storage_put_item_link (BseStorage     *storage,
			   BseItem        *from_item,
			   BseItem        *to_item)
{
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (BSE_IS_ITEM (from_item));
  g_return_if_fail (BSE_IS_ITEM (to_item));

  bse_storage_handle_break (storage);

  if (!to_item)						/* special case (1) */
    {
      bse_storage_puts (storage, SFI_SERIAL_NULL_TOKEN);
    }
  else		/* ordiniary object link within a project or other container */
    {
      BseItem *tmp, *common_ancestor;
      guint pbackup = 0;
      gchar *upath, *epath;

      g_return_if_fail (BSE_IS_ITEM (to_item));
      common_ancestor = bse_item_common_ancestor (from_item, to_item);
      g_return_if_fail (BSE_IS_CONTAINER (common_ancestor));

      /* figure number of parent backup levels to reach common ancestor */
      for (tmp = from_item; tmp != common_ancestor; tmp = tmp->parent)
	pbackup++;

      /* path to reach to_item */
      upath = bse_container_make_upath (BSE_CONTAINER (common_ancestor), to_item);

      /* store path reference */
      epath = g_strescape (upath, NULL);
      bse_storage_printf (storage, "(link %u \"%s\")", pbackup, epath);
      g_free (epath);
      g_free (upath);
    }
}

void
bse_storage_put_value (BseStorage   *storage,
		       const GValue *value,
		       GParamSpec   *bsepspec)
{
  const gchar *cstring;
  guint indent_len;
  GString *gstring;

  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (BSE_STORAGE_WRITABLE (storage));
  g_return_if_fail (G_IS_VALUE (value));

  if (bsepspec)
    {
      gboolean fixed = FALSE;
      GParamSpec *pspec;
      GValue *svalue;

      g_return_if_fail (G_IS_PARAM_SPEC (bsepspec));

      pspec = sfi_pspec_to_serializable (bsepspec);
      if (!pspec)
	g_error ("unable to serialize \"%s\" of type `%s'", bsepspec->name,
		 g_type_name (G_PARAM_SPEC_VALUE_TYPE (bsepspec)));

      gstring = g_string_new (NULL);
      svalue = sfi_value_empty ();
      g_value_init (svalue, G_PARAM_SPEC_VALUE_TYPE (pspec));
      if (!sfi_value_transform (value, svalue))
	{
	  g_warning ("unable to transform \"%s\" of type `%s' to `%s'",
		     bsepspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (bsepspec)),
		     g_type_name (G_VALUE_TYPE (svalue)));
	  goto cleanup;
	}
      else if (G_VALUE_TYPE (svalue) != G_VALUE_TYPE (value))
	fixed |= g_param_value_validate (pspec, svalue);
      
      if (fixed)
	g_message ("fixing up contents of \"%s\" during serialization", pspec->name);
      
      cstring = bse_storage_get_indent (storage);
      indent_len = cstring ? strlen (cstring) : 0;

      BSE_OBJECT_UNSET_FLAGS (storage, BSE_STORAGE_FLAG_NEEDS_BREAK);  // Sfi inserts newline and indent
      sfi_value_store_param (svalue, gstring, pspec, indent_len);
      
    cleanup:
      sfi_value_free (svalue);
      g_param_spec_unref (pspec);
    }
  else
    {
      gstring = g_string_new (NULL);
      bse_storage_handle_break (storage);
      sfi_value_store_typed (value, gstring);
    }
  bse_storage_puts (storage, gstring->str);
  g_string_free (gstring, TRUE);
}

GTokenType
bse_storage_parse_param_value (BseStorage *storage,
                               GValue     *value,
                               GParamSpec *bsepspec)
{
  GScanner *scanner;
  GParamSpec *pspec;
  GTokenType token;
  GValue *pvalue;

  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (G_IS_VALUE (value), G_TOKEN_ERROR);
  g_return_val_if_fail (G_IS_PARAM_SPEC (bsepspec), G_TOKEN_ERROR);

  scanner = storage->scanner;
  pspec = sfi_pspec_to_serializable (bsepspec);
  if (!pspec)
    g_error ("unable to serialize \"%s\" of type `%s'", bsepspec->name,
	     g_type_name (G_PARAM_SPEC_VALUE_TYPE (bsepspec)));
  pvalue = sfi_value_empty ();
  token = sfi_value_parse_param_rest (pvalue, scanner, pspec);
  if (token == G_TOKEN_NONE)
    {
      gboolean fixup = FALSE;
      fixup = g_param_value_validate (pspec, pvalue);
      if (!sfi_value_transform (pvalue, value))
	g_warning ("unable to value of type `%s' for \"%s\" to `%s'",
		   g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		   pspec->name,
		   g_type_name (G_VALUE_TYPE (value)));
      else if (G_VALUE_TYPE (pvalue) != G_VALUE_TYPE (value))
	fixup |= g_param_value_validate (bsepspec, value);
      if (fixup)
	g_scanner_warn (scanner, "fixing up contents of \"%s\"", pspec->name);
    }
  g_param_spec_unref (pspec);
  sfi_value_free (pvalue);
  return token;
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
bse_storage_parse_item_link (BseStorage           *storage,
			     BseItem              *from_item,
			     BseStorageRestoreLink restore_link,
			     gpointer              data)
{
  GScanner *scanner;
  BseStorageItemLink *ilink;
  GTokenType expected_token;

  g_return_val_if_fail (BSE_IS_STORAGE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), G_TOKEN_ERROR);
  g_return_val_if_fail (BSE_IS_ITEM (from_item), G_TOKEN_ERROR);
  g_return_val_if_fail (restore_link != NULL, G_TOKEN_ERROR);

  scanner = storage->scanner;

#define	parse_or_goto(etoken,label) \
  { expected_token = (etoken); if (g_scanner_get_next_token (scanner) != expected_token) goto label; }
#define	peek_or_goto(etoken,label)  \
  { expected_token = (etoken); if (g_scanner_peek_next_token (scanner) != expected_token) \
    { g_scanner_get_next_token (scanner); goto label; } }

  g_scanner_get_next_token (scanner);

  if (sfi_serial_check_parse_null_token (scanner))
    {
      ilink = storage_add_item_link (storage, from_item, restore_link, data, NULL);
    }
  else if (scanner->token == '(')
    {
      parse_or_goto (G_TOKEN_IDENTIFIER, error_parse_link);

      if (strcmp (scanner->value.v_identifier, "link") == 0)
	{
	  guint pbackup = 0;

	  if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
	    {
	      g_scanner_get_next_token (scanner);	/* eat int */
	      pbackup = scanner->value.v_int;
	    }

	  parse_or_goto (G_TOKEN_STRING, error_parse_link);
          peek_or_goto (')', error_parse_link);

	  ilink = storage_add_item_link (storage, from_item, restore_link, data, NULL);
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
  
#undef	parse_or_goto
#undef	peek_or_goto

 error_parse_link:
  ilink = storage_add_item_link (storage, from_item, restore_link, data, g_strdup ("failed to parse link path"));
  return expected_token;
}
