/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bstmaster.h"

#define	N_TRACKS 2 // FIXME: hack


/* --- prototypes --- */
static gboolean bst_master_prepare  (gpointer  source_data,
				     GTimeVal *current_time,
				     gint     *timeout,
				     gpointer  user_data);
static gboolean bst_master_check    (gpointer  source_data,
				     GTimeVal *current_time,
				     gpointer  user_data);
static gboolean bst_master_dispatch (gpointer  source_data,
				     GTimeVal *current_time,
				     gpointer  user_data);


/* --- variables --- */
static BstMaster	*bst_master = NULL;
static GSourceFuncs master_funcs = {
  bst_master_prepare,
  bst_master_check,
  bst_master_dispatch,
  (GDestroyNotify) bse_object_unref
};


/* --- functions --- */
static void
bst_destroy_poll_fd (gpointer data)
{
  GPollFD *pfd = data;

  g_main_remove_poll (pfd);
  g_free (pfd);
}

gchar*
bst_master_init (void)
{
  BseMaster *master;
  BseStream *stream;
  BseErrorType error;
  GPollFD *pfd;
  
  g_return_val_if_fail (bst_master == NULL, NULL);

  if (!bse_pcm_stream_default_type ())
    return g_strdup ("No PCM Streams available on this platform");

  /* PCM streams provide a default device name, when NULL is passed in */
  stream = bse_pcm_stream_new (bse_pcm_stream_default_type (), NULL);
  error = bse_stream_open (stream, FALSE, TRUE);

  /* setup output stream attributes */
  if (!error)
    {
      BsePcmStreamAttribs attributes;
      BsePcmStreamAttribMask attribute_mask = BSE_PCMSA_NONE;

      attributes.n_channels = N_TRACKS;
      attributes.play_frequency = BSE_MIX_FREQ;
      attribute_mask |= BSE_PCMSA_N_CHANNELS | BSE_PCMSA_PLAY_FREQUENCY;

      error = bse_pcm_stream_set_attribs (BSE_PCM_STREAM (stream), attribute_mask, &attributes);
      if (!error &&
	  attributes.play_frequency == BSE_MIX_FREQ &&
	  attributes.n_channels == N_TRACKS)
	{
	  g_print ("Using %s stream \"%s\" with %uHz in %u channel mode\n",
		   BSE_OBJECT_TYPE_NAME (stream),
		   stream->file_name,
		   attributes.play_frequency,
		   attributes.n_channels);
	  g_print ("output buffer size: %d\n", bse_globals->pcm_buffer_size);
	  g_print ("mix buffer size: %d\n", bse_globals->track_length);
	}
      else if (!error)
	error = BSE_ERROR_STREAM_SET_ATTRIB;
    }

  if (error)
    {
      gchar *string;

      string = g_strdup_printf ("Opening PCM stream `%s' failed: %s\n",
				stream->file_name,
				bse_error_blurb (error));
      bse_object_unref (BSE_OBJECT (stream));

      return string;
    }

  master = bse_master_new (stream, N_TRACKS);

  pfd = g_new0 (GPollFD, 1);
  pfd->fd = bse_pcm_stream_oss_get_unix_fd (BSE_PCM_STREAM_OSS (stream));
  g_main_add_poll (pfd, BST_MASTER_PRIORITY);
  g_source_add (BST_MASTER_PRIORITY, TRUE, &master_funcs, master, pfd, bst_destroy_poll_fd);
  
  bst_master = master;
  bse_object_add_data_notifier (master, "destroy", bse_nullify, &bst_master);
  
  bse_stream_start (stream);
  bse_object_unref (BSE_OBJECT (stream));

  return NULL;
}

void
bst_master_shutdown (void)
{
  g_return_if_fail (bst_master != NULL);

  g_source_remove_by_source_data (BSE_MASTER (bst_master));
  if (bst_master)
    g_warning ("BstMaster still in use");
}

BstMaster*
bst_master_ref (void)
{
  g_return_val_if_fail (bst_master != NULL, NULL);

  bse_object_ref (BSE_OBJECT (bst_master));

  return bst_master;
}

void
bst_master_unref (BstMaster *master)
{
  BseObject *master_object = (BseObject*) master;

  g_return_if_fail (master != NULL);
  g_return_if_fail (master == bst_master);
  g_return_if_fail (master_object->ref_count > 1);

  bse_object_unref (BSE_OBJECT (master));
}

static gboolean
bst_master_prepare (gpointer  source_data,
		    GTimeVal *current_time,
		    gint     *timeout,
		    gpointer  user_data)
{
  GPollFD *pfd = user_data;
  BseMaster *master;
  gboolean need_dispatch;

  master = BSE_MASTER (source_data);

  *timeout = -1;

  /* figure whether we need immediate dispatching */
  need_dispatch = (BSE_SOURCE (master)->n_inputs &&
		   !bse_stream_would_block (master->stream,
					    (sizeof (BseSampleValue) *
					     bse_globals->track_length *
					     N_TRACKS)));

  /* don't even bother selecting on the fd if there's
   * no data to write (BSE_SOURCE (master)->n_inputs == 0) or
   * we already know that we need further dispatching (in which
   * case *timeout is set to 0 automatically).
   */
  if (BSE_SOURCE (master)->n_inputs && !need_dispatch)
    pfd->events = G_IO_IN | G_IO_OUT | G_IO_PRI;
  else
    pfd->events = 0;
      
  return need_dispatch;
}

static gboolean
bst_master_check (gpointer  source_data,
		  GTimeVal *current_time,
		  gpointer  user_data)
{
  GPollFD *pfd = user_data;
  BseMaster *master;
  gboolean need_dispatch;
  
  master = BSE_MASTER (source_data);

  need_dispatch = (pfd->revents & G_IO_OUT) != 0;

  return need_dispatch;
}

static gboolean
bst_master_dispatch (gpointer  source_data,
		     GTimeVal *current_time,
		     gpointer  user_data)
{
  BseMaster *master;

  master = BSE_MASTER (source_data);

  bse_masters_cycle ();
  while (BSE_SOURCE (master)->n_inputs &&
	 !bse_stream_would_block (master->stream,
				  (sizeof (BseSampleValue) *
				   bse_globals->track_length *
				   N_TRACKS)))
    bse_masters_cycle ();

  return TRUE /* stay alive */;
}
