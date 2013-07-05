// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_TESTOBJECT_HH__
#define __BSE_TESTOBJECT_HH__

#include <bse/bseutils.hh>

namespace Bse {

class TestObjectImpl : public TestObjectIface {
public:
  explicit      TestObjectImpl ();
  virtual      ~TestObjectImpl ();
  virtual int   echo_test      (const std::string &msg);
};
typedef std::shared_ptr<TestObjectImpl> TestObjectImplP;

} // Bse

#endif // __BSE_TESTOBJECT_HH__
