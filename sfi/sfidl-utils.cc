#include "sfidl-utils.hh"

namespace Sfidl {

extern inline unichar unicode_tolower (unichar uc)    { return g_unichar_tolower (uc); }

String
string_tolower (const String &str)
{
  String s (str);
  for (size_t i = 0; i < s.size(); i++)
    s[i] = unicode_tolower (s[i]);
  return s;
}

#define LLI (long long int)
#define LLU (long long unsigned int)

String
string_from_int (int64 value)
{
  return string_format ("%lld", LLI value);
}

String
string_from_uint (uint64 value)
{
  return string_format ("%llu", LLU value);
}

static std::string
string_vformat (const char *format, va_list argv)
{
  if (!format)
    return "";
  int l;
  {
    const int length = 256;             // fast path, use small buffer
    char buffer[length + 1] = { 0, };   // zero fill buffer
    va_list argv2;
    va_copy (argv2, argv);
    l = vsnprintf (buffer, length, format, argv2);
    va_end (argv2);
    if (l >= 0 && l <= length)
      return buffer;
  }
  if (l < 0)
    return format;
  l += 1;                               // avoid off-by-one errors in printf
  std::string output;
  output.resize (l + 1, 0);
  va_list argv2;
  va_copy (argv2, argv);
  const int j = vsnprintf (&output[0], l, format, argv2);
  va_end (argv2);
  if (j < 0 || j > l)
    return format;
  return output;
}

std::string
string_format (const char *format, ...)
{
  va_list argv;
  va_start (argv, format);
  std::string output = string_vformat (format, argv);
  va_end (argv);
  return output;
}

void
printerr (const char *format, ...)
{
  va_list argv;
  va_start (argv, format);
  fflush (stdout);
  vfprintf (stderr, format, argv);
  fflush (stderr);
  va_end (argv);
}

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

#include "bse/glib-extra.inc.cc" // needed by sfidl
