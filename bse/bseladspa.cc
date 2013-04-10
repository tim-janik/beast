// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseladspa.hh"
#include "bseladspamodule.hh"
#include "bsecategories.hh"
#include <birnet/birnet.hh>
#include <string.h>
#include "ladspa.hh"
using namespace Birnet;

#define LDEBUG(...)     BSE_KEY_DEBUG ("ladspa", __VA_ARGS__)

#define	LADSPA_TYPE_NAME	"BseLadspaModule_"

/* --- prototypes --- */
static void     ladspa_plugin_iface_init	(GTypePluginClass      *iface);
static void	ladspa_plugin_use		(GTypePlugin		*gplugin);
static void	ladspa_plugin_unuse		(GTypePlugin		*gplugin);
static void	ladspa_plugin_complete_info	(GTypePlugin		*gplugin,
						 GType			 type,
						 GTypeInfo		*type_info,
						 GTypeValueTable	*value_vtable);
static const gchar*	ladspa_plugin_reinit_type_ids (BseLadspaPlugin           *self,
						       LADSPA_Descriptor_Function ldf);
/* --- variables --- */
static GSList *ladspa_plugins = NULL;
/* --- functions --- */
BSE_BUILTIN_TYPE (BseLadspaPlugin)
{
  static const GTypeInfo type_info = {
    sizeof (BseLadspaPluginClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) NULL,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BseLadspaPlugin),
    0 /* n_preallocs */,
    (GInstanceInitFunc) NULL,
  };
  static const GInterfaceInfo iface_info = {
    (GInterfaceInitFunc) ladspa_plugin_iface_init,
    NULL,               /* interface_finalize */
    NULL,               /* interface_data */
  };
  GType type;
  type = bse_type_register_static (G_TYPE_OBJECT,
				   "BseLadspaPlugin",
				   "LADSPA Plugin Loader",
                                   __FILE__, __LINE__,
                                   &type_info);
  g_type_add_interface_static (type, G_TYPE_TYPE_PLUGIN, &iface_info);
  return type;
}
static void
ladspa_plugin_iface_init (GTypePluginClass *iface)
{
  iface->use_plugin = ladspa_plugin_use;
  iface->unuse_plugin = ladspa_plugin_unuse;
  iface->complete_type_info = ladspa_plugin_complete_info;
}
static void
ladspa_plugin_use (GTypePlugin *gplugin)
{
  BseLadspaPlugin *self = BSE_LADSPA_PLUGIN (gplugin);
  g_object_ref (self);
  if (!self->use_count)
    {
      BIRNET_MAY_ALIAS LADSPA_Descriptor_Function ldf = NULL;
      const gchar *error = NULL;
      self->use_count++;
      LDEBUG ("%s: reloading plugin", self->fname);
      self->gmodule = g_module_open (self->fname, G_MODULE_BIND_LOCAL); /* reopen non-lazy for actual use */
      if (!self->gmodule)
	error = g_module_error ();
      if (!error)
	{
	  if (!g_module_symbol (self->gmodule, "ladspa_descriptor", (void**) &ldf) || !ldf)
	    error = g_module_error ();
	}
      if (!error)
	{
	  if (ldf (self->n_types) != NULL || ldf (self->n_types - 1) == NULL)
	    error = "plugin types changed on disk";
	}
      if (!error)
	error = ladspa_plugin_reinit_type_ids (self, ldf);
      if (error)
	g_error ("Fatal: failed to reinitialize plugin \"%s\": %s", self->fname, error);
    }
  else
    self->use_count++;
}
static void
ladspa_plugin_unload (BseLadspaPlugin *self)
{
  guint i;
  g_return_if_fail (self->gmodule != NULL);
  g_module_close (self->gmodule);
  self->gmodule = NULL;
  for (i = 0; i < self->n_types; i++)
    if (self->types[i].info)
      {
        bse_ladspa_info_free (self->types[i].info);
        self->types[i].info = NULL;
      }
  LDEBUG ("%s: plugin unloaded", self->fname);
}
static void
ladspa_plugin_unuse (GTypePlugin *gplugin)
{
  BseLadspaPlugin *self = BSE_LADSPA_PLUGIN (gplugin);
  g_return_if_fail (self->use_count > 0);
  self->use_count--;
  if (!self->use_count)
    ladspa_plugin_unload (self);
  g_object_unref (self);
}
static void
ladspa_plugin_complete_info (GTypePlugin	*gplugin,
			     GType		 type,
			     GTypeInfo		*type_info,
			     GTypeValueTable	*value_vtable)
{
  BseLadspaPlugin *self = BSE_LADSPA_PLUGIN (gplugin);
  guint j;
  for (j = 0; j < self->n_types; j++)
    if (self->types[j].type == type)
      {
	bse_ladspa_module_derived_type_info (type, self->types[j].info, type_info);
	break;
      }
}
#define to_upper(c)	((c) >='a' && (c) <='z' ? (c) - 'a' + 'A' : (c))
#define is_alnum(c)	(((c) >='A' && (c) <='Z') || ((c) >='a' && (c) <='z') || ((c) >='0' && (c) <='9'))
static inline gint
strcmp_alnum (const gchar *s1,
              const gchar *s2)
{
  while (*s1 && *s2)
    {
      if (is_alnum (*s1) && *s1 != *s2)
	break;
      s1++;
      s2++;
    }
  return *s1 - *s2;
}
static const gchar*
ladspa_plugin_reinit_type_ids (BseLadspaPlugin           *self,
			       LADSPA_Descriptor_Function ldf)
{
  guint j;
  for (j = 0; j < self->n_types; j++)
    {
      const gchar *label;
      const LADSPA_Descriptor *cld;
      if (!self->types[j].type)
	continue;
      label = g_type_name (self->types[j].type) + strlen (LADSPA_TYPE_NAME);
      /* we could try searching the ldf() indices for our type here,
       * however since ladspa_plugin_init_type_ids() creates a type entry
       * for each (even broken types) and the plugin must not change on
       * disk, our type index really should match the ldf() index.
       */
      cld = ldf (j);
      if (!cld || !cld->Label || strcmp_alnum (cld->Label, label) != 0)
        return "plugin type missing";
      self->types[j].info = bse_ladspa_info_assemble (self->fname, cld);
      if (self->types[j].info->broken)
	return "plugin type broke upon reload";
    }
  return NULL;
}
static const gchar*
ladspa_plugin_init_type_ids (BseLadspaPlugin           *self,
			     LADSPA_Descriptor_Function ldf)
{
  gchar *prefix = NULL, *error = NULL;
  guint i;
  /* check for multi module plugins */
  if (ldf (0) && ldf (1))
    {
      guint k, was_char = FALSE;
      prefix = strrchr (self->fname, '/');
      prefix = prefix ? g_strdup (prefix + 1) : g_strdup (self->fname);
      for (k = 0; prefix[k]; k++)
	if (prefix[k] == '_')
	  prefix[k] = ' ';
	else if (is_alnum (prefix[k]))
	  {
	    if (!was_char)
	      prefix[k] = to_upper (prefix[k]);
	    was_char = TRUE;
	  }
	else
	  was_char = FALSE;
    }
  for (i = 0; ; i++)
    {
      const LADSPA_Descriptor *cld = ldf (i);
      guint j;
      if (!cld)
	break;
      j = self->n_types++;
      self->types = (BseLadspaTypeInfo*) g_realloc (self->types, self->n_types * sizeof (self->types[0]));
      self->types[j].type = 0;
      self->types[j].info = bse_ladspa_info_assemble (self->fname, cld);
      if (!self->types[j].info->broken)
	{
	  gchar *string, *name;
	  guint k;
	  name = g_strconcat (LADSPA_TYPE_NAME, cld->Label, NULL);
	  for (k = 0; name[k]; k++)
	    if (!is_alnum (name[k]))
	      name[k] = '_';
          LDEBUG ("%s: registering plugin named: %s", self->fname, name);
	  if (g_type_from_name (name) != 0)
	    {
	      bse_ladspa_info_free (self->types[j].info);
	      self->types[j].info = NULL;
              LDEBUG ("%s: ignoring duplicate plugin type: %s",  self->fname, name);
	      g_free (name);
	      continue;
	    }
	  self->types[j].type = bse_type_register_dynamic (BSE_TYPE_LADSPA_MODULE, name,
                                                           G_TYPE_PLUGIN (self));
	  g_free (name);
	  string = g_strdup (self->types[j].info->name);
	  for (k = 0; string[k]; k++)
	    if (string[k] == '_')
	      string[k] = '-';
	    else if (string[k] == '/')
	      string[k] = '|';
	  name = g_strconcat ("/Modules/LADSPA/",
			      prefix ? prefix : "",
			      prefix ? "/" : "",
			      string, NULL);
	  g_free (string);
	  bse_categories_register (name, NULL, self->types[j].type, NULL);
	  g_free (name);
	}
      else
	{
	  bse_ladspa_info_free (self->types[j].info);
	  self->types[j].info = NULL;
	}
    }
  g_free (prefix);
  return error;
}
typedef struct {
  guint index;
  guint audio_input;
  guint audio_output;
  guint control_input;
  guint control_output;
} PortCounter;
static gboolean
bse_ladspa_info_add_port (BseLadspaInfo              *bli,
			  const gchar                *port_name,
			  guint                       port_flags,
			  const LADSPA_PortRangeHint *port_range,
			  guint			     *n_ports_p,
			  BseLadspaPort		    **ports_p,
			  PortCounter		     *pcounter)
{
  gboolean is_input = (port_flags & LADSPA_PORT_INPUT) != 0;
  gboolean is_output = (port_flags & LADSPA_PORT_OUTPUT) != 0;
  BseLadspaPort *port;
  guint i;
  if (!is_input && !is_output)
    {
      LDEBUG ("%s: ignoring port '%s' which is neither input nor output", bli->ident, port_name);
      return FALSE;
    }
  i = (*n_ports_p)++;
  (*ports_p) = g_renew (BseLadspaPort, (*ports_p), *n_ports_p);
  port = (*ports_p) + i;
  memset (port, 0, sizeof (*port));
  port->name = port_name;
  port->port_index = pcounter->index;
  port->audio_channel = (port_flags & LADSPA_PORT_AUDIO) != 0;
  port->input = is_input;
  port->output = is_output;
  if (port->audio_channel && port->input)
    port->ident = g_strdup_printf ("audio-in-%u", pcounter->audio_input++);
  else if (port->audio_channel) /* port->output */
    port->ident = g_strdup_printf ("audio-out-%u", pcounter->audio_output++);
  else if (port->input) /* !port->audio_channel */
    port->ident = g_strdup_printf ("icontrol-%u", pcounter->control_input++);
  else /* port->output && !port->audio_channel */
    port->ident = g_strdup_printf ("ocontrol-%u", pcounter->control_output++);
  port->minimum = G_MINFLOAT;
  port->default_value = 0;
  port->maximum = G_MAXFLOAT;
  if (port_range)
    {
      guint hints = port_range->HintDescriptor;
      if (hints & LADSPA_HINT_BOUNDED_BELOW)
	port->minimum = port_range->LowerBound;
      if (hints & LADSPA_HINT_BOUNDED_ABOVE)
	port->maximum = port_range->UpperBound;
      port->logarithmic = (hints & LADSPA_HINT_LOGARITHMIC) != 0;
      if (hints & LADSPA_HINT_SAMPLE_RATE)
	{
	  port->rate_relative = TRUE;
	  port->minimum = MAX (port->minimum, 0);
	}
      if (hints & LADSPA_HINT_INTEGER)
	{
	  port->integer_stepping = TRUE;
	  port->minimum = MAX (port->minimum, G_MININT);
	  port->maximum = MIN (port->maximum, G_MAXINT);
	}
      if (hints & LADSPA_HINT_TOGGLED)
	{
	  port->boolean = TRUE;
	  port->minimum = 0;
	  port->maximum = 1;
	}
      port->maximum = MAX (port->minimum, port->maximum);
      switch (hints & LADSPA_HINT_DEFAULT_MASK)
	{
	case LADSPA_HINT_DEFAULT_MINIMUM:
	  port->default_value = port->minimum;
	  break;
	case LADSPA_HINT_DEFAULT_MAXIMUM:
	  port->default_value = port->maximum;
	  break;
	case LADSPA_HINT_DEFAULT_0:
	  port->default_value = 0;
	  break;
	case LADSPA_HINT_DEFAULT_1:
	  port->default_value = 1;
	  break;
	case LADSPA_HINT_DEFAULT_100:
	  port->default_value = 100;
	  break;
	case LADSPA_HINT_DEFAULT_LOW:
	  if (port->logarithmic)
	    port->default_value = exp (log (port->minimum) * 0.75 +
				       log (port->maximum) * 0.25);
	  else
	    port->default_value = port->minimum * 0.75 + port->maximum * 0.25;
	  break;
	case LADSPA_HINT_DEFAULT_440:
	  port->concert_a = TRUE;
	  /* fall through to standard default-value picking */
	default:
	case 0: /* LADSPA_HINT_DEFAULT_NONE */
	  if (!(hints & LADSPA_HINT_BOUNDED_BELOW) ||
	      !(hints & LADSPA_HINT_BOUNDED_ABOVE))
	    break;
	  /* fall through to default-middle behaviour */
	case LADSPA_HINT_DEFAULT_MIDDLE:
	  if (port->logarithmic)
	    port->default_value = exp (log (port->minimum) * 0.5 +
				       log (port->maximum) * 0.5);
	  else
	    port->default_value = port->minimum * 0.5 + port->maximum * 0.5;
	  break;
	case LADSPA_HINT_DEFAULT_HIGH:
	  if (port->logarithmic)
	    port->default_value = exp (log (port->minimum) * 0.25 +
				       log (port->maximum) * 0.75);
	  else
	    port->default_value = port->minimum * 0.25 + port->maximum * 0.75;
	  break;
	}
      port->default_value = CLAMP (port->default_value, port->minimum, port->maximum);
      if (!port->boolean && !port->integer_stepping)
	{
	  /* interpretation heuristic */
	  if (port->minimum >= 0 && port->minimum <= 220 &&
	      port->maximum >= 1760 && port->maximum <= 24000 &&
	      port->logarithmic)
	    port->frequency = TRUE;
	  else if (port->rate_relative)
	    port->frequency = TRUE;
	}
    }
  return TRUE;
}
extern "C" gchar*
bse_ladspa_info_port_2str (BseLadspaPort *port)
{
  gchar flags[64];
  flags[0] = 0;
  if (port->input)
    strcat (flags, "w");
  if (port->output)
    strcat (flags, "r");
  if (port->boolean)
    strcat (flags, "b");
  if (port->integer_stepping)
    strcat (flags, "i");
  if (port->rate_relative)
    strcat (flags, "s");
  if (port->frequency)
    strcat (flags, "F");
  if (port->logarithmic)
    strcat (flags, "L");
  if (port->concert_a)
    strcat (flags, "A");
  return g_strdup_printf ("( %s, %f<=%f<=%f, %s )",
			  port->ident,
			  port->minimum, port->default_value, port->maximum,
			  flags);
}
extern "C" BseLadspaInfo*
bse_ladspa_info_assemble (const gchar  *file_path,
			  gconstpointer ladspa_descriptor)
{
  const LADSPA_Descriptor *cld = static_cast<const LADSPA_Descriptor*> (ladspa_descriptor);
  BseLadspaInfo *bli = g_new0 (BseLadspaInfo, 1);
  PortCounter pcounter = { 0, 1, 1, 1, 1 };
  bool seen_control_output = false, seen_audio_output = false;
  g_return_val_if_fail (cld != NULL, NULL);
  bli->file_path = g_strdup (file_path);
  if (!file_path)
    file_path = "";	/* ensure !=NULL for messages below */
  bli->plugin_id = cld->UniqueID;
  if (bli->plugin_id < 1 || bli->plugin_id >= 0x1000000)
    LDEBUG ("%s: plugin with suspicious ID: %u", file_path, bli->plugin_id);
  if (!cld->Label)
    {
      LDEBUG ("%s: ignoring plugin with NULL label", file_path);
      goto bail_broken;
    }
  else
    bli->ident = g_strdup_printf ("%s#%s", file_path, cld->Label);
  bli->name = cld->Name ? cld->Name : bli->ident;
  if (!cld->Maker)
    LDEBUG ("%s: plugin with 'Maker' field of NULL", bli->ident);
  bli->author = cld->Maker ? cld->Maker : "";
  if (!cld->Copyright || g_ascii_strcasecmp (cld->Copyright, "none") == 0)
    bli->copyright = "";
  else
    bli->copyright = cld->Copyright;
  bli->interactive = (cld->Properties & LADSPA_PROPERTY_REALTIME) != 0;
  bli->rt_capable = (cld->Properties & LADSPA_PROPERTY_HARD_RT_CAPABLE) != 0;
  if (!cld->PortCount)
    {
      LDEBUG ("%s: ignoring plugin without ports", bli->ident);
      goto bail_broken;
    }
  if (!cld->PortDescriptors)
    {
      LDEBUG ("%s: ignoring plugin without port descriptors", bli->ident);
      goto bail_broken;
    }
  if (!cld->PortNames)
    {
      LDEBUG ("%s: ignoring plugin without port names", bli->ident);
      goto bail_broken;
    }
  if (!cld->PortRangeHints)
    LDEBUG ("%s: port range hint array is NULL", bli->ident);
  for (pcounter.index = 0; pcounter.index < cld->PortCount; pcounter.index++)
    {
      const LADSPA_PortRangeHint *port_range = cld->PortRangeHints ? cld->PortRangeHints + pcounter.index : NULL;
      const gchar *port_name = cld->PortNames[pcounter.index];
      guint port_flags = cld->PortDescriptors[pcounter.index];
      if (!port_name)
	{
	  LDEBUG ("%s: ignoring plugin without port %u name", bli->ident, pcounter.index);
	  goto bail_broken;
	}
      switch (port_flags & (LADSPA_PORT_CONTROL | LADSPA_PORT_AUDIO))
	{
	case LADSPA_PORT_CONTROL:
	  if (!bse_ladspa_info_add_port (bli, port_name, port_flags, port_range,
					 &bli->n_cports, &bli->cports, &pcounter))
	    goto bail_broken;
          seen_control_output |= bli->cports[bli->n_cports - 1].output;
	  break;
	case LADSPA_PORT_AUDIO:
	  if (!bse_ladspa_info_add_port (bli, port_name, port_flags, port_range,
					 &bli->n_aports, &bli->aports, &pcounter))
	    goto bail_broken;
	  seen_audio_output |= bli->aports[bli->n_aports - 1].output;
	  break;
	case LADSPA_PORT_CONTROL | LADSPA_PORT_AUDIO:
	  LDEBUG ("%s: ignoriong plugin with port %u type which claims to be 'control' and 'audio'", bli->ident, pcounter.index);
	  goto bail_broken;
	default:
	case 0:
	  LDEBUG ("%s: ignoring plugin with port %u type which is neither 'control' nor 'audio'", bli->ident, pcounter.index);
	  goto bail_broken;
	}
    }
  if (!seen_audio_output)
    {
      LDEBUG ("%s: ignoring plugin without audio output channels", bli->ident);
      goto bail_broken;
    }
  if (!cld->instantiate)
    {
      LDEBUG ("%s: ignoring plugin without instantiate() function", bli->ident);
      goto bail_broken;
    }
  bli->descdata = cld;
  bli->instantiate = (void* (*) (void const*, gulong)) cld->instantiate;
  if (!cld->connect_port)
    {
      LDEBUG ("%s: ignoring plugin without connect_port() function", bli->ident);
      goto bail_broken;
    }
  bli->connect_port = cld->connect_port;
  if (!cld->run)
    {
      LDEBUG ("%s: ignoring plugin without run() function", bli->ident);
      goto bail_broken;
    }
  bli->run = cld->run;
  if (cld->run_adding && !cld->set_run_adding_gain)
    LDEBUG ("%s: plugin lacks set_run_adding_gain() function allthough run_adding() is provided", bli->ident);
  if (!cld->cleanup)
    {
      LDEBUG ("%s: ignoring plugin without cleanup() function", bli->ident);
      goto bail_broken;
    }
  bli->cleanup = cld->cleanup;
  bli->activate = cld->activate;
  bli->deactivate = cld->deactivate;
  return bli;
 bail_broken:
  bli->broken = TRUE;
  return bli;
}
extern "C" void
bse_ladspa_info_free (BseLadspaInfo *bli)
{
  guint i;
  g_return_if_fail (bli != NULL);
  for (i = 0; i < bli->n_cports; i++)
    {
      BseLadspaPort *port = bli->cports + i;
      g_free (port->ident);
    }
  g_free (bli->cports);
  for (i = 0; i < bli->n_aports; i++)
    {
      BseLadspaPort *port = bli->aports + i;
      g_free (port->ident);
    }
  g_free (bli->aports);
  g_free (bli->ident);
  g_free (bli->file_path);
  g_free (bli);
}
static BseLadspaPlugin*
ladspa_plugin_find (const gchar *fname)
{
  GSList *slist;
  for (slist = ladspa_plugins; slist; slist = slist->next)
    {
      BseLadspaPlugin *plugin = (BseLadspaPlugin*) slist->data;
      if (strcmp (plugin->fname, fname) == 0)
	return plugin;
    }
  return NULL;
}
extern "C" const gchar*
bse_ladspa_plugin_check_load (const gchar *file_name)
{
  BseLadspaPlugin *self;
  const gchar *error;
  GModule *gmodule;
  g_return_val_if_fail (file_name != NULL, "Internal Error");
  if (ladspa_plugin_find (file_name))
    return "Plugin already registered";
  /* load module once */
  gmodule = g_module_open (file_name, GModuleFlags (G_MODULE_BIND_LOCAL | G_MODULE_BIND_LAZY));
  if (!gmodule)
    return g_module_error ();
  /* check whether this is a LADSPA module */
  BIRNET_MAY_ALIAS LADSPA_Descriptor_Function ldf = NULL;
  if (!g_module_symbol (gmodule, "ladspa_descriptor", (void**) &ldf) || !ldf)
    {
      g_module_close (gmodule);
      return "Plugin without ladspa_descriptor";
    }
  /* create plugin and register types */
  self = (BseLadspaPlugin*) g_object_new (BSE_TYPE_LADSPA_PLUGIN, NULL);
  self->fname = g_strdup (file_name);
  self->gmodule = gmodule;
  error = ladspa_plugin_init_type_ids (self, ldf);
  /* keep plugin if types were successfully registered */
  ladspa_plugin_unload (self);
  if (self->n_types)
    {
      ladspa_plugins = g_slist_prepend (ladspa_plugins, self);
      g_object_ref (self);
    }
  else
    g_object_unref (self);
  return error;
}
#include "topconfig.h"
extern "C" SfiRing*
bse_ladspa_plugin_path_list_files (void)
{
  SfiRing *ring1, *ring2 = NULL, *ring3 = NULL;
  const gchar *paths;
  ring1 = sfi_file_crawler_list_files (BSE_PATH_LADSPA, "*.so", GFileTest (0));
  ring1 = sfi_ring_sort (ring1, (SfiCompareFunc) strcmp, NULL);
  paths = g_getenv ("LADSPA_PATH");
  if (paths && paths[0])
    ring2 = sfi_file_crawler_list_files (paths, "*.so", GFileTest (0));
  ring2 = sfi_ring_sort (ring2, (SfiCompareFunc) strcmp, NULL);
  paths = BSE_GCONFIG (ladspa_path);
  if (paths && paths[0])
    ring3 = sfi_file_crawler_list_files (paths, "*.so", GFileTest (0));
  ring3 = sfi_ring_sort (ring3, (SfiCompareFunc) strcmp, NULL);
  ring2 = sfi_ring_concat (ring2, ring3);
  return sfi_ring_concat (ring1, ring2);
}
#if 0
static void
ladspa_test_load (const gchar *file)
{
  LADSPA_Descriptor_Function ldf = NULL;
  const gchar *error;
  GModule *gmodule;
  gmodule = g_module_open (file, 0);
  error = g_module_error ();
  if (!error && gmodule)
    {
      if (!g_module_symbol (gmodule, "ladspa_descriptor", (gpointer) &ldf) || !ldf)
	error = g_module_error ();
    }
  if (!error && ldf)
    {
      guint i;
      const gchar *lfile = strrchr (file, '/');
      lfile = lfile ? lfile + 1 : file;
      for (i = 0; ; i++)
	{
	  const LADSPA_Descriptor *cld = ldf (i);
	  BseLadspaInfo *bli;
	  if (!cld)
	    break;
	  bli = bse_ladspa_info_assemble (file, cld);
	  if (!bli->broken)
	    g_print ("LADSPA: found %s\n", bli->ident);
	}
      if (i == 0)
	error = "missing LADSPA descriptor";
    }
  if (error)
    g_message ("failed to load LADSPA plugin \"%s\": %s", file, error);
  if (gmodule)
    g_module_close (gmodule);
}
#endif
