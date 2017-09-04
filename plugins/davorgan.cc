// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "davorgan.genidl.hh"
#include <bse/bsemathsignal.hh>
#include <bse/bsemain.hh>
#include <vector>

namespace Bse {
namespace Dav {

class Organ : public OrganBase {
  /* per mix_freq() tables */
  class Tables
  {
    vector<float> m_sine_table, m_triangle_table, m_pulse_table;
    uint	  m_ref_count, m_rate;
    Tables (uint urate) :
      m_sine_table (urate), m_triangle_table (urate), m_pulse_table (urate),
      m_ref_count (1), m_rate (urate)
    {
      double rate = urate, half = rate / 2, slope = rate / 10;
      int    i;
      /* Initialize sine table. */
      for (i = 0; i < rate; i++)
	m_sine_table[i] = sin (i / rate * 2.0 * PI) / 6.0;
      /* Initialize triangle table. */
      for (i = 0; i < rate / 2; i++)
	m_triangle_table[i] = (4 / rate * i - 1.0) / 6.0;
      for (; i < rate; i++)
	m_triangle_table[i] = (4 / rate * (rate - i) - 1.0) / 6.0;
      /* Initialize beveled pulse table:  _
       *                                 / \
       *                              \_/
       */
      for (i = 0; i < slope; i++)
	m_pulse_table[i] = -i / slope / 6.0;
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
    {} // private destructor; use ref_counting
    static map<uint, Tables*> table_map;        // map of rate specific tables
    static std::mutex         table_mutex;
  public:
    static Tables*
    ref (uint rate)
    {
      static std::lock_guard<std::mutex> locker (table_mutex);
      if (table_map[rate])
	table_map[rate]->m_ref_count++;
      else
	table_map[rate] = new Tables (rate);
      return table_map[rate];
    }
    void
    unref()
    {
      return_unless (m_ref_count > 0);
      static std::lock_guard<std::mutex> locker (table_mutex);
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
    Bse::MusicalTuning current_musical_tuning;
    Properties (Organ *organ) :
      OrganProperties (organ),
      current_musical_tuning (organ->current_musical_tuning())
    {}
  };
  class Module : public SynthesisModule {
  public:
    /* frequency */
    double	  m_transpose_factor, m_fine_tune_factor, m_base_freq;
    /* instrument flavour */
    bool	  m_flute, m_reed, m_brass;
    /* harmonics */
    double	  m_harm0, m_harm1, m_harm2, m_harm3, m_harm4, m_harm5;
    /* phase accumulators */
    uint32	  m_harm0_paccu, m_harm1_paccu, m_harm2_paccu, m_harm3_paccu, m_harm4_paccu, m_harm5_paccu;
    /* mix_freq() specific tables */
    Tables       *m_tables;
    Module() :
      m_tables (Tables::ref (mix_freq()))
    {}
    ~Module()
    {
      m_tables->unref();
      m_tables = NULL;
    }
    void
    config (Properties *properties)
    {
      m_base_freq = properties->base_freq;
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
      dfreq = std::min (fabs (dfreq), mix_freq() * 0.5);

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
	freq_256 = dfreq_to_freq_256 (m_base_freq);

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

	      vaccu  = table_pos (sine_table,  freq_256_harm0, mix_freq_256, &m_harm0_paccu) * m_harm0;
	      vaccu += table_pos (sine_table,  freq_256_harm1, mix_freq_256, &m_harm1_paccu) * m_harm1;
	      vaccu += table_pos (reed_table,  freq_256_harm2, mix_freq_256, &m_harm2_paccu) * m_harm2;
	      vaccu += table_pos (sine_table,  freq_256_harm3, mix_freq_256, &m_harm3_paccu) * m_harm3;
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

	      vaccu  = table_pos (sine_table,  freq_256_harm0, mix_freq_256, &m_harm0_paccu) * m_harm0;
	      vaccu += table_pos (sine_table,  freq_256_harm1, mix_freq_256, &m_harm1_paccu) * m_harm1;
	      vaccu += table_pos (sine_table,  freq_256_harm2, mix_freq_256, &m_harm2_paccu) * m_harm2;
	      vaccu += table_pos (reed_table,  freq_256_harm3, mix_freq_256, &m_harm3_paccu) * m_harm3;
	      vaccu += table_pos (sine_table,  freq_256_harm4, mix_freq_256, &m_harm4_paccu) * m_harm4;
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
std::mutex                Organ::Tables::table_mutex;

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Organ);

} } // Bse::Dav
