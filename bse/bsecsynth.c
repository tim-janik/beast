/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2003 Tim Janik
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
#include "bsecsynth.h"


/* --- parameters --- */
enum
{
  PARAM_0,
};


/* --- prototypes --- */
static void      bse_csynth_class_init             (BseCSynthClass *class);
static void      bse_csynth_init                   (BseCSynth      *self);
static void      bse_csynth_finalize               (GObject        *object);
static void      bse_csynth_set_property           (GObject        *object,
                                                    guint           param_id,
                                                    const GValue   *value,
                                                    GParamSpec     *pspec);
static void      bse_csynth_get_property           (GObject        *object,
                                                    guint           param_id,
                                                    GValue         *value,
                                                    GParamSpec     *pspec);


/* --- variables --- */
static GTypeClass          *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseCSynth)
{
  static const GTypeInfo type_info = {
    sizeof (BseCSynthClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_csynth_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BseCSynth),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_csynth_init,
  };
  return bse_type_register_static (BSE_TYPE_SNET, "BseCSynth", "BSE Synthesis (Filter) Network", &type_info);
}

static void
bse_csynth_class_init (BseCSynthClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_csynth_set_property;
  gobject_class->get_property = bse_csynth_get_property;
  gobject_class->finalize = bse_csynth_finalize;
}

static void
bse_csynth_init (BseCSynth *self)
{
  BSE_OBJECT_SET_FLAGS (self, BSE_SNET_FLAG_USER_SYNTH);
}

static void
bse_csynth_finalize (GObject *object)
{
  // BseCSynth *csynth = BSE_CSYNTH (object);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_csynth_set_property (GObject      *object,
                         guint         param_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  BseCSynth *self = BSE_CSYNTH (object);
  
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_csynth_get_property (GObject    *object,
                         guint       param_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  BseCSynth *self = BSE_CSYNTH (object);
  
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}
