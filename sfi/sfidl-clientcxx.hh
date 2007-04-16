/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2003-2007 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __SFIDL_CLIENTCXX_H__
#define __SFIDL_CLIENTCXX_H__

#include "sfidl-cxxbase.hh"
#include "sfidl-namespace.hh"

namespace Sfidl {

  class CodeGeneratorClientCxx : public CodeGeneratorCxxBase {
  protected:
    NamespaceHelper nspace;

    using CodeGeneratorCBase::createTypeCode;
    String createTypeCode (const String& type, const String& name, 
			   TypeCodeModel model);

    String typeArg (const String& type);
    String typeField (const String& type);
    String typeRet (const String& type);
    String funcNew (const String& type);
    String funcCopy (const String& type);
    String funcFree (const String& type);

    void printChoicePrototype   (NamespaceHelper& nspace);
    void printChoiceImpl        (NamespaceHelper& nspace);
    void printRecSeqForwardDecl (NamespaceHelper& nspace);
    void printRecSeqDefinition (NamespaceHelper& nspace);
    void printRecSeqImpl (NamespaceHelper& nspace);

    enum Style { STYLE_LOWER, STYLE_MIXED };
    Style style;

    String makeStyleName (const String& name);
    String makeProcName (const String& className, const String& procName);

    void printMethods (const Class& cdef);
    void printProperties (const Class& cdef);

  public:
    CodeGeneratorClientCxx (const Parser& parser) : CodeGeneratorCxxBase (parser), nspace (stdout), style (STYLE_LOWER) {
    }
    void help ();
    bool run ();

    OptionVector getOptions();
    void setOption (const String& option, const String& value);
  };
};

#endif  /* __SFIDL_CLIENTCXX_H__ */

/* vim:set ts=8 sts=2 sw=2: */
