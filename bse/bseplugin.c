/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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


#define DEBUG   sfi_debug_keyfunc ("plugins")


/* --- prototypes --- */
static void	    bse_plugin_init		(BsePlugin	  *plugin);
static void	    bse_plugin_class_init	(BsePluginClass	  *class);
static void	    bse_plugin_complete_info	(GTypePlugin	  *gplugin,
						 GType       	   type,
						 GTypeInfo 	  *type_info,
						 GTypeValueTable  *value_vtable);
static void	    bse_plugin_use		(GTypePlugin	  *gplugin);
static void	    bse_plugin_unuse		(GTypePlugin	  *gplugin);
static void         bse_plugin_init_types       (BsePlugin        *plugin);
static void         bse_plugin_reinit_types     (BsePlugin        *plugin);
static void	    type_plugin_iface_init	(GTypePluginClass *iface);

/* --- variables --- */
static GSList       *bse_plugins = NULL;
static BseExportNode builtin_export_chain_head = { NULL, BSE_EXPORT_NODE_LINK, };
BseExportIdentity    bse_builtin_export_identity = BSE_EXPORT_IDENTITY (NULL, builtin_export_chain_head);


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePlugin)
{
  static const GTypeInfo plugin_info = {
    sizeof (BsePluginClass),
    NULL,           /* base_init */
    NULL,           /* base_finalize */
    (GClassInitFunc) bse_plugin_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (BsePlugin),
    0,		/* n_preallocs */
    (GInstanceInitFunc) bse_plugin_init,
  };
  static const GInterfaceInfo iface_info = {
    (GInterfaceInitFunc) type_plugin_iface_init,
    NULL,		/* interface_finalize */
    NULL,		/* interface_data */
  };
  GType plugin_type;
  
  plugin_type = g_type_register_static (G_TYPE_OBJECT, "BsePlugin", &plugin_info, 0);
  
  g_type_add_interface_static (plugin_type, G_TYPE_TYPE_PLUGIN, &iface_info);
  
  return plugin_type;
}

static void
type_plugin_iface_init (GTypePluginClass *iface)
{
  iface->use_plugin = bse_plugin_use;
  iface->unuse_plugin = bse_plugin_unuse;
  iface->complete_type_info = bse_plugin_complete_info;
}

static void
bse_plugin_dispose (GObject *object)
{
  BsePlugin *plugin = BSE_PLUGIN (object);
  
  g_warning (G_STRLOC ": dispose should never happen for static plugins");
  
  g_object_ref (object);
  
  /* chain parent class handler */
  G_OBJECT_CLASS (g_type_class_peek_parent (BSE_PLUGIN_GET_CLASS (plugin)))->dispose (object);
}

static void
bse_plugin_class_init (BsePluginClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  
  gobject_class->dispose = bse_plugin_dispose;
}

static void
bse_plugin_init (BsePlugin *plugin)
{
  plugin->name = NULL;
  plugin->fname = NULL;
  plugin->gmodule = NULL;
  plugin->use_count = 0;
  plugin->exports_procedures = FALSE;
  plugin->exports_objects = FALSE;
  plugin->exports_enums = FALSE;
  plugin->n_types = 0;
  plugin->types = NULL;
}

void
bse_plugin_init_builtins (void)
{
  if (!bse_plugins)
    {
      /* include extern declarations of builtin init functions */
#include "bsebuiltin_externs.c"
      static BseExportNode* (* const builtin_inits[]) (void) = {
	/* and list them in an array */
#include "bsebuiltin_array.c"
      };
      static const guint n_builtin_inits = G_N_ELEMENTS (builtin_inits);
      guint i;
      
      /* initialize builtin types via pseudo plugin handle */
      for (i = 0; i < n_builtin_inits; i++)
	{
          BseExportNode *chain = builtin_inits[i] ();
          if (chain)
            {
              /* create resident plugin struct */
              BsePlugin *plugin = g_object_new (BSE_TYPE_PLUGIN, NULL);
              g_object_ref (plugin);
              plugin->use_count = 1;
              g_free (plugin->name);
              plugin->name = g_strdup ("BSE-BUILTIN");
              plugin->chain = chain;
              bse_plugins = g_slist_prepend (bse_plugins, plugin);
              bse_plugin_init_types (plugin);
            }
	}
      /* initialize builtin export nodes (used for C++ modules) */
      if (bse_builtin_export_identity.export_chain)
        {
          /* create resident plugin struct */
          BsePlugin *plugin = g_object_new (BSE_TYPE_PLUGIN, NULL);
          g_object_ref (plugin);
          plugin->use_count = 1;
          g_free (plugin->name);
          plugin->name = g_strdup ("BSE-CXX-BUILTIN");
          plugin->chain = bse_builtin_export_identity.export_chain;
          bse_plugins = g_slist_prepend (bse_plugins, plugin);
          bse_plugin_init_types (plugin);
        }
    }
}

static BseExportIdentity*
lookup_export_identity (GModule *gmodule)
{
  BseExportIdentity **symbol_p = NULL;
  if (g_module_symbol (gmodule, BSE_EXPORT_IDENTITY_STRING, (gpointer) &symbol_p))
    {
      if (symbol_p)
        return *symbol_p;
    }
  return NULL;
}

static void
bse_plugin_use (GTypePlugin *gplugin)
{
  BsePlugin *plugin = BSE_PLUGIN (gplugin);
  
  g_return_if_fail (plugin != NULL);
  
  g_object_ref (G_OBJECT (plugin));
  if (!plugin->use_count)
    {
      BseExportIdentity *plugin_identity;

      DEBUG ("reloading-plugin \"%s\" (\"%s\")", plugin->name, plugin->fname ? plugin->fname : "???NULL???");
      
      plugin->use_count++;
      plugin->gmodule = g_module_open (plugin->fname, 0); /* reopen for use non-lazy */
      plugin_identity = plugin->gmodule ? lookup_export_identity (plugin->gmodule) : NULL;
      if (!plugin->gmodule || !plugin_identity)
	g_error ("failed to reinitialize plugin: %s", g_module_error ());

      plugin->chain = plugin_identity->export_chain;

      bse_plugin_reinit_types (plugin);
    }
  else
    plugin->use_count++;
}

static void
bse_plugin_unload (BsePlugin *plugin)
{
  g_return_if_fail (plugin->gmodule != NULL && plugin->fname != NULL);
  
  g_module_close (plugin->gmodule);
  plugin->gmodule = NULL;
  
  /* reset plugin local pointers */
  plugin->chain = NULL;
  
  DEBUG ("unloaded-plugin \"%s\"", plugin->name);
}

static void
bse_plugin_unuse (GTypePlugin *gplugin)
{
  BsePlugin *plugin = BSE_PLUGIN (gplugin);
  
  g_return_if_fail (plugin->use_count > 0);
  
  plugin->use_count--;
  if (!plugin->use_count)
    {
      if (plugin->fname)
	bse_plugin_unload (plugin);
      else
	{
	  g_warning ("%s: attempt to unload builtin plugin due to use_count==0", G_STRLOC);
	  plugin->use_count = 1;
	}
    }
  g_object_unref (G_OBJECT (plugin));
}

static void
bse_plugin_complete_info (GTypePlugin     *gplugin,
			  GType            type,
			  GTypeInfo       *type_info,
			  GTypeValueTable *value_vtable)
{
  BsePlugin *plugin = BSE_PLUGIN (gplugin);
  BseExportNode *node;

  g_return_if_fail (plugin != NULL);
  g_return_if_fail (plugin->use_count > 0);
  
  for (node = plugin->chain; node && node->ntype; node = node->next)
    if (node->type == type)
      {
        switch (node->ntype)
          {
            BseExportNodeEnum *enode;
            BseExportNodeClass *cnode;
            BseExportNodeProc *pnode;
          case BSE_EXPORT_NODE_CLASS:
            cnode = (BseExportNodeClass*) node;
            type_info->class_size = cnode->class_size;
            type_info->class_init = cnode->class_init;
            type_info->class_finalize = cnode->class_finalize;
            type_info->instance_size = cnode->instance_size;
            type_info->instance_init = cnode->instance_init;
            break;
          case BSE_EXPORT_NODE_PROC:
            pnode = (BseExportNodeProc*) node;
            bse_procedure_complete_info (pnode, type_info);
            break;
          case BSE_EXPORT_NODE_LINK:
            break;
          case BSE_EXPORT_NODE_ENUM:
            enode = (BseExportNodeEnum*) node;
            g_enum_complete_type_info (type, type_info, enode->values);
            break;
          default: ;
          }
        break;
      }
  if (!node || node->type != type)
    g_error ("%s: unable to complete type from plugin: %s", plugin->name, g_type_name (type));
}

static void
bse_plugin_reinit_types (BsePlugin *plugin)
{
  guint n = plugin->n_types;
  GType *types = g_memdup (plugin->types, sizeof (plugin->types[0]) * n);
  BseExportNode *node;

  for (node = plugin->chain; node && node->ntype; node = node->next)
    {
      GType type = node->name ? g_type_from_name (node->name) : 0;
      if (type) // we might have left out this node upon initialization intentionally
        {
          guint i, found_type = FALSE;
          for (i = 0; i < n; i++)
            if (types[i] == type)
              {
                node->type = type;
                types[i] = types[--n];
                found_type = TRUE;
                break;
              }
          if (!found_type)
            g_message ("%s: plugin attempts to reregister foreign type: %s",
                       plugin->name, node->name);
        }
    }
  while (n--)
    g_message ("%s: plugin failed to reregister type: %s", plugin->name, g_type_name (types[n]));
  g_free (types);
}

static void
bse_plugin_init_types (BsePlugin *plugin)
{
  BseExportNode *node;

  /* check type uniqueness */
  for (node = plugin->chain; node && node->ntype; node = node->next)
    switch (node->ntype)
      {
        BseExportNodeClass *cnode;
        GType type;
      case BSE_EXPORT_NODE_CLASS:
        cnode = (BseExportNodeClass*) node;
        type = g_type_from_name (cnode->parent);
        if (!type)
          {
            g_message ("%s: plugin type %s derives from unknown parent type: %s",
                       plugin->name, node->name, cnode->parent);
            return;
          }
        if (!BSE_TYPE_IS_OBJECT (type))
          {
            g_message ("%s: plugin object type %s derives from non-object type: %s",
                       plugin->name, node->name, cnode->parent);
            return;
          }
      case BSE_EXPORT_NODE_LINK:
        break;
      case BSE_EXPORT_NODE_PROC:
      case BSE_EXPORT_NODE_ENUM:
        type = g_type_from_name (node->name);
        if (type)
          {
            g_message ("%s: plugin contains type already registered: %s",
                       plugin->name, node->name);
            return;
          }
      default: ;
      }

  /* register BSE module types */
  for (node = plugin->chain; node && node->ntype; node = node->next)
    {
      GType type = 0;
      switch (node->ntype)
        {
          BseExportNodeClass *cnode;
          const gchar *error;
        case BSE_EXPORT_NODE_LINK:
          break;
        case BSE_EXPORT_NODE_ENUM:
          type = bse_type_register_dynamic (G_TYPE_ENUM, node->name, NULL,
                                            G_TYPE_PLUGIN (plugin));
          /* FIXME: can't register dynamic type transforms with glib-2.2.1
           * g_value_register_transform_func (SFI_TYPE_CHOICE, type, sfi_value_choice2enum_simple);
           * g_value_register_transform_func (type, SFI_TYPE_CHOICE, sfi_value_enum2choice);
           */
          break;
        case BSE_EXPORT_NODE_CLASS:
          cnode = (BseExportNodeClass*) node;
          type = bse_type_register_dynamic (g_type_from_name (cnode->parent),
                                            node->name, node->blurb,
                                            G_TYPE_PLUGIN (plugin));
          break;
        case BSE_EXPORT_NODE_PROC:
          error = bse_procedure_type_register (node->name, node->blurb,
                                               plugin, &type);
          if (error)
            g_message ("%s: while registering procedure \"%s\": %s",
                       plugin->name, node->name, error);
          break;
        default:
          g_message ("%s: plugin contains invalid type node (%u)", plugin->name, node->ntype);
          node = NULL;
          break;
        }
      if (type)
        {
          guint n;
          if (node->category)
            bse_categories_register (node->category, type, node->pixstream);
          n = plugin->n_types++;
          plugin->types = g_renew (GType, plugin->types, plugin->n_types);
          plugin->types[n] = type;
          node->type = type;
        }
    }
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
  BseExportIdentity *plugin_identity;
  gchar *file_name = (gchar*) _file_name;
  gint fd;
  GScanner *scanner;
  GModule *gmodule;
  gchar *error = NULL;
  
  g_return_val_if_fail (file_name != NULL, NULL);
  
  /* open libtool archive */
  fd = open (file_name, O_RDONLY, 0);
  if (fd < 0)
    return (errno == ENOENT || errno == ENOTDIR || errno == ELOOP ?
	    bse_error_blurb (BSE_ERROR_FILE_NOT_FOUND) :
	    "Unable to access plugin");
  
  /* and search libtool's dlname specification */
  scanner = g_scanner_new (NULL);
  g_scanner_input_file (scanner, fd);
  scanner->config->symbol_2_token = TRUE;
  g_scanner_add_symbol (scanner, "dlname", GUINT_TO_POINTER (TOKEN_DLNAME));
  
  /* skip ahead */
  while (!g_scanner_eof (scanner) &&
	 g_scanner_peek_next_token (scanner) != TOKEN_DLNAME)
    g_scanner_get_next_token (scanner);
  
  /* parse dlname */
  if (g_scanner_get_next_token (scanner) != TOKEN_DLNAME ||
      g_scanner_get_next_token (scanner) != '=' ||
      g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    {
      g_scanner_destroy (scanner);
      close (fd);
      
      return "Plugin's dlname broken";
    }
  
  /* construct real module name */
  if (g_path_is_absolute (scanner->value.v_string))
    file_name = g_strdup (scanner->value.v_string);
  else
    {
      gchar *string = g_path_get_dirname (file_name);
      file_name = g_strconcat (string, G_DIR_SEPARATOR_S, scanner->value.v_string, NULL);
      g_free (string);
    }
  g_scanner_destroy (scanner);
  close (fd);
  
  /* load module */
  gmodule = g_module_open (file_name, G_MODULE_BIND_LAZY);
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

  /* verify plugin identity (BSE + version) */
  plugin_identity = lookup_export_identity (gmodule);
  if (!plugin_identity || !plugin_identity->name)
    {
      g_module_close (gmodule);
      g_free (file_name);
      return "Not a BSE Plugin";
    }
  if (plugin_identity->major != BSE_MAJOR_VERSION ||
      plugin_identity->minor != BSE_MINOR_VERSION ||
      plugin_identity->micro != BSE_MICRO_VERSION)
    {
      g_module_close (gmodule);
      g_free (file_name);
      return "Invalid BSE Plugin Version";
    }

  /* create plugin if this is a BSE plugin with valid type chain */
  if (plugin_identity->export_chain)
    {
      BsePlugin *plugin = g_object_new (BSE_TYPE_PLUGIN, NULL);
      g_free (plugin->name);
      plugin->name = g_strdup (plugin_identity->name);
      plugin->fname = file_name;
      plugin->gmodule = gmodule;
      plugin->chain = plugin_identity->export_chain;
      
      /* register BSE module types */
      bse_plugin_init_types (plugin);

      bse_plugins = g_slist_prepend (bse_plugins, plugin);
      bse_plugin_unload (plugin);
    }
  else
    {
      g_module_close (gmodule);
      g_free (file_name);
      error = NULL; /* empty plugin */
    }

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

#include "topconfig.h"

SfiRing*
bse_plugin_path_list_files (void)
{
  SfiRing *ring1, *ring2 = NULL;

  ring1 = sfi_file_crawler_list_files (BSE_PATH_PLUGINS, "*.la", 0);
  ring1 = sfi_ring_sort (ring1, (GCompareFunc) strcmp);

  if (BSE_GCONFIG (plugin_path) && BSE_GCONFIG (plugin_path)[0])
    ring2 = sfi_file_crawler_list_files (BSE_GCONFIG (plugin_path), "*.la", 0);
  ring2 = sfi_ring_sort (ring2, (GCompareFunc) strcmp);

  return sfi_ring_concat (ring1, ring2);
}
