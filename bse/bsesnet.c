/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include        "bsesnet.h"

#include        "bseproject.h"
#include        "bsecategories.h"
#include        "bsehunkmixer.h"
#include        "bsestorage.h"
#include        "bseheart.h"
#include        <string.h>
#include        <time.h>
#include        <fcntl.h>
#include        <unistd.h>

#include        "./icons/snet.c"

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_VOLUME_f,
  PARAM_VOLUME_dB,
  PARAM_VOLUME_PERC
};


/* --- prototypes --- */
static void      bse_snet_class_init             (BseSNetClass   *class);
static void      bse_snet_init                   (BseSNet        *snet);
static BseIcon*  bse_snet_do_get_icon            (BseObject      *object);
static void      bse_snet_do_destroy             (BseObject      *object);
static void      bse_snet_set_param              (BseSNet        *snet,
						  guint           param_id,
						  GValue         *value,
						  GParamSpec     *pspec,
						  const gchar    *trailer);
static void      bse_snet_get_param              (BseSNet        *snet,
						  guint           param_id,
						  GValue         *value,
						  GParamSpec     *pspec,
						  const gchar    *trailer);
static void      bse_snet_add_item               (BseContainer   *container,
						  BseItem        *item);
static void      bse_snet_forall_items           (BseContainer   *container,
						  BseForallItemsFunc func,
						  gpointer        data);
static void      bse_snet_remove_item            (BseContainer   *container,
						  BseItem        *item);
static void      bse_snet_prepare                (BseSource      *source,
						  BseIndex        index);
static BseChunk* bse_snet_calc_chunk             (BseSource      *source,
						  guint           ochannel_id);
static void      bse_snet_reset                  (BseSource      *source);


/* --- variables --- */
static GTypeClass     *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSNet)
{
  GType   snet_type;

  static const GTypeInfo snet_info = {
    sizeof (BseSNetClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_snet_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSNet),
    BSE_PREALLOC_N_SUPERS /* n_preallocs */,
    (GInstanceInitFunc) bse_snet_init,
  };
  
  snet_type = bse_type_register_static (BSE_TYPE_SUPER,
                                        "BseSNet",
                                        "BSE Synthesis (Filter) Network",
                                        &snet_info);
  // bse_categories_register_icon ("/Source/Projects/SNet", snet_type, &snet_pixdata);

  return snet_type;
}

static void
bse_snet_class_init (BseSNetClass *class)
{
  static const BsePixdata snet_pixdata = {
    SNET_PIXDATA_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    SNET_PIXDATA_WIDTH, SNET_PIXDATA_HEIGHT,
    SNET_PIXDATA_RLE_PIXEL_DATA,
  };
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (class);
  guint ichannel_id, ochannel_id;
  
  parent_class = g_type_class_peek (BSE_TYPE_SUPER);
  
  gobject_class->set_param = (GObjectSetParamFunc) bse_snet_set_param;
  gobject_class->get_param = (GObjectGetParamFunc) bse_snet_get_param;

  object_class->get_icon = bse_snet_do_get_icon;
  object_class->destroy = bse_snet_do_destroy;
  
  source_class->prepare = bse_snet_prepare;
  source_class->calc_chunk = bse_snet_calc_chunk;
  source_class->reset = bse_snet_reset;

  container_class->add_item = bse_snet_add_item;
  container_class->remove_item = bse_snet_remove_item;
  container_class->forall_items = bse_snet_forall_items;
  
  class->icon = bse_icon_ref_static (bse_icon_from_pixdata (&snet_pixdata));
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_f,
			      b_param_spec_float ("volume_f", "Master [float]", NULL,
						  0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						  bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB), 0.1,
						  B_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_dB,
			      b_param_spec_float ("volume_dB", "Master [dB]", NULL,
						  BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						  BSE_DFL_MASTER_VOLUME_dB, BSE_STP_VOLUME_dB,
						  B_PARAM_GUI |
						  B_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_PERC,
			      b_param_spec_uint ("volume_perc", "Master [%]", NULL,
						 0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						 bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB) * 100, 1,
						 B_PARAM_GUI |
						 B_PARAM_HINT_DIAL));

  ichannel_id = bse_source_class_add_ichannel (source_class, "multi_in", "Multi Track In", 1, 2);
  g_assert (ichannel_id == BSE_SNET_ICHANNEL_MULTI);
  ochannel_id = bse_source_class_add_ochannel (source_class, "stereo_out", "Stereo Out", 2);
  g_assert (ochannel_id == BSE_SNET_OCHANNEL_STEREO);
}

static void
bse_snet_init (BseSNet *snet)
{
  snet->sources = NULL;
  snet->volume_factor = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);
}

static void
bse_snet_do_destroy (BseObject *object)
{
  BseSNet *snet = BSE_SNET (object);
  
  while (snet->sources)
    bse_container_remove_item (BSE_CONTAINER (snet), snet->sources->data);

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static BseIcon*
bse_snet_do_get_icon (BseObject *object)
{
  BseSNet *snet = BSE_SNET (object);

  return BSE_SNET_GET_CLASS (snet)->icon;
}

static void
bse_snet_set_param (BseSNet     *snet,
		    guint        param_id,
		    GValue      *value,
		    GParamSpec  *pspec,
		    const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_VOLUME_f:
      snet->volume_factor = b_value_get_float (value);
      bse_object_param_changed (BSE_OBJECT (snet), "volume_dB");
      bse_object_param_changed (BSE_OBJECT (snet), "volume_perc");
      break;
    case PARAM_VOLUME_dB:
      snet->volume_factor = bse_dB_to_factor (b_value_get_float (value));
      bse_object_param_changed (BSE_OBJECT (snet), "volume_f");
      bse_object_param_changed (BSE_OBJECT (snet), "volume_perc");
      break;
    case PARAM_VOLUME_PERC:
      snet->volume_factor = b_value_get_uint (value) / 100.0;
      bse_object_param_changed (BSE_OBJECT (snet), "volume_f");
      bse_object_param_changed (BSE_OBJECT (snet), "volume_dB");
      break;
    default:
      G_WARN_INVALID_PARAM_ID (snet, param_id, pspec);
      break;
    }
}

static void
bse_snet_get_param (BseSNet     *snet,
		    guint        param_id,
		    GValue      *value,
		    GParamSpec  *pspec,
		    const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_VOLUME_f:
      b_value_set_float (value, snet->volume_factor);
      break;
    case PARAM_VOLUME_dB:
      b_value_set_float (value, bse_dB_from_factor (snet->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PARAM_VOLUME_PERC:
      b_value_set_uint (value, snet->volume_factor * 100.0 + 0.5);
      break;
    default:
      G_WARN_INVALID_PARAM_ID (snet, param_id, pspec);
      break;
    }
}

BseSNet*
bse_snet_new (BseProject  *project,
              const gchar *first_param_name,
              ...)
{
  BseObject *object;
  va_list var_args;
  
  if (project)
    g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);

  va_start (var_args, first_param_name);
  object = bse_object_new_valist (BSE_TYPE_SNET, first_param_name, var_args);
  va_end (var_args);

  if (project)
    bse_project_add_super (project, BSE_SUPER (object));
  
  return BSE_SNET (object);
}

BseSNet*
bse_snet_lookup (BseProject  *project,
                 const gchar *name)
{
  BseItem *item;

  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  item = bse_container_lookup_item (BSE_CONTAINER (project), name);

  return BSE_IS_SNET (item) ? BSE_SNET (item) : NULL;
}

static void
bse_snet_add_item (BseContainer *container,
                   BseItem      *item)
{
  BseSNet *snet = BSE_SNET (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOURCE))
    snet->sources = g_list_append (snet->sources, item);
  else
    g_warning ("BseSNet: cannot add non-source item type `%s'",
               BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_snet_forall_items (BseContainer      *container,
                       BseForallItemsFunc func,
                       gpointer           data)
{
  BseSNet *snet = BSE_SNET (container);
  GList *list;

  list = snet->sources;
  while (list)
    {
      BseItem *item;

      item = list->data;
      list = list->next;
      if (!func (item, data))
        return;
    }
}

static void
bse_snet_remove_item (BseContainer *container,
                      BseItem      *item)
{
  BseSNet *snet;

  snet = BSE_SNET (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOURCE))
    snet->sources = g_list_remove (snet->sources, item);
  else
    g_warning ("BseSNet: cannot remove non-source item type `%s'",
               BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

BseSource*
bse_snet_new_source (BseSNet     *snet,
                     GType        source_type,
                     const gchar *first_param_name,
                     ...)
{
  BseContainer *container;
  BseSource *source;
  va_list var_args;

  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  g_return_val_if_fail (g_type_is_a (source_type, BSE_TYPE_SOURCE), NULL);

  container = BSE_CONTAINER (snet);
  va_start (var_args, first_param_name);
  source = bse_object_new_valist (source_type, first_param_name, var_args);
  va_end (var_args);
  bse_container_add_item (container, BSE_ITEM (source));
  bse_object_unref (BSE_OBJECT (source));

  return source;
}

void
bse_snet_remove_source (BseSNet   *snet,
                        BseSource *source)
{
  BseContainer *container;
  BseItem *item;
  
  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (BSE_IS_SOURCE (source));

  container = BSE_CONTAINER (snet);
  item = BSE_ITEM (source);
  
  g_return_if_fail (item->parent == (BseItem*) container);

  bse_container_remove_item (container, item);
}

static void
bse_snet_prepare (BseSource *source,
		  BseIndex   index)
{
  BseSNet *snet = BSE_SNET (source);

  bse_object_lock (BSE_OBJECT (snet));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);

  /* FIXME: odevice hack */
  bse_heart_source_add_odevice (source, bse_heart_get_device (bse_heart_get_default_odevice ()));
}

static BseChunk*
bse_snet_calc_chunk (BseSource *source,
		     guint      ochannel_id)
{
  BseSNet *snet = BSE_SNET (source);
  BseSourceInput *input;
  BseChunk *ichunk;
  BseSampleValue *hunk;
  gfloat stereo_volumes[2];

  g_return_val_if_fail (ochannel_id == BSE_SNET_OCHANNEL_STEREO, NULL);

  input = bse_source_get_input (source, BSE_SNET_ICHANNEL_MULTI);
  if (!input) /* silence */
    return bse_chunk_new_static_zero (2);

  ichunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);
  if (ichunk->n_tracks == 2 &&
      BSE_EPSILON_CMP (1.0, snet->volume_factor) == 0) /* stereo already, no volume */
    return ichunk;

  /* ok, mix input (mono or stereo) to stereo with volume */
  stereo_volumes[0] = snet->volume_factor;
  stereo_volumes[1] = snet->volume_factor;
  hunk = bse_hunk_alloc (2);
  bse_hunk_mix (2, hunk, stereo_volumes, ichunk->n_tracks, ichunk->hunk);
  bse_chunk_unref (ichunk);

  return bse_chunk_new_orphan (2, hunk);
}

static void
bse_snet_reset (BseSource *source)
{
  BseSNet *snet = BSE_SNET (source);

#if 0 // FIXME 
  GList *list;

  /* reset all children */
  for (list = snet->sources; list; list = list->next)
    {
      BseSource *source = list->data;

      if (BSE_SOURCE_PREPARED (source))
	bse_source_reset (source);
    }
#endif
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);

  bse_object_unlock (BSE_OBJECT (snet));
}
