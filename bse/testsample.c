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
main (int   argc,
      char *argv[])
{
#if 0
  BseParserData *pdata;
  gchar *file_name;
  GSList *slist;
  BseSample *sample;
  BseStream *stream;
  BseErrorType error;
  
  bse_init (&argc, &argv);

  if (argc < 2)
    g_error ("need a file argument");

  /* parse a sample
   */
  file_name = "testsample.bse";
  file_name = "testbolingo.bse";
  file_name = argv[1];
  error = bse_parser_identify_file (file_name, NULL);
  if (error)
    g_error ("%s: not a bse file: %s", file_name, bse_error_blurb (error));
  pdata = bse_parser_new (file_name);
  if (!pdata)
    g_error ("couldn't setup parser for \"%s\"", file_name);
  if (bse_parser_parse_objects (pdata) || bse_parser_read_data (pdata))
    g_message ("parsing: %s", bse_error_blurb (pdata->last_error));

  printf ("\nparsed:");
  for (slist = pdata->supers; slist; slist = slist->next)
    printf (" %s", BSE_OBJECT_NAME (slist->data));
  printf ("\n");
  sample = pdata->supers->data;
  bse_object_ref (BSE_OBJECT (sample));
  bse_sample_fillup_munks (sample);
    
  bse_parser_destroy (pdata);


  /* play sample
   */
  if (!bse_pcm_stream_default_type ())
    {
      g_warning ("No PCM Streams available on this platform");
      return -1;
    }
  /* PCM streams provide a default device name, when NULL is passed in */
  stream = bse_pcm_stream_new (bse_pcm_stream_default_type (), NULL);
  error = bse_stream_open (stream, FALSE, TRUE);
  if (!error)
    {
      BsePcmStreamAttribs attributes;
      BsePcmStreamAttribMask attribute_mask = BSE_PCMSA_NONE;

      attributes.n_channels = 1;
      attributes.play_frequency = sample->recording_freq;
      attributes.fragment_size = 32;
      attribute_mask |= BSE_PCMSA_N_CHANNELS | BSE_PCMSA_PLAY_FREQUENCY | BSE_PCMSA_FRAGMENT_SIZE;

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
  
  bse_stream_start (stream);
x:  {
    BseSampleValue *v = ((BseBinData*) sample->munk[0].sample_data)->values;
    gint left = ((BseBinData*) sample->munk[0].sample_data)->n_values;
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
  }
  goto x;
  bse_object_unref (BSE_OBJECT (stream));
  bse_object_unref (BSE_OBJECT (sample));

#endif

  return 0;
}
