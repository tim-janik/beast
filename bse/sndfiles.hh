// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SNDFILES_HH__
#define __BSE_SNDFILES_HH__

#include <bse/floatutils.hh>

namespace Bse {

/// This namespace provides IO facilities for various sound file formats.
namespace Snd {

class WavWriter {
  std::string filename_;
  int         fd_ = -1;
  uint        n_bytes_ = 0;
  uint        n_bits_ = 0;
  uint        n_channels_ = 0;
  uint        sample_rate_ = 0;
  int         wheader   (uint n_data_bytes);
public:
  explicit    WavWriter ();
  explicit    WavWriter (const std::string &fname, uint n_channels, uint n_bits, uint sample_rate);
  int         open      (const std::string &fname, uint n_channels, uint n_bits, uint sample_rate);
  int         write     (const float *floats, uint n);
  void        close     ();
  bool        isopen    () const;
  std::string filename  () const;
  virtual    ~WavWriter ();
};

} // Snd

} // Bse

#endif // __BSE_SNDFILES_HH__
