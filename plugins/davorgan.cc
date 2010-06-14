/* DavOrgan - DAV Additive Organ Synthesizer
 * Copyright (c) 1999, 2000, 2002 David A. Bartold and Tim Janik
 * Copyright (c) 2006-2007 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "davorgan.genidl.hh"
#include <bse/bsemathsignal.h>
#include <bse/bsemain.h>
#include <vector>

namespace Bse { namespace Dav {

using namespace std;
using namespace Birnet;  // FIXME: move to Bse namespace
using Birnet::uint32;    // FIXME: move to Bse header

class Organ : public OrganBase {
  /* per mix_freq() tables */
  class Tables
  {
    uint	  m_ref_count;
    uint	  m_rate;

    vector<float> m_sine_table;
    vector<float> m_triangle_table;
    vector<float> m_pulse_table;

    Tables (uint urate) :
      m_ref_count (1),
      m_rate (urate),
      m_sine_table (urate),
      m_triangle_table (urate),
      m_pulse_table (urate)
    {
      double rate   = urate;
      double half   = rate / 2;
      double slope  = rate / 10;
      int    i;

      /* Initialize sine table. */
      for (i = 0; i < rate; i++)
	m_sine_table[i] = sin ((i / rate) * 2.0 * PI) / 6.0;

      /* Initialize triangle table. */
      for (i = 0; i < rate / 2; i++)
	m_triangle_table[i] = (4 / rate * i - 1.0) / 6.0;
      for (; i < rate; i++)
	m_triangle_table[i] = (4 / rate * (rate - i) - 1.0) / 6.0;

      /* Initialize pulse table. */
      for (i = 0; i < slope; i++)
	m_pulse_table[i] = (-i / slope) / 6.0;
      for (; i < half - slope; i++)
	m_pulse_table[i] = -1.0 / 6.0;
      for (; i < half + slope; i++)
	m_pulse_table[i] = ((i - half) / slope) / 6.0;
      for (; i < rate - slope; i++)
	m_pulse_table[i] = 1.0 / 6.0;
      for (; i < rate; i++)
	m_pulse_table[i] = ((rate - i) * 1.0 / slope) / 6.0;
    }
    ~Tables()
    {
      // private destructor; use ref_counting
    }

    static map<uint, Tables*> table_map;   /* rate -> rate specific tables */
    static Mutex              table_mutex;

  public:
    static Tables*
    ref (uint rate)
    {
      AutoLocker locker (table_mutex);

      if (table_map[rate])
	table_map[rate]->m_ref_count++;
      else
	table_map[rate] = new Tables (rate);

      return table_map[rate];
    }
    void
    unref()
    {
      AutoLocker locker (table_mutex);

      if (--m_ref_count == 0)
	{
	  table_map[m_rate] = 0;
	  delete this;
	}
    }
    const float*
    sine_table() const
    {
      return &m_sine_table[0];
    }
    const float*
    triangle_table() const
    {
      return &m_triangle_table[0];
    }
    const float*
    pulse_table() const
    {
      return &m_pulse_table[0];
    }
  };

  /* FIXME: get rid of this as soon as the modules have their own current_musical_tuning() accessor */
  struct Properties : public OrganProperties {
    BseMusicalTuningType current_musical_tuning;

    Properties (Organ *organ) :
      OrganProperties (organ),
      current_musical_tuning (organ->current_musical_tuning())
    {
    }
  };
  class Module : public SynthesisModule {
  public:
    /* frequency */
    double	  m_transpose_factor;
    double	  m_fine_tune_factor;
    double	  m_cfreq;

    /* instrument flavour */
    bool	  m_flute;
    bool	  m_reed;
    bool	  m_brass;

    /* harmonics */
    double	  m_harm0;
    double	  m_harm1;
    double	  m_harm2;
    double	  m_harm3;
    double	  m_harm4;
    double	  m_harm5;

    /* phase accumulators */
    uint32	  m_harm0_paccu;
    uint32	  m_harm1_paccu;
    uint32	  m_harm2_paccu;
    uint32	  m_harm3_paccu;
    uint32	  m_harm4_paccu;
    uint32	  m_harm5_paccu;

    /* mix_freq() specific tables */
    Tables       *m_tables;

    Module() :
      m_tables (Tables::ref (mix_freq()))
    {
    }
    ~Module()
    {
      m_tables->unref();
      m_tables = 0;
    }
    void
    config (Properties *properties)
    {
      m_cfreq = properties->base_freq;
      m_transpose_factor = bse_transpose_factor (properties->current_musical_tuning, properties->transpose);
      m_fine_tune_factor = bse_cent_tune_fast (properties->fine_tune);

      // percent -> factor conversions
      m_harm0 = properties->harm0 / 100.0;
      m_harm1 = properties->harm1 / 100.0;
      m_harm2 = properties->harm2 / 100.0;
      m_harm3 = properties->harm3 / 100.0;
      m_harm4 = properties->harm4 / 100.0;
      m_harm5 = properties->harm5 / 100.0;

      m_flute = properties->flute;
      m_reed = properties->reed;
      m_brass = properties->brass;
    }
    void
    reset()
    {
      uint32 rfactor = bse_main_args->allow_randomization ? 1 : 0;
      uint32 mix_freq_256 = mix_freq() * 256;

      /* to make all notes sound a bit different, randomize the initial phase of
       * each harmonic (except if the user requested deterministic behaviour)
       */
      m_harm0_paccu = rfactor * g_random_int_range (0, mix_freq_256);
      m_harm1_paccu = rfactor * g_random_int_range (0, mix_freq_256);
      m_harm2_paccu = rfactor * g_random_int_range (0, mix_freq_256);
      m_harm3_paccu = rfactor * g_random_int_range (0, mix_freq_256);
      m_harm4_paccu = rfactor * g_random_int_range (0, mix_freq_256);
      m_harm5_paccu = rfactor * g_random_int_range (0, mix_freq_256);
    }
    static inline float
    table_pos (const float *table,
	       uint	    freq_256,
	       uint	    mix_freq_256,
	       uint32	   *paccu)
    {
      *paccu += freq_256;
      while (*paccu >= mix_freq_256)
	*paccu -= mix_freq_256;

      return table[*paccu >> 8];
    }
    inline uint
    dfreq_to_freq_256 (double dfreq)
    {
      dfreq *= m_transpose_factor * m_fine_tune_factor;

      /* Make sure that the actual sound generation code will only see
       * frequencies in the range [0, mix_freq/2]. We map negative frequencies
       * (like -440 Hz) to their positive equivalents (+440 Hz).
       */
      dfreq = min (fabs (dfreq), mix_freq() * 0.5);

      /* round frequency with dtoi during conversion from floating point to our
       * fixed point representation, in order to minimize the conversion error
       */
      return dtoi (dfreq * 256);
    }
    void
    process (unsigned int n_values)
    {
      const float *sine_table = m_tables->sine_table();
      const float *flute_table = m_flute ? m_tables->triangle_table() : sine_table;
      const float *reed_table = m_reed ? m_tables->pulse_table() : sine_table;
      const float *ifreq = istream (ICHANNEL_FREQ_IN).values;
      float	  *ovalues = ostream (OCHANNEL_AUDIO_OUT).values;
      uint         freq_256;

      if (istream (ICHANNEL_FREQ_IN).connected)
	freq_256 = dfreq_to_freq_256 (BSE_FREQ_FROM_VALUE (ifreq[0]));
      else
	freq_256 = dfreq_to_freq_256 (m_cfreq);

      uint mix_freq_256 = mix_freq() * 256;
      uint freq_256_harm0 = freq_256 / 2;
      uint freq_256_harm1 = freq_256;

      if (m_brass)
	{
	  uint freq_256_harm2 = freq_256 * 2;
	  uint freq_256_harm3 = freq_256_harm2 * 2;
	  uint freq_256_harm4 = freq_256_harm3 * 2;
	  uint freq_256_harm5 = freq_256_harm4 * 2;

	  for (uint i = 0; i < n_values; i++)
	    {
	      float vaccu;

	      vaccu = table_pos (sine_table, freq_256_harm0, mix_freq_256, &m_harm0_paccu) * m_harm0;
	      vaccu += table_pos (sine_table, freq_256_harm1, mix_freq_256, &m_harm1_paccu) * m_harm1;
	      vaccu += table_pos (reed_table, freq_256_harm2, mix_freq_256, &m_harm2_paccu) * m_harm2;
	      vaccu += table_pos (sine_table, freq_256_harm3, mix_freq_256, &m_harm3_paccu) * m_harm3;
	      vaccu += table_pos (flute_table, freq_256_harm4, mix_freq_256, &m_harm4_paccu) * m_harm4;
	      vaccu += table_pos (flute_table, freq_256_harm5, mix_freq_256, &m_harm5_paccu) * m_harm5;
	      ovalues[i] = vaccu;
	    }
	}
      else
	{
	  uint freq_256_harm2 = freq_256 * 3 / 2;
	  uint freq_256_harm3 = freq_256 * 2;
	  uint freq_256_harm4 = freq_256 * 3;
	  uint freq_256_harm5 = freq_256_harm3 * 2;

	  for (uint i = 0; i < n_values; i++)
	    {
	      float vaccu;

	      vaccu = table_pos (sine_table, freq_256_harm0, mix_freq_256, &m_harm0_paccu) * m_harm0;
	      vaccu += table_pos (sine_table, freq_256_harm1, mix_freq_256, &m_harm1_paccu) * m_harm1;
	      vaccu += table_pos (sine_table, freq_256_harm2, mix_freq_256, &m_harm2_paccu) * m_harm2;
	      vaccu += table_pos (reed_table, freq_256_harm3, mix_freq_256, &m_harm3_paccu) * m_harm3;
	      vaccu += table_pos (sine_table, freq_256_harm4, mix_freq_256, &m_harm4_paccu) * m_harm4;
	      vaccu += table_pos (flute_table, freq_256_harm5, mix_freq_256, &m_harm5_paccu) * m_harm5;
	      ovalues[i] = vaccu;
	    }
	}
    }
  };
public:
  bool
  property_changed (OrganPropertyID prop_id)
  {
    switch (prop_id)
      {
      /* implement special handling of GUI properties */
      case PROP_BASE_FREQ:
	base_note = bse_note_from_freq (current_musical_tuning(), base_freq);
	notify ("base_note");
	break;
      case PROP_BASE_NOTE:
	base_freq = bse_note_to_freq (current_musical_tuning(), base_note);
	notify ("base_freq");
	break;
      default: ;
      }
    return false;
  }

  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (Organ, Module, Properties);
};

map<uint, Organ::Tables*> Organ::Tables::table_map;
Mutex                     Organ::Tables::table_mutex;

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Organ);

} // Dav
} // Bse
