/* BseMixer - BSE Mixer
 * Copyright (C) 1999, 2000-2001 Tim Janik
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
#include "bsemixer.h"

#include <bse/bseengine.h>

#include <string.h>

#define	DEFAULT_DB_VOLUME	(0)

/* --- parameters --- */
enum
{
  PROP_0,
  PROP_MVOLUME_f,
  PROP_MVOLUME_dB,
  PROP_MVOLUME_PERC,
  /* don't add params after here */
  PROP_NTH_VOLUME_f,
  PROP_NTH_VOLUME_dB,
  PROP_NTH_VOLUME_PERC
};


/* --- prototypes --- */
static void	 bse_mixer_init			(BseMixer	*self);
static void	 bse_mixer_class_init		(BseMixerClass	*class);
static void	 bse_mixer_set_property		(GObject	*object,
						 guint           param_id,
						 const GValue   *value,
						 GParamSpec     *pspec);
static void	 bse_mixer_get_property		(GObject	*object,
						 guint           param_id,
						 GValue         *value,
						 GParamSpec     *pspec);
static void	 bse_mixer_context_create	(BseSource      *source,
						 guint           context_handle,
						 BseTrans       *trans);
static void	 bse_mixer_update_modules	(BseMixer	*self,
						 BseTrans       *trans);


/* --- Export to BSE --- */
#include "./icons/mixer.c"
BSE_REGISTER_OBJECT (BseMixer, BseSource, "/Modules/Routing/Mixer",
                     "The Mixer module sums up incomming signals, and allowes for fine "
                     "adjusted weighting (volume setting) of the input sources",
                     mixer_icon,
                     bse_mixer_class_init, NULL, bse_mixer_init);
BSE_DEFINE_EXPORTS (BSE_PLUGIN_NAME);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
static void
bse_mixer_class_init (BseMixerClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ichannel, ochannel;
  guint i;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_mixer_set_property;
  gobject_class->get_property = bse_mixer_get_property;
  
  source_class->context_create = bse_mixer_context_create;
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PROP_MVOLUME_f,
			      sfi_pspec_real ("master_volume_f", "Master [float]", NULL,
					      bse_db_to_factor (DEFAULT_DB_VOLUME),
					      0, bse_db_to_factor (BSE_MAX_VOLUME_dB), 0.1,
					      SFI_PARAM_STORAGE ":f"));
  bse_object_class_add_param (object_class, "Adjustments",
			      PROP_MVOLUME_dB,
			      sfi_pspec_real ("master_volume_dB", "Master [dB]", NULL,
					      DEFAULT_DB_VOLUME,
					      BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
					      BSE_GCONFIG (step_volume_dB),
					      SFI_PARAM_GUI ":dial"));
  bse_object_class_add_param (object_class, "Adjustments",
			      PROP_MVOLUME_PERC,
			      sfi_pspec_int ("master_volume_perc", "Master [%]", NULL,
					     bse_db_to_factor (DEFAULT_DB_VOLUME) * 100,
					     0, bse_db_to_factor (BSE_MAX_VOLUME_dB) * 100, 1,
					     SFI_PARAM_GUI ":dial"));
  ochannel = bse_source_class_add_ochannel (source_class, "audio-out", _("Audio Out"), _("Sole Output"));
  g_assert (ochannel == BSE_MIXER_OCHANNEL_MONO);
  for (i = 1; i <= BSE_MIXER_N_INPUTS; i++)
    {
      gchar *group, *ident, *label, *blurb;
      
      group = g_strdup_printf (_("Channel%u"), i);
      ident = g_strdup_printf ("volume_f%u", i);
      label = g_strdup_printf (_("Channel%u [float]"), i);
      bse_object_class_add_param (object_class, group,
				  PROP_NTH_VOLUME_f + (i - 1) * 3,
				  sfi_pspec_real (ident, label, NULL,
						  bse_db_to_factor (DEFAULT_DB_VOLUME),
						  0, bse_db_to_factor (BSE_MAX_VOLUME_dB), 0.1,
						  SFI_PARAM_STORAGE));
      g_free (ident);
      g_free (label);
      ident = g_strdup_printf ("volume_dB%u", i);
      label = g_strdup_printf (_("Channel%u [dB]"), i);
      bse_object_class_add_param (object_class, group,
				  PROP_NTH_VOLUME_dB + (i - 1) * 3,
				  sfi_pspec_real (ident, label, NULL,
						  DEFAULT_DB_VOLUME,
						  BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						  BSE_GCONFIG (step_volume_dB),
						  SFI_PARAM_GUI ":dial"));
      g_free (ident);
      g_free (label);
      ident = g_strdup_printf ("volume_perc%u", i);
      label = g_strdup_printf (_("Channel%u [%%]"), i);
      bse_object_class_add_param (object_class, group,
				  PROP_NTH_VOLUME_PERC + (i - 1) * 3,
				  sfi_pspec_int (ident, label, NULL,
						 bse_db_to_factor (DEFAULT_DB_VOLUME) * 100,
						 0, bse_db_to_factor (BSE_MAX_VOLUME_dB) * 100, 1,
						 SFI_PARAM_GUI ":dial"));
      g_free (group);
      g_free (ident);
      g_free (label);
      ident = g_strdup_printf ("audio-in%u", i);
      label = g_strdup_printf (_("Audio In%u"), i);
      blurb = g_strdup_printf (_("Input Channel %u"), i);
      ichannel = bse_source_class_add_ichannel (source_class, ident, label, blurb);
      g_assert (ichannel == i - 1);
      g_free (blurb);
      g_free (label);
      g_free (ident);
    }
}

static void
bse_mixer_init (BseMixer *self)
{
  guint i;
  
  self->master_volume_factor = bse_db_to_factor (DEFAULT_DB_VOLUME);
  for (i = 0; i < BSE_MIXER_N_INPUTS; i++)
    self->volume_factors[i] = bse_db_to_factor (DEFAULT_DB_VOLUME);
}

static void
bse_mixer_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  BseMixer *self = BSE_MIXER (object);
  
  switch (param_id)
    {
      guint indx, n;
    case PROP_MVOLUME_f:
      self->master_volume_factor = sfi_value_get_real (value);
      bse_mixer_update_modules (self, NULL);
      g_object_notify (object, "master_volume_dB");
      g_object_notify (object, "master_volume_perc");
      break;
    case PROP_MVOLUME_dB:
      self->master_volume_factor = bse_db_to_factor (sfi_value_get_real (value));
      bse_mixer_update_modules (self, NULL);
      g_object_notify (object, "master_volume_f");
      g_object_notify (object, "master_volume_perc");
      break;
    case PROP_MVOLUME_PERC:
      self->master_volume_factor = sfi_value_get_int (value) / 100.0;
      bse_mixer_update_modules (self, NULL);
      g_object_notify (object, "master_volume_f");
      g_object_notify (object, "master_volume_dB");
      break;
    default:
      indx = (param_id - PROP_NTH_VOLUME_f) % 3;
      n = (param_id - PROP_NTH_VOLUME_f) / 3;
      switch (indx)
	{
	  gchar *prop;
	case PROP_NTH_VOLUME_f - PROP_NTH_VOLUME_f:
	  self->volume_factors[n] = sfi_value_get_real (value);
	  bse_mixer_update_modules (self, NULL);
	  prop = g_strdup_printf ("volume_dB%u", n + 1);
	  g_object_notify (object, prop);
	  g_free (prop);
          prop = g_strdup_printf ("volume_perc%u", n + 1);
	  g_object_notify (object, prop);
	  g_free (prop);
	  break;
	case PROP_NTH_VOLUME_dB - PROP_NTH_VOLUME_f:
	  self->volume_factors[n] = bse_db_to_factor (sfi_value_get_real (value));
	  bse_mixer_update_modules (self, NULL);
	  prop = g_strdup_printf ("volume_f%u", n + 1);
	  g_object_notify (object, prop);
	  g_free (prop);
          prop = g_strdup_printf ("volume_perc%u", n + 1);
	  g_object_notify (object, prop);
	  g_free (prop);
	  break;
	case PROP_NTH_VOLUME_PERC - PROP_NTH_VOLUME_f:
	  self->volume_factors[n] = sfi_value_get_int (value) / 100.0;
	  bse_mixer_update_modules (self, NULL);
	  prop = g_strdup_printf ("volume_f%u", n + 1);
	  g_object_notify (object, prop);
	  g_free (prop);
	  prop = g_strdup_printf ("volume_dB%u", n + 1);
	  g_object_notify (object, prop);
	  g_free (prop);
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
	  break;
	}
    }
}

static void
bse_mixer_get_property (GObject    *object,
			guint       param_id,
			GValue     *value,
			GParamSpec *pspec)
{
  BseMixer *self = BSE_MIXER (object);
  
  switch (param_id)
    {
      guint indx, n;
    case PROP_MVOLUME_f:
      sfi_value_set_real (value, self->master_volume_factor);
      break;
    case PROP_MVOLUME_dB:
      sfi_value_set_real (value, bse_db_from_factor (self->master_volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PROP_MVOLUME_PERC:
      sfi_value_set_int (value, self->master_volume_factor * 100.0 + 0.5);
      break;
    default:
      indx = (param_id - PROP_NTH_VOLUME_f) % 3;
      n = (param_id - PROP_NTH_VOLUME_f) / 3;
      switch (indx)
	{
	case PROP_NTH_VOLUME_f - PROP_NTH_VOLUME_f:
	  sfi_value_set_real (value, self->volume_factors[n]);
	  break;
	case PROP_NTH_VOLUME_dB - PROP_NTH_VOLUME_f:
	  sfi_value_set_real (value, bse_db_from_factor (self->volume_factors[n], BSE_MIN_VOLUME_dB));
	  break;
	case PROP_NTH_VOLUME_PERC - PROP_NTH_VOLUME_f:
	  sfi_value_set_int (value, self->volume_factors[n] * 100.0 + 0.5);
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
	  break;
	}
    }
}

typedef struct
{
  gfloat volumes[BSE_MIXER_N_INPUTS];
} Mixer;

static void
bse_mixer_update_modules (BseMixer *self,
			  BseTrans *trans)
{
  gfloat volumes[BSE_MIXER_N_INPUTS];
  guint i;
  
  for (i = 0; i < BSE_MIXER_N_INPUTS; i++)
    volumes[i] = self->volume_factors[i] * self->master_volume_factor;
  if (BSE_SOURCE_PREPARED (self))
    bse_source_update_modules (BSE_SOURCE (self),
			       G_STRUCT_OFFSET (Mixer, volumes),
			       volumes,
			       sizeof (volumes),
			       trans);
}

static void
mixer_process (BseModule *module,
	       guint      n_values)
{
  Mixer *mixer = module->user_data;
  gfloat *wave_out = BSE_MODULE_OBUFFER (module, 0);
  gfloat *wave_bound = wave_out + n_values;
  
  if (module->ostreams[0].connected)
    {
      guint n;
      
      for (n = 0; n < BSE_MODULE_N_ISTREAMS (module); n++)
	if (module->istreams[n].connected)
	  {
	    const gfloat *wave_in = BSE_MODULE_IBUFFER (module, n);
	    gfloat *w = wave_out;
	    gfloat volume = mixer->volumes[n];
	    
	    if (volume != 1.0)
	      do { *w++ = volume * *wave_in++; } while (w < wave_bound);
	    else
	      do { *w++ = *wave_in++; } while (w < wave_bound);
	    break;
	  }
      if (n >= BSE_MODULE_N_ISTREAMS (module))
	memset (wave_out, 0, sizeof (wave_out[0]) * n_values);
      for (n += 1; n < BSE_MODULE_N_ISTREAMS (module); n++)
	if (module->istreams[n].connected)
	  {
	    const gfloat *wave_in = BSE_MODULE_IBUFFER (module, n);
	    gfloat *w = wave_out;
	    gfloat volume = mixer->volumes[n];
	    
	    if (volume != 1.0)
	      do { *w++ += volume * *wave_in++; } while (w < wave_bound);
	    else
	      do { *w++ += *wave_in++; } while (w < wave_bound);
	  }
    }
}

static void
bse_mixer_context_create (BseSource *source,
			  guint      context_handle,
			  BseTrans  *trans)
{
  static const BseModuleClass mixer_class = {
    BSE_MIXER_N_INPUTS,		/* n_istreams */
    0,                          /* n_jstreams */
    1,				/* n_ostreams */
    mixer_process,		/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    (BseModuleFreeFunc) g_free,	/* free */
    BSE_COST_CHEAP,		/* flags */
  };
  Mixer *mixer = g_new0 (Mixer, 1);
  BseModule *module;
  
  module = bse_module_new (&mixer_class, mixer);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
  
  /* update module data */
  bse_mixer_update_modules (BSE_MIXER (source), trans);
}
