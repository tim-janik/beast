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


/* --- prototypes --- */
static void        bse_heart_init          (BseHeart      *heart);
static void        bse_heart_class_init    (BseHeartClass *class);
static void        bse_heart_shutdown      (BseObject     *object);
static void        bse_heart_destroy       (BseObject     *object);
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
  (GDestroyNotify) bse_object_unref
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
				   "Project container for administration "
				   "of Source networks",
				   &heart_info);
}

static void
bse_heart_class_init (BseHeartClass *class)
{
  BseObjectClass *object_class;

  parent_class = bse_type_class_peek (BSE_TYPE_OBJECT);
  object_class = BSE_OBJECT_CLASS (class);

  object_class->shutdown = bse_heart_shutdown;
  object_class->destroy = bse_heart_destroy;
}

static void
bse_heart_init (BseHeart *heart)
{
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

  g_message ("BseIndex: %lld", bse_heart_beat_index);
  bse_chunk_debug ();
}

BseHeart*
bse_heart_get_global (gboolean with_ref)
{
  if (!bse_global_heart)
    {
      if (with_ref)
	bse_global_heart = bse_object_new (BSE_TYPE_HEART, NULL);
    }
  else if (with_ref)
    bse_object_ref ((BseObject*) bse_global_heart);

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
  g_return_if_fail (!BSE_PCM_DEVICE_REGISTERED (pdev));
  g_return_if_fail (!BSE_PCM_DEVICE_OPEN (pdev));
  g_return_if_fail (symbolic_name != NULL);

  heart = bse_heart_get_global (TRUE);
  bse_object_ref (BSE_OBJECT (pdev));

  name = g_strdup (symbolic_name);
  while (bse_heart_get_device (name))
    {
      g_free (name);
      name = g_strdup_printf ("%s-%u", symbolic_name, ++i);
    }
  BSE_OBJECT_SET_FLAGS (pdev, BSE_PCM_FLAG_REGISTERED);
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
  g_return_if_fail (BSE_PCM_DEVICE_REGISTERED (pdev));
  g_return_if_fail (!BSE_PCM_DEVICE_OPEN (pdev));

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
  BSE_OBJECT_UNSET_FLAGS (pdev, BSE_PCM_FLAG_REGISTERED);
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
  g_return_val_if_fail (BSE_PCM_DEVICE_REGISTERED (pdev), NULL);

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
  g_return_if_fail (BSE_PCM_DEVICE_REGISTERED (idev));

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
  g_return_if_fail (BSE_PCM_DEVICE_REGISTERED (idev));

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
  g_return_if_fail (BSE_PCM_DEVICE_REGISTERED (odev));
  g_return_if_fail (BSE_SOURCE_N_OCHANNELS (source) == 1);
  g_return_if_fail (BSE_SOURCE_OCHANNEL_DEF (source, 1)->n_tracks <= 2);

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
  g_return_if_fail (BSE_PCM_DEVICE_REGISTERED (odev));

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

      if (!BSE_PCM_DEVICE_OPEN (pdev) && (hdevice->n_isources || hdevice->n_osources))
	{
	  BseErrorType error;

	  error = bse_pcm_device_update_caps (pdev);
	  if (!error)
	    error = bse_pcm_device_open (pdev,
					 hdevice->n_isources,
					 hdevice->n_osources,
					 2,
					 BSE_MIX_FREQ,
					 2 * BSE_TRACK_LENGTH * sizeof (BseSampleValue));
	  if (error)
	    g_warning ("failed to open PCM Device \"%s\": %s",
		       pdev->device_name,
		       bse_error_blurb (error));
	  if (BSE_PCM_DEVICE_OPEN (pdev))
	    {
	      bse_pcm_device_set_capture_cache (pdev,
						g_new0 (BseSampleValue,
							BSE_TRACK_LENGTH * pdev->n_channels),
						g_free);
	      if (!heart->n_open_devices)
		heart->mix_buffer = g_new (BseMixValue, BSE_TRACK_LENGTH * BSE_MAX_N_TRACKS);
	      heart->n_open_devices++;
	    }
	}
      else if (BSE_PCM_DEVICE_OPEN (pdev) && !hdevice->n_isources && !hdevice->n_osources)
	{
	  bse_pcm_device_close (pdev);
	  bse_pcm_device_set_capture_cache (pdev, NULL, NULL);
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
    heart->device_open_handler_id = g_idle_add_full (BSE_HEART_PRIORITY - 1,
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

  /* turbo-cycle recently added sources */
  for (i = 0; i < heart->n_sources; i++) /* FIXME */
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
							  1,
							  bse_heart_beat_index));

  return slist;
}

BseSampleValue*
bse_heart_mix_chunks (BseHeart *heart,
		      GSList   *chunk_list,
		      guint     n_tracks)
{
  BseMixValue *mb, *mbe, *mv;
  BseSampleValue *sv;
  GSList *slist;
  guint track_length = BSE_TRACK_LENGTH;

  g_return_val_if_fail (BSE_IS_HEART (heart), NULL);

  mb = heart->mix_buffer;
  memset (mb, 0, sizeof (BseMixValue) * n_tracks * track_length);
  mbe = mb + track_length * n_tracks;

  for (slist = chunk_list; slist; slist = slist->next)
    {
      BseChunk *chunk = slist->data;

      sv = chunk->hunk;
      if (n_tracks == chunk->n_tracks)
	for (mv = mb; mv < mbe; mv++)
	  *mv += *(sv++);
      else if (n_tracks == chunk->n_tracks * 2)
	for (mv = mb; mv < mbe; mv++)
	  {
	    register BseMixValue v = *(sv++);

	    v += *(sv++);
	    *mv += v >> 2;
	  }
      else if (n_tracks * 2 == chunk->n_tracks)
	for (mv = mb; mv < mbe; mv++)
	  {
	    *(mv++) += *sv;
	    *mv += *(sv++);
	  }
      else
	g_assert_not_reached ();
    }

  sv = (BseSampleValue*) mb;
  for (mv = mb; mv < mbe; mv++)
    {
      register BseMixValue v = *mv;

      if (v > 32767)
	v = 32767;
      else if (v < -32768)
	v = -32768;
      *(sv++) = v;
    }

  return (BseSampleValue*) mb;
}

static gboolean
bse_heart_prepare (gpointer  source_data,
		   GTimeVal *current_time,
		   gint     *timeout,
		   gpointer  user_data)
{
  BseHeart *heart = BSE_HEART (source_data);
  gboolean can_dispatch;
  guint i;

  can_dispatch = heart->n_open_devices > 0;

  for (i = 0; i < heart->n_devices; i++)
    {
      BseHeartDevice *hdevice = heart->devices + i;
      BsePcmDevice *pdev = hdevice->device;

      if (hdevice->n_osources && BSE_PCM_DEVICE_WRITABLE (pdev))
	{
	  if (!bse_pcm_device_oready (pdev, BSE_TRACK_LENGTH * pdev->n_channels))
	    {
	      can_dispatch = FALSE;
	      pdev->pfd.events |= G_IO_OUT;
	    }
	  else
	    pdev->pfd.events &= ~G_IO_OUT;
	}
      if (hdevice->n_isources && BSE_PCM_DEVICE_READABLE (pdev))
	{
	  if (!bse_pcm_device_iready (pdev, BSE_TRACK_LENGTH * pdev->n_channels))
	    {
	      can_dispatch = FALSE;
	      pdev->pfd.events |= G_IO_IN;
	    }
	  else
	    pdev->pfd.events &= ~G_IO_IN;
	}
    }

  return can_dispatch;
}

static gboolean
bse_heart_check (gpointer  source_data,
		 GTimeVal *current_time,
		 gpointer  user_data)
{
  BseHeart *heart = BSE_HEART (source_data);
  gboolean can_dispatch;
  guint i;

  can_dispatch = heart->n_open_devices > 0;

  for (i = 0; i < heart->n_devices; i++)
    {
      BseHeartDevice *hdevice = heart->devices + i;
      BsePcmDevice *pdev = hdevice->device;

      if (BSE_PCM_DEVICE_WRITABLE (pdev) && pdev->pfd.events & G_IO_OUT)
	{
	  if (pdev->pfd.revents & G_IO_OUT)
	    {
	      pdev->pfd.events &= ~G_IO_OUT;
	      pdev->pfd.revents &= ~G_IO_OUT;
	    }
	  else
	    can_dispatch = FALSE;
	}
      if (BSE_PCM_DEVICE_READABLE (pdev) && pdev->pfd.events & G_IO_IN)
	{
	  if (pdev->pfd.revents & G_IO_IN)
	    {
	      pdev->pfd.events &= ~G_IO_IN;
	      pdev->pfd.revents &= ~G_IO_IN;
	    }
	  else
	    can_dispatch = FALSE;
	}
    }
  
  return can_dispatch;
}

static gboolean
bse_heart_dispatch (gpointer  source_data,
		    GTimeVal *current_time,
		    gpointer  user_data)
{
  BseHeart *heart = BSE_HEART (source_data);
  guint i;

  for (i = 0; i < heart->n_devices; i++)
    {
      BseHeartDevice *hdevice = heart->devices + i;
      BsePcmDevice *pdev = hdevice->device;

      if (BSE_PCM_DEVICE_READABLE (pdev))
	bse_pcm_device_read (pdev, BSE_TRACK_LENGTH * pdev->n_channels, pdev->capture_cache);
    }

  bse_heart_beat (heart); /* pet shop action ;) */

  for (i = 0; i < heart->n_devices; i++)
    {
      BseHeartDevice *hdevice = heart->devices + i;
      BsePcmDevice *pdev = hdevice->device;

      if (BSE_PCM_DEVICE_WRITABLE (pdev) && hdevice->n_osources)
	{
	  BseSampleValue *obuf;
	  GSList *node, *slist = bse_heart_collect_chunks (heart, hdevice);

	  /* FIXME: optimize here for 1 chunk with n_tracks == odev->n_channels */
	  obuf = bse_heart_mix_chunks (heart, slist, pdev->n_channels);
	  bse_pcm_device_write (pdev, BSE_TRACK_LENGTH * pdev->n_channels, obuf);
	  for (node = slist; node; node = node->next)
	    bse_chunk_unref (node->data);
	  g_slist_free (slist);
	}
    }

  return TRUE /* stay alive */;
}
