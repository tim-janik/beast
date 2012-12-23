// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

#include "sfidl-generator.hh"
#include "sfidl-factory.hh"
#include <list>

using namespace Sfidl;

namespace {

class CodeGeneratorTypeList : public CodeGenerator {
public:
  CodeGeneratorTypeList (const Parser &parser) : CodeGenerator (parser) {
  }
  bool run ()
  {
    vector<String>::const_iterator ti;

    for(ti = parser.getTypes().begin(); ti != parser.getTypes().end(); ti++)
      {
	if (parser.fromInclude (*ti)) continue;

	printf ("%s\n", makeMixedName (*ti).c_str());
      }
    return true;
  }
};

class TypeListFactory : public Factory {
public:
  String option() const	      { return "--list-types"; }
  String description() const  { return "print all types defined in the idlfile"; }
  
  CodeGenerator *create (const Parser& parser) const
  {
    return new CodeGeneratorTypeList (parser);
  }
} typelist_factory;

} // anon

/* vim:set ts=8 sts=2 sw=2: */
