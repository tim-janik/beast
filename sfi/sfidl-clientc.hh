// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFIDL_CLIENTC_H__
#define __SFIDL_CLIENTC_H__
#include <map>
#include <iostream>
#include <algorithm>
#include "sfidl-utils.hh"
#include "sfidl-namespace.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"
#include "sfidl-cbase.hh"
#include "sfiparams.hh" /* scatId (SFI_SCAT_*) */
namespace Sfidl {
  class CodeGeneratorClientC : public CodeGeneratorCBase {
  protected:
    String prefix;
    Method methodWithObject (const Class& cd, const Method& md);
    void printProcedurePrototypes (PrefixSymbolMode mode);
    void printClassMacros();
    void printProcedureImpl ();
    void addBindingSpecificFiles (const String& binding_specific_files);
  public:
    CodeGeneratorClientC(const Parser& parser) : CodeGeneratorCBase (parser) {
    }
    void help ();
    bool run ();
    OptionVector getOptions();
    void setOption (const String& option, const String& value);
  };
};
#endif  /* __SFIDL_CLIENTC_H__ */
/* vim:set ts=8 sts=2 sw=2: */
