#ifndef __AIDA_CXXSTUB_SERVER_CC__
#define __AIDA_CXXSTUB_SERVER_CC__

namespace { // Anon

namespace __AIDA_Local__ {
using namespace Aida;

// types
typedef ServerConnection::EmitResultHandler EmitResultHandler;
typedef ServerConnection::MethodRegistry    MethodRegistry;
typedef ServerConnection::MethodEntry       MethodEntry;

static_assert (std::is_base_of<Aida::ImplicitBase, $AIDA_iface_base$>::value,
               "IDL interface base '$AIDA_iface_base$' must derive 'Aida::ImplicitBase'");

// objects
static inline ProtoMsg*
new_call_result (ProtoReader &fbr, uint64 h, uint64 l, uint32 n = 1)
{
  return ProtoMsg::renew_into_result (fbr, Aida::MSGID_CALL_RESULT, h, l, n);
}

static inline ProtoMsg*
new_connect_result (ProtoReader &fbr, uint64 h, uint64 l, uint32 n = 1)
{
  return ProtoMsg::renew_into_result (fbr, Aida::MSGID_CONNECT_RESULT, h, l, n);
}

// slot
template<class SharedPtr, class R, class... Args> std::function<R (Args...)>
slot (SharedPtr sp, R (*fp) (const SharedPtr&, Args...))
{
  return [sp, fp] (Args... args) { return fp (sp, args...); };
}

} } // Anon::__AIDA_Local__

#endif // __AIDA_CXXSTUB_SERVER_CC__
