/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bse.h"


int
main (int argc, char *argv[])
{
  BseErrorType error;
  BseSample *sample;
  gchar *f_name;
  BseStream *stream;


  /* BSE Initialization
   */
  bse_init (&argc, &argv);

  /* Setup output stream, we use the soundcard (PCM).
   */
  if (!bse_pcm_stream_default_type ())
    {
      g_warning ("No PCM Streams available on this platform");
      return -1;
    }
  /* PCM streams provide a default device name, when NULL is passed in */
  stream = bse_pcm_stream_new (bse_pcm_stream_default_type (), NULL);
  
  /* Read samples
   */
  f_name = "../sample.bsw";
  f_name = "./test.bsw";
  f_name = "./song.bss";
  f_name = "./bolingo-cut.bse";
  f_name = "/samples/bolingo.bse";
  printf ("start reading...\n");
#if 0
  {
    BseIoData *io_data;
    io_data = bse_io_load_auto (f_name, BSE_IO_DATA_SAMPLES | BSE_IO_DATA_BINARY_APPENDIX);
    if (io_data->error)
      printf ("reading \"%s\" failed: %s\n", f_name, bse_error_blurb (io_data->error));
    else
      printf ("no errors encountered loading \"%s\"\n", f_name);
    
    error = bse_load_wav ("/public/sound/wav/koks.wav", &sample);
    error = BSE_ERROR_UNIMPLEMENTED;
    if (error)
      {
	g_print ("WAVE file loading failed: %s\n", bse_error_blurb (error));
	return -1;
      }
    // bse_io_data_destroy (io_data);
  }
#endif

  /* Open output stream
   */
  error = bse_stream_open (stream, FALSE, TRUE);
  if (!error)
    {
      BsePcmStreamAttribs attributes;
      BsePcmStreamAttribMask attribute_mask = BSE_PCMSA_NONE;
      
      attributes.n_channels = 1;
      attributes.play_frequency = 22150;
      attributes.play_frequency = 44100;
      attribute_mask |= BSE_PCMSA_N_CHANNELS | BSE_PCMSA_PLAY_FREQUENCY;
      
      error = bse_pcm_stream_set_attribs (BSE_PCM_STREAM (stream), attribute_mask, &attributes);
      if (!error)
	{
	  g_print ("Using %s stream \"%s\" with %uHz in %u channel mode\n",
		   BSE_OBJECT_TYPE_NAME (stream),
		   stream->file_name,
		   attributes.play_frequency,
		   attributes.n_channels);
	  g_print ("output buffer size: %d\n", bse_globals->pcm_buffer_size);
	}
    }
  if (error)
    {
      g_warning ("Opening PCM stream `%s' failed: %s\n",
		 stream->file_name,
		 bse_error_blurb (error));
      return -1;
    }



  sample = bse_sample_lookup (NULL, "bolingo");
  g_assert (sample != NULL);

  g_print ("Sample to play: blurb-\"%s\" n_channels(%d) rec_freq(%d)\n",
	   bse_object_get_blurb (BSE_OBJECT (sample)),
	   sample->n_tracks,
	   sample->rec_freq);
  
  bse_stream_start (stream);

  FIXME ({
    BseSampleValue *v = sample->munk[0].values;
    gint left = sample->munk[0].n_values;
    gint n;
    
    n = BSE_PCM_STREAM (stream)->attribs.fragment_size * 2;
    
    if (n > left)
      n = left;
    while (left > 0)
      {
	if (bse_stream_would_block (stream, n))
	  {
	    do
	      {
		printf ("waiting...\n");
		usleep (100 * 1000);
	      }
	    while (bse_stream_would_block (stream, n));
	  }
	bse_stream_write_sv (stream, n, v);
	
	v += n;
	left -= n;
	if (n > left)
	  n = left;
      }
  });

  bse_object_unref (BSE_OBJECT (stream));

  return 0;
}

/*
x()
{
  gboolean break = FALSE;
  
  bse_song_play_init(song);
  while (bse_song_elapses() && !break)
  {
    while (!bse_pcm_would_block())
    {
      BseSampleValue *values;
      
      bse_fill_buffer (n_values, values);
      bse_pcm_play(n_values, values);
    }
    
  }
  bse_song_play_shutdown();
}
*/
