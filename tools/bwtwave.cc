/* BseWaveTool - BSE Wave manipulation tool             -*-mode: c++;-*-
 * Copyright (C) 2001-2004 Tim Janik
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
#include "bwtwave.h"
#include <bse/bsemath.h>
#include <bse/gsldatautils.h>
#include <bse/gslloader.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <algorithm>
#include <vector>


namespace BseWaveTool {

WaveChunk::WaveChunk ()
{
  dhandle = NULL;
  midi_note = 0;
}

WaveChunk&
WaveChunk::operator= (const WaveChunk &rhs)
{
  memcpy (this, &rhs, sizeof (*this));
  if (dhandle)
    gsl_data_handle_open (dhandle);
  return *this;
}

WaveChunk::WaveChunk (const WaveChunk &rhs)
{
  *this = rhs;
  if (dhandle)
    gsl_data_handle_open (dhandle);
}

WaveChunk::~WaveChunk ()
{
  if (dhandle)
    gsl_data_handle_close (dhandle);
}

Wave::Wave (const gchar    *wave_name,
            guint           n_ch) :
  n_channels (n_ch),
  name (wave_name)
{
}

void
Wave::add_chunk (GslDataHandle  *dhandle)
{
  g_return_if_fail (dhandle != NULL);

  BseErrorType error = gsl_data_handle_open (dhandle);
  if (!error)
    {
      WaveChunk wc;
      wc.dhandle = dhandle;
      chunks.push_front (wc);
    }
}

void
Wave::remove (list<WaveChunk>::iterator it)
{
  chunks.erase (it);
}

void
Wave::sort ()
{
  /* brrrr, lists aren't sortable due to iterator issues */
  struct Sub {
    static float
    chunk_cmp (const WaveChunk &wc1,
               const WaveChunk &wc2)
    {
      return wc1.dhandle->setup.osc_freq - wc2.dhandle->setup.osc_freq;
    }
  };
#if 0 /* brrrr, lists aren't sortable due to iterator issues */
  std::sort (chunks.begin(), chunks.end(), Sub::chunk_cmp);
#else
  vector<WaveChunk> vwc;
  vwc.assign (chunks.begin(), chunks.end());
  stable_sort (vwc.begin(), vwc.end(), Sub::chunk_cmp);
  chunks.assign (vwc.begin(), vwc.end());
#endif
}

BseErrorType
Wave::store (const gchar *file_name)
{
  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  gint fd = open (file_name, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0)
    return bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);

  SfiWStore *wstore = sfi_wstore_new ();
  wstore->comment_start = '#';
  sfi_wstore_puts (wstore, "#BseWave\n\n");
  sfi_wstore_puts (wstore, "wave {\n");
  gchar *str = g_strescape (name.c_str(), NULL);
  sfi_wstore_printf (wstore, "  name = \"%s\"\n", str);
  g_free (str);
  sfi_wstore_printf (wstore, "  byte_order = %s\n", gsl_byte_order_to_string (BYTE_ORDER));

  for (list<WaveChunk>::iterator it = chunks.begin(); it != chunks.end(); it++)
    {
      WaveChunk *chunk = &*it;
      sfi_wstore_puts (wstore, "  chunk {\n");
      int midi_note = bse_xinfos_get_num (chunk->dhandle->setup.xinfos, "midi-note");
      if (midi_note)
        sfi_wstore_printf (wstore, "    midi_note = %u\n", midi_note);
      else
        sfi_wstore_printf (wstore, "    osc_freq = %.3f\n", chunk->dhandle->setup.osc_freq);

#if 0
      if (chunk->oggname)
        {
          sfi_wstore_puts (wstore, "    ogglink = ");
          if (chunk->loop_type)
            {
              OggStoreContext *oc = g_new0 (OggStoreContext, 1);
              oc->fd = -1;
              oc->oggfile = g_strdup (chunk->oggname);
              oc->cutter = gsl_vorbis_cutter_new ();
              gsl_vorbis_cutter_set_cutpoint (oc->cutter, chunk->loop_end + 1, GSL_VORBIS_CUTTER_PACKET_BOUNDARY);
              sfi_wstore_put_binary (wstore, ogg_store_context_reader, oc, ogg_store_context_destroy);
            }
          else
            {
              TmpStoreContext *tc = g_new0 (TmpStoreContext, 1);
              tc->fd = -1;
              tc->tmpfile = g_strdup (chunk->oggname);
              sfi_wstore_put_binary (wstore, tmp_store_context_reader, tc, tmp_store_context_destroy);
            }
          sfi_wstore_puts (wstore, "\n");
        }
      else
#endif
        {
          sfi_wstore_puts (wstore, "    rawlink = ");
          gsl_data_handle_dump_wstore (chunk->dhandle, wstore, GSL_WAVE_FORMAT_SIGNED_16, G_LITTLE_ENDIAN);
          sfi_wstore_puts (wstore, "\n");
          sfi_wstore_printf (wstore, "    mix_freq = %.3f\n", chunk->dhandle->setup.mix_freq);
        }

      if (chunk->dhandle->setup.xinfos)
        for (guint i = 0; chunk->dhandle->setup.xinfos[i]; i++)
          if (chunk->dhandle->setup.xinfos[i][0] != '.')
            {
              const gchar *key = chunk->dhandle->setup.xinfos[i];
              const gchar *value = strchr (key, '=') + 1;
              gchar *ckey = g_strndup (key, value - key - 1);
              static const gchar *skip_keys[] = {
                "midi-note",
              };
              guint j;
              for (j = 0; j < G_N_ELEMENTS (skip_keys); j++)
                if (strcmp (ckey, skip_keys[j]) == 0)
                  break;
              if (j >= G_N_ELEMENTS (skip_keys))
                {
                  gchar *str = g_strescape (value, NULL);
                  sfi_wstore_printf (wstore, "    xinfo[\"%s\"] = \"%s\"\n", ckey, str);
                  g_free (str);
                }
              g_free (ckey);
            }

      sfi_wstore_puts (wstore, "  }\n");
    }

  sfi_wstore_puts (wstore, "}\n");
  sfi_wstore_flush_fd (wstore, fd);
  sfi_wstore_destroy (wstore);
  close (fd);

  return BSE_ERROR_NONE;
}

Wave::~Wave ()
{
}

} // BseWaveTool
