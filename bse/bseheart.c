/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include "bseheart.h"

#include "bsechunk.h"
#include "bsehunkmixer.h"

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_LATENCY
};


/* --- prototypes --- */
static void        bse_heart_init          (BseHeart      *heart);
static void        bse_heart_class_init    (BseHeartClass *class);
static void        bse_heart_shutdown      (BseObject     *object);
static void        bse_heart_destroy       (BseObject     *object);
static void        bse_heart_set_param     (BseHeart	  *heart,
					    BseParam      *param,
					    guint          param_id);
static void        bse_heart_get_param     (BseHeart	  *heart,
					    BseParam      *param,
					    guint          param_id);
static gboolean	   bse_heart_prepare	   (gpointer       source_data,
					    GTimeVal      *current_time,
					    gint          *timeout,
					    gpointer       user_data);
static gboolean	   bse_heart_check	   (gpointer  	   source_data,
					    GTimeVal      *current_time,
					    gpointer       user_data);
static gboolean	   bse_heart_dispatch	   (gpointer       source_data,
					    GTimeVal      *current_time,
					    gpointer       user_data);


/* --- variables --- */
static gpointer     parent_class = NULL;
static BseHeart    *bse_global_heart = NULL;
static BseIndex     bse_heart_beat_index = 0;
static GSourceFuncs bse_heart_gsource_funcs = {
  bse_heart_prepare,
  bse_heart_check,
  bse_heart_dispatch,
  (GDestroyNotify) NULL
};


/* --- functions --- */
BSE_BUILTIN_TYPE (BseHeart)
{
  static const BseTypeInfo heart_info = {
    sizeof (BseHeartClass),

    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_heart_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseHeart),
    16 /* n_preallocs */,
    (BseObjectInitFunc) bse_heart_init,
  };

  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BseHeart",
				   "BSE Heart - fundamental audio router",
				   &heart_info);
}

static void
bse_heart_class_init (BseHeartClass *class)
{
  BseObjectClass *object_class;

  parent_class = bse_type_class_peek (BSE_TYPE_OBJECT);
  object_class = BSE_OBJECT_CLASS (class);

  object_class->set_param = (BseObjectSetParamFunc) bse_heart_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_heart_get_param;
  object_class->shutdown = bse_heart_shutdown;
  object_class->destroy = bse_heart_destroy;

  bse_object_class_add_param (object_class, NULL,
			      PARAM_LATENCY,
			      bse_param_spec_uint ("latency", "Latency [msecs]", NULL,
						   2, 2 * 1000,
						   50,
						   1000,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_SCALE));
}

static void
bse_heart_init (BseHeart *heart)
{
  heart->latency = 1000;
  heart->n_sources = 0;
  heart->sources = NULL;
  heart->n_devices = 0;
  heart->devices = NULL;
  heart->n_open_devices = 0;
  heart->default_odevice = NULL;
  heart->default_idevice = NULL;
  heart->device_open_handler_id = 0;
  heart->device_open_list = NULL;
  heart->mix_buffer = NULL;
  heart->gsource_id = g_source_add (BSE_HEART_PRIORITY,
				    FALSE,
				    &bse_heart_gsource_funcs,
				    heart,
				    NULL, NULL);
}

static void
bse_heart_shutdown (BseObject *object)
{
  BseHeart *heart = BSE_HEART (object);

  g_source_remove (heart->gsource_id);
  heart->gsource_id = 0;

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
bse_heart_destroy (BseObject *object)
{
  BseHeart *heart = BSE_HEART (object);

  if (bse_global_heart == heart)
    bse_global_heart = NULL;

  if (heart->device_open_handler_id)
    {
      g_source_remove (heart->device_open_handler_id);
      heart->device_open_handler_id = 0;
    }

  if (heart->n_sources || heart->n_devices ||
      heart->n_open_devices || heart->device_open_list || heart->mix_buffer)
    g_warning ("Eeek, freeing BseHeart with stale members");

  g_free (heart->default_odevice);
  heart->default_odevice = NULL;
  g_free (heart->default_idevice);
  heart->default_idevice = NULL;

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);

  BSE_IF_DEBUG (CHUNKS)
    {
      g_message ("BseIndex: %lld", bse_heart_beat_index);
      bse_chunk_debug ();
    }
}

static void
bse_heart_set_param (BseHeart *heart,
		     BseParam *param,
		     guint     param_id)
{
  switch (param_id)
    {
    case PARAM_LATENCY:
      heart->latency = param->value.v_uint;
      break;
    default:
      BSE_UNHANDLED_PARAM_ID (heart, param, param_id);
      break;
    }
}

static void
bse_heart_get_param (BseHeart *heart,
                     BseParam *param,
		     guint     param_id)
{
  switch (param_id)
    {
    case PARAM_LATENCY:
      param->value.v_uint = heart->latency;
      break;
    default:
      BSE_UNHANDLED_PARAM_ID (heart, param, param_id);
      break;
    }
}

BseHeart*
bse_heart_get_global (gboolean with_ref)
{
  if (with_ref)
    {
      if (!bse_global_heart)
	bse_global_heart = bse_object_new (BSE_TYPE_HEART, NULL);
      else
	bse_object_ref ((BseObject*) bse_global_heart);
    }
  
  return bse_global_heart;
}

void
bse_heart_register_device (const gchar  *symbolic_name,
			   BsePcmDevice *pdev)
{
  BseHeart *heart;
  guint i = 0;
  gchar *name;

  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (!BSE_DEVICE_REGISTERED (pdev));
  g_return_if_fail (!BSE_DEVICE_OPEN (pdev));
  g_return_if_fail (symbolic_name != NULL);

  heart = bse_heart_get_global (TRUE);
  bse_object_ref (BSE_OBJECT (pdev));

  name = g_strdup (symbolic_name);
  while (bse_heart_get_device (name))
    {
      g_free (name);
      name = g_strdup_printf ("%s-%u", symbolic_name, ++i);
    }
  BSE_OBJECT_SET_FLAGS (pdev, BSE_DEVICE_FLAG_REGISTERED);
  i = heart->n_devices++;
  heart->devices = g_renew (BseHeartDevice, heart->devices, heart->n_devices);
  heart->devices[i].device = pdev;
  heart->devices[i].name = name;
  heart->devices[i].n_isources = 0;
  heart->devices[i].isources = NULL;
  heart->devices[i].n_osources = 0;
  heart->devices[i].osources = NULL;
  heart->devices[i].ochunks = NULL;
}

void
bse_heart_unregister_device (BsePcmDevice *pdev)
{
  BseHeartDevice *hdevice;
  BseHeart *heart;
  guint i;

  g_return_if_fail (BSE_IS_PCM_DEVICE (pdev));
  g_return_if_fail (BSE_DEVICE_REGISTERED (pdev));
  g_return_if_fail (!BSE_DEVICE_OPEN (pdev));

  heart = bse_heart_get_global (FALSE);
  for (i = 0; i < heart->n_devices; i++)
    if (heart->devices[i].device == pdev)
      break;
  g_return_if_fail (i < heart->n_devices);	/* paranoid */

  g_return_if_fail (heart->devices[i].n_isources == 0);
  g_return_if_fail (heart->devices[i].n_osources == 0);

  hdevice = heart->devices + i;
  heart->device_open_list = g_slist_remove (heart->device_open_list, hdevice);
  g_free (hdevice->name);
  g_free (hdevice->isources);
  g_free (hdevice->osources);
  g_free (hdevice->ochunks);
  heart->n_devices--;
  if (i < heart->n_devices)
    heart->devices[i] = heart->devices[heart->n_devices];
  BSE_OBJECT_UNSET_FLAGS (pdev, BSE_DEVICE_FLAG_REGISTERED);
  bse_object_unref (BSE_OBJECT (pdev));
  bse_object_unref (BSE_OBJECT (heart));
}

BsePcmDevice*
bse_heart_get_device (const gchar *symbolic_name)
{
  BseHeart *heart = bse_heart_get_global (FALSE);

  if (heart && symbolic_name)
    {
      guint i;

      for (i = 0; i < heart->n_devices; i++)
	if (bse_string_equals (heart->devices[i].name, symbolic_name))
	  return heart->devices[i].device;
    }

  return NULL;
}

gchar*
bse_heart_get_device_name (BsePcmDevice *pdev)
{
  BseHeart *heart;

  g_return_val_if_fail (BSE_IS_PCM_DEVICE (pdev), NULL);
  g_return_val_if_fail (BSE_DEVICE_REGISTERED (pdev), NULL);

  heart = bse_heart_get_global (FALSE);
  if (heart)
    {
      guint i;

      for (i = 0; i < heart->n_devices; i++)
	if (heart->devices[i].device == pdev)
	  return heart->devices[i].name;
    }

  return NULL;
}

void
bse_heart_set_default_odevice (const gchar *symbolic_name)
{
  BseHeart *heart = bse_heart_get_global (FALSE);

  if (heart)
    {
      g_free (heart->default_odevice);
      heart->default_odevice = g_strdup (symbolic_name);
    }
}

void
bse_heart_set_default_idevice (const gchar *symbolic_name)
{
  BseHeart *heart = bse_heart_get_global (FALSE);

  if (heart)
    {
      g_free (heart->default_idevice);
      heart->default_idevice = g_strdup (symbolic_name);
    }
}

gchar*
bse_heart_get_default_odevice (void)
{
  BseHeart *heart = bse_heart_get_global (FALSE);

  return heart ? heart->default_odevice : NULL;
}

gchar*
bse_heart_get_default_idevice (void)
{
  BseHeart *heart = bse_heart_get_global (FALSE);

  return heart ? heart->default_idevice : NULL;
}

void
bse_heart_unregister_all_devices (void)
{
  BseHeart *heart = bse_heart_get_global (TRUE);

  if (heart->n_sources)
    g_warning ("BseHeart: can't unregister all devices while in playback mode");
  else while (heart->n_devices)
    bse_heart_unregister_device (heart->devices[0].device);
      
  bse_object_unref (BSE_OBJECT (heart));
}

void
bse_heart_reset_all_attach (void)
{
  BseHeart *heart = bse_heart_get_global (TRUE);
  
  while (heart->n_sources)
    bse_source_reset (heart->sources[0]);
      
  bse_object_unref (BSE_OBJECT (heart));
}

void
bse_heart_attach (BseSource *source)
{
  BseHeart *heart;
  guint i;

  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (!BSE_SOURCE_ATTACHED (source));
  g_return_if_fail (!BSE_SOURCE_IATTACHED (source));
  g_return_if_fail (!BSE_SOURCE_OATTACHED (source));

  heart = bse_heart_get_global (TRUE);
  bse_object_ref (BSE_OBJECT (source));
  i = heart->n_sources++;
  heart->sources = g_renew (BseSource*, heart->sources, 1 << g_bit_storage (heart->n_sources - 1));
  heart->sources[i] = source;
  BSE_OBJECT_SET_FLAGS (source, BSE_SOURCE_FLAG_ATTACHED);
}

void
bse_heart_detach (BseSource *source)
{
  BseHeart *heart;
  guint i;

  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_ATTACHED (source));

  heart = bse_heart_get_global (FALSE);
  
  /* we are nice here in doing the necessary search to remove
   * input/output device connections automatically, but it is
   * recommended to call bse_heart_source_remove_?device()
   * prior to calling this function
   */
  for (i = 0;
       (BSE_SOURCE_IATTACHED (source) || BSE_SOURCE_OATTACHED (source)) && i < heart->n_devices;
       i++)
    {
      BseHeartDevice *hdevice = heart->devices + i;
      guint j;
      
      if (BSE_SOURCE_IATTACHED (source))
	for (j = 0; j < hdevice->n_isources; j++)
	  if (hdevice->isources[j] == source)
	    {
	      bse_heart_source_remove_idevice (source, hdevice->device);
	      g_return_if_fail (!BSE_SOURCE_IATTACHED (source));
	      break;
	    }
      if (BSE_SOURCE_OATTACHED (source))
	for (j = 0; j < hdevice->n_osources; j++)
	  if (hdevice->osources[j] == source)
	    {
	      bse_heart_source_remove_odevice (source, hdevice->device);
	      g_return_if_fail (!BSE_SOURCE_OATTACHED (source));
	      break;
	    }
    }
  
  for (i = 0; i < heart->n_sources; i++)
    if (heart->sources[i] == source)
      break;
  g_return_if_fail (i < heart->n_sources);	/* paranoid */

  heart->n_sources--;
  if (i < heart->n_sources)
    heart->sources[i] = heart->sources[heart->n_sources];
  BSE_OBJECT_UNSET_FLAGS (source, BSE_SOURCE_FLAG_ATTACHED);
  bse_object_unref (BSE_OBJECT (source));
  bse_object_unref (BSE_OBJECT (heart));
}

void
bse_heart_source_add_idevice (BseSource    *source,
			      BsePcmDevice *idev)
{
  BseHeartDevice *hdevice = NULL;
  BseHeart *heart;
  guint i;
  
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_ATTACHED (source));
  g_return_if_fail (!BSE_SOURCE_IATTACHED (source));
  g_return_if_fail (BSE_IS_PCM_DEVICE (idev));
  g_return_if_fail (BSE_DEVICE_REGISTERED (idev));

  heart = bse_heart_get_global (FALSE);
  
  for (i = 0; i < heart->n_devices; i++)
    if (heart->devices[i].device == idev)
      hdevice = heart->devices + i;
  g_return_if_fail (hdevice != NULL);	/* paranoid */
  
  i = hdevice->n_isources++;
  hdevice->isources = g_renew (BseSource*, hdevice->isources, hdevice->n_isources);
  hdevice->isources[i] = source;
  BSE_OBJECT_SET_FLAGS (source, BSE_SOURCE_FLAG_IATTACHED);
  bse_heart_queue_device (heart, hdevice);
}

void
bse_heart_source_remove_idevice (BseSource    *source,
				 BsePcmDevice *idev)
{
  BseHeartDevice *hdevice = NULL;
  BseHeart *heart;
  guint i;
  
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_ATTACHED (source));
  g_return_if_fail (BSE_SOURCE_IATTACHED (source));
  g_return_if_fail (BSE_IS_PCM_DEVICE (idev));
  g_return_if_fail (BSE_DEVICE_REGISTERED (idev));

  heart = bse_heart_get_global (FALSE);
  
  for (i = 0; i < heart->n_devices; i++)
    if (heart->devices[i].device == idev)
      hdevice = heart->devices + i;
  g_return_if_fail (hdevice != NULL);	/* paranoid */
  
  for (i = 0; i < hdevice->n_isources; i++)
    if (hdevice->isources[i] == source)
      break;
  g_return_if_fail (i < hdevice->n_isources);	/* paranoid */

  hdevice->n_isources--;
  if (i < hdevice->n_isources)
    hdevice->isources[i] = hdevice->isources[hdevice->n_isources];
  BSE_OBJECT_UNSET_FLAGS (source, BSE_SOURCE_FLAG_IATTACHED);
  if (hdevice->n_isources == 0 && hdevice->n_osources == 0)
    bse_heart_queue_device (heart, hdevice);
}

void
bse_heart_source_add_odevice (BseSource    *source,
			      BsePcmDevice *odev)
{
  BseHeartDevice *hdevice = NULL;
  BseHeart *heart;
  guint i;
  
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_ATTACHED (source));
  g_return_if_fail (!BSE_SOURCE_OATTACHED (source));
  g_return_if_fail (BSE_IS_PCM_DEVICE (odev));
  g_return_if_fail (BSE_DEVICE_REGISTERED (odev));
  g_return_if_fail (BSE_SOURCE_N_OCHANNELS (source) >= BSE_DFL_OCHANNEL_ID);
  g_return_if_fail (BSE_SOURCE_OCHANNEL_DEF (source, BSE_DFL_OCHANNEL_ID)->n_tracks <= 2);

  heart = bse_heart_get_global (FALSE);
  
  for (i = 0; i < heart->n_devices; i++)
    if (heart->devices[i].device == odev)
      hdevice = heart->devices + i;
  g_return_if_fail (hdevice != NULL);	/* paranoid */
  
  i = hdevice->n_osources++;
  hdevice->osources = g_renew (BseSource*, hdevice->osources, hdevice->n_osources);
  hdevice->ochunks = g_renew (BseChunk*, hdevice->ochunks, hdevice->n_osources);
  hdevice->osources[i] = source;
  hdevice->ochunks[i] = NULL;
  BSE_OBJECT_SET_FLAGS (source, BSE_SOURCE_FLAG_OATTACHED);
  bse_heart_queue_device (heart, hdevice);
}

void
bse_heart_source_remove_odevice (BseSource    *source,
				 BsePcmDevice *odev)
{
  BseHeartDevice *hdevice = NULL;
  BseHeart *heart;
  guint i;
  
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_ATTACHED (source));
  g_return_if_fail (BSE_SOURCE_OATTACHED (source));
  g_return_if_fail (BSE_IS_PCM_DEVICE (odev));
  g_return_if_fail (BSE_DEVICE_REGISTERED (odev));

  heart = bse_heart_get_global (FALSE);
  
  for (i = 0; i < heart->n_devices; i++)
    if (heart->devices[i].device == odev)
      hdevice = heart->devices + i;
  g_return_if_fail (hdevice != NULL);	/* paranoid */
  
  for (i = 0; i < hdevice->n_osources; i++)
    if (hdevice->osources[i] == source)
      break;
  g_return_if_fail (i < hdevice->n_osources);	/* paranoid */

  if (hdevice->ochunks[i])
    bse_chunk_unref (hdevice->ochunks[i]);
  hdevice->n_osources--;
  if (i < hdevice->n_osources)
    {
      hdevice->osources[i] = hdevice->osources[hdevice->n_osources];
      hdevice->ochunks[i] = hdevice->ochunks[hdevice->n_osources];
    }
  BSE_OBJECT_UNSET_FLAGS (source, BSE_SOURCE_FLAG_OATTACHED);
  if (hdevice->n_isources == 0 && hdevice->n_osources == 0)
    bse_heart_queue_device (heart, hdevice);
}

static gboolean
device_open_handler (gpointer data)
{
  BseHeart *heart = BSE_HEART (data);

  while (heart->device_open_list)
    {
      GSList *slist = heart->device_open_list;
      BseHeartDevice *hdevice = slist->data;
      BsePcmDevice *pdev = hdevice->device;

      heart->device_open_list = slist->next;
      g_slist_free_1 (slist);

      if (!BSE_DEVICE_OPEN (pdev) && (hdevice->n_isources || hdevice->n_osources))
	{
	  BseErrorType error;

	  error = bse_pcm_device_update_caps (pdev);
	  if (!error)
	    error = bse_pcm_device_open (pdev,
					 hdevice->n_isources,
					 hdevice->n_osources,
					 2,
					 BSE_MIX_FREQ);
	  if (error)
	    g_warning ("failed to open PCM Device \"%s\": %s",
		       bse_device_get_device_name (BSE_DEVICE (pdev)),
		       bse_error_blurb (error));
	  if (BSE_DEVICE_OPEN (pdev))
	    {
	      bse_pcm_device_retrigger (pdev);
	      if (!heart->n_open_devices)
		heart->mix_buffer = g_new (BseMixValue, BSE_TRACK_LENGTH * BSE_MAX_N_TRACKS);
	      heart->n_open_devices++;
	    }
	}
      else if (BSE_DEVICE_OPEN (pdev) && !hdevice->n_isources && !hdevice->n_osources)
	{
	  bse_device_close (BSE_DEVICE (pdev));
	  heart->n_open_devices--;
	  if (!heart->n_open_devices)
	    {
	      g_free (heart->mix_buffer);
	      heart->mix_buffer = NULL;
	    }
	}
    }

  heart->device_open_handler_id = 0;

  return FALSE;
}

void
bse_heart_queue_device (BseHeart       *heart,
			BseHeartDevice *hdevice)
{
  g_return_if_fail (BSE_IS_HEART (heart));
  g_return_if_fail (hdevice != NULL);

  if (g_slist_find (heart->device_open_list, hdevice))
    return;

  heart->device_open_list = g_slist_prepend (heart->device_open_list, hdevice);
  if (!heart->device_open_handler_id)
    heart->device_open_handler_id = g_idle_add_full (BSE_DEVICE_PRIORITY,
						     device_open_handler,
						     heart,
						     NULL);
}

void
bse_heart_beat (BseHeart *heart)
{
  guint i;

  g_return_if_fail (BSE_IS_HEART (heart));

  /* no, sources may NOT be removed or added during cycling
   */

  /* turbo-cycle recently added sources */ /* FIXME */
  for (i = 0; i < heart->n_sources; i++)
    {
      BseSource *source = heart->sources[i];

      while (source->index < bse_heart_beat_index)
	bse_source_cycle (source);
    }

  bse_heart_beat_index++;
  
  for (i = 0; i < heart->n_sources; i++)
    bse_source_cycle (heart->sources[i]);
}

BseIndex
bse_heart_get_beat_index (void)
{
  return bse_heart_beat_index;
}

GSList* /* free and unref result */
bse_heart_collect_chunks (BseHeart       *heart,
			  BseHeartDevice *hdevice)
{
  GSList *slist = NULL;
  guint i;

  g_return_val_if_fail (BSE_IS_HEART (heart), NULL);
  g_return_val_if_fail (hdevice != NULL, NULL);

  for (i = 0; i < hdevice->n_osources; i++)
    slist = g_slist_prepend (slist, bse_source_ref_chunk (hdevice->osources[i],
							  BSE_DFL_OCHANNEL_ID,
							  bse_heart_beat_index));

  return slist;
}

BseChunk* /* unref result */
bse_heart_mix_chunks (BseHeart *heart,
		      GSList   *chunk_list, /* auto frees list and unrefs chunks */
		      guint     n_tracks)
{
  BseMixValue *mix_buffer, *bound, *mv;
  BseChunk *chunk;
  BseSampleValue *sv;
  GSList *slist;
  guint track_length = BSE_TRACK_LENGTH;

  g_return_val_if_fail (BSE_IS_HEART (heart), NULL);

  mix_buffer = heart->mix_buffer;
  memset (mix_buffer, 0, sizeof (BseMixValue) * n_tracks * track_length);
  bound = mix_buffer + track_length * n_tracks;

  for (slist = chunk_list; slist; slist = slist->next)
    {
      BseChunk *chunk = slist->data;

      sv = chunk->hunk;
      if (n_tracks == chunk->n_tracks)
	for (mv = mix_buffer; mv < bound; mv++)
	  *mv += *(sv++);
      else if (n_tracks == chunk->n_tracks * 2)
	for (mv = mix_buffer; mv < bound; mv++)
	  {
	    register BseMixValue v = *(sv++);

	    v += *(sv++);
	    *mv += v >> 2;
	  }
      else if (n_tracks * 2 == chunk->n_tracks)
	for (mv = mix_buffer; mv < bound; mv++)
	  {
	    *(mv++) += *sv;
	    *mv += *(sv++);
	  }
      else
	g_assert_not_reached ();
      bse_chunk_unref (chunk);
    }
  g_slist_free (chunk_list);

  chunk = bse_chunk_new (n_tracks);
  bse_hunk_clip_mix_buffer (n_tracks, chunk->hunk, 1.0, mix_buffer);
  chunk->hunk_filled = TRUE;

  return chunk;
}

static gboolean
bse_heart_prepare (gpointer  source_data,
		   GTimeVal *current_time,
		   gint     *timeout,
		   gpointer  user_data)
{
  BseHeart *heart = BSE_HEART (source_data);
  gulong msecs_wait = ~0;
  guint i;

  for (i = 0; i < heart->n_devices; i++)
    {
      BseHeartDevice *hdevice = heart->devices + i;
      BsePcmDevice *pdev = hdevice->device;

      if (BSE_DEVICE_OPEN (pdev)) /* hdevice->n_osources || hdevice->n_isources */
	{
	  guint msecs;

	  bse_pcm_device_time_warp (pdev);
	  msecs = bse_pcm_device_need_processing (pdev, heart->latency);
	  msecs_wait = MIN (msecs_wait, msecs);
	  if (msecs == 0)
	    break;
	}
    }

  BSE_IF_DEBUG (LOOP)
    g_message ("prepare, timeout=%ld", msecs_wait);

  if (msecs_wait < ~0)
    *timeout = msecs_wait;

  return !msecs_wait;
}

static gboolean
bse_heart_check (gpointer  source_data,
		 GTimeVal *current_time,
		 gpointer  user_data)
{
  BseHeart *heart = BSE_HEART (source_data);
  gboolean need_dispatch = FALSE;
  guint i;

  for (i = 0; i < heart->n_devices; i++)
    {
      BseHeartDevice *hdevice = heart->devices + i;
      BsePcmDevice *pdev = hdevice->device;

      if (BSE_DEVICE_OPEN (pdev)) /* hdevice->n_osources || hdevice->n_isources */
	{
	  bse_pcm_device_time_warp (pdev);
	  if (bse_pcm_device_need_processing (pdev, heart->latency) == 0)
	    {
	      need_dispatch = TRUE;
	      break;
	    }
	}
    }

  BSE_IF_DEBUG (LOOP)
    g_message ("check, need_dispatch=%d", need_dispatch);

  return need_dispatch;
}

static gboolean
bse_heart_dispatch (gpointer  source_data,
		    GTimeVal *current_time,
		    gpointer  user_data)
{
  BseHeart *heart = BSE_HEART (source_data);
  gboolean need_cycle = FALSE;
  guint i;
  
  BSE_IF_DEBUG (LOOP)
    g_message ("dispatching");
  
  for (i = 0; i < heart->n_devices; i++)
    {
      BseHeartDevice *hdevice = heart->devices + i;
      BsePcmDevice *pdev = hdevice->device;
      
      if (BSE_DEVICE_OPEN (pdev)) /* hdevice->n_osources || hdevice->n_isources */
	{
	  bse_pcm_device_time_warp (pdev);
	  need_cycle |= bse_pcm_device_process (pdev, heart->latency);
	}
    }
  
  if (need_cycle)
    {
      for (i = 0; i < heart->n_devices; i++)
	{
	  BseHeartDevice *hdevice = heart->devices + i;
	  BsePcmDevice *pdev = hdevice->device;
	  
	  if (BSE_DEVICE_READABLE (pdev) &&
	      bse_pcm_device_iqueue_peek (pdev) == NULL)
	    {
	      BseChunk *chunk = bse_chunk_new_static_zero (pdev->n_channels);
	      
	      g_message ("UNDERRUN detected for \"%s\", padding...\007",
			 bse_device_get_device_name (BSE_DEVICE (pdev)));
	      
              bse_pcm_device_retrigger (pdev);
	      bse_pcm_device_iqueue_push (pdev, chunk);
	      bse_chunk_unref (chunk);
	    }
	}
      
      bse_heart_beat (heart); /* pet shop action ;) */
      
      for (i = 0; i < heart->n_devices; i++)
	{
	  BseHeartDevice *hdevice = heart->devices + i;
	  BsePcmDevice *pdev = hdevice->device;
	  
	  if (BSE_DEVICE_WRITABLE (pdev) && hdevice->n_osources)
	    {
	      BseChunk *chunk;
	      GSList *slist = bse_heart_collect_chunks (heart, hdevice);
	      
	      /* FIXME: optimize here for 1 chunk with n_tracks == odev->n_channels */
	      chunk = bse_heart_mix_chunks (heart, slist, pdev->n_channels);
	      bse_pcm_device_oqueue_push (pdev, chunk);
	      bse_chunk_unref (chunk);
	    }
	}

      for (i = 0; i < heart->n_devices; i++)
	{
	  BseHeartDevice *hdevice = heart->devices + i;
	  BsePcmDevice *pdev = hdevice->device;
	  
	  if (BSE_DEVICE_READABLE (pdev))
	    bse_pcm_device_iqueue_pop (pdev);
	}
    }
  else
    {
      for (i = 0; i < heart->n_devices; i++)
	{
	  BseHeartDevice *hdevice = heart->devices + i;
	  BsePcmDevice *pdev = hdevice->device;
	  
	  if (BSE_DEVICE_READABLE (pdev) &&
	      pdev->iqueue && pdev->iqueue->next && pdev->iqueue->next->next) /* FIXME: msecs->chunks comparision */
	    {
	      g_message ("OVERRUN detected for \"%s\", skipping...\007",
			 bse_device_get_device_name (BSE_DEVICE (pdev)));
	      
	      while (pdev->iqueue->next)
		bse_pcm_device_iqueue_pop (pdev);
	    }
	}
    }
  
  return TRUE /* stay alive */;
}
