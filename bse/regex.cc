// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "regex.hh"
#include <regex>

namespace Bse {
namespace Re {

std::string
sub (const std::string &regex, const std::string &subst, const std::string &input)
{
  return std::regex_replace (input, std::regex (regex), subst);
}

} // Re
} // Bse
