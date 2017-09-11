// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __AIDA_HH__
#define __AIDA_HH__

#include "aidacc/aida.hh"
#include <assert.h>

// == Macros ==
#define TEST_CPP_PASTE2(a,b)    a ## b                          // Indirection helper, required to expand macros like __LINE__
#define TEST_CPP_PASTE(a,b)     TEST_CPP_PASTE2 (a,b)           ///< Paste two macro arguments into one C symbol name
#define TEST_CPP_STRINGIFY2(s)  #s                              // Indirection helper, required to expand macros like __LINE__
#define TEST_CPP_STRINGIFY(s)   TEST_CPP_STRINGIFY2 (s)         ///< Convert macro argument into a C const char*.
#define TASSERT(cond)           assert (cond)
#define TCMP(a,op,b)            assert (a op b)

// == TestChain ==
class TestChain {
  std::string           name_;
  std::function<void()> func_;
  const TestChain      *const next_;
  static TestChain*&    global_test_chain() { static TestChain *gchain = NULL; return gchain; }
public:
  static int run       (int argc, const char **argv);
  /*ctor*/   TestChain (std::function<void()> tfunc, const std::string &tname);
};
#define TEST_ADD(func)  static const ::TestChain TEST_CPP_PASTE (__TESTS_TestChain__, __LINE__) (func, TEST_CPP_STRINGIFY (func))


// == Implementations ==
TestChain::TestChain (std::function<void()> tfunc, const std::string &tname) :
  name_ (tname), func_ (tfunc), next_ (global_test_chain())
{
  AIDA_ASSERT_RETURN (next_ == global_test_chain());
  global_test_chain() = this;
}

int
TestChain::run (int argc, const char **argv)
{
  if (argc >= 1 && argv)
    printf ("  TEST     %s\n", argv[0]);
  for (const TestChain *t = global_test_chain(); t; t = t->next_)
    {
      fflush (stderr);
      printf ("  RUN…     %s\n", t->name_.c_str());
      fflush (stdout);
      t->func_();
      fflush (stderr);
      printf ("  PASS     %s\n", t->name_.c_str());
      fflush (stdout);
    }
  return 0;
}

#endif // __AIDA_HH__
