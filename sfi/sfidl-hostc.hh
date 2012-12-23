// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFIDL_HOSTC_H__
#define __SFIDL_HOSTC_H__
#include <map>
#include <iostream>
#include <algorithm>
#include "sfidl-utils.hh"
#include "sfidl-cbase.hh"
namespace Sfidl {
  class CodeGeneratorHostC : public CodeGeneratorCBase {
  protected:
    String prefix;
    String generateInitFunction;
    void printChoiceMethodPrototypes (PrefixSymbolMode mode);
    void printChoiceMethodImpl();
    void printRecordFieldDeclarations();
    void printInitFunction (const String& initFunction);    
  public:
    CodeGeneratorHostC (const Parser& parser) : CodeGeneratorCBase (parser) {
    }
    void help();
    bool run();
    OptionVector getOptions();
    void setOption (const String& option, const String& value);
  };
};
#endif  /* __SFIDL_HOSTC_H__ */
/* vim:set ts=8 sts=2 sw=2: */
