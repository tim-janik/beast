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
#include	"bsepcmdevice-oss.h"


int
main (int argc, char *argv[])
{
  BseErrorType error;
  BseSample *sample;
  gchar *f_name, *blurb;
  BsePcmDevice *pdev;


  /* BSE Initialization
   */
  bse_init (&argc, &argv);

  /* Setup output pcm device, we use the OSS PCM Driver.
   */
  pdev = (BsePcmDevice*) bse_object_new (BSE_TYPE_PCM_DEVICE_OSS, NULL);
  
  /* Read samples
   */
  f_name = "../sample.bsw";
  f_name = "./test.bsw";
  f_name = "./song.bss";
  f_name = "./bolingo-cut.bse";
  f_name = "/samples/bolingo.bse";
  g_print ("start reading...\n");
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
  
  sample = bse_sample_lookup (NULL, "bolingo");
  g_assert (sample != NULL);
  
  g_object_get (G_OBJECT (sample), "blurb", &blurb, NULL);
  g_print ("Sample to play: blurb-\"%s\" n_channels(%d) rec_freq(%d)\n",
	   blurb,
	   sample->n_tracks,
	   sample->rec_freq);
  
  /* Open output stream
   */
  error = bse_pcm_device_open (pdev, FALSE, TRUE, 2, 44100);
  if (error)
    {
      g_warning ("Opening %s \"%s\" failed: %s\n",
		 BSE_OBJECT_TYPE_NAME (pdev),
		 bse_device_get_device_name (BSE_DEVICE (pdev)),
		 bse_error_blurb (error));
      return -1;
    }
  else
    {
      g_print ("Using %s \"%s\" with %uHz in %u channel mode\n",
	       BSE_OBJECT_TYPE_NAME (pdev),
	       bse_device_get_device_name (BSE_DEVICE (pdev)),
	       (guint) pdev->sample_freq,
	       pdev->n_channels);
    }
  
  FIXME ({
    BseSampleValue *v = sample->munk[0].values;
    gint left = sample->munk[0].n_values;
    gint n;
    
    n = pdev->fragment_size * 2;
    
    if (n > left)
      n = left;
    while (left > 0)
      {
	if (!bse_pcm_device_oready (pdev, n))
	  {
	    do
	      {
		printf ("waiting...\n");
		usleep (100 * 1000);
	      }
	    while (!bse_pcm_device_oready (pdev, n));
	  }
	bse_pcm_device_write (stream, n, v);
	
	v += n;
	left -= n;
	if (n > left)
	  n = left;
      }
  });

  bse_device_close (BSE_DEVICE (pdev));
  bse_object_unref (BSE_OBJECT (pdev));

  return 0;
}
