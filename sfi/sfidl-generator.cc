/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Stefan Westerfeld
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
#include "sfidl-generator.h"
#include "sfidl-factory.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "sfidl-namespace.h"
#include "sfidl-options.h"
#include "sfidl-parser.h"
#include "sfiparams.h" /* scatId (SFI_SCAT_*) */

using namespace Sfidl;
using namespace std;

/*--- common functions ---*/

vector<string> CodeGenerator::splitName (const string& name)
{
  bool lastupper = true, upper = true, lastunder = true, remove_caps = false;
  string::const_iterator i;
  vector<string> words;
  string word;

  /*
   * we try to guess here whether we need to remove caps
   * or keep them
   *
   * if our input is BseSNet, it is vital to keep the caps
   * if our input is IO_ERROR, we need to remove it
   */
  for(i = name.begin(); i != name.end(); i++)
    {
      if (*i == '_')
	remove_caps = true;
    }

  for(i = name.begin(); i != name.end(); i++)
    {
      lastupper = upper;
      upper = isupper(*i);
      if (!lastupper && upper && !lastunder)
	{
	  words.push_back (word);
	  word = "";
	  lastunder = true;
	}
      if(*i == ':' || *i == '_')
	{
	  if(!lastunder)
	    {
	      words.push_back (word);
	      word = "";
	      lastunder = true;
	    }
	}
      else
	{
	  if (remove_caps)
	    word += tolower(*i);
	  else
	    word += *i;
	  lastunder = false;
	}
    }

  if (word != "")
    words.push_back (word);

  return words;
}


string CodeGenerator::makeLowerName (const string& name, char seperator)
{
  string result;
  const vector<string>& words = splitName (name);

  for (vector<string>::const_iterator wi = words.begin(); wi != words.end(); wi++)
    {
      if (result != "") result += seperator;

      for (string::const_iterator i = wi->begin(); i != wi->end(); i++)
	result += tolower (*i);
    }
  
  return result;
}

string CodeGenerator::makeUpperName (const string& name)
{
  string lname = makeLowerName (name);
  string result;
  string::const_iterator i;
  
  for(i = lname.begin(); i != lname.end(); i++)
    result += toupper(*i);
  return result;
}

string CodeGenerator::makeMixedName (const string& name)
{
  string result;
  const vector<string>& words = splitName (name);

  for (vector<string>::const_iterator wi = words.begin(); wi != words.end(); wi++)
    {
      bool first = true;

      for (string::const_iterator i = wi->begin(); i != wi->end(); i++)
	{
	  if (first)
	    result += toupper (*i);
	  else
	    result += *i;
	  first = false;
	}
    }
  
  return result;
}

string CodeGenerator::makeLMixedName (const string& name)
{
  string result = makeMixedName (name);

  if (!result.empty()) result[0] = tolower (result[0]);
  return result;
}

string CodeGenerator::toWordCase (const string& word, WordCase wc)
{
  string result;
  for (string::const_iterator si = word.begin(); si != word.end(); si++)
    {
      bool first = (si == word.begin());
      switch (wc)
	{
	  case lower:	      result += tolower (*si);
			      break;
	  case Capitalized:   result += first ? toupper (*si) : *si;
			      break;
	  case UPPER:	      result += toupper (*si);
			      break;
	  default:	      g_assert_not_reached();
	}
    }
  return result;
}

string CodeGenerator::joinName (const vector<string>& name, const string& seperator, WordCase wc)
{
  string result;

  for (vector<string>::const_iterator wi = name.begin(); wi != name.end(); wi++)
    {
      if (result != "")
	result += seperator;
      if (wc == semiCapitalized)
	{
	  if (result == "")
	    result += toWordCase (*wi, lower);
	  else
	    result += toWordCase (*wi, Capitalized);
	}
      else
	result += toWordCase (*wi, wc);
    }
  return result;
}

string
CodeGenerator::rename (NamespaceType namespace_type, const string& name, WordCase namespace_wc,
		       const string &namespace_join, const vector<string> &namespace_append,
		       WordCase typename_wc, const string &typename_join)
{
  string result;
  vector<string> namespace_words;

  if (namespace_type == ABSOLUTE)
    {
      /*
       * note that if namespace_join is "::", then "::" will also be prefixed to the result,
       * whereas if namespace_join is "_", it will only be used to seperate the namespaces
       * (this is required/useful for C++)
       */
      if (namespace_join == "::")
	result = namespace_join;
      namespace_words = splitName (NamespaceHelper::namespaceOf (name));
    }

  namespace_words.insert (namespace_words.end(), namespace_append.begin(), namespace_append.end());
  if (!namespace_words.empty())
    {
      result += joinName (namespace_words, namespace_join, namespace_wc);
      result += namespace_join;
    }

  vector<string> words = splitName (NamespaceHelper::nameOf (name));
  result += joinName (words, typename_join, typename_wc);
  return result;
}

string
CodeGenerator::rename (NamespaceHelper& nsh, const string& name, WordCase namespace_wc,
		       const string& namespace_join, const vector<string>& namespace_append,
		       WordCase typename_wc, const string& typename_join)
{
  g_assert_not_reached ();
  string pform = nsh.printableForm (name);
  return pform;
}

OptionVector
CodeGenerator::getOptions()
{
  OptionVector opts;

  opts.push_back (make_pair ("--header", false));
  opts.push_back (make_pair ("--source", false));

  return opts;
}

void
CodeGenerator::setOption (const string& option, const string& value)
{
  if (option == "--header")
    {
      generateHeader = true;
      generateSource = false;
    }
  else if (option == "--source")
    {
      generateSource = true;
      generateHeader = false;
    }
}

void
CodeGenerator::help()
{
  fprintf (stderr, " --header                    generate header file (default)\n");
  fprintf (stderr, " --source                    generate source file\n");
}

/* vim:set ts=8 sts=2 sw=2: */
