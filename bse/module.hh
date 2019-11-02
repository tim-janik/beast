// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MODULE_HH__
#define __BSE_MODULE_HH__

#include <bse/bsesource.hh>

namespace Bse {

class ModuleImpl : public ObjectImpl, public virtual ModuleIface {
  const String module_type_;
protected:
  friend class           FriendAllocator<ModuleImpl>;
  virtual               ~ModuleImpl       ();
public:
  explicit               ModuleImpl       (const String &module_type);
  virtual String         get_module_type  () override;
  virtual ModuleTypeInfo module_type_info () override;
  static ModuleTypeInfo  module_type_info  (const String &module_id);
  static StringSeq       list_module_types ();
};
using ModuleImplP = std::shared_ptr<ModuleImpl>;

} // Bse

#endif // __BSE_MODULE_HH__
