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
#include <bse/gsldatahandle-vorbis.h>
#include <bse/gslloader.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <algorithm>
#include <vector>
#include <map>


namespace BseWaveTool {

WaveChunk::WaveChunk ()
{
  dhandle = NULL;
}

WaveChunk&
WaveChunk::operator= (const WaveChunk &rhs)
{
  temp_file = rhs.temp_file;
  dhandle = rhs.dhandle;
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

BseErrorType
WaveChunk::set_dhandle_from_file (const string &fname,
                                  gdouble       osc_freq,
                                  gchar       **copy_xinfos)
{
  BseErrorType error = BSE_ERROR_NONE;
  GslWaveFileInfo *wfi = gsl_wave_file_info_load (fname.c_str(), &error);
  GslDataHandle *xhandle = NULL;
  if (wfi)
    {
      GslWaveDsc *wdc = gsl_wave_dsc_load (wfi, 0, FALSE, &error);
      if (wdc)
        {
          xhandle = gsl_wave_handle_create (wdc, 0, &error);
          if (xhandle)
            {
              gchar **xinfos = bse_xinfos_dup_consolidated (copy_xinfos, FALSE);
              if (osc_freq > 0)
                xinfos = bse_xinfos_add_float (xinfos, "osc-freq", osc_freq);
              if (xinfos)
                {
                  GslDataHandle *tmp_handle = gsl_data_handle_new_add_xinfos (xhandle, xinfos);
                  g_strfreev (xinfos);
                  gsl_data_handle_unref (xhandle);
                  xhandle = tmp_handle;
                }
              error = gsl_data_handle_open (xhandle);
              if (error)
                {
                  gsl_data_handle_close (xhandle);
                  gsl_data_handle_unref (xhandle);
                  xhandle = NULL;
                }
              else
                gsl_data_handle_unref (xhandle);
            }
          gsl_wave_dsc_free (wdc);
        }
      gsl_wave_file_info_unref  (wfi);
    }
  if (xhandle)
    {
      if (dhandle)
        gsl_data_handle_close (dhandle);
      dhandle = xhandle;
      temp_file = fname;
      return BSE_ERROR_NONE;
    }
  else
    return error;
}

WaveChunk::~WaveChunk ()
{
  if (dhandle)
    gsl_data_handle_close (dhandle);
}

Wave::Wave (const gchar    *wave_name,
            guint           n_ch,
            gchar         **xinfos) :
  n_channels (n_ch),
  name (wave_name),
  wave_xinfos (bse_xinfos_dup_consolidated (xinfos, FALSE))
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
      return gsl_data_handle_osc_freq (wc1.dhandle) - gsl_data_handle_osc_freq (wc2.dhandle);
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
Wave::store (const string file_name)
{
  g_return_val_if_fail (file_name.c_str() != NULL, BSE_ERROR_INTERNAL);

  /* save to temporary file */
  gint fd;
  gchar *temp_file = NULL;
  do
    {
      g_free (temp_file);
      temp_file = g_strdup_printf ("%s.tmp%06xyXXXXXX", file_name.c_str(), rand() & 0xfffffd);
      mktemp (temp_file); /* this is save, due to use of: O_CREAT | O_EXCL */
      fd = open (temp_file, O_WRONLY | O_CREAT | O_EXCL, 0666);
    }
  while (fd < 0 && errno == EEXIST);
  if (fd < 0)
    {
      g_free (temp_file);
      return bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
    }

  /* figure default mix_freq */
  guint n_raw_handles = 0;
  map<float, guint> mf_counters;
  for (list<WaveChunk>::iterator it = chunks.begin(); it != chunks.end(); it++)
    {
      WaveChunk *chunk = &*it;
      GslDataHandle *dhandle, *tmp_handle = chunk->dhandle;
      do        /* skip comment or cache handles */
        {
          dhandle = tmp_handle;
          tmp_handle = gsl_data_handle_get_source (dhandle);
        }
      while (tmp_handle);
      GslVorbis1Handle *vhandle = gsl_vorbis1_handle_new (dhandle);
      if (!vhandle)
        {
          mf_counters[gsl_data_handle_mix_freq (chunk->dhandle)] += 1;
          n_raw_handles += 1;
        }
      else
        gsl_vorbis1_handle_destroy (vhandle);
    }
  float dfl_mix_freq = 0;
  guint max_count = 2;
  for (map<float, guint>::iterator it = mf_counters.begin(); it != mf_counters.end(); it++)
    if (it->second > max_count)
      {
        max_count = it->second;
        dfl_mix_freq = it->first;
      }

  /* dump wave header */
  SfiWStore *wstore = sfi_wstore_new ();
  wstore->comment_start = '#';
  sfi_wstore_puts (wstore, "#BseWave\n\n");
  sfi_wstore_puts (wstore, "wave {\n");
  gchar *str = g_strescape (name.c_str(), NULL);
  sfi_wstore_printf (wstore, "  name = \"%s\"\n", str);
  g_free (str);
  sfi_wstore_printf (wstore, "  n-channels = %u\n", n_channels);
  guint byte_order = G_LITTLE_ENDIAN;
  if (n_raw_handles)
    sfi_wstore_printf (wstore, "  byte-order = %s\n", gsl_byte_order_to_string (byte_order));
  if (n_raw_handles && dfl_mix_freq > 0)
    sfi_wstore_printf (wstore, "  mix-freq = %.3f\n", dfl_mix_freq);
  gchar **xinfos = bse_xinfos_dup_consolidated (wave_xinfos, FALSE);
  if (xinfos)
    for (guint i = 0; xinfos[i]; i++)
      if (xinfos[i][0] != '.')
        {
          const gchar *key = xinfos[i];
          const gchar *value = strchr (key, '=') + 1;
          gchar *ckey = g_strndup (key, value - key - 1);
          gchar *str = g_strescape (value, NULL);
          sfi_wstore_printf (wstore, "  xinfo[\"%s\"] = \"%s\"\n", ckey, str);
          g_free (str);
          g_free (ckey);
        }
  g_strfreev (xinfos);

  /* dump chunks */
  for (list<WaveChunk>::iterator it = chunks.begin(); it != chunks.end(); it++)
    {
      WaveChunk *chunk = &*it;
      sfi_wstore_puts (wstore, "  chunk {\n");
      int midi_note = bse_xinfos_get_num (chunk->dhandle->setup.xinfos, "midi-note");
      if (midi_note)
        sfi_wstore_printf (wstore, "    midi-note = %u\n", midi_note);
      else
        sfi_wstore_printf (wstore, "    osc-freq = %.3f\n", gsl_data_handle_osc_freq (chunk->dhandle));

      GslDataHandle *dhandle, *tmp_handle = chunk->dhandle;
      do        /* skip comment or cache handles */
        {
          dhandle = tmp_handle;
          tmp_handle = gsl_data_handle_get_source (dhandle);
        }
      while (tmp_handle);
      GslVorbis1Handle *vhandle = gsl_vorbis1_handle_new (dhandle); // FIXME: deamnd certain serialno
      if (vhandle)      /* save already compressed Ogg/Vorbis data */
        {
          sfi_wstore_puts (wstore, "    vorbis-link = ");
          gsl_vorbis1_handle_put_wstore (vhandle, wstore);
          sfi_wstore_puts (wstore, "\n");
        }
      else
        {
          sfi_wstore_puts (wstore, "    raw-link = ");
          gsl_data_handle_dump_wstore (chunk->dhandle, wstore, GSL_WAVE_FORMAT_SIGNED_16, byte_order);
          sfi_wstore_puts (wstore, "\n");
          gfloat mix_freq = gsl_data_handle_mix_freq (chunk->dhandle);
          if (mix_freq != dfl_mix_freq)
            sfi_wstore_printf (wstore, "    mix-freq = %.3f\n", mix_freq);
        }

      if (chunk->dhandle->setup.xinfos)
        for (guint i = 0; chunk->dhandle->setup.xinfos[i]; i++)
          if (chunk->dhandle->setup.xinfos[i][0] != '.')
            {
              const gchar *key = chunk->dhandle->setup.xinfos[i];
              const gchar *value = strchr (key, '=') + 1;
              gchar *ckey = g_strndup (key, value - key - 1);
              static const gchar *skip_keys[] = {
                "midi-note", "osc-freq", "mix-freq",
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

  /* replace output file by temporary file */
  BseErrorType error;
  if (0 /* in case of error */)
    {
      unlink (temp_file);
      error = BSE_ERROR_FILE_WRITE_FAILED;
    }
  else if (rename (temp_file, file_name.c_str()) < 0)
    {
      error = bse_error_from_errno (errno, BSE_ERROR_FILE_WRITE_FAILED);
      unlink (temp_file);
    }
  else /* success */
    error = BSE_ERROR_NONE;
  g_free (temp_file);

  return error;
}

Wave::~Wave ()
{
  g_strfreev (wave_xinfos);
}

} // BseWaveTool
