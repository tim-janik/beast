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
#include "bsebus.h"
#include "bsecategories.h"
#include "bsetrack.h"
#include "bsesong.h"
#include "bseengine.h"


/* --- parameters --- */
enum
{
  PROP_0,
  PROP_MVOLUME_f,
  PROP_MVOLUME_dB,
  PROP_MVOLUME_PERC
};


/* --- variables --- */
static gpointer		 bus_parent_class = NULL;


/* --- functions --- */
static void
bse_bus_init (BseBus *self)
{
  bse_sub_synth_set_null_shortcut (BSE_SUB_SYNTH (self), TRUE); // FIXME
}

static void
bse_bus_dispose (GObject *object)
{
  BseBus *self = BSE_BUS (object);
  while (self->inputs)
    bse_bus_disconnect (self, self->inputs->data);
  /* chain parent class' handler */
  G_OBJECT_CLASS (bus_parent_class)->dispose (object);
}

static void
bse_bus_finalize (GObject *object)
{
  BseBus *self = BSE_BUS (object);
  g_assert (self->inputs == NULL);
  g_assert (self->summation == NULL);
  /* chain parent class' handler */
  G_OBJECT_CLASS (bus_parent_class)->finalize (object);
}

static void
bse_bus_set_property (GObject      *object,
                      guint         param_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  // BseBus *self = BSE_BUS (object);
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
bse_bus_get_property (GObject    *object,
                      guint       param_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  // BseBus *self = BSE_BUS (object);
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
bse_bus_set_parent (BseItem *item,
                    BseItem *parent)
{
  BseBus *self = BSE_BUS (item);
  /* chain parent class' handler */
  BSE_ITEM_CLASS (bus_parent_class)->set_parent (item, parent);

  while (self->inputs)
    bse_bus_disconnect (self, self->inputs->data);
  
  if (self->summation)
    {
      BseItem *sitem = BSE_ITEM (self->summation);
      self->summation = NULL;
      BseContainer *container = BSE_CONTAINER (sitem->parent);
      bse_container_remove_item (container, sitem);
    }
}

static void
bse_bus_prepare (BseSource *source)
{
  // BseBus *iput = BSE_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (bus_parent_class)->prepare (source);
}

static void
bse_bus_context_create (BseSource *source,
                        guint      context_handle,
                        GslTrans  *trans)
{
  // BseBus *iput = BSE_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (bus_parent_class)->context_create (source, context_handle, trans);
}

static void
bse_bus_context_connect (BseSource *source,
                         guint      context_handle,
                         GslTrans  *trans)
{
  // BseBus *iput = BSE_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (bus_parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_bus_reset (BseSource *source)
{
  // BseBus *iput = BSE_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (bus_parent_class)->reset (source);
}

static gboolean
bse_bus_ensure_summation (BseBus *self)
{
  if (!self->summation)
    {
      BseItem *item = BSE_ITEM (self);
      if (BSE_IS_SONG (item->parent))
        self->summation = bse_song_create_summation (BSE_SONG (item->parent));
      if (self->summation)
        {
          bse_source_must_set_input (BSE_SOURCE (self), 0, self->summation, 0);
          bse_source_must_set_input (BSE_SOURCE (self), 1, self->summation, 1);
        }
    }
  return self->summation != NULL;
}

static void
bus_uncross_input (BseItem *owner,
                   BseItem *item)
{
  /* delete item via procedure so deletion is recorded to undo */
  if (BSE_IS_TRACK (item))
    bse_item_exec_void (owner, "disconnect-track", item);
  else /* IS_BUS */
    bse_item_exec_void (owner, "disconnect-bus", item);
}

BseErrorType
bse_bus_connect (BseBus  *self,
                 BseItem *trackbus)
{
  BseSource *osource;
  if (BSE_IS_TRACK (trackbus))
    osource = bse_track_get_output (BSE_TRACK (trackbus));
  else if (BSE_IS_BUS (trackbus))
    osource = BSE_SOURCE (trackbus);
  else
    return BSE_ERROR_SOURCE_TYPE_INVALID;
  if (!osource || !bse_bus_ensure_summation (self))
    return BSE_ERROR_SOURCE_PARENT_MISMATCH;
  BseErrorType error = bse_source_set_input (self->summation, 0, osource, 0);
  if (!error)
    {
      bse_source_must_set_input (self->summation, 1, osource, 1);
      self->inputs = sfi_ring_append (self->inputs, trackbus);
      bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (trackbus), bus_uncross_input);
      bse_object_proxy_notifies (trackbus, self, "inputs-changed");
    }
  return error;
}

BseErrorType
bse_bus_disconnect (BseBus  *self,
                    BseItem *trackbus)
{
  BseSource *osource;
  if (BSE_IS_TRACK (trackbus))
    osource = bse_track_get_output (BSE_TRACK (trackbus));
  else if (BSE_IS_BUS (trackbus))
    osource = BSE_SOURCE (trackbus);
  else
    return BSE_ERROR_SOURCE_TYPE_INVALID;
  if (!osource || !self->summation || !sfi_ring_find (self->inputs, trackbus))
    return BSE_ERROR_SOURCE_PARENT_MISMATCH;
  bse_object_unproxy_notifies (trackbus, self, "inputs-changed");
  bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (trackbus), bus_uncross_input);
  self->inputs = sfi_ring_remove (self->inputs, trackbus);
  BseErrorType error1 = bse_source_unset_input (self->summation, 0, osource, 0);
  BseErrorType error2 = bse_source_unset_input (self->summation, 1, osource, 1);
  return error1 ? error1 : error2;
}

SfiRing*
bse_bus_list_inputs (BseBus *self)
{
  return sfi_ring_copy (self->inputs);
}

static void
bse_bus_class_init (BseBusClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel_id;
  
  bus_parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_bus_set_property;
  gobject_class->get_property = bse_bus_get_property;
  gobject_class->dispose = bse_bus_dispose;
  gobject_class->finalize = bse_bus_finalize;
  
  item_class->set_parent = bse_bus_set_parent;
  
  source_class->prepare = bse_bus_prepare;
  source_class->context_create = bse_bus_context_create;
  source_class->context_connect = bse_bus_context_connect;
  source_class->reset = bse_bus_reset;
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PROP_MVOLUME_f,
			      sfi_pspec_real ("gain_volume_f", "Bus Gain [float]", NULL,
					      bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB),
					      0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
					      0.1,
					      SFI_PARAM_GUI ":dial"));
  
  channel_id = bse_source_class_add_ichannel (source_class, "left-audio-in", _("Left Audio In"), _("Left channel input"));
  g_assert (channel_id == BSE_BUS_ICHANNEL_LEFT);
  channel_id = bse_source_class_add_ichannel (source_class, "right-audio-in", _("Right Audio In"), _("Right channel input"));
  g_assert (channel_id == BSE_BUS_ICHANNEL_RIGHT);
  channel_id = bse_source_class_add_ochannel (source_class, "left-audio-out", _("Left Audio Out"), _("Left channel output"));
  g_assert (channel_id == BSE_BUS_OCHANNEL_LEFT);
  channel_id = bse_source_class_add_ochannel (source_class, "right-audio-out", _("Right Audio Out"), _("Right channel output"));
  g_assert (channel_id == BSE_BUS_OCHANNEL_RIGHT);
}

BSE_BUILTIN_TYPE (BseBus)
{
  static const GTypeInfo bus_info = {
    sizeof (BseBusClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_bus_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BseBus),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_bus_init,
  };
  GType type = bse_type_register_static (BSE_TYPE_SUB_SYNTH,
                                         "BseBus",
                                         _("Bus implementation for songs, used to route track audio signals "
                                           "to the master output."),
                                         &bus_info);
  return type;
}
