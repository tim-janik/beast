// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_REGEX_HH__
#define __BSE_REGEX_HH__

#include <bse/cxxaux.hh>

namespace Bse {

/// Some std::regex wrappers to simplify usage and reduce compilation time
namespace Re {

std::string sub (const std::string &regex, const std::string &subst, const std::string &input);

} // Re
} // Bse

#endif // __BSE_REGEX_HH__
