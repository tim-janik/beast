/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2000,2002 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef _SFIDL_NAMESPACE_H__
#define _SFIDL_NAMESPACE_H__

#include <stdio.h>
#include <string>
#include <list>

/*
 * This class is used during code generation. It generates the namespace
 * opening and closing code.
 */
class NamespaceHelper {
 protected:
  FILE *out;
  std::list<std::string> currentNamespace;
  
 public:	
  NamespaceHelper(FILE *outputfile);
  ~NamespaceHelper();
  
  /*
   * This method will cause the NamespaceHelper to enter the namespace the
   * symbol is in. That means setFromSymbol("Arts::Object") will enter the
   * namespace Arts. Since this generates code, it should only be called
   * outside of class definitions.
   */
  void setFromSymbol(std::string symbol);
  
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
  std::string printableForm(std::string symbol);
  
  /*
   * Returns only the last component of the symbol (the name) cutting the
   * namespace components
   */
  static std::string nameOf(std::string symbol);
  
  /*
   * Returns everything but the last component of the symbol, which is
   * the namespace (e.g. namespaceOf("Arts::Object") returns Arts, and
   * nameOf("Arts::Object") returns Object).
   */
  static std::string namespaceOf(std::string symbol);
};

#endif	/* _SFIDL_NAMESPACE_H__ */

/* vim:set ts=8 sts=2 sw=2: */
