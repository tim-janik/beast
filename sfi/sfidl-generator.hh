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
    /*
     * returns the options supported by this code generator (used by the option parser)
     * the first element of the pair is the option (i.e. "--source")
     * the second element of the pair is true when the option should be followed by a
     *   user argument (as in "--prefix beast"), false otherwise (i.e. "--source")
     */
    virtual std::vector< std::pair<std::string,bool> > getOptions();

    /*
     * called by the option parser when an option is set
     * option is the option (i.e. "--prefix")
     * value is the value (i.e. "beast"), and "1" for options without user argument
     */
    virtual void setOption (const std::string& option, const std::string& value);

    /*
     * run generates the code, and should return true if successful, false otherwise
     * (for instance if inconsistent options were given to the code generator)
     */
    virtual bool run () = 0;
    virtual ~CodeGenerator() {
    }
  };

};

#endif  /* __SFIDL_GENERATOR_H__ */

/* vim:set ts=8 sts=2 sw=2: */
