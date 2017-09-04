// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfidl-generator.hh"
#include "sfidl-factory.hh"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "sfidl-namespace.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"
#include "sfiparams.hh" /* scatId (SFI_SCAT_*) */

using namespace Sfidl;
using std::make_pair;

/*--- common functions ---*/

vector<String> CodeGenerator::splitName (const String& name)
{
  bool lastunder = true, remove_caps = false;
  String::const_iterator i;
  vector<String> words;

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

  /*
   * here, we split "name" into words
   *
   * as a rule for what is a word boundary, the following criteria are used
   *  - two colons denote word boundaries:  "Bse::Container" => "Bse", "Container"
   *  - underscores denote word boundaries: "audio_channel" => "audio", "channel"
   *  - single caps are word boundaries:    "AudioChannel" => "Audio", "Channel"
   *  - double caps followed by single
   *    constitute a seperate word:         "WMHints" => "WM", "Hints"
   *  - triple and higher caps are also
   *    treated this way:                   "FFTSize" => "FFT", "Size"
   */
  int caps = 0;
  String::const_iterator word_start = name.begin(), word_end = name.begin();
  for(i = name.begin(); i != name.end(); i++)
    {
      if (isupper (*i))
        caps++;
      else
        caps = 0;

      if (*i == ':' || *i == '_') /* underscore/colon indicates word boundary */
	{
	  if (!lastunder)
	    {
	      words.push_back (String (word_start, word_end));
	      lastunder = true;
	    }
          word_start = word_end = i + 1;
	}
      else
        {
          bool next_lower = (i + 1) != name.end() && islower (*(i + 1));

          if ((caps == 1 && word_start != word_end) || (caps > 2 && next_lower)) /* caps indicate word boundary */
            {
              words.push_back (String (word_start, word_end));
              word_start = word_end = i;
            }
          word_end++;
          lastunder = false;
        }
    }

  if (word_start != word_end) /* handle last word in string */
    words.push_back (String (word_start, word_end));

  if (remove_caps)
    for (vector<String>::iterator wi = words.begin(); wi != words.end(); wi++)
      *wi = string_tolower (*wi);

  return words;
}


String CodeGenerator::makeLowerName (const String& name, char seperator)
{
  String result;
  const vector<String>& words = splitName (name);

  for (vector<String>::const_iterator wi = words.begin(); wi != words.end(); wi++)
    {
      if (result != "") result += seperator;

      for (String::const_iterator i = wi->begin(); i != wi->end(); i++)
	result += tolower (*i);
    }

  return result;
}

String CodeGenerator::makeUpperName (const String& name)
{
  String lname = makeLowerName (name);
  String result;
  String::const_iterator i;

  for(i = lname.begin(); i != lname.end(); i++)
    result += toupper(*i);
  return result;
}

String CodeGenerator::makeMixedName (const String& name)
{
  String result;
  const vector<String>& words = splitName (name);

  for (vector<String>::const_iterator wi = words.begin(); wi != words.end(); wi++)
    {
      bool first = true;

      for (String::const_iterator i = wi->begin(); i != wi->end(); i++)
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

String CodeGenerator::makeLMixedName (const String& name)
{
  String result = makeMixedName (name);

  if (!result.empty()) result[0] = tolower (result[0]);
  return result;
}

String CodeGenerator::toWordCase (const String& word, WordCase wc)
{
  String result;
  for (String::const_iterator si = word.begin(); si != word.end(); si++)
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
	  default:	     assert_return_unreached ("");
	}
    }
  return result;
}

String CodeGenerator::joinName (const vector<String>& name, const String& seperator, WordCase wc)
{
  String result;

  for (vector<String>::const_iterator wi = name.begin(); wi != name.end(); wi++)
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

String
CodeGenerator::rename (NamespaceType namespace_type, const String& name, WordCase namespace_wc,
		       const String &namespace_join, const vector<String> &namespace_append,
		       WordCase typename_wc, const String &typename_join)
{
  String result;
  vector<String> namespace_words;

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

  vector<String> words = splitName (NamespaceHelper::nameOf (name));
  result += joinName (words, typename_join, typename_wc);
  return result;
}

String
CodeGenerator::rename (NamespaceHelper& nsh, const String& name, WordCase namespace_wc,
		       const String& namespace_join, const vector<String>& namespace_append,
		       WordCase typename_wc, const String& typename_join)
{
  assert_return_unreached ("");
  String pform = nsh.printableForm (name);
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
CodeGenerator::setOption (const String& option, const String& value)
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
