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
#ifndef __SFIDL_CXX_H__
#define __SFIDL_CXX_H__

#include "sfidl-generator.h"
#include "sfidl-namespace.h"

namespace Sfidl {

  class CodeGeneratorCxxBase : public CodeGeneratorCBase {
  protected:
    using CodeGeneratorCBase::createTypeCode;
    std::string createTypeCode (const std::string& type, const std::string& name, 
				TypeCodeModel model);

    std::string typeArg (const std::string& type);
    std::string typeField (const std::string& type);
    std::string typeRet (const std::string& type);
    std::string funcNew (const std::string& type);
    std::string funcCopy (const std::string& type);
    std::string funcFree (const std::string& type);

    std::string untyped_pspec_constructor (const Param &param);
    std::string typed_pspec_constructor   (const Param &param);

    void printChoicePrototype   (NamespaceHelper& nspace);
    void printChoiceImpl        (NamespaceHelper& nspace);
    void printRecSeqForwardDecl (NamespaceHelper& nspace);
    void printRecSeqDefinition (NamespaceHelper& nspace);
    void printRecSeqImpl (NamespaceHelper& nspace);

  public:
    CodeGeneratorCxxBase (const Parser& parser) : CodeGeneratorCBase (parser) {
    }
  };

  class CodeGeneratorCxx : public CodeGeneratorCxxBase {
  protected:
    NamespaceHelper nspace;
    std::string makeProcName (const std::string& className, const std::string& procName);
    void printMethods (const Class& cdef);
    void printProperties (const Class& cdef);

  public:
    CodeGeneratorCxx (const Parser& parser) : CodeGeneratorCxxBase (parser), nspace (stdout) {
    }
    bool run ();
  };
};

#endif  /* __SFIDL_CXX_H__ */

/* vim:set ts=8 sts=2 sw=2: */
