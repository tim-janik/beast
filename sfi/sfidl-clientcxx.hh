/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2003 Stefan Westerfeld
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
#ifndef __SFIDL_CLIENTCXX_H__
#define __SFIDL_CLIENTCXX_H__

#include "sfidl-cxxbase.hh"
#include "sfidl-namespace.hh"

namespace Sfidl {

  class CodeGeneratorClientCxx : public CodeGeneratorCxxBase {
  protected:
    NamespaceHelper nspace;

    using CodeGeneratorCBase::createTypeCode;
    std::string createTypeCode (const std::string& type, const std::string& name, 
				TypeCodeModel model);

    std::string typeArg (const std::string& type);
    std::string typeField (const std::string& type);
    std::string typeRet (const std::string& type);
    std::string funcNew (const std::string& type);
    std::string funcCopy (const std::string& type);
    std::string funcFree (const std::string& type);

    void printChoicePrototype   (NamespaceHelper& nspace);
    void printChoiceImpl        (NamespaceHelper& nspace);
    void printRecSeqForwardDecl (NamespaceHelper& nspace);
    void printRecSeqDefinition (NamespaceHelper& nspace);
    void printRecSeqImpl (NamespaceHelper& nspace);

    enum Style { STYLE_LOWER, STYLE_MIXED };
    Style style;

    std::string makeStyleName (const std::string& name);
    std::string makeProcName (const std::string& className, const std::string& procName);

    void printMethods (const Class& cdef);
    void printProperties (const Class& cdef);

  public:
    CodeGeneratorClientCxx (const Parser& parser) : CodeGeneratorCxxBase (parser), nspace (stdout), style (STYLE_LOWER) {
    }
    void help ();
    bool run ();

    OptionVector getOptions();
    void setOption (const std::string& option, const std::string& value);
  };
};

#endif  /* __SFIDL_CLIENTCXX_H__ */

/* vim:set ts=8 sts=2 sw=2: */
