/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsesongbus.h"
#include "bsecategories.h"
#include "gslengine.h"


/* --- parameters --- */
enum
{
  PROP_0,
  PROP_MVOLUME_f,
  PROP_MVOLUME_dB,
  PROP_MVOLUME_PERC
};


/* --- variables --- */
static gpointer		 song_bus_parent_class = NULL;


/* --- functions --- */
static void
bse_song_bus_init (BseSongBus *iput)
{
}

static void
bse_song_bus_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  // BseSongBus *self = BSE_SONG_BUS (object);
  switch (param_id)
    {
    case PROP_MVOLUME_f:
      // self->volume_factor = sfi_value_get_real (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_song_bus_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  // BseSongBus *self = BSE_SONG_BUS (object);
  switch (param_id)
    {
    case PROP_MVOLUME_f:
      // sfi_value_set_real (value, self->volume_factor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_song_bus_prepare (BseSource *source)
{
  // BseSongBus *iput = BSE_SONG_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (song_bus_parent_class)->prepare (source);
}

static void
bse_song_bus_context_create (BseSource *source,
			      guint      context_handle,
			      GslTrans  *trans)
{
  // BseSongBus *iput = BSE_SONG_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (song_bus_parent_class)->context_create (source, context_handle, trans);
}

static void
bse_song_bus_context_connect (BseSource *source,
			       guint      context_handle,
			       GslTrans  *trans)
{
  // BseSongBus *iput = BSE_SONG_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (song_bus_parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_song_bus_reset (BseSource *source)
{
  // BseSongBus *iput = BSE_SONG_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (song_bus_parent_class)->reset (source);
}

static void
bse_song_bus_class_init (BseSongBusClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel_id;
  
  song_bus_parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_song_bus_set_property;
  gobject_class->get_property = bse_song_bus_get_property;
  
  source_class->prepare = bse_song_bus_prepare;
  source_class->context_create = bse_song_bus_context_create;
  source_class->context_connect = bse_song_bus_context_connect;
  source_class->reset = bse_song_bus_reset;
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PROP_MVOLUME_f,
			      sfi_pspec_real ("gain_volume_f", "Bus Gain [float]", NULL,
					      bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB),
					      0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
					      0.1,
					      SFI_PARAM_GUI ":dial"));
  
  channel_id = bse_source_class_add_ichannel (source_class, "left-audio-in", _("Left Audio In"), _("Left channel input"));
  g_assert (channel_id == BSE_SONG_BUS_ICHANNEL_LEFT);
  channel_id = bse_source_class_add_ichannel (source_class, "right-audio-in", _("Right Audio In"), _("Right channel input"));
  g_assert (channel_id == BSE_SONG_BUS_ICHANNEL_RIGHT);
  channel_id = bse_source_class_add_ochannel (source_class, "left-audio-out", _("Left Audio Out"), _("Left channel output"));
  g_assert (channel_id == BSE_SONG_BUS_OCHANNEL_LEFT);
  channel_id = bse_source_class_add_ochannel (source_class, "right-audio-out", _("Right Audio Out"), _("Right channel output"));
  g_assert (channel_id == BSE_SONG_BUS_OCHANNEL_RIGHT);
}

BSE_BUILTIN_TYPE (BseSongBus)
{
  static const GTypeInfo song_bus_info = {
    sizeof (BseSongBusClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_song_bus_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BseSongBus),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_song_bus_init,
  };
  GType type = bse_type_register_static (BSE_TYPE_SUB_SYNTH,
                                         "BseSongBus",
                                         _("Bus implementation for songs, used to to route track audio signals "
                                           "to the master output."),
                                         &song_bus_info);
  // bse_categories_register_stock_module (N_("/Bus & Output/SONG Bus"), type, mic_pixstream);
  return type;
}
