/* DavXtalStrings - DAV Physical Modelling String Synthesizer
 * Copyright (c) 2000 David A. Bartold, 2001 Tim Janik
 *
 * Generate a string pluck sound using a modified Karplus-Strong algorithm
 * and then use Brensenham's algorithm to correct the frequency.
 *
 * This software uses technology under patent US 4,649,783, which is
 * set to expire in May of 2004.  In the meantime, a non-patented
 * alternative to this module is in the works.
 *
 ***********************************************************************
 *
 * Any commercial use of this module requires a license from
 * Stanford University.
 *
 ***********************************************************************
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "davxtalstrings.h"

#include <bse/gslengine.h>


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_BASE_FREQ,
  PARAM_BASE_NOTE,
  PARAM_TRIGGER_HIT,
  PARAM_TRIGGER_VEL,
  PARAM_NOTE_DECAY,
  PARAM_TENSION_DECAY,
  PARAM_METALLIC_FACTOR,
  PARAM_SNAP_FACTOR
};


/* --- prototypes --- */
static void	   dav_xtal_strings_init	     (DavXtalStrings	   *strings);
static void	   dav_xtal_strings_class_init	     (DavXtalStringsClass  *class);
static void	   dav_xtal_strings_class_finalize   (DavXtalStringsClass  *class);
static void	   dav_xtal_strings_set_property     (DavXtalStrings	   *strings,
						      guint                 param_id,
						      GValue               *value,
						      GParamSpec           *pspec);
static void	   dav_xtal_strings_get_property     (DavXtalStrings	   *strings,
						      guint                 param_id,
						      GValue               *value,
						      GParamSpec           *pspec);
static void	   dav_xtal_strings_context_create   (BseSource		   *source,
						      guint		    context_handle,
						      GslTrans		   *trans);
static void	   dav_xtal_strings_trigger	     (DavXtalStrings	   *strings);


/* --- variables --- */
static GType	       type_id_xtal_strings = 0;
static gpointer	       parent_class = NULL;
static const GTypeInfo type_info_xtal_strings = {
  sizeof (DavXtalStringsClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) dav_xtal_strings_class_init,
  (GClassFinalizeFunc) dav_xtal_strings_class_finalize,
  NULL /* class_data */,
  
  sizeof (DavXtalStrings),
  0 /* n_preallocs */,
  (GInstanceInitFunc) dav_xtal_strings_init,
};


/* --- functions --- */
static void
dav_xtal_strings_class_init (DavXtalStringsClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) dav_xtal_strings_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) dav_xtal_strings_get_property;
  
  source_class->context_create = dav_xtal_strings_context_create;
  
  bse_object_class_add_param (object_class, "Base Frequency", PARAM_BASE_FREQ,
			      bse_param_spec_float ("base_freq", "Frequency", NULL,
						    27.5, 4000.0,
						    BSE_KAMMER_FREQ_f, 10.0,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  
  bse_object_class_add_param (object_class, "Base Frequency",
			      PARAM_BASE_NOTE,
			      bse_param_spec_note ("base_note", "Note", NULL,
						   BSE_NOTE_A(-3), BSE_NOTE_B (3),
						   BSE_NOTE_VOID, 1, FALSE,
						   BSE_PARAM_GUI));
  
  bse_object_class_add_param (object_class, "Trigger", PARAM_TRIGGER_VEL,
			      bse_param_spec_float ("trigger_vel", "Trigger Velocity [%]",
						    "Set the velocity of the string pluck",
						    0.0, 100.0, 100.0, 1,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_SCALE));
  
  bse_object_class_add_param (object_class, "Trigger", PARAM_TRIGGER_HIT,
			      bse_param_spec_bool ("trigger_pulse", "Trigger Hit", "Pluck the string",
						   FALSE, BSE_PARAM_GUI));
  
  bse_object_class_add_param (object_class, "Decay", PARAM_NOTE_DECAY,
			      bse_param_spec_float ("note_decay", "Note Decay",
						    "Set the 'half-life' of the note's decay in seconds",
						    0.001, 4.0, 0.4, 0.01,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  
  bse_object_class_add_param (object_class, "Decay", PARAM_TENSION_DECAY,
			      bse_param_spec_float ("tension_decay", "Tension Decay",
						    "Set the tension of the string",
						    0.001, 1.0, 0.04, 0.01,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  
  bse_object_class_add_param (object_class, "Flavour", PARAM_METALLIC_FACTOR,
			      bse_param_spec_float ("metallic_factor", "Metallic Factor [%]",
						    "Set the metallicness of the string",
						    0.0, 100.0, 16.0, 1,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  
  bse_object_class_add_param (object_class, "Flavour", PARAM_SNAP_FACTOR,
			      bse_param_spec_float ("snap_factor", "Snap Factor [%]",
						    "Set the snappiness of the string",
						    0.0, 100.0, 34.0, 1,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "xtal_strings_out", "XtalStringsOutput");
  g_assert (ochannel_id == DAV_XTAL_STRINGS_OCHANNEL_MONO);
}

static void
dav_xtal_strings_class_finalize (DavXtalStringsClass *class)
{
}

static void
dav_xtal_strings_init (DavXtalStrings *strings)
{
  strings->params.freq = 440.0;
  strings->params.trigger_vel = 1.0;
  strings->params.note_decay = 0.4;
  strings->params.tension_decay = 0.04;
  strings->params.metallic_factor = 0.16;
  strings->params.snap_factor = 0.34;
#if 0
  strings->string = NULL;
  strings->size = 1;
  strings->pos = 0;
  strings->count = 0;
  
  strings->a = 0.0;
  strings->damping_factor = 0.0;
#endif
}

static void
dav_xtal_strings_set_property (DavXtalStrings *strings,
			       guint           param_id,
			       GValue         *value,
			       GParamSpec     *pspec)
{
  switch (param_id)
    {
    case PARAM_BASE_FREQ:
      strings->params.freq = g_value_get_float (value);
      bse_object_param_changed (BSE_OBJECT (strings), "base_note");
      break;
    case PARAM_BASE_NOTE:
      strings->params.freq = bse_note_to_freq (bse_value_get_note (value));
      bse_object_param_changed (BSE_OBJECT (strings), "base_freq");
      break;
    case PARAM_TRIGGER_HIT:
      dav_xtal_strings_trigger (strings);
      break;
    case PARAM_TRIGGER_VEL:
      strings->params.trigger_vel = g_value_get_float (value) / 100.0;
      break;
    case PARAM_NOTE_DECAY:
      strings->params.note_decay = g_value_get_float (value);
      break;
    case PARAM_TENSION_DECAY:
      strings->params.tension_decay = g_value_get_float (value);
      break;
    case PARAM_METALLIC_FACTOR:
      strings->params.metallic_factor = g_value_get_float (value) / 100.0;
      break;
    case PARAM_SNAP_FACTOR:
      strings->params.snap_factor = g_value_get_float (value) / 100.0;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (strings, param_id, pspec);
      break;
    }
}

static void
dav_xtal_strings_get_property (DavXtalStrings *strings,
			       guint           param_id,
			       GValue         *value,
			       GParamSpec     *pspec)
{
  switch (param_id)
    {
    case PARAM_BASE_FREQ:
      g_value_set_float (value, strings->params.freq);
      break;
    case PARAM_BASE_NOTE:
      bse_value_set_note (value, bse_note_from_freq (strings->params.freq));
      break;
    case PARAM_TRIGGER_HIT:
      g_value_set_boolean (value, FALSE);
      break;
    case PARAM_TRIGGER_VEL:
      g_value_set_float (value, strings->params.trigger_vel * 100.0);
      break;
    case PARAM_NOTE_DECAY:
      g_value_set_float (value, strings->params.note_decay);
      break;
    case PARAM_TENSION_DECAY:
      g_value_set_float (value, strings->params.tension_decay);
      break;
    case PARAM_METALLIC_FACTOR:
      g_value_set_float (value, strings->params.metallic_factor * 100.0);
      break;
    case PARAM_SNAP_FACTOR:
      g_value_set_float (value, strings->params.snap_factor * 100.0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (strings, param_id, pspec);
      break;
    }
}

static gfloat inline
calc_factor (gfloat freq,
	     gfloat t)
{
  return pow (0.5, 1.0 / (freq * t));
}

/* the GSL engine module that generates the signal, there may be many
 * modules per DavXtalStrings object
 */
typedef struct {
  gfloat      trigger_freq;
  gfloat      a;
  gfloat      damping_factor;
  gfloat      d;
  gint        pos;
  gint	      size;
  guint	      count;
  gfloat     *string;
} XtalStringsModule;

/* trigger a XtalStringsModule by altering its state, this function
 * gets called subsequently for all modules of a XtalStringsModule object
 */
static void
xmod_access (GslModule *module,
	     gpointer   data)
{
  XtalStringsModule *xmod = module->user_data;
  DavXtalStringsParams *params = data;
  guint i;
  guint pivot;

  xmod->trigger_freq = params->freq;
  xmod->pos = 0;
  xmod->count = 0;
  xmod->size = (int) ((BSE_MIX_FREQ + xmod->trigger_freq - 1) / xmod->trigger_freq);
  
  xmod->a = calc_factor (xmod->trigger_freq, params->tension_decay);
  xmod->damping_factor = calc_factor (xmod->trigger_freq, params->note_decay);
  
  /* Create envelope. */
  pivot = xmod->size / 5;
  
  for (i = 0; i <= pivot; i++)
    xmod->string[i] = ((float) i) / pivot;
  
  for (; i < xmod->size; i++)
    xmod->string[i] = ((float) (xmod->size - i - 1)) / (xmod->size - pivot - 1);
  
  /* Add some snap. */
  for (i = 0; i < xmod->size; i++)
    xmod->string[i] = pow (xmod->string[i], params->snap_factor * 10.0F + 1.0F);
  
  /* Add static to displacements. */
  for (i = 0; i < xmod->size; i++)
    xmod->string[i] = xmod->string[i] * (1.0F - params->metallic_factor) +
      (bse_rand_bool () ? -1.0F : 1.0F) * params->metallic_factor;
  
  /* Set velocity. */
  for (i = 0; i < xmod->size; i++)
    xmod->string[i] *= params->trigger_vel;
}

static void
dav_xtal_strings_trigger (DavXtalStrings *strings)
{
  if (BSE_SOURCE_PREPARED (strings))
    {
      /* trigger all XtalStringsModules associated with the
       * output channel DAV_XTAL_STRINGS_OCHANNEL_MONO. we
       * have to copy the parameters to this function since
       * it's called in another thread than the one our
       * DavXtalStrings object lives in. also, this function
       * will be called multiple times for all XtalStringsModules,
       * the data will be freed after the last execution occoured
       */
      bse_source_access_omodules (BSE_SOURCE (strings),
				  DAV_XTAL_STRINGS_OCHANNEL_MONO,
				  xmod_access,
				  g_memdup (&strings->params, sizeof (strings->params)),
				  g_free,
				  NULL);
    }
}

static void
xmod_process (GslModule *module,
	      guint      n_values)
{
  XtalStringsModule *xmod = module->user_data;
  BseSampleValue *wave_out = GSL_MODULE_OBUFFER (module, 0);
  guint i;
  gint32 pos2;
  gfloat sample;
  guint real_freq_256, actual_freq_256;

  real_freq_256 = (int) (xmod->trigger_freq * 256);
  actual_freq_256 = (int) (BSE_MIX_FREQ_f * 256 / xmod->size);
  
  for (i = 0; i < n_values; i++)
    {
      /* Get next position. */
      pos2 = xmod->pos + 1;
      if (pos2 >= xmod->size)
	pos2 = 0;
      
      /* Linearly interpolate sample. */
      sample = xmod->string[xmod->pos] * (1.0 - (((float) xmod->count) / actual_freq_256));
      sample += xmod->string[pos2] * (((float) xmod->count) / actual_freq_256);
      
      /* Store sample. */
      wave_out[i] = CLAMP (sample, -1.0, 1.0);
      
      /* Use Bresenham's algorithm to advance to the next position. */
      xmod->count += real_freq_256;
      while (xmod->count >= actual_freq_256)
	{
	  xmod->d = ((xmod->d * (1.0 - xmod->a)) + (xmod->string[xmod->pos] * xmod->a)) * xmod->damping_factor;
	  xmod->string[xmod->pos] = xmod->d;
	  
	  xmod->pos++;
	  if (xmod->pos >= xmod->size)
	    xmod->pos = 0;
	  
	  xmod->count -= actual_freq_256;
	}
    }
}

static void
xmod_free (gpointer        data,
	   const GslClass *klass)
{
  XtalStringsModule *xmod = data;

  g_free (xmod->string);
  g_free (xmod);
}

static void
dav_xtal_strings_context_create (BseSource *source,
				 guint      context_handle,
				 GslTrans  *trans)
{
  static const GslClass xmod_class = {
    0,				/* n_istreams */
    0,                          /* n_jstreams */
    1,				/* n_ostreams */
    xmod_process,		/* process */
    xmod_free,			/* free */
    GSL_COST_NORMAL,		/* cost */
  };
  // DavXtalStrings *strings = DAV_XTAL_STRINGS (source);
  XtalStringsModule *xmod = g_new0 (XtalStringsModule, 1);
  GslModule *module;

  xmod->string = g_new (gfloat, (BSE_MIX_FREQ + 19) / 20);
  xmod->string[0] = 0.0;
  xmod->size = 1;
  xmod->pos = 0;
  xmod->count = 0;
  xmod->a = 0.0;
  xmod->damping_factor = 0.0;
  xmod->trigger_freq = 440.0;
  module = gsl_module_new (&xmod_class, xmod);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}


/* --- Export to BSE --- */
#include "./icons/noicon.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_xtal_strings, "DavXtalStrings", "BseSource",
    "DavXtalStrings is a string synthesizer - Any commercial use of this module requires a license from Stanford University",
    &type_info_xtal_strings,
    "/Modules/XtalStrings",
    { NOICON_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NOICON_WIDTH, NOICON_HEIGHT,
      NOICON_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
