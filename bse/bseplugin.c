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

#include	"bsecategories.h"
#include	"bseprocedure.h"
#include	"bseobject.h"
#include	"bseenums.h"
#include	<gmodule.h>
#include	<string.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<unistd.h>



/* --- prototypes --- */
static const gchar* bse_plugin_register_types	(BsePlugin      *plugin,
						 gconstpointer   array_p,
						 BseExportType   exp_type);
static void	    bse_plugin_complete_info	(BsePlugin	*plugin,
						 GType       	 type,
						 GTypeInfo 	*type_info);


/* --- variables --- */
gconstpointer BSE_EXPORT_IMPL_S (Procedure) = NULL;	/* placeholder for proc-less builtins */
gconstpointer BSE_EXPORT_IMPL_S (Object) = NULL;	/* placeholder for obj-less builtins */
BSE_EXPORT_IMPL_T = NULL;				/* bse_plugin_builtin_init() declaration */
static GSList    *bse_plugins = NULL;
static GTypePluginVTable bse_plugin_vtable = {
  (GTypePluginRef) bse_plugin_ref,
  (GTypePluginUnRef) bse_plugin_unref,
  (GTypePluginFillTypeInfo) bse_plugin_complete_info,
  (GTypePluginFillInterfaceInfo) NULL,
};


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
	  plugin->gtype_plugin.vtable = &bse_plugin_vtable;
	  plugin->name = "BSE-Builtin"; /* this is usually reset */
	  plugin->fname = NULL;
	  plugin->gmodule = NULL;
	  plugin->module_refs = 42;

	  BSE_IF_DEBUG (PLUGINS)
	    g_message ("register-builtin-plugin \"%s\"", plugin->name);

	  name = plugin->name;
	  error = builtin_inits[i] (plugin);
	  if (error)
	    g_warning ("Plugin \"%s\" initialization failed: %s", plugin->name, error);
	  if (name == plugin->name)
	    g_warning ("builtin plugin initializer (%p) doesn't assign plugin name",
		       builtin_inits[i]);

	}
      bse_plugin_builtin_init = NULL;
    }
}

static const gchar*
bse_plugin_register_types (BsePlugin    *plugin,
			   gconstpointer array_p,
			   BseExportType exp_type)
{
  switch (plugin && array_p ? exp_type : 0)
    {
      const BseExportProcedure *pspec;
      const BseExportObject *ospec;
      const BseExportEnum *espec;
      
    case BSE_EXPORT_TYPE_PROCS:
      for (pspec = array_p; pspec->type_p; pspec++)
	{
	  if (pspec->name &&
	      pspec->init &&
	      pspec->exec)
	    {
	      GType   type;
	      guint on = plugin->n_proc_types;
	      const gchar *error;
	      
	      BSE_IF_DEBUG (PLUGINS)
		g_message ("register-procedure: \"%s\"", pspec->name);
	      
	      type = g_type_from_name (pspec->name);
	      if (type)
		return "Attempt to register known procedure";
	      
	      error = bse_procedure_type_register (pspec->name,
						   pspec->blurb,
						   plugin,
						   &type);
	      if (error)
		return error;
	      
	      if (pspec->category)
		bse_categories_register_icon (pspec->category, type, &pspec->pixdata);
	      
	      plugin->n_proc_types++;
	      plugin->proc_types = g_renew (GType  ,
					    plugin->proc_types,
					    plugin->n_proc_types);
	      plugin->proc_types[on] = type;
	      
	      plugin->exports_procedures = TRUE;
	      *(pspec->type_p) = type;
	    }
	}
      plugin->e_procs = array_p;
      return NULL;
      
    case BSE_EXPORT_TYPE_OBJECTS:
      for (ospec = array_p; ospec->type_p; ospec++)
	{
	  if (ospec->name &&
	      ospec->object_info)
	    {
	      GType   type;
	      guint on = plugin->n_object_types;
	      const gchar *error;
	      
	      BSE_IF_DEBUG (PLUGINS)
		g_message ("register-object: \"%s\"", ospec->name);
	      
	      type = g_type_from_name (ospec->name);
	      if (type)
		return "Attempt to register known object";
	      
	      error = bse_object_type_register (ospec->name,
						ospec->parent_type,
						ospec->blurb,
						plugin,
						&type);
	      
	      if (error)
		return error;
	      
	      if (ospec->category)
		bse_categories_register_icon (ospec->category, type, &ospec->pixdata);
	      
	      plugin->n_object_types++;
	      plugin->object_types = g_renew (GType  ,
					      plugin->object_types,
					      plugin->n_object_types);
	      plugin->object_types[on] = type;
	      
	      plugin->exports_objects = TRUE;
	      *(ospec->type_p) = type;
	    }
	}
      plugin->e_objects = array_p;
      return NULL;
      
    case BSE_EXPORT_TYPE_ENUMS:
      for (espec = array_p; espec->type_p; espec++)
	{
	  if (espec->name &&
	      espec->parent_type &&
	      espec->values)
	    {
	      GType   type;
	      guint on = plugin->n_enum_types;
	      
	      BSE_IF_DEBUG (PLUGINS)
		g_message ("register-enum: \"%s\"", espec->name);
	      
	      type = g_type_from_name (espec->name);
	      if (type)
		return "Attempt to register known type";
	      if (!G_TYPE_IS_ENUM (espec->parent_type) && !G_TYPE_IS_FLAGS (espec->parent_type))
		return "Invalid enum/flags registration attempt";

	      type = bse_type_register_dynamic (espec->parent_type,
						espec->name, NULL,
						plugin);

	      plugin->n_enum_types++;
	      plugin->enum_types = g_renew (GType  ,
					    plugin->enum_types,
					    plugin->n_enum_types);
	      plugin->enum_types[on] = type;
	      
	      plugin->exports_enums = TRUE;
	      *(espec->type_p) = type;
	    }
	}
      plugin->e_enums = array_p;
      return NULL;
    }
  
  return bse_error_blurb (BSE_ERROR_INTERNAL);
}

static const gchar*
bse_plugin_reinit_type_ids (BsePlugin *plugin)
{
  const BseExportProcedure *pspecs = plugin->e_procs;
  const BseExportObject *ospecs = plugin->e_objects;
  const BseExportEnum *especs = plugin->e_enums;
  gchar *error = NULL;
  
  /* procedure types
   */
  if (pspecs)
    {
      const BseExportProcedure *pspec;
      GType   *exp_types = plugin->proc_types;
      GType   *exp_last = plugin->proc_types + plugin->n_proc_types;
      
      for (pspec = pspecs; pspec->type_p && exp_types < exp_last; pspec++)
	if (pspec->name &&
	    pspec->init &&
	    pspec->exec &&
	    g_type_from_name (pspec->name) == *exp_types)
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

  /* object types
   */
  if (ospecs)
    {
      const BseExportObject *ospec;
      GType   *exp_types = plugin->object_types;
      GType   *exp_last = plugin->object_types + plugin->n_object_types;
      
      for (ospec = ospecs; ospec->type_p && exp_types < exp_last; ospec++)
	if (ospec->object_info &&
	    g_type_from_name (ospec->name) == *exp_types)
	  {
	    if (*ospec->type_p)
	      g_warning ("while reinitializing \"%s\", type id for `%s' already assigned?",
			 plugin->name,
			 ospec->name);
	    *ospec->type_p = *(exp_types++);
	  }
      if (exp_types < exp_last)
	error = "Unable to rematch object types from previous initialization";
    }

  /* enum types
   */
  if (especs)
    {
      const BseExportEnum *espec;
      GType   *exp_types = plugin->enum_types;
      GType   *exp_last = plugin->enum_types + plugin->n_enum_types;
      
      for (espec = especs; espec->type_p && exp_types < exp_last; espec++)
	if (espec->name &&
	    espec->parent_type &&
	    espec->values &&
	    g_type_from_name (espec->name) == *exp_types)
	  {
	    if (*espec->type_p)
	      g_warning ("while reinitializing \"%s\", type id for `%s' already assigned?",
			 plugin->name,
			 espec->name);
	    *espec->type_p = *(exp_types++);
	  }
      if (exp_types < exp_last)
	error = "Unable to rematch enum types from previous initialization";
    }

  return error;
}

void
bse_plugin_ref (BsePlugin *plugin)
{
  gboolean need_reinit = FALSE;

  g_return_if_fail (plugin != NULL);
  
  if (!plugin->module_refs)
    {
      const gchar *error = NULL;
      
      BSE_IF_DEBUG (PLUGINS)
	g_message ("reloading-plugin \"%s\"", plugin->name);
      
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
	      need_reinit |= TRUE;
	    }
	  else
	    error = g_module_error ();
	}
      if (!error && plugin->exports_objects)
	{
	  gpointer *sym_array;

	  if (g_module_symbol (plugin->gmodule,
			       BSE_EXPORT_SYMBOL (Object),
			       (gpointer) &sym_array))
	    {
	      plugin->e_objects = sym_array;
	      need_reinit |= TRUE;
	    }
	  else
	    error = g_module_error ();
	}
      if (!error && plugin->exports_enums)
	{
	  gpointer *sym_array;

	  if (g_module_symbol (plugin->gmodule,
			       BSE_EXPORT_SYMBOL (Enum),
			       (gpointer) &sym_array))
	    {
	      if (sym_array)
		{
		  plugin->e_enums = *sym_array;
		  need_reinit |= TRUE;
		}
	      else
		error = "Failed to refetch enum types";
	    }
	  else
	    error = g_module_error ();
	}

      if (!error)
	error = bse_plugin_reinit_type_ids (plugin);

      if (error)
	g_error ("Fatal plugin error, failed to reinitialize plugin: %s", error);
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
  plugin->e_objects = NULL;
  plugin->e_enums = NULL;

  BSE_IF_DEBUG (PLUGINS)
    g_message ("unloaded-plugin \"%s\"", plugin->name);
}

void
bse_plugin_unref (BsePlugin *plugin)
{
  g_return_if_fail (plugin != NULL);
  g_return_if_fail (plugin->module_refs > 0);
  
  plugin->module_refs--;
  if (!plugin->module_refs)
    bse_plugin_unload (plugin);
}

#define POINTER_ADD(p,n_bytes)	((void*) (((const char*) (p)) + (unsigned long) (n_bytes)))

static inline const BseExportSpec*
bse_plugin_get_export_spec (BsePlugin    *plugin,
			    GType         type,
			    gconstpointer export_specs,
			    guint         spec_size)
{
  g_return_val_if_fail (export_specs && spec_size > sizeof (GType*), NULL);

  if (export_specs)
    {
      const BseExportSpec* spec;

      for (spec = export_specs; spec->type_p; spec = POINTER_ADD (spec, spec_size))
	if (*(spec->type_p) == type)
	  return spec;
    }
  
  return NULL;
}

static void
enum_flags_complete_info (const BseExportSpec *spec,
			  GTypeInfo           *info)
{
  const BseExportEnum *espec = &spec->s_enum;
  GType type = *(espec->type_p);

  if (G_TYPE_IS_ENUM (type))
    g_enum_complete_type_info (type, info, espec->values);
  else if (G_TYPE_IS_FLAGS (type))
    g_flags_complete_type_info (type, info, espec->values);
  else
    g_assert_not_reached ();
}

static void
bse_plugin_complete_info (BsePlugin   *plugin,
			  GType        type,
			  GTypeInfo *type_info)
{
  const BseExportSpec *export_spec = NULL;
  gconstpointer specs_p = NULL;
  guint spec_size = 0;
  void (*complete_info) (const BseExportSpec *, GTypeInfo *) = NULL;
  
  g_return_if_fail (plugin != NULL);
  g_return_if_fail (plugin->module_refs > 0);

  /* we are an internal function, and thusly need to behave pretty conservative,
   * so in effect we only guarrantee to always fill the type_info structure we
   * get passed.
   */
  
  switch (G_TYPE_FUNDAMENTAL (type))
    {
    case BSE_TYPE_PROCEDURE:
      specs_p = plugin->e_procs;
      spec_size = sizeof (BseExportProcedure);
      complete_info = bse_procedure_complete_info;
      break;
    case G_TYPE_OBJECT:
      specs_p = plugin->e_objects;
      spec_size = sizeof (BseExportObject);
      complete_info = bse_object_complete_info;
      break;
    case G_TYPE_ENUM:
    case G_TYPE_FLAGS:
      specs_p = plugin->e_enums;
      spec_size = sizeof (BseExportEnum);
      complete_info = enum_flags_complete_info;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  export_spec = bse_plugin_get_export_spec (plugin, type, specs_p, spec_size);
  if (!export_spec || !complete_info)
    g_error ("unable to find export spec for `%s' in \"%s\"",
	     g_type_name (type),
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

gchar*
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
  gpointer *sym_array;
  gchar *error = NULL;
  
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
      
      return "Plugin's dlname broken";
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

      return "Plugin already registered (clone?)";
    }

  /* create plugin and feature type registration
   */
  plugin = g_new0 (BsePlugin, 1); /* FIXME: alloc_only */
  plugin->gtype_plugin.vtable = &bse_plugin_vtable;
  plugin->name = g_strdup (*sym_begin);
  plugin->fname = file_name;
  plugin->gmodule = gmodule;
  plugin->module_refs = 0;

  if (g_module_symbol (gmodule, BSE_EXPORT_SYMBOL (Procedure), (gpointer) &sym_array))
    error = (gchar*) bse_plugin_register_types (plugin, sym_array, BSE_EXPORT_TYPE_PROCS);
  if (!error && g_module_symbol (gmodule, BSE_EXPORT_SYMBOL (Object), (gpointer) &sym_array))
    error = (gchar*) bse_plugin_register_types (plugin, sym_array, BSE_EXPORT_TYPE_OBJECTS);
  if (!error && g_module_symbol (gmodule, BSE_EXPORT_SYMBOL (Enum), (gpointer) &sym_array))
    {
      if (sym_array)
	error = (gchar*) bse_plugin_register_types (plugin, *sym_array, BSE_EXPORT_TYPE_ENUMS);
      else
	error = "Failed to fetch enum types";
    }

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
		file_list = g_list_prepend (file_list, g_strconcat (name,
								    "/",
								    d_entry->d_name,
								    NULL));
	      d_entry = readdir (dd);
	    }
	  closedir (dd);
	}
    }
  g_free (free_me);

  return g_list_sort (file_list, (GCompareFunc) strcmp);
}
