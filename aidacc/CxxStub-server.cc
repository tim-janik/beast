#ifndef __AIDA_CXXSTUB_SERVER_CC__
#define __AIDA_CXXSTUB_SERVER_CC__

namespace { // Anon

namespace __AIDA_Local__ {
using namespace Aida;

static_assert (std::is_base_of<Aida::ImplicitBase, $AIDA_iface_base$>::value,
               "IDL interface base '$AIDA_iface_base$' must derive 'Aida::ImplicitBase'");

// slot
template<class SharedPtr, class R, class... Args> std::function<R (Args...)>
slot (SharedPtr sp, R (*fp) (const SharedPtr&, Args...))
{
  return [sp, fp] (Args... args) { return fp (sp, args...); };
}

} } // Anon::__AIDA_Local__

#endif // __AIDA_CXXSTUB_SERVER_CC__
