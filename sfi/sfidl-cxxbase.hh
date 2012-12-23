// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFIDL_CXXBASE_H__
#define __SFIDL_CXXBASE_H__
#include "sfidl-cbase.hh"
#include "sfidl-namespace.hh"
#include "sfidl-utils.hh"
namespace Sfidl {
  class CodeGeneratorCxxBase : public CodeGeneratorCBase {
  protected:
    String untyped_pspec_constructor (const Param &param);
    String typed_pspec_constructor   (const Param &param);
  public:
    CodeGeneratorCxxBase (const Parser& parser) : CodeGeneratorCBase (parser) {
    }
  };
};
#endif  /* __SFIDL_CXXBASE_H__ */
/* vim:set ts=8 sts=2 sw=2: */
