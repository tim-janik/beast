// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_UNICODE_HH__
#define __BSE_UNICODE_HH__

#include <sfi/cxxaux.hh>

namespace Bse {

std::string             string_from_unicode     (const std::vector<uint32_t> &codepoints);
std::string             string_from_unicode     (const uint32_t *codepoints, size_t n_codepoints);
size_t                  utf8_to_unicode         (const std::string &str, std::vector<uint32_t> &codepoints);
size_t                  utf8_to_unicode         (const char *str, uint32_t *codepoints);
size_t                  utf8len                 (const std::string &str);
size_t                  utf8len                 (const char *str);
constexpr inline bool   valid_unicode           (uint32_t u);

// == Implementations ==
/// Return whether @a u matches any of the defined Unicode planes.
constexpr inline bool
valid_unicode (uint32_t u)
{
  return ((u >=   0x0000 && u <=   0xFFFF) ||   // BMP - Basic Multilingual Plane
          (u >=  0x10000 && u <=  0x14FFF) ||   // SMP - Supplementary Multilingual Plane
          (u >=  0x16000 && u <=  0x18FFF) ||   // SMP - Supplementary Multilingual Plane
          (u >=  0x1B000 && u <=  0x1BFFF) ||   // SMP - Supplementary Multilingual Plane
          (u >=  0x1D000 && u <=  0x1FFFF) ||   // SMP - Supplementary Multilingual Plane
          (u >=  0x20000 && u <=  0x2FFFF) ||   // SIP - Supplementary Ideographic Plane
          (u >=  0xE0000 && u <=  0xE0FFF) ||   // SSP - Supplementary Special-purpose Plane
          (u >=  0xF0000 && u <=  0xFFFFF) ||   // SPUA-A - Supplementary Private Use Area Plane
          (u >= 0x100000 && u <= 0x10FFFF));    // SPUA-B - Supplementary Private Use Area Plane
}

} // Bse

#endif // __BSE_UNICODE_HH__
