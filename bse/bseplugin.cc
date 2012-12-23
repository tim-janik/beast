// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseplugin.hh"

#include "bsecategories.hh"
#include "bseprocedure.hh"
#include "bseobject.hh"
#include "bseenums.hh"
#include "bsemain.hh"
#include <gmodule.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>


static SFI_MSG_TYPE_DEFINE (debug_plugins, "plugins", SFI_MSG_DEBUG, NULL);
#define DEBUG(...)      sfi_debug (debug_plugins, __VA_ARGS__)


/* --- prototypes --- */
static void	    bse_plugin_init		(BsePlugin	  *plugin);
static void	    bse_plugin_class_init	(BsePluginClass	  *klass);
static void	    bse_plugin_complete_info	(GTypePlugin	  *gplugin,
						 GType       	   type,
						 GTypeInfo 	  *type_info,
						 GTypeValueTable  *value_vtable);
static void	    bse_plugin_use		(GTypePlugin	  *gplugin);
static void	    bse_plugin_unuse		(GTypePlugin	  *gplugin);
static void         bse_plugin_init_types       (BsePlugin        *plugin);
static void         bse_plugin_reinit_types     (BsePlugin        *plugin);
static void         bse_plugin_uninit_types     (BsePlugin        *plugin);
static void	    type_plugin_iface_init	(GTypePluginClass *iface);

/* --- variables --- */
static GSList       *bse_plugins = NULL;
static BseExportNode builtin_export_chain_head = { NULL, BSE_EXPORT_NODE_LINK, };
BseExportIdentity    bse_builtin_export_identity = BSE_EXPORT_IDENTITY (builtin_export_chain_head);


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
  GType plugin_type = g_type_register_static (G_TYPE_OBJECT, "BsePlugin", &plugin_info, GTypeFlags (0));
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

  if (plugin->gmodule || plugin->use_count || plugin->n_types)
    g_warning ("%s: plugin partially initialized during destruciton", G_STRFUNC);
  
  /* chain parent class handler */
  G_OBJECT_CLASS (g_type_class_peek_parent (BSE_PLUGIN_GET_CLASS (plugin)))->dispose (object);
}

static void
bse_plugin_finalize (GObject *object)
{
  BsePlugin *plugin = BSE_PLUGIN (object);

  if (plugin->gmodule || plugin->use_count || plugin->n_types)
    g_warning ("%s: plugin partially initialized during destruciton", G_STRFUNC);
  
  /* chain parent class handler */
  G_OBJECT_CLASS (g_type_class_peek_parent (BSE_PLUGIN_GET_CLASS (plugin)))->finalize (object);

  g_free (plugin->fname);
  g_free (plugin->types);
}

static void
bse_plugin_class_init (BsePluginClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->dispose = bse_plugin_dispose;
  gobject_class->finalize = bse_plugin_finalize;
}

static void
bse_plugin_init (BsePlugin *plugin)
{
  plugin->fname = NULL;
  plugin->gmodule = NULL;
  plugin->missing_export_flags = 0;
  plugin->use_count = 0;
  plugin->version_match = 1;
  plugin->force_clean = 0;
  plugin->resident_types = 0;
  plugin->n_types = 0;
  plugin->types = NULL;
}

#include "bsebuiltin_externs.cc" // include extern declarations of builtin init functions

void
bse_plugin_init_builtins (void)
{
  if (!bse_plugins)
    {
      static BseExportNode* (* const builtin_inits[]) (void) = {
	/* and list them in an array */
#include "bsebuiltin_array.cc"
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
              BsePlugin *plugin = (BsePlugin*) g_object_new (BSE_TYPE_PLUGIN, NULL);
              g_object_ref (plugin);
              plugin->use_count = 1;
              plugin->fname = g_strdup ("BSE-BUILTIN");
              plugin->chain = chain;
              bse_plugins = g_slist_prepend (bse_plugins, plugin);
              bse_plugin_init_types (plugin);
            }
	}
      /* initialize builtin export nodes (used for C++ modules) */
      if (bse_builtin_export_identity.export_chain)
        {
          /* create resident plugin struct */
          BsePlugin *plugin = (BsePlugin*) g_object_new (BSE_TYPE_PLUGIN, NULL);
          g_object_ref (plugin);
          plugin->use_count = 1;
          plugin->fname = g_strdup ("BSE-CXX-BUILTIN");
          plugin->chain = bse_builtin_export_identity.export_chain;
          bse_plugins = g_slist_prepend (bse_plugins, plugin);
          bse_plugin_init_types (plugin);
        }
    }
}

static guint64
runtime_export_config (void)
{
  SfiCPUInfo cinfo = sfi_cpu_info();
  guint64 emask = 0;
  if (cinfo.x86_mmx)
    emask |= BSE_EXPORT_FLAG_MMX;
  if (cinfo.x86_mmxext)
    emask |= BSE_EXPORT_FLAG_MMXEXT;
  if (cinfo.x86_3dnow)
    emask |= BSE_EXPORT_FLAG_3DNOW;
  if (cinfo.x86_3dnowext)
    emask |= BSE_EXPORT_FLAG_3DNOWEXT;
  if (cinfo.x86_sse && cinfo.x86_ssesys)
    emask |= BSE_EXPORT_FLAG_SSE;
  if (cinfo.x86_sse2 && cinfo.x86_ssesys)
    emask |= BSE_EXPORT_FLAG_SSE2;
  if (cinfo.x86_sse3 && cinfo.x86_ssesys)
    emask |= BSE_EXPORT_FLAG_SSE3;
  if (cinfo.x86_sse4 && cinfo.x86_ssesys)
    emask |= BSE_EXPORT_FLAG_SSE4;
  return emask;
}

static BsePlugin *startup_plugin = NULL;

void
bse_plugin_make_resident()
{
  g_assert (startup_plugin != NULL);
  startup_plugin->resident_types = TRUE;
}

BsePlugin*
bse_exports__add_node (const BseExportIdentity *identity,
                       BseExportNode           *enode)
{
  if (!startup_plugin)
    g_error ("%s: plugin startup called without plugin", G_STRFUNC);
  if (!enode || enode->next)
    return NULL;
  if (identity->major != BSE_MAJOR_VERSION ||
      identity->minor != BSE_MINOR_VERSION ||
      identity->micro != BSE_MICRO_VERSION)
    startup_plugin->version_match = false;
  startup_plugin->missing_export_flags = identity->export_flags & ~runtime_export_config();
  if (startup_plugin->version_match && !startup_plugin->missing_export_flags)
    {
      enode->next = startup_plugin->chain;
      startup_plugin->chain = enode;
    }
  return startup_plugin;
}

static const char*
plugin_check_identity (BsePlugin *plugin, GModule *gmodule)
{
  if (!plugin->chain)
    {
      /* handle legacy C plugins */
      BseExportIdentity **symbol_p = NULL;
      if (g_module_symbol (gmodule, BSE_EXPORT_IDENTITY_STRING, (void**) &symbol_p) && *symbol_p)
        {
          BseExportIdentity *identity = *symbol_p;
          if (identity->major != BSE_MAJOR_VERSION ||
              identity->minor != BSE_MINOR_VERSION ||
              identity->micro != BSE_MICRO_VERSION)
            plugin->version_match = false;
          plugin->missing_export_flags = identity->export_flags & ~runtime_export_config();
          plugin->chain = identity->export_chain;
          plugin->force_clean = true;
        }
    }

  if (!plugin->version_match)
    return "Invalid BSE Plugin Version";
  if (plugin->missing_export_flags)
    return "Incompatible CPU requirements";

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
      DEBUG ("reloading-plugin: %s", plugin->fname);
      plugin->use_count++;
      startup_plugin = plugin;
      plugin->gmodule = g_module_open (plugin->fname, GModuleFlags (0)); /* reopen for use non-lazy */
      startup_plugin = NULL;
      if (!plugin->gmodule)
	g_error ("failed to reinitialize plugin \"%s\": %s", plugin->fname, g_module_error ());
      const char *cerror = plugin_check_identity (plugin, (GModule*) plugin->gmodule);
      if (cerror)
	g_error ("failed to reinitialize plugin \"%s\": %s", plugin->fname, cerror);
      if (!plugin->chain)
	g_error ("failed to reinitialize plugin \"%s\": %s", plugin->fname, "empty plugin");

      bse_plugin_reinit_types (plugin);
    }
  else
    plugin->use_count++;
}

void
bse_exports__del_node (BsePlugin               *plugin,
                       BseExportNode           *enode)
{
  if (!plugin || !enode)
    {
      g_warning ("%s: invalid plugin shutdown", G_STRFUNC);
      return;
    }
  BseExportNode *last = NULL, *link;
  for (link = plugin->chain; link; last = link, link = last->next)
    if (enode == link)
      {
        if (last)
          last->next = link->next;
        else
          plugin->chain = link->next;
        return;
      }
  g_warning ("%s: plugin attempt to unregister invalid export node: %s", plugin->fname, enode->name);
}

static void
bse_plugin_unload (BsePlugin *plugin)
{
  g_return_if_fail (plugin->gmodule != NULL && plugin->fname != NULL);
  g_return_if_fail (plugin->use_count == 0);
  g_return_if_fail (plugin->resident_types == 0);

  bse_plugin_uninit_types (plugin);
  g_module_close ((GModule*) plugin->gmodule);
  plugin->gmodule = NULL;

  /* reset plugin local pointers */
  if (plugin->force_clean)
    plugin->chain = NULL;

  DEBUG ("unloaded-plugin: %s", plugin->fname);
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
bse_plugin_uninit_types (BsePlugin *plugin)
{
  BseExportNode *node;
  for (node = plugin->chain; node && node->ntype; node = node->next)
    {
      GType type = node->type;
      if (type) // we might have left out this node upon initialization intentionally
        {
          if (node->ntype == BSE_EXPORT_NODE_ENUM)
            sfi_enum_type_set_choice_value_getter (type, NULL);
          else if (node->ntype == BSE_EXPORT_NODE_RECORD ||
              node->ntype == BSE_EXPORT_NODE_SEQUENCE)
            bse_type_uninit_boxed ((BseExportNodeBoxed*) node);
        }
    }
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
          case BSE_EXPORT_NODE_LINK:
          case BSE_EXPORT_NODE_HOOK:
            break;
          case BSE_EXPORT_NODE_ENUM:
            enode = (BseExportNodeEnum*) node;
            g_enum_complete_type_info (type, type_info, enode->get_enum_values());
            break;
          case BSE_EXPORT_NODE_RECORD:
          case BSE_EXPORT_NODE_SEQUENCE:
            /* nothing to do, since boxed types are static to the type system */
            break;
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
          default: ;
          }
        break;
      }
  if (!node || node->type != type)
    g_error ("%s: unable to complete type from plugin: %s", plugin->fname, g_type_name (type));
}

static void
bse_plugin_reinit_types (BsePlugin *plugin)
{
  guint n = plugin->n_types;
  GType *types = (GType*) g_memdup (plugin->types, sizeof (plugin->types[0]) * n);
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
                       plugin->fname, node->name);
          else if (node->ntype == BSE_EXPORT_NODE_ENUM)
            {
              BseExportNodeEnum *enode = (BseExportNodeEnum*) node;
              if (enode->get_choice_values)
                sfi_enum_type_set_choice_value_getter (type, (SfiChoiceValueGetter) enode->get_choice_values);
            }
          else if (node->ntype == BSE_EXPORT_NODE_RECORD ||
                   node->ntype == BSE_EXPORT_NODE_SEQUENCE)
            bse_type_reinit_boxed ((BseExportNodeBoxed*) node);
        }
    }
  while (n--)
    g_warning ("%s: plugin failed to reregister type: %s", plugin->fname, g_type_name (types[n]));
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
      case BSE_EXPORT_NODE_LINK:
        break;
      case BSE_EXPORT_NODE_CLASS:
        cnode = (BseExportNodeClass*) node;
        type = g_type_from_name (cnode->parent);
        if (!type)
          {
            g_message ("%s: plugin type %s derives from unknown parent type: %s",
                       plugin->fname, node->name, cnode->parent);
            return;
          }
        if (!BSE_TYPE_IS_OBJECT (type))
          {
            g_message ("%s: plugin object type %s derives from non-object type: %s",
                       plugin->fname, node->name, cnode->parent);
            return;
          }
      case BSE_EXPORT_NODE_HOOK:
      case BSE_EXPORT_NODE_ENUM:
      case BSE_EXPORT_NODE_RECORD:
      case BSE_EXPORT_NODE_SEQUENCE:
      case BSE_EXPORT_NODE_PROC:
        type = g_type_from_name (node->name);
        if (type)
          {
            g_message ("%s: plugin contains type already registered: %s",
                       plugin->fname, node->name);
            return;
          }
        break;
      default: ;
      }

  /* register BSE module types */
  for (node = plugin->chain; node && node->ntype; node = node->next)
    {
      GType type = 0;
      switch (node->ntype)
        {
          BseExportNodeClass *cnode;
          BseExportNodeEnum *enode;
          BseExportNodeHook *hnode;
          const gchar *error;
        case BSE_EXPORT_NODE_LINK:
          break;
        case BSE_EXPORT_NODE_HOOK:
          hnode = (BseExportNodeHook*) node;
          hnode->hook (hnode->data);
          if (hnode->make_static)
            plugin->use_count += 1;
          break;
        case BSE_EXPORT_NODE_ENUM:
          enode = (BseExportNodeEnum*) node;
          type = bse_type_register_dynamic (G_TYPE_ENUM, node->name, G_TYPE_PLUGIN (plugin));
          if (enode->get_choice_values)
            sfi_enum_type_set_choice_value_getter (type, (SfiChoiceValueGetter) enode->get_choice_values);
          g_value_register_transform_func (SFI_TYPE_CHOICE, type, sfi_value_choice2enum_simple);
          g_value_register_transform_func (type, SFI_TYPE_CHOICE, sfi_value_enum2choice);
          break;
        case BSE_EXPORT_NODE_RECORD:
        case BSE_EXPORT_NODE_SEQUENCE:
          type = bse_type_register_loadable_boxed ((BseExportNodeBoxed*) node, G_TYPE_PLUGIN (plugin));
          node->type = type;
          bse_type_reinit_boxed ((BseExportNodeBoxed*) node);
          break;
        case BSE_EXPORT_NODE_CLASS:
          cnode = (BseExportNodeClass*) node;
          type = bse_type_register_dynamic (g_type_from_name (cnode->parent),
                                            node->name, G_TYPE_PLUGIN (plugin));
          break;
        case BSE_EXPORT_NODE_PROC:
          error = bse_procedure_type_register (node->name, plugin, &type);
          if (error)
            g_message ("%s: while registering procedure \"%s\": %s",
                       plugin->fname, node->name, error);
          break;
        default:
          g_message ("%s: plugin contains invalid type node (%u)", plugin->fname, node->ntype);
          node = NULL;
          break;
        }
      if (type)
        {
          const char *i18n_category = NULL;
          guint n;
          if (node->options && node->options[0])
            bse_type_add_options (type, node->options);
          n = plugin->n_types++;
          plugin->types = g_renew (GType, plugin->types, plugin->n_types);
          plugin->types[n] = type;
          node->type = type;
          if (node->fill_strings)
            {
              BseExportStrings export_strings = { 0, };
              node->fill_strings (&export_strings);
              if (export_strings.blurb && export_strings.blurb[0])
                bse_type_add_blurb (type, export_strings.blurb, export_strings.file, export_strings.line);
              if (export_strings.authors && export_strings.authors[0])
                bse_type_add_authors (type, export_strings.authors);
              if (export_strings.license && export_strings.license[0])
                bse_type_add_license (type, export_strings.license);
              if (export_strings.i18n_category && export_strings.i18n_category[0])
                i18n_category = export_strings.i18n_category;
            }
          if (node->category)
            bse_categories_register (node->category, i18n_category, type, node->pixstream);
        }
    }
}

static inline BsePlugin*
bse_plugin_find (GModule *gmodule)
{
  for (GSList *slist = bse_plugins; slist; slist = slist->next)
    {
      BsePlugin *plugin = (BsePlugin*) slist->data;
      if (plugin->gmodule == gmodule)
	return plugin;
    }
  return NULL;
}

const gchar*
bse_plugin_check_load (const gchar *const_file_name)
{
  gchar *file_name;
  GModule *gmodule;
  gchar *error = NULL;
  const gchar *cerror = NULL;
  
  g_return_val_if_fail (const_file_name != NULL, NULL);

  if (0)        /* want to read .la files? */
    {
      const gint TOKEN_DLNAME = G_TOKEN_LAST + 1;
      GScanner *scanner;
      /* open libtool archive */
      gint fd = open (const_file_name, O_RDONLY, 0);
      if (fd < 0)
        return (errno == ENOENT || errno == ENOTDIR || errno == ELOOP ?
                bse_error_blurb (BSE_ERROR_FILE_NOT_FOUND) :
                "Unable to access plugin");
      
      /* and search libtool's dlname specification */
      scanner = g_scanner_new64 (NULL);
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
          gchar *string = g_path_get_dirname (const_file_name);
          file_name = g_strconcat (string, G_DIR_SEPARATOR_S, scanner->value.v_string, NULL);
          g_free (string);
        }
      g_scanner_destroy (scanner);
      close (fd);
    }
  else
    file_name = g_strdup (const_file_name);

  DEBUG ("register: %s", file_name);

  /* load module */
  BsePlugin *plugin = (BsePlugin*) g_object_new (BSE_TYPE_PLUGIN, NULL);
  plugin->fname = g_strdup (file_name);
  startup_plugin = plugin;
  gmodule = g_module_open (file_name, G_MODULE_BIND_LAZY);
  startup_plugin = NULL;
  if (!gmodule)
    {
      cerror = g_module_error ();
      DEBUG ("error: %s: %s", file_name, cerror);
      g_free (file_name);
      g_object_unref (plugin);
      return cerror;
    }
  if (bse_plugin_find (gmodule))
    {
      g_module_close (gmodule);
      cerror = "Plugin already loaded";
      DEBUG ("error: %s: %s", file_name, cerror);
      g_free (file_name);
      g_object_unref (plugin);
      return cerror;
    }

  /* verify plugin identity (BSE + version) */
  cerror = plugin_check_identity (plugin, gmodule);
  if (cerror)
    {
      g_module_close (gmodule);
      DEBUG ("error: %s: %s", file_name, cerror);
      g_free (file_name);
      g_object_unref (plugin);
      return cerror;
    }

  /* create plugin if this is a BSE plugin with valid type chain */
  if (plugin->chain)
    {
      plugin->fname = file_name;
      plugin->gmodule = gmodule;

      /* register BSE module types */
      bse_plugin_init_types (plugin);

      bse_plugins = g_slist_prepend (bse_plugins, plugin);
      if (plugin->use_count == 0)
        bse_plugin_unload (plugin);
    }
  else if (plugin->resident_types)
    {
      plugin->use_count += 1; // make plugin resident
      plugin->fname = file_name;
      plugin->gmodule = gmodule;
      bse_plugins = g_slist_prepend (bse_plugins, plugin);
    }
  else
    {
      g_module_close (gmodule);
      error = NULL; /* empty plugin */
      DEBUG ("plugin empty: %s", file_name);
      g_free (file_name);
      g_object_unref (plugin);
    }

  return error;
}

#include "topconfig.h"

static bool
plugin_extension_filter (const char  *fname,
                         guint        n,
                         const char **exts)
{
  const char *bname = strrchr (fname, G_DIR_SEPARATOR);
  if (!bname)
    bname = fname;
  const char *ext = strchr (bname, '.');
  if (!ext)
    ext = bname + strlen (bname);
  guint i;
  for (i = 0; i < n; i++)
    if (strcmp (ext, exts[i]) == 0)
      return true;
  return false;
}

#ifdef WIN32
#define PLUGIN_EXTENSION ".dll"
#else
#define PLUGIN_EXTENSION ".so"
#endif

SfiRing*
bse_plugin_path_list_files (gboolean include_drivers,
                            gboolean include_plugins)
{
  SfiRing *files, *ring = NULL;
  if (bse_main_args->override_plugin_globs)
    {
      /* expect filename globs */
      files = sfi_file_crawler_list_files (bse_main_args->override_plugin_globs, NULL, G_FILE_TEST_IS_REGULAR);
      ring = sfi_ring_concat (ring, sfi_ring_sort (files, (SfiCompareFunc) strcmp, NULL));
    }
  else
    {
      if (include_drivers)
        {
          files = sfi_file_crawler_list_files (BSE_PATH_DRIVERS, "*" PLUGIN_EXTENSION, G_FILE_TEST_IS_REGULAR);
          ring = sfi_ring_concat (ring, sfi_ring_sort (files, (SfiCompareFunc) strcmp, NULL));
          files = sfi_file_crawler_list_files (BSE_PATH_DRIVERS, "*.o", G_FILE_TEST_IS_REGULAR);
          ring = sfi_ring_concat (ring, sfi_ring_sort (files, (SfiCompareFunc) strcmp, NULL));
        }
      if (include_plugins)
        {
          files = sfi_file_crawler_list_files (BSE_PATH_PLUGINS, "*" PLUGIN_EXTENSION, G_FILE_TEST_IS_REGULAR);
          ring = sfi_ring_concat (ring, sfi_ring_sort (files, (SfiCompareFunc) strcmp, NULL));
          files = sfi_file_crawler_list_files (BSE_PATH_PLUGINS, "*.o", G_FILE_TEST_IS_REGULAR);
          ring = sfi_ring_concat (ring, sfi_ring_sort (files, (SfiCompareFunc) strcmp, NULL));
        }
      if (include_plugins && BSE_GCONFIG (plugin_path) && BSE_GCONFIG (plugin_path)[0])
        {
          files = sfi_file_crawler_list_files (BSE_GCONFIG (plugin_path), "*" PLUGIN_EXTENSION, G_FILE_TEST_IS_REGULAR);
          ring = sfi_ring_concat (ring, sfi_ring_sort (files, (SfiCompareFunc) strcmp, NULL));
          files = sfi_file_crawler_list_files (BSE_GCONFIG (plugin_path), "*.o", G_FILE_TEST_IS_REGULAR);
          ring = sfi_ring_concat (ring, sfi_ring_sort (files, (SfiCompareFunc) strcmp, NULL));
          /* allow file names in plugin_path */
          files = sfi_file_crawler_list_files (BSE_GCONFIG (plugin_path), NULL, G_FILE_TEST_IS_REGULAR);
          ring = sfi_ring_concat (ring, sfi_ring_sort (files, (SfiCompareFunc) strcmp, NULL));
        }
    }
  if (true)
    {
      const SfiCPUInfo cpu_info = sfi_cpu_info();
      const char *exts[] = { ".FPU" PLUGIN_EXTENSION, ".FPU.la", PLUGIN_EXTENSION, ".la", };
      if (BSE_WITH_SSE_FLAGS && !bse_main_args->force_fpu &&
          cpu_info.x86_mmx && cpu_info.x86_sse && cpu_info.x86_ssesys)
        {
          exts[0] = ".SSE" PLUGIN_EXTENSION;  /* !".FPU.so" / ".FPU.dll" */
          exts[1] = ".SSE.la";  /* !".FPU.la" */
        }
      SfiRing *fname;
      files = ring;
      ring = NULL;
      for (fname = files; fname; fname = sfi_ring_next (fname, files))
        {
          char *name = (char*) fname->data;
          bool match = plugin_extension_filter (name, G_N_ELEMENTS (exts), exts);
          DEBUG ("PluginExtensionFilter: %s: %s", name, match ? "(match)" : "(ignored)");
          if (match)
            ring = sfi_ring_append (ring, name);
          else
            g_free (name);
        }
      sfi_ring_free (files);
    }
  return ring;
}
