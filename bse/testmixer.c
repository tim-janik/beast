#if 0
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
#include	<stdio.h>
#include	"bse.h"

static gint     note_pat[64][3] =
{
  { 0, BSE_NOTE_C(0), BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { BSE_NOTE_C(1), 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { 0, 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { BSE_NOTE_C(1), 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { 0, 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { BSE_NOTE_C(1), 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { 0, 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { BSE_NOTE_C(1), 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0), BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { BSE_NOTE_C(1), 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { 0, 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { BSE_NOTE_C(1), 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { 0, 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { BSE_NOTE_C(1), 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { 0, 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),0 },
  { 0, 0, 0 },
  { BSE_NOTE_C(1), 0, BSE_NOTE_C(0) },
  { 0, 0, 0 },
  { 0, BSE_NOTE_C(0),BSE_NOTE_C(0) },
  { 0, 0, BSE_NOTE_C(0) },
};

static gchar *pre_load_sample[] =
{
  "/samples/olaf/bass/synthbass2.bse",
  "/samples/olaf/bassdrum/909kick1.bse",
  "/samples/olaf/solos/pizzicato_2080.bse",
  "/samples/olaf/clap/909clap.bse",
  "/samples/olaf/string/junostring.bse",
  "/samples/olaf/bass/acidtb.bse",
};
static guint n_pre_load_sample = sizeof (pre_load_sample) / sizeof (pre_load_sample[0]);

int
main (int argc, char *argv[])
{
  BseErrorType error;
  BseStream *stream;
  register guint n_voices = 4;
  BseNote  note[n_voices];
  BseInstrument *instr1;
  BseInstrument *instr2;
  BseInstrument *instr3;
  BseSong *song;
  BseSample *sample1;
  BseSample *sample2;
  BseSample *sample3;
  BseVoice *voices;
  BseMixValue *mix_buffer;
  register guint i;
  gint     row, count;
  
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
  /* Open output stream
   */
  error = bse_stream_open (stream, FALSE, TRUE);
  if (!error)
    {
      BsePcmStreamAttribs attributes;
      BsePcmStreamAttribMask attribute_mask = BSE_PCMSA_NONE;

      attributes.n_channels = 2;
      attributes.play_frequency = 4800;
      attribute_mask |= BSE_PCMSA_N_CHANNELS | BSE_PCMSA_PLAY_FREQUENCY;

      error = bse_pcm_stream_set_attribs (BSE_PCM_STREAM (stream), attribute_mask, &attributes);
      if (!error)
	{
	  g_print ("Using %s stream \"%s\" with %uHz in %u channel mode\n",
		   BSE_OBJECT_TYPE_NAME (stream),
		   stream->name,
		   attributes.play_frequency,
		   attributes.n_channels);
	  g_print ("output buffer size: %d\n", bse_globals->pcm_buffer_size);
	}
    }
  if (error)
    {
      g_warning ("Opening PCM stream `%s' failed: %s\n",
		 stream->name,
		 bse_error_blurb (error));
      return -1;
    }
  bse_mixer_set_mix_freq (BSE_PCM_STREAM (stream)->attribs.play_frequency);

  for (i = 0; i < n_pre_load_sample; i++)
    {
      BseIoData *io_data;

      FIXME(io_data = bse_io_load_auto (pre_load_sample[i], BSE_IO_DATA_SAMPLES | BSE_IO_DATA_BINARY_APPENDIX););
      if (io_data->error)
	{
	  printf ("failed to load sample \"%s\": %s\n",
		  pre_load_sample[i],
		  bse_error_blurb (io_data->error));
	  return -1;
	}
      bse_object_ref (io_data->samples->data);
      bse_io_data_destroy (io_data);
    }

  // sample1 = bse_sample_lookup( "junostring.aiff" );
  sample3 = bse_sample_lookup ("", "909kick1");

  sample2 = bse_sample_lookup ("", "acidtb");
  sample2 = bse_sample_lookup ("", "boling");
  sample2 = bse_sample_lookup ("", "synthbass2");

  sample1 = bse_sample_lookup ("", "909clap");

  g_assert (sample1);
  g_assert (sample2);
  g_assert (sample3);
  
  sample1->type = BSE_SAMPLE_EFFECT_MUNKS;
  sample2->type = BSE_SAMPLE_EFFECT_MUNKS;
  sample3->type = BSE_SAMPLE_EFFECT_MUNKS;
  
  song = bse_song_new (NULL, n_voices);
  //  instr3 = bse_song_sample_instrument_new (song, sample3);
  //  instr2 = bse_song_sample_instrument_new (song, sample2);
  //  instr1 = bse_song_sample_instrument_new (song, sample1);
FIXME(need the above); exit(1);

  //  instr1->volume = 40;
  //  instr2->volume = 40;
  //  instr3->volume = 40;
  voices = bse_voice_block_alloc (n_voices);

  note[0].note = BSE_NOTE_C(1);
  note[0].instrument = instr1;
  note[0].effects = NULL;
  note[1].note = BSE_NOTE_G(1);
  note[1].instrument = instr2;
  note[1].effects = NULL;
  note[2].note = BSE_NOTE_C(2);
  note[2].instrument = instr3;
  note[2].effects = NULL;

  bse_mixer_init (1);
  mix_buffer = bse_mixer_get_buffer (0);

  bse_stream_start (stream);

  count = 0;
  row = -1;
  while (1)
  {
    BseSampleValue obuffer[4096];
    guint i;

    ++count;
    count %= 10;
    if ( count == 0 )
      {
      ++row;
      row %= 64;
      for ( i = 0;i < 3; ++i )
        {
        if ( note_pat[row][i] )
          {
      	  note[i].note = note_pat[row][i];

          bse_mixer_activate_voice( &voices[i], &note[i]);
          }
        if ( note_pat[row][i] == -1 )
	  {
	    bse_voice_reset( &voices[i]);
	  }
        }
      }

    bse_mixer_fill_buffer( mix_buffer , n_voices, voices);

    /* konvertierung von 32 -> 16 bit */

    memset (obuffer, 0, 4096* sizeof(BseSampleValue));
    
    for (i = 0; i < bse_mixer_get_n_buffer_values (); i++)
      {
	register BseMixValue mix_v;

	mix_v = mix_buffer[i];
	// clipping
	if ( mix_v > 32767 )
	  {
	    mix_v = 0x7fff;
	  }
	else if ( mix_v < -32768 )
	  {
	    mix_v = 0x8000;
	  }
	obuffer[i] = mix_v;
      }
    bse_stream_write_sv (stream, bse_mixer_get_n_buffer_values (), obuffer);
    
    {
      FILE *file;
      
      file = NULL;
      //    file = fopen( "out.raw", "w" );
      if ( file )
	{
	  fwrite( mix_buffer, 800*4, 1, file );
	  fclose( file );
	}
    }
  }
  
  bse_stream_stop (stream);

  bse_object_unref (BSE_OBJECT (stream));

  return 0;
}
#endif
void main () {}
