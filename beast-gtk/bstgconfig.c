/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999-2002 Tim Janik and Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include	"bstgconfig.h"


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_SNET_ANTI_ALIASED,
  PARAM_SNET_EDIT_FALLBACK,
  PARAM_SNET_SWAP_IO_CHANNELS,
  PARAM_XKB_FORCE_QUERY,
  PARAM_XKB_SYMBOL,
  PARAM_DISABLE_ALSA,
  PARAM_TAB_WIDTH,
  PARAM_SAMPLE_SWEEP,
  PARAM_PE_KEY_FOCUS_UNSELECTS,
  PARAM_RC_VERSION
};


/* --- prototypes --- */
static void	 bst_gconfig_init		(BstGConfig	  *gconf);
static void	 bst_gconfig_class_init		(BstGConfigClass  *class);
static void	 bst_gconfig_class_finalize	(BstGConfigClass  *class);
static void      bst_gconfig_set_property          (BstGConfig	  *gconf,
						 guint             param_id,
						 GValue           *value,
						 GParamSpec       *pspec,
						 const gchar      *trailer);
static void      bst_gconfig_get_property          (BstGConfig	  *gconf,
						 guint             param_id,
						 GValue           *value,
						 GParamSpec       *pspec,
						 const gchar      *trailer);
static void	 bst_gconfig_do_finalize	(GObject     	  *object);
static void	 bst_gconfig_do_apply		(BseGConfig	  *bconf);
static void	 bst_gconfig_do_revert		(BseGConfig	  *bconf);
static void      bst_globals_copy               (const BstGlobals *globals_src,
						 BstGlobals       *globals);
static void      bst_globals_unset              (BstGlobals       *globals);



/* --- variables --- */
GType                    bst_type_id_BstGConfig = 0;
static gpointer          parent_class = NULL;
static BstGlobals        bst_globals_current = { 0, };
const BstGlobals * const bst_globals = &bst_globals_current;
static const BstGlobals  bst_globals_defaults = {
  NULL			/* xkb_symbol */,
  FALSE			/* xkb_force_query */,
  TRUE			/* snet_anti_aliased */,
  TRUE			/* snet_edit_fallback */,
  FALSE			/* snet_swap_io_channels */,
  FALSE			/* disable_alsa */,
  TRUE			/* sample_sweep */,
  FALSE			/* pe_key_focus_unselects */,
  0			/* tab_width */,
  0			/* rc_version */,
};


/* --- functions --- */
void
bst_globals_init (void)
{
  static const GTypeInfo gconfig_info = {
    sizeof (BstGConfigClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bst_gconfig_class_init,
    (GClassFinalizeFunc) bst_gconfig_class_finalize,
    NULL /* class_data */,

    sizeof (BstGConfig),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bst_gconfig_init,
  };

  if (bst_type_id_BstGConfig)
    return;

  bst_type_id_BstGConfig = bse_type_register_static (BSE_TYPE_GCONFIG,
						     "BstGConfig",
						     "BEAST global configuration object",
						     &gconfig_info);
  bst_globals_copy (&bst_globals_defaults, &bst_globals_current);
}

static void
bst_gconfig_class_finalize (BstGConfigClass *class)
{
}

static void
bst_gconfig_init (BstGConfig *gconf)
{
  bst_globals_copy (NULL, &gconf->globals);
}

static void
bst_gconfig_do_finalize (GObject *object)
{
  BstGConfig *gconf = BST_GCONFIG (object);

  bst_globals_unset (&gconf->globals);
  
  /* chain parent class' finalize handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bst_gconfig_class_init (BstGConfigClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseGConfigClass *bconfig_class = BSE_GCONFIG_CLASS (class);
  BstGlobals globals_defaults = { 0, };
  
  parent_class = g_type_class_peek (BSE_TYPE_GCONFIG);
  
  G_OBJECT_CLASS (class)->finalize = bst_gconfig_do_finalize;

  gobject_class->set_property = (GObjectSetPropertyFunc) bst_gconfig_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bst_gconfig_get_property;
  
  bconfig_class->apply = bst_gconfig_do_apply;
  bconfig_class->revert = bst_gconfig_do_revert;
  
  bst_globals_copy (NULL, &globals_defaults);
  bse_object_class_add_param (object_class, "Keyboard Layout",
			      PARAM_XKB_FORCE_QUERY,
			      bse_param_spec_bool ("xkb_force_query", "Always query X server on startup", NULL,
						 globals_defaults.xkb_force_query,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Keyboard Layout",
			      PARAM_XKB_SYMBOL,
			      bse_param_spec_string ("xkb_symbol", "Keyboard Layout", NULL,
						   globals_defaults.xkb_symbol,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Samples",
			      PARAM_SAMPLE_SWEEP,
			      bse_param_spec_bool ("sample_sweep", "Auto sweep",
						 "Automatically remove (sweep) unused samples of a project",
						 globals_defaults.sample_sweep,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Synthesis Networks",
			      PARAM_SNET_ANTI_ALIASED,
			      bse_param_spec_bool ("snet_anti_aliased", "Anti aliased display", NULL,
						 globals_defaults.snet_anti_aliased,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Synthesis Networks",
			      PARAM_SNET_EDIT_FALLBACK,
			      bse_param_spec_bool ("snet_edit_fallback", "Auto fallback into Edit mode",
						 "Fallback into Edit mode after a new source has been added",
						 globals_defaults.snet_edit_fallback,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Synthesis Networks",
			      PARAM_SNET_SWAP_IO_CHANNELS,
			      bse_param_spec_bool ("snet_swap_io_channels", "Swap input/output channels", NULL,
						 globals_defaults.snet_swap_io_channels,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Pattern Editor",
			      PARAM_PE_KEY_FOCUS_UNSELECTS,
			      bse_param_spec_bool ("pe_key_focus_unselects", "Focus moves reset selection",
						 "Reset the pattern editor's selection when keyboard moves"
						 "the focus",
						 globals_defaults.pe_key_focus_unselects,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Geometry",
			      PARAM_TAB_WIDTH,
			      bse_param_spec_uint ("tab_width", "Project tabulator width",
						 "This is the width of the project notebook's "
						 "tabulators that show the song, network or sample names. "
						 "Setting it to a fixed width avoids window resizing when "
						 "samples are added or removed.",
						 0, 1024, globals_defaults.tab_width, 5,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Debugging",
			      PARAM_DISABLE_ALSA,
			      bse_param_spec_bool ("disable_alsa", "Disable support for ALSA PCM driver", NULL,
						 globals_defaults.disable_alsa,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Internal",
			      PARAM_RC_VERSION,
			      bse_param_spec_uint ("rc_version", "RC Version", NULL,
						   0, G_MAXUINT, globals_defaults.rc_version, 1,
						   BSE_PARAM_STORAGE));
  bst_globals_unset (&globals_defaults);
}

static void
bst_gconfig_set_property (BstGConfig  *gconf,
                       guint        param_id,
		       GValue      *value,
		       GParamSpec  *pspec,
		       const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_SNET_ANTI_ALIASED:
      gconf->globals.snet_anti_aliased = g_value_get_boolean (value);
      break;
    case PARAM_SNET_EDIT_FALLBACK:
      gconf->globals.snet_edit_fallback = g_value_get_boolean (value);
      break;
    case PARAM_SNET_SWAP_IO_CHANNELS:
      gconf->globals.snet_swap_io_channels = g_value_get_boolean (value);
      break;
    case PARAM_XKB_FORCE_QUERY:
      gconf->globals.xkb_force_query = g_value_get_boolean (value);
      break;
    case PARAM_XKB_SYMBOL:
      g_free (gconf->globals.xkb_symbol);
      gconf->globals.xkb_symbol = bse_strdup_stripped (g_value_get_string (value));
      break;
    case PARAM_DISABLE_ALSA:
      gconf->globals.disable_alsa = g_value_get_boolean (value);
      break;
    case PARAM_TAB_WIDTH:
      gconf->globals.tab_width = g_value_get_uint (value);
      break;
    case PARAM_SAMPLE_SWEEP:
      gconf->globals.sample_sweep = g_value_get_boolean (value);
      break;
    case PARAM_PE_KEY_FOCUS_UNSELECTS:
      gconf->globals.pe_key_focus_unselects = g_value_get_boolean (value);
      break;
    case PARAM_RC_VERSION:
      gconf->globals.rc_version = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gconf, param_id, pspec);
      break;
    }
}

static void
bst_gconfig_get_property (BstGConfig  *gconf,
                       guint        param_id,
		       GValue      *value,
		       GParamSpec  *pspec,
		       const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_SNET_ANTI_ALIASED:
      g_value_set_boolean (value, gconf->globals.snet_anti_aliased);
      break;
    case PARAM_SNET_EDIT_FALLBACK:
      g_value_set_boolean (value, gconf->globals.snet_edit_fallback);
      break;
    case PARAM_SNET_SWAP_IO_CHANNELS:
      g_value_set_boolean (value, gconf->globals.snet_swap_io_channels);
      break;
    case PARAM_XKB_FORCE_QUERY:
      g_value_set_boolean (value, gconf->globals.xkb_force_query);
      break;
    case PARAM_XKB_SYMBOL:
      g_value_set_string (value, gconf->globals.xkb_symbol);
      break;
    case PARAM_DISABLE_ALSA:
      g_value_set_boolean (value, gconf->globals.disable_alsa);
      break;
    case PARAM_TAB_WIDTH:
      g_value_set_uint (value, gconf->globals.tab_width);
      break;
    case PARAM_SAMPLE_SWEEP:
      g_value_set_boolean (value, gconf->globals.sample_sweep);
      break;
    case PARAM_PE_KEY_FOCUS_UNSELECTS:
      g_value_set_boolean (value, gconf->globals.pe_key_focus_unselects);
      break;
    case PARAM_RC_VERSION:
      g_value_set_uint (value, gconf->globals.rc_version);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gconf, param_id, pspec);
      break;
    }
}

static void
bst_gconfig_do_apply (BseGConfig *bconf)
{
  BstGConfig *gconf = BST_GCONFIG (bconf);
  
  bst_globals_copy (&gconf->globals, NULL);

  /* chain parent class' handler */
  BSE_GCONFIG_CLASS (parent_class)->apply (bconf);
}

static void
bst_gconfig_do_revert (BseGConfig *bconf)
{
  BstGConfig *gconf = BST_GCONFIG (bconf);

  bst_globals_unset (&gconf->globals);
  bst_globals_copy (bst_globals, &gconf->globals);

  /* chain parent class' handler */
  BSE_GCONFIG_CLASS (parent_class)->revert (bconf);
}

void
bst_globals_copy (const BstGlobals *globals_src,
		  BstGlobals       *globals)
{
  if (!globals_src)
    globals_src = &bst_globals_defaults;
  if (!globals)
    {
      g_return_if_fail (bse_globals_locked () == FALSE);

      bst_globals_unset (&bst_globals_current);
      globals = &bst_globals_current;
    }

  *globals = *globals_src;
  globals->xkb_symbol = g_strdup (globals_src->xkb_symbol);
}

void
bst_globals_unset (BstGlobals *globals)
{
  g_return_if_fail (globals != NULL);

  g_free (globals->xkb_symbol);
  memset (globals, 0, sizeof (*globals));
}

void
bst_globals_set_xkb_symbol (const gchar *xkb_symbol)
{
  g_return_if_fail (bse_globals_locked () == FALSE);

  g_free (bst_globals_current.xkb_symbol);
  bst_globals_current.xkb_symbol = g_strdup (xkb_symbol);
}

void
bst_globals_set_rc_version (guint rc_version)
{
  g_return_if_fail (bse_globals_locked () == FALSE);

  bst_globals_current.rc_version = rc_version;
}
