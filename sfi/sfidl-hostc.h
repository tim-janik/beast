/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Stefan Westerfeld, 2003 Tim Janik
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
#ifndef __SFIDL_GENERATOR_H__
#define __SFIDL_GENERATOR_H__

#include <sfi/glib-extra.h>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <string>
#include "sfidl-namespace.h"
#include "sfidl-options.h"
#include "sfidl-parser.h"
#include "sfiparams.h" /* scatId (SFI_SCAT_*) */

namespace Sfidl {
  
  class CodeGenerator {
  protected:
    const Parser& parser;
    const Options& options;
    
    std::vector<std::string> splitName (const std::string& name);
    std::string makeNamespaceSubst (const std::string& name);
    std::string makeLowerName (const std::string& name, char seperator = '_');
    std::string makeUpperName (const std::string& name);
    std::string makeMixedName (const std::string& name);
    std::string makeLMixedName (const std::string& name);
    std::string makeStyleName (const std::string& name);

    enum WordCase {
      lower,
      Capitalized,
      semiCapitalized,
      UPPER
    };

    /*
     * translates a word into a given word case
     * i.e. toWordCase ("Hello", UPPER) == "HELLO"
     */
    std::string toWordCase (const std::string& word, WordCase wc);
    std::string joinName (const std::vector<std::string>& name, const std::string& seperator, WordCase wc);

    enum NamespaceType {
      NONE,     /* no namespace */
      ABSOLUTE, /* in C++ means :: prefix */
      /* RELATIVE indicated by NamespaceHelper instead */
    };

    /*
     * rename is a function for renaming types according to rules
     * 
     * name: the name to be renamed
     * namespace_wc: the desired case for the namespace
     * namespace_join: how to join the namespace - note that if namespace_join
     *   is "::", then "::" will also be prefixed to the result, whereas if
     *   namespace_join is "_", it will only be used to seperate the namespaces
     *   (this is required/useful for C++)
     * namespace_append: words to append to the namespace
     * typename_wc: the desired case for the typename
     * typename_join: how to join the typename
     */
    std::string
    rename (NamespaceType namespace_type, const std::string& name, WordCase namespace_wc,
	    const std::string &namespace_join, const std::vector<std::string> &namespace_append,
	    WordCase typename_wc, const std::string &typename_join);

    /*
     * rename is a function for renaming types according to rules
     * 
     * nsh: namespace helper indicates which namespace to be relative to
     * namespace_wc: the desired case for the namespace
     * namespace_join: how to join the namespace
     * namespace_append: words to append to the namespace
     * typename_wc: the desired case for the typename
     * typename_join: how to join the typename
     */
    std::string
    rename (NamespaceHelper& nsh, const std::string& name, WordCase namespace_wc,
	    const std::string& namespace_join, const std::vector<std::string>& namespace_append,
	    WordCase typename_wc, const std::string& typename_join);
    
    CodeGenerator(const Parser& parser) : parser (parser), options (*Options::the()) {
    }
   
  public:
    virtual void run () = 0;
    virtual ~CodeGenerator() {
    }
   };

  /*
   * Base class for C and C++-like CodeGenerators
   */
  class CodeGeneratorCBase : public CodeGenerator {
  protected:
    enum TypeCodeModel {
      MODEL_FROM_VALUE, MODEL_TO_VALUE,
      MODEL_VCALL, MODEL_VCALL_ARG, 
      MODEL_VCALL_CARG, MODEL_VCALL_CONV, MODEL_VCALL_CFREE,
      MODEL_VCALL_RET, MODEL_VCALL_RCONV, MODEL_VCALL_RFREE
    };

    const gchar *makeCStr (const std::string& str);

    std::string scatId (SfiSCategory c);
    void printProcedure (const Method& mdef, bool proto = false, const std::string& className = "");
    void printChoiceConverters ();
    virtual std::string makeProcName (const std::string& className, const std::string& procName);

    std::string makeGTypeName (const std::string& name);
    std::string makeParamSpec (const Param& pdef);
    std::string createTypeCode (const std::string& type, TypeCodeModel model);

    /*
     * data types: the following models deal with how to represent a certain
     * SFI type in the binding
     */

    // how "type" looks like when passed as argument to a function
    virtual std::string typeArg (const std::string& type);
    const gchar *cTypeArg (const std::string& type) { return makeCStr (typeArg (type)); }

    // how "type" looks like when stored as member in a struct or class
    virtual std::string typeField (const std::string& type);
    const gchar *cTypeField (const std::string& type) { return makeCStr (typeField (type)); }

    // how the return type of a function returning "type" looks like
    virtual std::string typeRet (const std::string& type);
    const gchar *cTypeRet (const std::string& type) { return makeCStr (typeRet (type)); }

    // how an array of "type"s looks like ( == MODEL_MEMBER + "*" ?)
    virtual std::string typeArray (const std::string& type);
    const gchar *cTypeArray (const std::string& type) { return makeCStr (typeArray (type)); }

    /*
     * function required to create a new "type" (blank return value allowed)
     * example: funcNew ("FBlock") => "sfi_fblock_new" (in C)
     */
    virtual std::string funcNew (const std::string& type);
    const gchar *cFuncNew (const std::string& type) { return makeCStr (funcNew (type)); }

    /*
     * function required to copy a "type" (blank return value allowed)
     * example: funcCopy ("FBlock") => "sfi_fblock_ref" (in C)
     */ 
    virtual std::string funcCopy (const std::string& type);
    const gchar *cFuncCopy (const std::string& type) { return makeCStr (funcNew (type)); }
    
    /*
     * function required to free a "type" (blank return value allowed)
     * example: funcFree ("FBlock") => "sfi_fblock_unref" (in C)
     */ 
    virtual std::string funcFree (const std::string& type);
    const gchar *cFuncFree (const std::string& type) { return makeCStr (funcNew (type)); }

    virtual std::string createTypeCode (const std::string& type, const std::string& name, 
				        TypeCodeModel model);

    CodeGeneratorCBase (const Parser& parser) : CodeGenerator (parser) {
    }
  };

  class CodeGeneratorC : public CodeGeneratorCBase {
  protected:
    void printInfoStrings (const std::string& name, const Map<std::string,IString>& infos);
    bool choiceReverseSort(const ChoiceValue& e1, const ChoiceValue& e2);
    
  public:
    CodeGeneratorC(const Parser& parser) : CodeGeneratorCBase (parser) {
    }
    void run ();
  };

  class CodeGeneratorQt : public CodeGenerator {
    public:
      CodeGeneratorQt(Parser& parser) : CodeGenerator(parser) {
      }
      void run ();
  };
};

#endif  /* __SFIDL_GENERATOR_H__ */

/* vim:set ts=8 sts=2 sw=2: */
