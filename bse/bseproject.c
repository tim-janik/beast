/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include	"bseproject.h"

#include	"bsesuper.h"
#include	"bsestorage.h"
#include	"bsemagic.h"
#include	"bsesample.h"
#include	"bsesong.h"
#include	"bseheart.h"
#include	<string.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<errno.h>


/* --- prototypes --- */
static void	bse_project_class_init	(BseProjectClass	*class);
static void	bse_project_init	(BseProject		*project,
					 gpointer		 rclass);
static void	bse_project_do_shutdown	(BseObject		*object);
static void	bse_project_add_item	(BseContainer		*container,
					 BseItem		*item);
static void	bse_project_remove_item	(BseContainer		*container,
					 BseItem		*item);
static void	bse_project_forall_items(BseContainer		*container,
					 BseForallItemsFunc	 func,
					 gpointer		 data);
static guint	bse_project_item_seqid	(BseContainer		*container,
					 BseItem		*item);
static BseItem*	bse_project_get_item	(BseContainer		*container,
					 BseType		 item_type,
					 guint		         seqid);


/* --- variables --- */
static BseTypeClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseProject)
{
  static const BseTypeInfo project_info = {
    sizeof (BseProjectClass),
    
    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_project_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseProject),
    BSE_PREALLOC_N_PROJECTS /* n_preallocs */,
    (BseObjectInitFunc) bse_project_init,
  };
  
  return bse_type_register_static (BSE_TYPE_CONTAINER,
				   "BseProject",
				   "BSE Super container type",
				   &project_info);
}

static void
bse_project_class_init (BseProjectClass *class)
{
  BseObjectClass *object_class;
  BseContainerClass *container_class;
  
  parent_class = bse_type_class_peek (BSE_TYPE_CONTAINER);
  object_class = BSE_OBJECT_CLASS (class);
  container_class = BSE_CONTAINER_CLASS (class);
  
  object_class->shutdown = bse_project_do_shutdown;

  container_class->add_item = bse_project_add_item;
  container_class->remove_item = bse_project_remove_item;
  container_class->forall_items = bse_project_forall_items;
  container_class->item_seqid = bse_project_item_seqid;
  container_class->get_item = bse_project_get_item;
}

static void
bse_project_init (BseProject *project,
		  gpointer    rclass)
{
  BseObject *object;
  
  object = BSE_OBJECT (project);

  project->supers = NULL;
}

static void
bse_project_do_shutdown (BseObject *object)
{
  BseProject *project;
  
  project = BSE_PROJECT (object);

  while (project->supers)
    bse_container_remove_item (BSE_CONTAINER (project), project->supers->data);

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

BseProject*
bse_project_new (const gchar *name)
{
  BseProject *project;

  g_return_val_if_fail (name != NULL, NULL);

  project = bse_object_new (BSE_TYPE_PROJECT,
			    "name", name,
			    NULL);

  return project;
}

void
bse_project_add_super (BseProject *project,
		       BseSuper   *super)
{
  g_return_if_fail (BSE_IS_PROJECT (project));
  g_return_if_fail (BSE_IS_SUPER (super));
  g_return_if_fail (BSE_ITEM (super)->parent == NULL);

  bse_container_add_item (BSE_CONTAINER (project), BSE_ITEM (super));
}

static void
bse_project_add_item (BseContainer *container,
		      BseItem      *item)
{
  BseProject *project = BSE_PROJECT (container);

  if (BSE_IS_SUPER (item))
    project->supers = g_slist_append (project->supers, item);
  else
    g_warning ("BseProject: cannot add unknown item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

void
bse_project_remove_super (BseProject *project,
			  BseSuper   *super)
{
  g_return_if_fail (BSE_IS_PROJECT (project));
  g_return_if_fail (BSE_IS_SUPER (super));
  g_return_if_fail (BSE_ITEM (super)->parent == BSE_ITEM (project));

  bse_container_remove_item (BSE_CONTAINER (project), BSE_ITEM (super));
}

static void
bse_project_remove_item (BseContainer *container,
			 BseItem      *item)
{
  BseProject *project = BSE_PROJECT (container);

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);

  if (BSE_IS_SUPER (item))
    project->supers = g_slist_remove (project->supers, item);
  else
    g_warning ("BseProject: cannot remove unknown item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));
}

static void
bse_project_forall_items (BseContainer      *container,
			  BseForallItemsFunc func,
			  gpointer           data)
{
  BseProject *project = BSE_PROJECT (container);
  GSList *slist;

  slist = project->supers;
  while (slist)
    {
      BseItem *item;

      item = slist->data;
      slist = slist->next;
      if (!func (item, data))
	return;
    }
}

static guint
bse_project_item_seqid (BseContainer *container,
			BseItem      *item)
{
  BseProject *project = BSE_PROJECT (container);

  return 1 + g_slist_index (project->supers, item);
}

static BseItem*
bse_project_get_item (BseContainer *container,
		      BseType       item_type,
		      guint         seqid)
{
  BseProject *project = BSE_PROJECT (container);
  GSList *slist;

  slist = (bse_type_is_a (item_type, BSE_TYPE_SUPER) ?
	   g_slist_nth (project->supers, seqid - 1) :
	   NULL);

  return slist ? slist->data : NULL;
}

GList*
bse_project_list_supers (BseProject *project,
			 BseType     super_type)
{
  GList *list = NULL;
  GSList *slist;

  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  g_return_val_if_fail (bse_type_is_a (super_type, BSE_TYPE_SUPER), NULL);

  for (slist = project->supers; slist; slist = slist->next)
    if (bse_type_is_a (BSE_OBJECT_TYPE (slist->data), super_type))
      list = g_list_prepend (list, slist->data);

  return list;
}

static gboolean
make_nick_paths (BseItem *item,
		 gpointer data_p)
{
  gpointer *data = data_p;
  BseType item_type = GPOINTER_TO_UINT (data[1]);
  gchar *prefix = data[2];

  if (bse_type_is_a (BSE_OBJECT_TYPE (item), item_type))
    data[0] = g_list_prepend (data[0], g_strconcat (prefix, BSE_OBJECT_NAME (item), NULL));
  if (BSE_IS_CONTAINER (item))
    {
      data[2] = g_strconcat (prefix, BSE_OBJECT_NAME (item), ".", NULL);
      bse_container_forall_items (BSE_CONTAINER (item), make_nick_paths, data);
      g_free (data[2]);
      data[2] = prefix;
    }

  return TRUE;
}

GList* /* free result (strings and list) */
bse_project_list_nick_paths (BseProject *project,
			     BseType     item_type)
{
  gpointer data[3] = { NULL, GUINT_TO_POINTER (item_type), "" };

  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  g_return_val_if_fail (bse_type_is_a (item_type, BSE_TYPE_ITEM), NULL);

  bse_container_forall_items (BSE_CONTAINER (project), make_nick_paths, data);

  return g_list_reverse (data[0]);
}

static BseItem*
container_find_nick_item (BseContainer *container,
			  gchar        *nick)
{
  BseItem *item;
  gchar *next = strchr (nick, '.');

  if (next)
    {
      *(next++) = 0;
      next = *next ? next : NULL;
    }

  item = bse_container_lookup_item (container, nick);

  if (next)
    item = BSE_IS_CONTAINER (item) ? container_find_nick_item (BSE_CONTAINER (item), next) : NULL;

  return item;
}

BseItem*
bse_project_item_from_nick_path (BseProject  *project,
				 const gchar *nick_path)
{
  BseItem *item;
  gchar *nick;

  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  g_return_val_if_fail (nick_path != NULL, NULL);

  nick = g_strdup (nick_path);
  item = container_find_nick_item (BSE_CONTAINER (project), nick);
  g_free (nick);

  return item;
}

static inline GTokenType
parse_statement (BseContainer *container,
		 BseStorage   *storage)
{
  GScanner *scanner = storage->scanner;
  BseItem *item;

  if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return G_TOKEN_IDENTIFIER;

  item = bse_container_item_from_path (container, scanner->value.v_identifier);
  if (!item)
    return bse_storage_warn_skip (storage,
				  "unable to resolve `%s', no such item",
				  scanner->value.v_identifier);

  return bse_object_restore (BSE_OBJECT (item), storage);
}

BseErrorType
bse_project_restore (BseProject *project,
		     BseStorage *storage)
{
  GScanner *scanner;
  GTokenType expected_token = G_TOKEN_NONE;
  
  g_return_val_if_fail (BSE_IS_PROJECT (project), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_IS_STORAGE (storage), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STORAGE_READABLE (storage), BSE_ERROR_INTERNAL);

  /* FIXME: this should integrate with the object system better */
  
  scanner = storage->scanner;

  bse_object_ref (BSE_OBJECT (project));
  
  while (!bse_storage_input_eof (storage) &&
	 expected_token == G_TOKEN_NONE)
    {
      g_scanner_get_next_token (scanner);
      
      if (scanner->token == G_TOKEN_EOF)
	break;
      else if (scanner->token == '(')
	expected_token = parse_statement (BSE_CONTAINER (project), storage);
      else
	expected_token = G_TOKEN_EOF; /* '('; */
    }
  
  if (expected_token != G_TOKEN_NONE)
    bse_storage_unexp_token (storage, expected_token);

  expected_token = expected_token != G_TOKEN_NONE;
  BSE_NOTIFY (project, complete_restore, NOTIFY (OBJECT, storage, expected_token, DATA));
  
  bse_object_unref (BSE_OBJECT (project));

  return (scanner->parse_errors >= scanner->max_parse_errors ?
	  BSE_ERROR_PARSE_ERROR :
	  BSE_ERROR_NONE);
}

BseErrorType
bse_project_store_bse (BseProject  *project,
		       const gchar *bse_file)
{
  BseStorage *storage;
  gint fd;
  gchar *string;
  BseMagicFlags mflags;
  GSList *slist;
  guint l;
  
  g_return_val_if_fail (BSE_IS_PROJECT (project), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (bse_file != NULL, BSE_ERROR_INTERNAL);

  /* FIXME: this should integrate with the object system better */

  fd = open (bse_file,
	     O_WRONLY | O_CREAT | O_EXCL,
	     0666);

  if (fd < 0)
    return (errno == EEXIST ? BSE_ERROR_FILE_EXISTS : BSE_ERROR_FILE_IO);

  storage = bse_storage_new ();
  bse_storage_prepare_write (storage, FALSE);
  bse_container_store_items (BSE_CONTAINER (project), storage);

  mflags = storage->wblocks ? BSE_MAGIC_BSE_BIN_EXTENSION : 0;
  for (slist = project->supers; slist; slist = slist->next)
    if (BSE_IS_SAMPLE (slist->data))
      mflags |= BSE_MAGIC_BSE_SAMPLE;
    else if (BSE_IS_SONG (slist->data))
      mflags |= BSE_MAGIC_BSE_SONG;

  string = g_strdup_printf (";BseProjectV0\n\n"); /* %010o mflags */
  do
    l = write (fd, string, strlen (string));
  while (l < 0 && errno == EINTR);
  g_free (string);

  bse_storage_flush_fd (storage, fd);
  bse_storage_destroy (storage);

  return close (fd) < 0 ? BSE_ERROR_FILE_IO : BSE_ERROR_NONE;
}

BseObject*
bse_project_path_resolver (gpointer     func_data,
			   BseStorage  *storage,
			   BseType      required_type,
			   const gchar *path)
{
  BseProject *project = func_data;
  gpointer item = NULL;

  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  g_return_val_if_fail (BSE_IS_STORAGE (storage), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  /* FIXME: need error handling, storage warnings.... */

  if (bse_type_is_a (required_type, BSE_TYPE_ITEM))
    item = bse_container_item_from_path (BSE_CONTAINER (project), path);

  return item;
}

void
bse_project_start_playback (BseProject *project)
{
  g_return_if_fail (BSE_IS_PROJECT (project));
  
  if (!BSE_SOURCE_PREPARED (project))
    {
      BSE_OBJECT_SET_FLAGS (project, BSE_SOURCE_FLAG_PREPARED);
      BSE_SOURCE_GET_CLASS (project)->prepare (BSE_SOURCE (project), bse_heart_get_beat_index ());
    }
}

void
bse_project_stop_playback (BseProject *project)
{
  g_return_if_fail (BSE_IS_PROJECT (project));
  
  if (BSE_SOURCE_PREPARED (project))
    bse_source_reset (BSE_SOURCE (project));
}
