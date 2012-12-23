// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef _SFIDL_NAMESPACE_H__
#define _SFIDL_NAMESPACE_H__
#include <stdio.h>
#include "sfidl-utils.hh"
namespace Sfidl {
/*
 * This class is used during code generation. It generates the namespace
 * opening and closing code.
 */
class NamespaceHelper {
 protected:
  FILE *out;
  std::list<String> currentNamespace;
 public:	
  NamespaceHelper(FILE *outputfile);
  ~NamespaceHelper();
  /*
   * This method will cause the NamespaceHelper to enter the namespace the
   * symbol is in. That means setFromSymbol("Arts::Object") will enter the
   * namespace Arts. Since this generates code, it should only be called
   * outside of class definitions.
   */
  void setFromSymbol (String symbol);
  /*
   * This leaves all open namespaces which is useful if you want to include
   * a file or such, or if you are at the end of a file.
   */
  void leaveAll();
  /*
   * The shortest printable form of a symbol - using "Arts::Object" as
   * example, this would be "Arts::Object", if you are in no namespace,
   * ::Arts::Object, if you are in a different namespace, and just Object,
   * if you are in the Arts namespace.
   */
  String printableForm (String symbol);
  const char* printable_form (String symbol);
  /*
   * Returns only the last component of the symbol (the name) cutting the
   * namespace components
   */
  static String nameOf (String symbol);
  /*
   * Returns everything but the last component of the symbol, which is
   * the namespace (e.g. namespaceOf("Arts::Object") returns Arts, and
   * nameOf("Arts::Object") returns Object).
   */
  static String namespaceOf (String symbol);
};
}
#endif	/* _SFIDL_NAMESPACE_H__ */
/* vim:set ts=8 sts=2 sw=2: */
