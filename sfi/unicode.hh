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
constexpr inline bool   unicode_is_valid        (uint32_t u);
constexpr inline bool   unicode_is_assigned     (uint32_t u);
constexpr inline bool   unicode_is_noncharacter (uint32_t u);
constexpr inline bool   unicode_is_character    (uint32_t u);
constexpr inline bool   unicode_is_control_code (uint32_t u);
constexpr inline bool   unicode_is_private      (uint32_t u);
constexpr uint32_t      unicode_last_codepoint  = 0x10FFFF;


// == Implementations ==
/// Return whether @a u matches any of the assigned Unicode planes.
constexpr inline bool
unicode_is_assigned (uint32_t u)
{
  const bool assigned =
    (/*u >= 0x00 &&*/ u <=   0xD7FF) ||   // BMP - Basic Multilingual Plane (below surrogates at 0xD800)
    (u >=   0xE000 && u <=   0xFFFF) ||   // BMP - Basic Multilingual Plane (above surrogates at 0xDFFF)
    (u >=  0x10000 && u <=  0x14FFF) ||   // SMP - Supplementary Multilingual Plane
    (u >=  0x16000 && u <=  0x18FFF) ||   // SMP - Supplementary Multilingual Plane
    (u >=  0x1B000 && u <=  0x1BFFF) ||   // SMP - Supplementary Multilingual Plane
    (u >=  0x1D000 && u <=  0x1FFFF) ||   // SMP - Supplementary Multilingual Plane
    (u >=  0x20000 && u <=  0x2FFFF) ||   // SIP - Supplementary Ideographic Plane
    (u >=  0xE0000 && u <=  0xE0FFF) ||   // SSP - Supplementary Special-purpose Plane
    (u >=  0xF0000 && u <=  0xFFFFF) ||   // SPUA-A - Supplementary Private Use Area Plane
    (u >= 0x100000 && u <= 0x10FFFF);     // SPUA-B - Supplementary Private Use Area Plane
  return __builtin_expect (assigned, true);
}

/// Return whether @a u is an allowed Unicode codepoint within 0x10FFFF and not part of a UTF-16 surrogate pair.
constexpr inline bool
unicode_is_valid (uint32_t u)
{
  const bool valid = u <= 0x10FFFF && (u & 0x1FF800) != 0xD800;
  return __builtin_expect (valid, true);
}

/// Return whether @a u is one of the 66 Unicode noncharacters.
constexpr inline bool
unicode_is_noncharacter (uint32_t u)
{
  const bool noncharacter = (u >= 0xFDD0 && u <= 0xFDEF) || (u & 0xFFFE) == 0xFFFE;
  return __builtin_expect (noncharacter, false);
}

/// Return whether @a u is not one of the 66 Unicode noncharacters.
constexpr inline bool
unicode_is_character (uint32_t u)
{
  return __builtin_expect (!unicode_is_noncharacter (u), true);
}

/// Return whether @a u is one of the 65 Unicode control codes.
constexpr inline bool
unicode_is_control_code (uint32_t u)
{
  const bool control = (/*u >= 0x00 &&*/ u <= 0x1F) || (u >= 0x7F && u <= 0x9f);
  return __builtin_expect (control, false);
}

/// Return whether @a u is in one of the 3 private use areas of Unicode.
constexpr inline bool
unicode_is_private (uint32_t u)
{
  const bool priv = (u >= 0xE000 && u <= 0xF8FF) || (u >= 0xF0000 && u <= 0xFFFFD) || (u >= 0x100000 && u <= 0x10FFFD);
  return __builtin_expect (priv, false);
}

} // Bse

#endif // __BSE_UNICODE_HH__
