// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bwtwave.hh"
#include <bse/bsemath.hh>
#include <bse/gsldatautils.hh>
#include <bse/gsldatahandle-vorbis.hh>
#include <bse/bsedatahandle-flac.hh>
#include <bse/bseloader.hh>
#include <bse/bsecxxutils.hh>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <map>

using Bse::Flac1Handle;

namespace BseWaveTool {

WaveChunk::WaveChunk ()
{
  dhandle = NULL;
}

WaveChunk&
WaveChunk::operator= (const WaveChunk &rhs)
{
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

Bse::Error
WaveChunk::change_dhandle (GslDataHandle *xhandle,
                           gdouble        osc_freq,
                           gchar        **copy_xinfos)
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
  Bse::Error error = gsl_data_handle_open (xhandle);
  gsl_data_handle_unref (xhandle);
  if (error == 0)
    {
      if (dhandle)
        gsl_data_handle_close (dhandle);
      dhandle = xhandle;
      return Bse::Error::NONE;
    }
  else
    return error;
}

Bse::Error
WaveChunk::set_dhandle_from_file (const string &fname,
                                  gdouble       osc_freq,
                                  gchar       **xinfos)
{
  Bse::Error error = Bse::Error::NONE;
  BseWaveFileInfo *wfi = bse_wave_file_info_load (fname.c_str(), &error);
  GslDataHandle *xhandle = NULL;
  if (wfi)
    {
      BseWaveDsc *wdc = bse_wave_dsc_load (wfi, 0, FALSE, &error);
      if (wdc)
        {
          xhandle = bse_wave_handle_create (wdc, 0, &error);
          bse_wave_dsc_free (wdc);
        }
      bse_wave_file_info_unref  (wfi);
    }
  if (xhandle)
    return change_dhandle (xhandle, osc_freq, xinfos);
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

Bse::Error
Wave::add_chunk (GslDataHandle  *dhandle,
                 gchar         **xinfos)
{
  assert_return (dhandle != NULL, Bse::Error::INTERNAL);

  if (xinfos)
    {
      GslDataHandle *tmp_handle = gsl_data_handle_new_add_xinfos (dhandle, xinfos);
      gsl_data_handle_unref (dhandle);
      dhandle = tmp_handle;
    }
  else
    gsl_data_handle_ref (dhandle);

  Bse::Error error = gsl_data_handle_open (dhandle);
  if (error == 0)
    {
      WaveChunk wc;
      wc.dhandle = dhandle;
      chunks.push_front (wc);
    }
  gsl_data_handle_unref (dhandle);
  return error;
}

void
Wave::set_xinfo (const gchar    *key,
                 const gchar    *value)
{
  if (value && value[0])
    wave_xinfos = bse_xinfos_add_value (wave_xinfos, key, value);
  else
    wave_xinfos = bse_xinfos_del_value (wave_xinfos, key);
}

void
Wave::set_chunks_xinfo (const gchar    *key,
                        const gchar    *value,
                        gfloat          osc_freq,
                        bool            all_chunks)
{
  for (list<WaveChunk>::iterator it = chunks.begin(); it != chunks.end(); it++)
    if (all_chunks || fabs (gsl_data_handle_osc_freq (it->dhandle) - osc_freq) < 0.01)
      {
        WaveChunk &wchunk = *it;
        GslDataHandle *tmp_handle;
        if (value && value[0])
          {
            gchar *xinfos[2] = { g_strconcat (key, "=", value, NULL), NULL };
            tmp_handle = gsl_data_handle_new_add_xinfos (wchunk.dhandle, xinfos);
            g_free (xinfos[0]);
          }
        else
          {
            gchar *xinfos[2] = { (gchar*) key, NULL };
            tmp_handle = gsl_data_handle_new_remove_xinfos (wchunk.dhandle, xinfos);
          }
        gsl_data_handle_open (tmp_handle); /* wchunk.dhandle already opened */
        gsl_data_handle_unref (tmp_handle);
        gsl_data_handle_close (wchunk.dhandle);
        wchunk.dhandle = tmp_handle;
      }
}

GslDataHandle*
Wave::lookup (gfloat osc_freq)
{
  for (list<WaveChunk>::iterator it = chunks.begin(); it != chunks.end(); it++)
    if (fabs (gsl_data_handle_osc_freq (it->dhandle) - osc_freq) < 0.01)
      return it->dhandle;
  return NULL;
}

static int
compare_floats (float f1,
                float f2)
{
  return f1 < f2 ? -1 : f1 > f2;
}

bool
Wave::match (const WaveChunk &wchunk,
             vector<float>   &sorted_freqs)
{
  gfloat osc_freq = gsl_data_handle_osc_freq (wchunk.dhandle);
  vector<float>::iterator it = Bse::binary_lookup_sibling (sorted_freqs.begin(), sorted_freqs.end(), compare_floats, osc_freq);
  if (it == sorted_freqs.end())
    return false;
  if (fabs (*it - osc_freq) < 0.01)
    return true;
  if (it != sorted_freqs.begin() && fabs (*(it - 1) - osc_freq) < 0.01)
    return true;
  if (it + 1 != sorted_freqs.end() && fabs (*(it + 1) - osc_freq) < 0.01)
    return true;
  return false;
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
    static bool
    is_smaller (const WaveChunk &wc1,
                const WaveChunk &wc2)
    {
      return gsl_data_handle_osc_freq (wc1.dhandle) < gsl_data_handle_osc_freq (wc2.dhandle);
    }
  };
#if 0 /* brrrr, lists aren't sortable due to iterator issues */
  std::sort (chunks.begin(), chunks.end(), Sub::chunk_cmp);
#else
  vector<WaveChunk> vwc;
  vwc.assign (chunks.begin(), chunks.end());
  stable_sort (vwc.begin(), vwc.end(), Sub::is_smaller);
  chunks.assign (vwc.begin(), vwc.end());
#endif
}

Bse::Error
Wave::store (const string file_name)
{
  assert_return (file_name.c_str() != NULL, Bse::Error::INTERNAL);

  /* save to temporary file */
  gint fd;
  gchar *temp_file = NULL;
  do
    {
      g_free (temp_file);
      temp_file = g_strdup_format ("%s.tmp%06x", file_name.c_str(), rand() & 0xfffffd);
      fd = open (temp_file, O_WRONLY | O_CREAT | O_EXCL, 0666);
    }
  while (fd < 0 && errno == EEXIST);
  if (fd < 0)
    {
      g_free (temp_file);
      return bse_error_from_errno (errno, Bse::Error::FILE_OPEN_FAILED);
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
      GslVorbis1Handle *vhandle = gsl_vorbis1_handle_new (dhandle, 0);
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
  sfi_wstore_puts (wstore, "#BseWave1\n\n");
  sfi_wstore_puts (wstore, "wave {\n");
  gchar *str = g_strescape (name.c_str(), NULL);
  sfi_wstore_printf (wstore, "  name = \"%s\"\n", str);
  g_free (str);
  sfi_wstore_printf (wstore, "  n-channels = %u\n", n_channels);
  guint byte_order = G_LITTLE_ENDIAN;
  if (n_raw_handles)
    sfi_wstore_printf (wstore, "  byte-order = %s\n", gsl_byte_order_to_string (byte_order));
  if (n_raw_handles && dfl_mix_freq > 0)
    {
      sfi_wstore_printf (wstore, "  mix-freq = ");
      sfi_wstore_putf (wstore, dfl_mix_freq);
      sfi_wstore_puts (wstore, "\n");
    }
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
      int midi_note = 0; // FIXME: bse_xinfos_get_num (chunk->dhandle->setup.xinfos, "midi-note");
      if (midi_note)
        sfi_wstore_printf (wstore, "    midi-note = %u\n", midi_note);
      else
        {
          sfi_wstore_printf (wstore, "    osc-freq = ");
          sfi_wstore_putf (wstore, gsl_data_handle_osc_freq (chunk->dhandle));
          sfi_wstore_puts (wstore, "\n");
        }

      GslDataHandle *dhandle, *tmp_handle = chunk->dhandle;
      do        /* skip comment or cache handles */
        {
          dhandle = tmp_handle;
          tmp_handle = gsl_data_handle_get_source (dhandle);
        }
      while (tmp_handle);
      GslVorbis1Handle *vhandle = gsl_vorbis1_handle_new (dhandle, gsl_vorbis_make_serialno());
      Flac1Handle      *flac_handle = Flac1Handle::create (dhandle);
      if (vhandle)      /* save already compressed Ogg/Vorbis data */
        {
          sfi_wstore_puts (wstore, "    vorbis-link = ");
          gsl_vorbis1_handle_put_wstore (vhandle, wstore);
          sfi_wstore_puts (wstore, "\n");
        }
      else if (flac_handle)
        {
          sfi_wstore_puts (wstore, "    flac-link = ");
          flac_handle->put_wstore (wstore);
          sfi_wstore_puts (wstore, "\n");
        }
      else
        {
          sfi_wstore_puts (wstore, "    raw-link = ");
          gsl_data_handle_dump_wstore (chunk->dhandle, wstore, GSL_WAVE_FORMAT_SIGNED_16, byte_order);
          sfi_wstore_puts (wstore, "\n");
          gfloat mix_freq = gsl_data_handle_mix_freq (chunk->dhandle);
          if (mix_freq != dfl_mix_freq)
            {
              sfi_wstore_printf (wstore, "    mix-freq = ");
              sfi_wstore_putf (wstore, mix_freq);
              sfi_wstore_puts (wstore, "\n");
            }
        }

      if (chunk->dhandle->setup.xinfos)
        for (guint i = 0; chunk->dhandle->setup.xinfos[i]; i++)
          if (chunk->dhandle->setup.xinfos[i][0] != '.')
            {
              const gchar *key = chunk->dhandle->setup.xinfos[i];
              const gchar *value = strchr (key, '=') + 1;
              gchar *ckey = g_strndup (key, value - key - 1);
              static const gchar *skip_keys[] = {
                "osc-freq", "mix-freq", // FIXME: "midi-note",
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
  gint nerrno = sfi_wstore_flush_fd (wstore, fd);
  Bse::Error error = bse_error_from_errno (-nerrno, Bse::Error::FILE_WRITE_FAILED);
  if (close (fd) < 0 && error == Bse::Error::NONE)
    error = bse_error_from_errno (errno, Bse::Error::FILE_WRITE_FAILED);
  sfi_wstore_destroy (wstore);

  /* replace output file by temporary file */
  if (error != Bse::Error::NONE)
    {
      unlink (temp_file);
    }
  else if (rename (temp_file, file_name.c_str()) < 0)
    {
      error = bse_error_from_errno (errno, Bse::Error::FILE_WRITE_FAILED);
      unlink (temp_file);
    }
  g_free (temp_file);

  return error;
}

Wave::~Wave ()
{
  g_strfreev (wave_xinfos);
}

} // BseWaveTool
