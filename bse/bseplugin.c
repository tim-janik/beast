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
#include	"bseplugin.h"

#include	"bseprocedure.h"
#include	<gmodule.h>
#include	<string.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<unistd.h>


#define BSE_DEBUG(y,x)	G_STMT_START { x ; } G_STMT_END


/* --- prototypes --- */
static const gchar* bse_plugin_register_types	(BsePlugin      *plugin,
						 gconstpointer   array_p,
						 BseExportType   exp_type);


/* --- variables --- */
gconstpointer     BSE_EXPORT_IMPL_S (Procedure) = NULL;
                  BSE_EXPORT_IMPL_C = NULL;
static GSList    *bse_plugins = NULL;

/* --- functions --- */
void
bse_plugin_init (void)
{
  /* this function is called upon initialization of BSE
   */
  if (!bse_plugins)
    {
      /* include extern declarations of builtin init functions */
#include "bsebuiltin_externs.c"
      static const gchar* (* const builtin_inits[]) (BsePlugin*) = {
	/* and list them in an array */
#include "bsebuiltin_array.c"
      };
      static const guint n_builtin_inits = (sizeof (builtin_inits) /
					    sizeof (builtin_inits[0]));
      guint i;
      
      
      /* initialize builtin plugins
       */
      bse_plugin_builtin_init = bse_plugin_register_types;
      for (i = 0; i < n_builtin_inits; i++)
	{
	  const gchar *name, *error;
	  BsePlugin *plugin = g_new0 (BsePlugin, 1); /* FIXME: alloc only */

	  bse_plugins = g_slist_prepend (bse_plugins, plugin);
	  plugin->name = "BSE-Builtin"; /* this is usually reset */
	  plugin->fname = NULL;
	  plugin->gmodule = NULL;
	  plugin->module_refs = 42;

	  name = plugin->name;
	  error = builtin_inits[i] (plugin);
	  if (error)
	    g_warning ("Plugin \"%s\" initialization failed: %s", plugin->name, error);
	  if (name == plugin->name)
	    g_warning ("builtin plugin initializer (%p) doesn't assign plugin name",
		       builtin_inits[i]);

	  BSE_DEBUG (PLUGINS, {
	    g_message ("register-builtin-plugin: \"%s\"", plugin->name);
	  });
	}
      bse_plugin_builtin_init = NULL;
    }
}

static const gchar*
bse_plugin_register_types (BsePlugin    *plugin,
			   gconstpointer array_p,
			   BseExportType exp_type)
{
  const BseExportProcedure *pspecs = array_p;
  guint have_defs = 0;

  switch (plugin && array_p ? exp_type : 0)
    {
      const BseExportProcedure *pspec;

    case BSE_EXPORT_TYPE_PROCS:
      for (pspec = pspecs; pspec->type_p; pspec++)
	{
	  if (pspec->name &&
	      pspec->init &&
	      pspec->exec)
	    {
	      BseType type;
	      guint o = plugin->n_proc_types;
	      const gchar *error;
	      
	      type = bse_type_from_name (pspec->name);
	      if (type)
		return "Attempt to register known symbol";

	      error = bse_procedure_type_register (pspec->name,
						   pspec->blurb,
						   plugin,
						   pspec->category,
						   &type);
	      if (error)
		return error;

	      plugin->n_proc_types++;
	      plugin->proc_types = g_renew (BseType,
					    plugin->proc_types,
					    plugin->n_proc_types);
	      plugin->proc_types[o] = type;

	      plugin->exports_procedures = TRUE;
	      have_defs++;
	      *(pspec->type_p) = type;
	      
	      BSE_DEBUG (PROCS, {
		g_message ("register-procedure: \"%s\"", pspec->name);
	      });
	    }
	}
      plugin->e_procs = (gpointer) pspecs;

      return NULL;
    }

  return bse_error_blurb (BSE_ERROR_INTERNAL);
}

static const gchar*
bse_plugin_reinit_type_ids (BsePlugin *plugin)
{
  BseExportProcedure *pspecs = plugin->e_procs;
  gchar *error = NULL;
  
  /* procedure types
   */
  if (pspecs)
    {
      BseExportProcedure *pspec;
      BseType *exp_types = plugin->proc_types;
      BseType *exp_last = plugin->proc_types + plugin->n_proc_types;
      
      for (pspec = pspecs; pspec->type_p && exp_types < exp_last; pspec++)
	if (pspec->name &&
	    pspec->init &&
	    pspec->exec &&
	    bse_type_from_name (pspec->name) == *exp_types)
	  {
	    if (*pspec->type_p)
	      g_warning ("while reinitializing \"%s\", type id for `%s' already assigned?",
			 plugin->name,
			 pspec->name);
	    *pspec->type_p = *(exp_types++);
	  }
      if (exp_types < exp_last)
	error = "Unable to rematch procedure types from previous initialization";
    }

  return error;
}

void
bse_plugin_ref (BsePlugin *plugin)
{
  g_return_if_fail (plugin != NULL);
  
  if (!plugin->module_refs)
    {
      const gchar *error = NULL;
      
      plugin->gmodule = g_module_open (plugin->fname, 0);
      if (!plugin->gmodule)
	error = g_module_error ();
      if (!error && plugin->exports_procedures)
	{
	  gpointer *sym_array;

	  if (g_module_symbol (plugin->gmodule,
			       BSE_EXPORT_SYMBOL (Procedure),
			       (gpointer) &sym_array))
	    {
	      plugin->e_procs = sym_array;
	      error = bse_plugin_reinit_type_ids (plugin);
	    }
	  else
	    error = g_module_error ();
	}
      
      if (error)
	g_error ("Fatal plugin error, failed to reinitialize plugin: %s", error);
      
      BSE_DEBUG (PLUGINS, {
	g_message ("plugin-reloaded: \"%s\"", plugin->name);
      });
    }
  plugin->module_refs++;
}

static void
bse_plugin_unload (BsePlugin *plugin)
{
  g_return_if_fail (plugin->gmodule != NULL && plugin->fname != NULL);

  g_module_close (plugin->gmodule);
  plugin->gmodule = NULL;
  
  /* reset plugin pointers */
  plugin->e_procs = NULL;

  BSE_DEBUG (PLUGINS, {
    g_message ("plugin-unloaded: \"%s\"", plugin->name);
  });
}

void
bse_plugin_unref (BsePlugin *plugin)
{
  g_return_if_fail (plugin != NULL);
  g_return_if_fail (plugin->module_refs > 0);
  
  plugin->module_refs--;
  if (!plugin->module_refs)
    /* FIXME: need timeout handler for module unloading */
    bse_plugin_unload (plugin);
}

static inline BseExportSpec*
bse_plugin_get_export_specs (BsePlugin *plugin,
			     BseType    type,
			     gpointer   export_specs)
{
  if (export_specs)
    {
      BseExportSpec* spec;

      for (spec = export_specs; spec->type_p; spec++)
	if (*(spec->type_p) == type)
	  return spec;
    }
  
  return NULL;
}

void
bse_plugin_complete_info (BsePlugin   *plugin,
			  BseType      type,
			  BseTypeInfo *type_info)
{
  BseExportSpec *export_spec = NULL;
  gpointer specs_p = NULL;
  void (*complete_info) (BseExportSpec *, BseTypeInfo *) = NULL;
  
  g_return_if_fail (plugin != NULL);
  g_return_if_fail (plugin->module_refs > 0);

  /* we are an internal function, and thusly behave pretty conservative,
   * so in effect we guarrantee to always fill the type_info
   */
  
  switch (BSE_FUNDAMENTAL_TYPE (type))
    {
    case BSE_TYPE_PROCEDURE:
      specs_p = plugin->e_procs;
      complete_info = bse_procedure_complete_info;
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  export_spec = bse_plugin_get_export_specs (plugin, type, specs_p);
  if (!export_spec || !complete_info)
    g_error ("unable to find export spec for `%s' in \"%s\"",
	     bse_type_name (type),
	     plugin->name);

  complete_info (export_spec, type_info);
}

static inline BsePlugin*
bse_plugin_find (GModule *gmodule)
{
  GSList *slist;

  for (slist = bse_plugins; slist; slist = slist->next)
    {
      BsePlugin *plugin = slist->data;

      if (plugin->gmodule == gmodule)
	return plugin;
    }

  return NULL;
}

const gchar*
bse_plugin_check_load (const gchar *_file_name)
{
  const gint TOKEN_DLNAME = G_TOKEN_LAST + 1;
  gchar *string, *file_name = (gchar*) _file_name;
  gint fd;
  GScanner *scanner;
  GModule *gmodule;
  BsePlugin *plugin;
  const BseExportBegin *sym_begin;
  const BseExportEnd *sym_end;
  gconstpointer *sym_array;
  const gchar *error = NULL;
  
  g_return_val_if_fail (file_name != NULL, NULL);

  /* open libtool archive
   */
  fd = open (file_name, O_RDONLY, 0);
  if (fd < 0)
    return (errno == ENOENT || errno == ENOTDIR || errno == ELOOP ?
	    bse_error_blurb (BSE_ERROR_FILE_NOT_FOUND) :
	    "Unable to access plugin");

  /* and search libtool's dlname specification
   */
  scanner = g_scanner_new (NULL);
  g_scanner_input_file (scanner, fd);
  scanner->config->symbol_2_token = TRUE;
  g_scanner_add_symbol (scanner, "dlname", GUINT_TO_POINTER (TOKEN_DLNAME));
  
  /* skip ahead */
  while (!g_scanner_eof (scanner) &&
	 g_scanner_peek_next_token (scanner) != TOKEN_DLNAME)
    g_scanner_get_next_token (scanner);
  
  /* parse dlname
   */
  if (g_scanner_get_next_token (scanner) != TOKEN_DLNAME ||
      g_scanner_get_next_token (scanner) != '=' ||
      g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    {
      g_scanner_destroy (scanner);
      close (fd);
      
      return "Plugin broken";
    }

  /* construct real module name
   */
  string = g_dirname (file_name);
  file_name = g_strconcat (string, G_DIR_SEPARATOR_S, scanner->value.v_string, NULL);
  g_free (string);
  
  g_scanner_destroy (scanner);
  close (fd);

  /* load module
   */
  gmodule = g_module_open (file_name, 0);
  
  if (!gmodule)
    {
      g_free (file_name);

      return g_module_error ();
    }

  if (bse_plugin_find (gmodule))
    {
      g_module_close (gmodule);
      g_free (file_name);

      return "Plugin already loaded";
    }

  /* check whether this is a BSE module
   */
  if (!g_module_symbol (gmodule, BSE_EXPORT_SYMBOL (Begin), (gpointer) &sym_begin) ||
      !g_module_symbol (gmodule, BSE_EXPORT_SYMBOL (End), (gpointer) &sym_end) ||
      *sym_end != BSE_MAGIC)
    {
      g_module_close (gmodule);
      g_free (file_name);
      
      return "Not a BSE Plugin";
    }

  if (bse_plugin_lookup (*sym_begin))
    {
      g_module_close (gmodule);
      g_free (file_name);

      return "Plugin already loaded";
    }

  /* create plugin and feature type registration
   */
  plugin = g_new0 (BsePlugin, 1); /* FIXME: alloc_only */
  plugin->name = g_strdup (*sym_begin);
  plugin->fname = file_name;
  plugin->gmodule = gmodule;
  plugin->module_refs = 0;

  if (g_module_symbol (gmodule, BSE_EXPORT_SYMBOL (Procedure), (gpointer) &sym_array))
    error = bse_plugin_register_types (plugin, sym_array, BSE_EXPORT_TYPE_PROCS);

  bse_plugins = g_slist_prepend (bse_plugins, plugin);

  bse_plugin_unload (plugin);
  
  return error;
}

BsePlugin*
bse_plugin_lookup (const gchar *name)
{
  GSList *slist;

  g_return_val_if_fail (name != NULL, NULL);

  for (slist = bse_plugins; slist; slist = slist->next)
    {
      BsePlugin *plugin = slist->data;

      if (bse_string_equals (name, plugin->name))
	return plugin;
    }

  return NULL;
}

/* --- directory scanning --- */
#include <sys/types.h>
#include <dirent.h>
GList*
bse_plugin_dir_list_files (const gchar *_dir_list)
{
  GList *file_list = NULL;
  gchar *free_me, *dir_list = g_strdup (_dir_list);

  g_return_val_if_fail (dir_list != NULL, NULL);

  free_me = dir_list;

  while (dir_list && dir_list[0])
    {
      gchar *name = dir_list;
      DIR *dd;

      dir_list = strchr (dir_list, G_SEARCHPATH_SEPARATOR);
      if (dir_list)
	*(dir_list++) = 0;

      dd = opendir (name);
      if (dd)
	{
	  struct dirent *d_entry = readdir (dd);

	  while (d_entry)
	    {
	      guint l = strlen (d_entry->d_name);

	      /* select libtool archives */
	      if (l > 3 &&
		  d_entry->d_name[l - 3] == '.' &&
		  d_entry->d_name[l - 2] == 'l' &&
		  d_entry->d_name[l - 1] == 'a')
		file_list = g_list_prepend (file_list,
					    g_strconcat (name,
							 "/",
							 d_entry->d_name,
							 NULL));
	      d_entry = readdir (dd);
	    }
	  closedir (dd);
	}
    }
  g_free (free_me);

  return file_list;
}
