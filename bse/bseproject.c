/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2002 Tim Janik
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
#include	"bsesong.h"
#include	"bsesnet.h"
#include	"bsemarshal.h"
#include	"bsewaverepo.h"
#include	"bswprivate.h"
#include	"gslengine.h"
#include	<string.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<errno.h>


enum {
  SIGNAL_COMPLETE_RESTORE,
  SIGNAL_LAST
};

/* --- macros --- */
#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return


/* --- prototypes --- */
static void	bse_project_class_init		(BseProjectClass	*class);
static void	bse_project_class_finalize	(BseProjectClass	*class);
static void	bse_project_init		(BseProject		*project,
						 gpointer		 rclass);
static void	bse_project_do_destroy		(BseObject		*object);
static void	bse_project_add_item		(BseContainer		*container,
						 BseItem		*item);
static void	bse_project_remove_item		(BseContainer		*container,
						 BseItem		*item);
static void	bse_project_forall_items	(BseContainer		*container,
						 BseForallItemsFunc	 func,
						 gpointer		 data);
static BseItem* bse_project_retrieve_child	(BseContainer           *container,
						 GType                   child_type,
						 const gchar            *uname);
static void	bse_project_prepare		(BseSource		*source);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static guint       project_signals[SIGNAL_LAST] = { 0, };


/* --- functions --- */
BSE_BUILTIN_TYPE (BseProject)
{
  static const GTypeInfo project_info = {
    sizeof (BseProjectClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_project_class_init,
    (GClassFinalizeFunc) bse_project_class_finalize,
    NULL /* class_data */,
    
    sizeof (BseProject),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_project_init,
  };
  
  return bse_type_register_static (BSE_TYPE_CONTAINER,
				   "BseProject",
				   "BSE Super container type",
				   &project_info);
}

static void
bse_project_class_init (BseProjectClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->destroy = bse_project_do_destroy;

  source_class->prepare = bse_project_prepare;

  container_class->add_item = bse_project_add_item;
  container_class->remove_item = bse_project_remove_item;
  container_class->forall_items = bse_project_forall_items;
  container_class->retrieve_child = bse_project_retrieve_child;

  project_signals[SIGNAL_COMPLETE_RESTORE] = bse_object_class_add_signal (object_class, "complete-restore",
									  bse_marshal_VOID__POINTER_BOOLEAN, NULL, // FIXME: OBJECT
									  G_TYPE_NONE,
									  2, G_TYPE_POINTER, // FIXME TYPE_OBJECT
									  G_TYPE_BOOLEAN);
}

static void
bse_project_class_finalize (BseProjectClass *class)
{
}

static void
bse_project_init (BseProject *project,
		  gpointer    rclass)
{
  BseObject *object;
  BseWaveRepo *wrepo;

  object = BSE_OBJECT (project);

  project->supers = NULL;
  project->items = NULL;

  /* we always have a wave-repo */
  wrepo = bse_object_new (BSE_TYPE_WAVE_REPO,
			  "uname", "Wave-Repository",
			  NULL);
  bse_container_add_item (BSE_CONTAINER (project), BSE_ITEM (wrepo));
  g_object_unref (wrepo);
  /* with fixed uname */
  BSE_OBJECT_SET_FLAGS (wrepo, BSE_OBJECT_FLAG_FIXED_UNAME);
}

static void
bse_project_do_destroy (BseObject *object)
{
  BseProject *project;
  
  project = BSE_PROJECT (object);

  while (project->items)
    bse_container_remove_item (BSE_CONTAINER (project), project->items->data);
  while (project->supers)
    bse_container_remove_item (BSE_CONTAINER (project), project->supers->data);

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_project_add_item (BseContainer *container,
		      BseItem      *item)
{
  BseProject *project = BSE_PROJECT (container);

  if (BSE_IS_SUPER (item))
    project->supers = g_slist_append (project->supers, item);
  else
    project->items = g_slist_append (project->items, item);

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
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
    project->items = g_slist_remove (project->items, item);
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

  slist = project->items;
  while (slist)
    {
      BseItem *item;

      item = slist->data;
      slist = slist->next;
      if (!func (item, data))
	return;
    }
}

static BseItem*
bse_project_retrieve_child (BseContainer *container,
			    GType         child_type,
			    const gchar  *uname)
{
  BseProject *project = BSE_PROJECT (container);

  /* always hand out the same wave repo */
  if (g_type_is_a (child_type, BSE_TYPE_WAVE_REPO))
    {
      GSList *slist;

      for (slist = project->supers; slist; slist = slist->next)
	if (g_type_is_a (G_OBJECT_TYPE (slist->data), BSE_TYPE_WAVE_REPO))
	  return slist->data;
      g_printerr("eeeeeek wave-repo not found\n");
      return NULL;	/* shouldn't happen */
    }
  else
    return BSE_CONTAINER_CLASS (parent_class)->retrieve_child (container, child_type, uname);
}

static gboolean
add_item_upaths (BseItem *item,
		 gpointer data_p)
{
  gpointer *data = data_p;
  BswIterString *iter = data[0];
  GType item_type = (GType) data[1];
  BseContainer *container = data[2];

  if (g_type_is_a (BSE_OBJECT_TYPE (item), item_type))
    bsw_iter_add_string_take_ownership (iter, bse_container_make_upath (container, item));
  if (BSE_IS_CONTAINER (item))
    bse_container_forall_items (BSE_CONTAINER (item), add_item_upaths, data);

  return TRUE;
}

BswIterString*
bse_project_list_upaths (BseProject *project,
			 GType       item_type)
{
  gpointer data[3];
  BswIterString *iter;

  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  g_return_val_if_fail (g_type_is_a (item_type, BSE_TYPE_ITEM), NULL);

  iter = bsw_iter_create (BSW_TYPE_ITER_STRING, 16);
  data[0] = iter;
  data[1] = (gpointer) item_type;
  data[2] = project;
  bse_container_forall_items (BSE_CONTAINER (project), add_item_upaths, data);

  return iter;
}

BseErrorType
bse_project_store_bse (BseProject  *project,
		       const gchar *bse_file,
		       gboolean     self_contained)
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
  if (self_contained)
    BSE_STORAGE_SET_FLAGS (storage, BSE_STORAGE_FLAG_SELF_CONTAINED);
  bse_storage_prepare_write (storage, FALSE);
  bse_container_store_items (BSE_CONTAINER (project), storage, "bse-project-restore");

  mflags = storage->wblocks ? BSE_MAGIC_BSE_BIN_EXTENSION : 0;
  for (slist = project->supers; slist; slist = slist->next)
    if (BSE_IS_SONG (slist->data))
      mflags |= BSE_MAGIC_BSE_SONG;

  string = g_strdup_printf (";BseProjectV1\n\n"); /* %010o mflags */
  do
    l = write (fd, string, strlen (string));
  while (l < 0 && errno == EINTR);
  g_free (string);

  bse_storage_flush_fd (storage, fd);
  bse_storage_destroy (storage);

  return close (fd) < 0 ? BSE_ERROR_FILE_IO : BSE_ERROR_NONE;
}

static inline GTokenType
parse_statement (BseProject *project,
		 BseStorage *storage)
{
  GScanner *scanner = storage->scanner;
  BseItem *item;

  parse_or_return (scanner, G_TOKEN_IDENTIFIER);

  if (strcmp ("bse-project-restore", scanner->value.v_identifier) == 0)
    {
      parse_or_return (scanner, G_TOKEN_STRING);	/* type_uname argument */

      item = bse_container_retrieve_child (BSE_CONTAINER (project), scanner->value.v_string);
      if (!item)
	return bse_storage_warn_skip (storage, "unable to create object from (invalid) reference: %s",
				      scanner->value.v_string);
    }
  else		/* deprecated compat, the uname path is an identifier */
    {
      item = bse_container_retrieve_child (BSE_CONTAINER (project), scanner->value.v_identifier);

      if (!item)	/* probably not a compat case */
	return G_TOKEN_STRING;

      bse_storage_warn (storage, "deprecated syntax: non-string uname path: %s", scanner->value.v_identifier);
    }

  /* now let the child restore itself
   */
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
	expected_token = parse_statement (project, storage);
      else
	expected_token = G_TOKEN_EOF; /* '('; */
    }
  
  if (expected_token != G_TOKEN_NONE)
    bse_storage_unexp_token (storage, expected_token);

  bse_storage_resolve_item_links (storage);

  expected_token = expected_token != G_TOKEN_NONE;
  g_signal_emit (project, project_signals[SIGNAL_COMPLETE_RESTORE], 0, storage, expected_token);
  
  bse_object_unref (BSE_OBJECT (project));

  return (scanner->parse_errors >= scanner->max_parse_errors ?
	  BSE_ERROR_PARSE_ERROR :
	  BSE_ERROR_NONE);
}

BseObject*
bse_project_upath_resolver (gpointer     func_data,
			    GType        required_type,
			    const gchar *upath,
			    gchar      **error_p)
{
  BseProject *project = func_data;
  gpointer item = NULL;

  if (error_p)
    *error_p = NULL;
  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  g_return_val_if_fail (upath != NULL, NULL);

  /* FIXME: need error handling, warnings.... */

  if (g_type_is_a (required_type, BSE_TYPE_ITEM))
    item = bse_container_resolve_upath (BSE_CONTAINER (project), upath);
  else if (error_p)
    *error_p = g_strdup_printf ("unable to resolve object of type `%s' from upath: %s", g_type_name (required_type), upath);
  
  return item;
}

BseItem*
bse_project_lookup_typed_item (BseProject  *project,
			       GType	    item_type,
			       const gchar *uname)
{
  BseItem *item;

  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  g_return_val_if_fail (uname != NULL, NULL);

  item = bse_container_lookup_item (BSE_CONTAINER (project), uname);
  if (item && G_OBJECT_TYPE (item) == item_type)
    return item;

  return NULL;
}

static void
bse_project_prepare (BseSource *source)
{
  BseProject *project = BSE_PROJECT (source);
  GSList *slist;
  
  /* make sure Wave repositories are prepared first */
  for (slist = project->supers; slist; slist = slist->next)
    if (BSE_IS_WAVE_REPO (slist->data))
      bse_source_prepare (slist->data);

  /* chain parent class' handler to prepare the rest */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

void
bse_project_start_playback (BseProject *project)
{
  g_return_if_fail (BSE_IS_PROJECT (project));
  
  if (!BSE_SOURCE_PREPARED (project))
    {
      GslTrans *trans = gsl_trans_open ();
      GSList *slist;
      
      bse_source_prepare (BSE_SOURCE (project));
      
      for (slist = project->supers; slist; slist = slist->next)
	{
	  BseSuper *super = BSE_SUPER (slist->data);
	  
	  if (super->auto_activate)
	    {
	      BseSNet *snet = BSE_SNET (super);

	      super->auto_activate_context_handle = 0;
	      super->auto_activate_context_handle = bse_snet_create_context (snet, trans);
	      bse_source_connect_context (BSE_SOURCE (snet), super->auto_activate_context_handle, trans);
	    }
	  else
	    super->auto_activate_context_handle = ~0;
	}
      gsl_trans_commit (trans);
    }
}

void
bse_project_stop_playback (BseProject *project)
{
  g_return_if_fail (BSE_IS_PROJECT (project));
  
  if (BSE_SOURCE_PREPARED (project))
    {
      GslTrans *trans = gsl_trans_open ();
      GSList *slist;
      
      for (slist = project->supers; slist; slist = slist->next)
	{
          BseSuper *super = BSE_SUPER (slist->data);
	  
	  if (super->auto_activate_context_handle != ~0)
	    {
	      BseSource *source = BSE_SOURCE (super);
	      
	      bse_source_dismiss_context (source, super->auto_activate_context_handle, trans);
	      super->auto_activate_context_handle = ~0;
	    }
	}
      gsl_trans_commit (trans);

      /* wait until after all modules have actually been dismissed */
      gsl_engine_wait_on_trans ();

      bse_source_reset (BSE_SOURCE (project));
    }
}
