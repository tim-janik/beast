// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsedatapocket.hh"
#include "bsecxxplugin.hh"

#include "bsemain.hh"
#include "bsestorage.hh"

#include <string.h>

/* --- macros --- */
#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return


/* --- structures --- */
typedef struct _Notify Notify;
struct _Notify
{
  Notify        *next;
  BseDataPocket *pocket;
  uint           entry_id;
};


/* --- prototypes --- */
static void	    bse_data_pocket_init		(BseDataPocket		*pocket);
static void	    bse_data_pocket_class_init		(BseDataPocketClass	*klass);
static void	    bse_data_pocket_class_finalize	(BseDataPocketClass	*klass);
static void	    bse_data_pocket_dispose		(GObject		*object);
static void	    bse_data_pocket_finalize		(GObject		*object);
static gboolean     bse_data_pocket_needs_storage       (BseItem                *item,
                                                         BseStorage             *storage);
static void	    bse_data_pocket_do_store_private	(BseObject		*object,
							 BseStorage		*storage);
static GTokenType   bse_data_pocket_restore_private	(BseObject		*object,
							 BseStorage		*storage,
                                                         GScanner               *scanner);


/* --- variables --- */
static void         *parent_class = NULL;
static uint          signal_entry_added = 0;
static uint          signal_entry_removed = 0;
static uint          signal_entry_changed = 0;
static Notify       *changed_notify_list = NULL;
static GQuark	     quark_set_data = 0;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseDataPocket)
{
  static const GTypeInfo data_pocket_info = {
    sizeof (BseDataPocketClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_data_pocket_class_init,
    (GClassFinalizeFunc) bse_data_pocket_class_finalize,
    NULL /* class_data */,

    sizeof (BseDataPocket),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_data_pocket_init,
  };

  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseDataPocket",
				   "Data pocket type",
                                   __FILE__, __LINE__,
                                   &data_pocket_info);
}

static void
bse_data_pocket_class_init (BseDataPocketClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  quark_set_data = g_quark_from_static_string ("set-data");

  gobject_class->dispose = bse_data_pocket_dispose;
  gobject_class->finalize = bse_data_pocket_finalize;

  object_class->store_private = bse_data_pocket_do_store_private;
  object_class->restore_private = bse_data_pocket_restore_private;

  item_class->needs_storage = bse_data_pocket_needs_storage;

  signal_entry_added = bse_object_class_add_signal (object_class, "entry-added",
						    G_TYPE_NONE, 1, G_TYPE_INT);
  signal_entry_removed = bse_object_class_add_signal (object_class, "entry-removed",
						      G_TYPE_NONE, 1, G_TYPE_INT);
  signal_entry_changed = bse_object_class_add_signal (object_class, "entry-changed",
						      G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
bse_data_pocket_class_finalize (BseDataPocketClass *klass)
{
}

static void
bse_data_pocket_init (BseDataPocket *pocket)
{
  pocket->free_id = 1;
  pocket->n_entries = 0;
  pocket->entries = NULL;
  pocket->need_store = 0;
  pocket->cr_items = NULL;
}

static void
bse_data_pocket_dispose (GObject *object)
{
  BseDataPocket *pocket = BSE_DATA_POCKET (object);

  /* set disposal flag early, since we check for it internally */
  BSE_OBJECT_SET_FLAGS (object, BSE_OBJECT_FLAG_DISPOSING);

  while (pocket->n_entries)
    _bse_data_pocket_delete_entry (pocket, pocket->entries[0].id);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);

  g_return_if_fail (pocket->cr_items == NULL);
}

static void
bse_data_pocket_finalize (GObject *object)
{
  BseDataPocket *pocket = BSE_DATA_POCKET (object);
  Notify *notify, *last = NULL;

  while (pocket->n_entries)
    _bse_data_pocket_delete_entry (pocket, pocket->entries[0].id);

  for (notify = changed_notify_list; notify; )
    {
      if (notify->pocket == pocket)
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

  g_return_if_fail (pocket->cr_items == NULL);
}

static gboolean
changed_notify_handler (void *data)
{
  BSE_THREADS_ENTER ();

  while (changed_notify_list)
    {
      Notify *notify = changed_notify_list;

      changed_notify_list = notify->next;
      if (!BSE_OBJECT_DISPOSING (notify->pocket))
	g_signal_emit (notify->pocket, signal_entry_changed, 0, notify->entry_id);
      g_free (notify);
    }

  BSE_THREADS_LEAVE ();

  return FALSE;
}

static void
changed_notify_add (BseDataPocket *pocket,
		    uint           entry_id)
{
  Notify *notify;

  if (!changed_notify_list)
    bse_idle_notify (changed_notify_handler, NULL);
  for (notify = changed_notify_list; notify; notify = notify->next)
    if (notify->pocket == pocket && notify->entry_id == entry_id)
      return;
  notify = g_new (Notify, 1);
  notify->pocket = pocket;
  notify->entry_id = entry_id;
  notify->next = changed_notify_list;
  changed_notify_list = notify;
}

static void
pocket_uncross (BseItem *pitem,
		BseItem *item)
{
  BseDataPocket *pocket = BSE_DATA_POCKET (pitem);
  uint i;

  for (i = 0; i < pocket->n_entries; i++)
    {
      BseDataPocketEntry *entry = pocket->entries + i;
      uint n, have_this_id = 0;

      for (n = 0; n < entry->n_items; n++)
	if (entry->items[n].type == BSE_DATA_POCKET_OBJECT &&
	    entry->items[n].value.v_object == item)
	  {
	    if (!have_this_id++)
	      changed_notify_add (pocket, entry->id);
	    entry->items[n].value.v_object = NULL;
	  }
    }

  g_object_ref (pocket);
  pocket->cr_items = g_slist_remove (pocket->cr_items, item);
  g_object_unref (pocket);
}

static void
add_cross_ref (BseDataPocket *pocket,
	       BseItem       *item)
{
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (bse_item_common_ancestor (BSE_ITEM (pocket), item) != NULL); // FIXME: delete

  if (!g_slist_find (pocket->cr_items, item))
    {
      bse_item_cross_link (BSE_ITEM (pocket), item, pocket_uncross);
      pocket->cr_items = g_slist_prepend (pocket->cr_items, item);
    }
}

static void
remove_cross_ref (BseDataPocket *pocket,
		  BseItem       *item)
{
  uint i;

  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (bse_item_common_ancestor (BSE_ITEM (pocket), item) != NULL); // FIXME: delete
  g_return_if_fail (g_slist_find (pocket->cr_items, item) != NULL);

  for (i = 0; i < pocket->n_entries; i++)
    {
      BseDataPocketEntry *entry = pocket->entries + i;
      uint n;

      for (n = 0; n < entry->n_items; n++)
	if (entry->items[n].type == BSE_DATA_POCKET_OBJECT &&
	    entry->items[n].value.v_object == item)
	  return;
    }

  pocket->cr_items = g_slist_remove (pocket->cr_items, item);
  bse_item_cross_unlink (BSE_ITEM (pocket), item, pocket_uncross);
}

uint
_bse_data_pocket_create_entry (BseDataPocket *pocket)
{
  uint id, i;

  g_return_val_if_fail (BSE_IS_DATA_POCKET (pocket), 0);

  id = pocket->free_id++;
  g_assert (id != 0);

  i = pocket->n_entries++;
  pocket->entries = g_renew (BseDataPocketEntry, pocket->entries, pocket->n_entries);
  pocket->entries[i].id = id;
  pocket->entries[i].n_items = 0;
  pocket->entries[i].items = NULL;

  g_signal_emit (pocket, signal_entry_added, 0, id);

  return id;
}

gboolean
_bse_data_pocket_delete_entry (BseDataPocket *pocket,
			       uint           entry_id)
{
  BseDataPocketEntry *entry;
  GSList *cr_del = NULL;
  uint i, n;

  g_return_val_if_fail (BSE_IS_DATA_POCKET (pocket), FALSE);
  g_return_val_if_fail (entry_id > 0, FALSE);

  for (i = 0; i < pocket->n_entries; i++)
    if (pocket->entries[i].id == entry_id)
      break;
  if (i >= pocket->n_entries)
    return FALSE;

  entry = pocket->entries + i;
  for (n = 0; n < entry->n_items; n++)
    {
      if (entry->items[n].type == BSE_DATA_POCKET_STRING)
	g_free (entry->items[n].value.v_string);
      else if (entry->items[n].type == BSE_DATA_POCKET_OBJECT &&
	       entry->items[n].value.v_object &&
	       !g_slist_find (cr_del, entry->items[n].value.v_object))
	cr_del = g_slist_prepend (cr_del, entry->items[n].value.v_object);
    }
  g_free (entry->items);

  pocket->need_store -= entry->n_items;
  n = entry->id;

  pocket->n_entries--;
  if (i < pocket->n_entries)
    pocket->entries[i] = pocket->entries[pocket->n_entries];

  while (cr_del)
    {
      GSList *tmp = cr_del;

      cr_del = tmp->next;
      remove_cross_ref (pocket, BSE_ITEM (tmp->data));
      g_slist_free_1 (tmp);
    }

  if (!BSE_OBJECT_DISPOSING (pocket))
    g_signal_emit (pocket, signal_entry_removed, 0, n);

  return TRUE;
}

gboolean
_bse_data_pocket_entry_set (BseDataPocket     *pocket,
			    uint               id,
			    GQuark             data_quark,
			    char               type,
			    BseDataPocketValue value)
{
  BseDataPocketEntry *entry;
  uint i, n;
  bool delete_item;

  g_return_val_if_fail (BSE_IS_DATA_POCKET (pocket), FALSE);
  g_return_val_if_fail (id > 0, FALSE);
  g_return_val_if_fail (data_quark > 0, FALSE);
  if (type == BSE_DATA_POCKET_OBJECT && value.v_object)
    g_return_val_if_fail (BSE_IS_ITEM (value.v_object), FALSE);

  delete_item = ((type == BSE_DATA_POCKET_INT && value.v_int == 0) ||
	         (type == BSE_DATA_POCKET_INT64 && value.v_int64 == 0) ||
	         (type == BSE_DATA_POCKET_FLOAT && value.v_float == 0.0) ||
	         (type == BSE_DATA_POCKET_STRING && value.v_string == NULL) ||
	         (type == BSE_DATA_POCKET_OBJECT && value.v_object == NULL));

  for (i = 0; i < pocket->n_entries; i++)
    if (pocket->entries[i].id == id)
      break;
  if (i >= pocket->n_entries)
    return FALSE;

  entry = pocket->entries + i;
  for (n = 0; n < entry->n_items; n++)
    if (entry->items[n].quark == data_quark)
      break;

  /* check premature exit paths and grow as required */
  if (n >= entry->n_items)
    {
      if (delete_item)
	return TRUE;

      n = entry->n_items++;
      entry->items = (BseDataPocketEntry::Item*) g_realloc (entry->items, sizeof (entry->items[0]) * entry->n_items);
      entry->items[n].type = 0;
      entry->items[n].quark = data_quark;
      pocket->need_store++;
    }
  else if (memcmp (&value, &entry->items[n].value, sizeof (value)) == 0)
    return TRUE;

  /* cleanup */
  if (entry->items[n].type == BSE_DATA_POCKET_STRING)
    g_free (entry->items[n].value.v_string);
  else if (entry->items[n].type == BSE_DATA_POCKET_OBJECT)
    {
      entry->items[n].type = 0;
      remove_cross_ref (pocket, value.v_object);
    }

  /* assignment */
  if (delete_item)
    {
      entry->n_items--;
      if (n < entry->n_items)
	entry->items[n] = entry->items[entry->n_items];
      pocket->need_store--;
    }
  else
    {
      entry->items[n].type = type;
      entry->items[n].value = value;
      if (type == BSE_DATA_POCKET_STRING)
	entry->items[n].value.v_string = g_strdup (value.v_string);
      else if (type == BSE_DATA_POCKET_OBJECT)
	add_cross_ref (pocket, value.v_object);
    }

  changed_notify_add (pocket, entry->id);

  return TRUE;
}

char
_bse_data_pocket_entry_get (BseDataPocket      *pocket,
			    uint                id,
			    GQuark              data_quark,
			    BseDataPocketValue *value)
{
  BseDataPocketEntry *entry;
  uint i, n;

  g_return_val_if_fail (BSE_IS_DATA_POCKET (pocket), 0);

  if (!data_quark)
    return 0;

  for (i = 0; i < pocket->n_entries; i++)
    if (pocket->entries[i].id == id)
      break;
  if (i >= pocket->n_entries)
    return 0;

  entry = pocket->entries + i;

  for (n = 0; n < entry->n_items; n++)
    if (entry->items[n].quark == data_quark)
      break;
  if (n >= entry->n_items)
    return 0;

  *value = entry->items[n].value;

  return entry->items[n].type;
}

static gboolean
bse_data_pocket_needs_storage (BseItem    *item,
                               BseStorage *storage)
{
  BseDataPocket *self = BSE_DATA_POCKET (item);
  return self->need_store > 0;
}

static void
bse_data_pocket_do_store_private (BseObject  *object,
				  BseStorage *storage)
{
  BseDataPocket *pocket = BSE_DATA_POCKET (object);
  uint i, j;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  for (i = 0; i < pocket->n_entries; i++)
    {
      BseDataPocketEntry *entry = pocket->entries + i;

      if (!entry->n_items)
	continue;

      bse_storage_break (storage);
      bse_storage_printf (storage, "(create-entry");
      bse_storage_push_level (storage);

      for (j = 0; j < entry->n_items; j++)
	{
	  bse_storage_break (storage);
	  bse_storage_printf (storage,
			      "(set-data \"%s\" %c ",
			      g_quark_to_string (entry->items[j].quark),
			      entry->items[j].type);
	  switch (entry->items[j].type)
	    {
	      char *string;
	      uint v_uint;
	    case BSE_DATA_POCKET_INT:	bse_storage_printf (storage, "%u", entry->items[j].value.v_int);	break;
	    case BSE_DATA_POCKET_FLOAT:	bse_storage_putf (storage, entry->items[j].value.v_float);		break;
	    case BSE_DATA_POCKET_INT64:
	      v_uint = entry->items[j].value.v_int64 >> 32;
	      bse_storage_printf (storage, "%u ", v_uint);
	      v_uint = entry->items[j].value.v_int64 & 0xffffffff;
	      bse_storage_printf (storage, "%u", v_uint);
	      break;
	    case BSE_DATA_POCKET_STRING:
	      string = g_strescape (entry->items[j].value.v_string, NULL);
	      bse_storage_printf (storage, "\"%s\"", string);
	      g_free (string);
	      break;
	    case BSE_DATA_POCKET_OBJECT:
	      bse_storage_put_item_link (storage, BSE_ITEM (pocket), entry->items[j].value.v_object);
	      break;
	    default:
	      g_assert_not_reached ();
	    }
	  bse_storage_putc (storage, ')');
	}
      bse_storage_pop_level (storage);
      bse_storage_putc (storage, ')');
    }
}

typedef struct {
  uint id;
  GQuark quark;
} ObjectEntry;

static void
object_entry_resolved (void           *data,
		       BseStorage     *storage,
		       BseItem        *from_item,
		       BseItem        *to_item,
		       const char     *error)
{
  ObjectEntry *oentry = (ObjectEntry*) data;
  BseDataPocket *pocket = BSE_DATA_POCKET (from_item);

  if (error)
    bse_storage_warn (storage, "%s", error);
  else if (oentry->id)
    {
      BseDataPocketValue value;

      value.v_object = to_item;
      _bse_data_pocket_entry_set (pocket, oentry->id, oentry->quark, BSE_DATA_POCKET_OBJECT, value);
    }
  g_free (oentry);
}

static GTokenType
parse_set_data (BseDataPocket *pocket,
		uint           id,
		BseStorage    *storage,
                GScanner      *scanner)
{
  BseDataPocketValue value;
  ObjectEntry *oentry = NULL;
  GQuark quark;
  uint ttype;
  gboolean char_2_token;

  parse_or_return (scanner, G_TOKEN_STRING);
  quark = g_quark_from_string (scanner->value.v_string);

  char_2_token = scanner->config->char_2_token;
  scanner->config->char_2_token = FALSE;
  g_scanner_get_next_token (scanner);
  scanner->config->char_2_token = char_2_token;
  if (scanner->token != G_TOKEN_CHAR)
    return G_TOKEN_CHAR;
  ttype = scanner->value.v_char;

  switch (ttype)
    {
      GTokenType token;
      gboolean negate;
    case BSE_DATA_POCKET_INT:
      parse_or_return (scanner, G_TOKEN_INT);
      value.v_int = scanner->value.v_int64;
      break;
    case BSE_DATA_POCKET_FLOAT:
      negate = g_scanner_peek_next_token (scanner) == '-';
      if (negate)
	g_scanner_get_next_token (scanner);	/* eat '-' */
      parse_or_return (scanner, G_TOKEN_FLOAT);
      value.v_float = negate ? scanner->value.v_float : -scanner->value.v_float;
      break;
    case BSE_DATA_POCKET_INT64:
      parse_or_return (scanner, G_TOKEN_INT);
      peek_or_return (scanner, G_TOKEN_INT);
      value.v_int64 = scanner->value.v_int64;
      value.v_int64 <<= 32;
      g_scanner_get_next_token (scanner); /* read next int */
      value.v_int64 |= scanner->value.v_int64 & 0xffffffff;
      break;
    case BSE_DATA_POCKET_STRING:
      parse_or_return (scanner, G_TOKEN_STRING);
      value.v_string = scanner->value.v_string;
      break;
    case BSE_DATA_POCKET_OBJECT:
      oentry = g_new0 (ObjectEntry, 1);
      oentry->id = id;
      oentry->quark = quark;
      token = bse_storage_parse_item_link (storage, BSE_ITEM (pocket), object_entry_resolved, oentry);
      if (token != G_TOKEN_NONE)
	return token;
      if (g_scanner_peek_next_token (scanner) != ')')
	{
	  oentry->id = 0;
	  return GTokenType (')');
	}
      break;
    default:
      /* unmatched data type */
      return bse_storage_warn_skip (storage,
                                    "invalid data type specification `%c' for \"%s\"",
                                    ttype,
                                    g_quark_to_string (quark));
    }
  peek_or_return (scanner, ')');

  /* caution, value might still point to scanner->value.v_string */
  if (!oentry)
    _bse_data_pocket_entry_set (pocket, id, quark, ttype, value);

  g_scanner_get_next_token (scanner); /* eat ')' */

  return G_TOKEN_NONE;
}

static GTokenType
bse_data_pocket_restore_private (BseObject  *object,
				 BseStorage *storage,
                                 GScanner   *scanner)
{
  BseDataPocket *pocket = BSE_DATA_POCKET (object);
  GTokenType expected_token;

  /* support storage commands */
  if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER &&
      bse_string_equals ("create-entry", scanner->next_value.v_identifier))
    {
      uint id = _bse_data_pocket_create_entry (pocket);

      parse_or_return (scanner, G_TOKEN_IDENTIFIER);	/* eat identifier */

      while (g_scanner_peek_next_token (scanner) != ')')
	{
	  g_scanner_get_next_token (scanner); /* read token */
	  if (scanner->token == '(')
	    {
	      parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	      if (g_quark_try_string (scanner->value.v_identifier) == quark_set_data)
		{
		  expected_token = (GTokenType) parse_set_data (pocket, id, storage, scanner);
		  if (expected_token != G_TOKEN_NONE)
		    return expected_token;
		}
	      else
		bse_storage_warn_skip (storage, "unknown directive `%s'", scanner->next_value.v_identifier);
	    }
	  else
	    return GTokenType (')');
	}
      parse_or_return (scanner, ')');
      expected_token = G_TOKEN_NONE;
    }
  else /* chain parent class' handler */
    expected_token = (GTokenType) BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);

  return expected_token;
}
