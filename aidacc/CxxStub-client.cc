#ifndef __AIDA_CXXSTUB_CLIENT_CC__
#define __AIDA_CXXSTUB_CLIENT_CC__

namespace { // Anon

namespace __AIDA_Local__ {
using namespace Aida;

// helper
static inline ProtoMsg*
new_emit_result (const ProtoMsg *fb, uint64 h, uint64 l, uint32 n)
{
  return ProtoMsg::renew_into_result (const_cast<ProtoMsg*> (fb), Aida::MSGID_EMIT_RESULT, h, l, n);
}

} } // Anon::__AIDA_Local__

#endif // __AIDA_CXXSTUB_CLIENT_CC__
