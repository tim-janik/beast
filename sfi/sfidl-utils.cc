#include "sfidl-utils.hh"

namespace Sfidl {

/** returns true for C++ style identifiers (Foo::BAR) - only the colons are checked, not individual chars
 */
bool
isCxxTypeName (const String& str)
{
  enum { valid, colon1, colon2, invalid } state = valid;
  for (String::const_iterator i = str.begin(); i != str.end(); i++)
    {
      if (state == valid)
	{
	  if (*i == ':')
	    state = colon1;
	}
      else if (state == colon1)
	{
	  if (*i == ':')
	    state = colon2;
	  else
	    state = invalid;
	}
      else if (state == colon2)
	{
	  if (*i == ':')
	    state = invalid;
	  else
	    state = valid;
	}
    }
  return (state == valid) && (str.size() != 0);
}

list<String>
symbolToList (const String& symbol)
{
  list<String> result;
  String current;

  assert_return (isCxxTypeName (symbol), result);

  for (String::const_iterator si = symbol.begin(); si != symbol.end(); si++)
    {
      if (*si != ':')
	{
	  current += *si;
	}
      else
	{
	  if (current != "")
	    result.push_back(current);

	  current = "";
	}
    }

  result.push_back(current);
  return result;
}

} // Sfidl

#include "glib-extra.cc" // needed by sfidl
