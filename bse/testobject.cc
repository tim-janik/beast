// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "testobject.hh"

namespace Bse {

TestObjectImpl::TestObjectImpl ()
{}

TestObjectImpl::~TestObjectImpl ()
{}

int
TestObjectImpl::echo_test (const std::string &msg)
{
  g_print ("ServerImpl::echo_test: %s\n", msg.c_str());
  sig_echo_reply.emit (msg);
  return msg.size();
}

} // Bse
