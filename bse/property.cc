// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "property.hh"
#include "internal.hh"

namespace Bse {

// == PropertyWrapper ==
PropertyWrapper::~PropertyWrapper()
{}

// == PropertyImpl ==
PropertyImplP
PropertyImpl::create (PropertyWrapperP &&wrapper)
{
  assert_return (wrapper != nullptr, nullptr);
  return FriendAllocator<PropertyImpl>::make_shared (std::move (wrapper));
}

PropertyImpl::PropertyImpl (PropertyWrapperP &&wrapper) :
  wrapper_ (std::move (wrapper))
{}

PropertyImpl::~PropertyImpl ()
{}

} // Bse
