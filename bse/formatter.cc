// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "formatter.hh"
#include <unistd.h>     // isatty
#include <cstring>

/** @TODO:
 * - StringFormatter: support directives: %%n %%S %%ls
 * - StringFormatter: support directive flags: I
 */

#ifdef __clang__ // clang++-3.8.0: work around 'variable length array of non-POD element type'
#define CC_DECLARE_VLA(Type, var, count)        std::vector<Type> var (count)
#else // sane c++
#define CC_DECLARE_VLA(Type, var, count)        Type var[count]
#endif

namespace Bse {
namespace Lib {

template<class... Args> static std::string
system_string_printf (const char *format, Args... args)
{
  char *cstr = NULL;
  int ret = asprintf (&cstr, format, args...);
  if (ret >= 0 && cstr)
    {
      std::string result = cstr;
      free (cstr);
      return result;
    }
  return format;
}

static bool
parse_unsigned_integer (const char **stringp, uint64_t *up)
{ // '0' | [1-9] [0-9]* : <= 18446744073709551615
  const char *p = *stringp;
  // zero
  if (*p == '0' && !(p[1] >= '0' && p[1] <= '9'))
    {
      *up = 0;
      *stringp = p + 1;
      return true;
    }
  // first digit
  if (!(*p >= '1' && *p <= '9'))
    return false;
  uint64_t u = *p - '0';
  p++;
  // rest digits
  while (*p >= '0' && *p <= '9')
    {
      const uint64_t last = u;
      u = u * 10 + (*p - '0');
      p++;
      if (u < last) // overflow
        return false;
    }
  *up = u;
  *stringp = p;
  return true;
}

static bool
parse_positional (const char **stringp, uint64_t *ap)
{ // [0-9]+ '$'
  const char *p = *stringp;
  uint64_t ui64 = 0;
  if (parse_unsigned_integer (&p, &ui64) && *p == '$')
    {
      p++;
      *ap = ui64;
      *stringp = p;
      return true;
    }
  return false;
}

const char*
StringFormatter::parse_directive (const char **stringp, size_t *indexp, Directive *dirp)
{ // '%' positional? [-+#0 '']* ([0-9]*|[*]positional?) ([.]([0-9]*|[*]positional?))? [hlLjztqZ]* [spmcCdiouXxFfGgEeAa]
  const char *p = *stringp;
  size_t index = *indexp;
  Directive fdir;
  // '%' directive start
  if (*p != '%')
    return "missing '%' at start";
  p++;
  // positional argument
  uint64_t ui64 = -1;
  if (parse_positional (&p, &ui64))
    {
      if (ui64 > 0 && ui64 <= 2147483647)
        fdir.value_index = ui64;
      else
        return "invalid positional specification";
    }
  // flags
  const char *flags = "-+#0 '";
  while (strchr (flags, *p))
    switch (*p)
      {
      case '-': fdir.adjust_left = true;        goto default_case;
      case '+': fdir.add_sign = true;           goto default_case;
      case '#': fdir.alternate_form = true;     goto default_case;
      case '0': fdir.zero_padding = true;       goto default_case;
      case ' ': fdir.add_space = true;          goto default_case;
      case '\'': fdir.locale_grouping = true;   goto default_case;
      default: default_case:
        p++;
        break;
      }
  // field width
  ui64 = 0;
  if (*p == '*')
    {
      p++;
      if (parse_positional (&p, &ui64))
        {
          if (ui64 > 0 && ui64 <= 2147483647)
            fdir.width_index = ui64;
          else
            return "invalid positional specification";
        }
      else
        fdir.width_index = index++;
      fdir.use_width = true;
    }
  else if (parse_unsigned_integer (&p, &ui64))
    {
      if (ui64 <= 2147483647)
        fdir.field_width = ui64;
      else
        return "invalid field width specification";
      fdir.use_width = true;
    }
  // precision
  if (*p == '.')
    {
      fdir.use_precision = true;
      p++;
    }
  if (*p == '*')
    {
      p++;
      if (parse_positional (&p, &ui64))
        {
          if (ui64 > 0 && ui64 <= 2147483647)
            fdir.precision_index = ui64;
          else
            return "invalid positional specification";
        }
      else
        fdir.precision_index = index++;
    }
  else if (parse_unsigned_integer (&p, &ui64))
    {
      if (ui64 <= 2147483647)
        fdir.precision = ui64;
      else
        return "invalid precision specification";
    }
  // modifiers
  const char *modifiers = "hlLjztqZ";
  while (strchr (modifiers, *p))
    p++;
  // conversion
  const char *conversion = "dioucCspmXxEeFfGgAa%";
  if (!strchr (conversion, *p))
    return "missing conversion specifier";
  if (fdir.value_index == 0 && !strchr ("m%", *p))
    fdir.value_index = index++;
  fdir.conversion = *p++;
  if (fdir.conversion == 'C')   // %lc in SUSv2
    fdir.conversion = 'c';
  // success
  *dirp = fdir;
  *indexp = index;
  *stringp = p;
  return NULL; // OK
}

const StringFormatter::FormatArg&
StringFormatter::format_arg (size_t nth)
{
  if (nth && nth <= nargs_)
    return fargs_[nth-1];
  static const FormatArg zero_arg = { { 0, }, 0 };
  return zero_arg;
}

StringFormatter::LDouble
StringFormatter::arg_as_ldouble (size_t nth)
{
  const FormatArg &farg = format_arg (nth);
  switch (farg.kind)
    {
    case '1':   return farg.i1;
    case '2':   return farg.i2;
    case '4':   return farg.i4;
    case '6':   return farg.i6;
    case '8':   return farg.i8;
    case 'f':   return farg.f;
    case 'd':   return farg.d;
    case 'p':   return ULLong (farg.p);
    case 's':   return ULLong (farg.s);
    default:    return 0;
    }
}

const char*
StringFormatter::arg_as_chars (size_t nth)
{
  if (!(nth && nth <= nargs_))
    return "";
  if ((fargs_[nth-1].kind == 's' || fargs_[nth-1].kind == 'p') && fargs_[nth-1].p == NULL)
    return "(null)";
  return fargs_[nth-1].kind != 's' ? "" : fargs_[nth-1].s;
}

void*
StringFormatter::arg_as_ptr (size_t nth)
{
  if (!(nth && nth <= nargs_))
    return NULL;
  return fargs_[nth-1].p;
}

StringFormatter::LLong
StringFormatter::arg_as_longlong (size_t nth)
{
  const FormatArg &farg = format_arg (nth);
  switch (farg.kind)
    {
    case '1':   return farg.i1;
    case '2':   return farg.i2;
    case '4':   return farg.i4;
    case '6':   return farg.i6;
    case '8':   return farg.i8;
    case 'f':   return farg.f;
    case 'd':   return farg.d;
    case 'p':   return LLong (farg.p);
    case 's':   return LLong (farg.s);
    default:    return 0;
    }
}

uint32_t
StringFormatter::arg_as_width (size_t nth)
{
  int32_t w = arg_as_longlong (nth);
  w = std::abs (w);
  return w < 0 ? std::abs (w + 1) : w; // turn -2147483648 into +2147483647
}

uint32_t
StringFormatter::arg_as_precision (size_t nth)
{
  const int32_t precision = arg_as_longlong (nth);
  return std::max (0, precision);
}

template<class Arg> std::string
StringFormatter::render_arg (const Directive &dir, const char *modifier, Arg arg)
{
  std::string result, format;
  const int field_width = !dir.use_width || !dir.width_index ? dir.field_width : arg_as_width (dir.width_index);
  const int field_precision = !dir.use_precision || !dir.precision_index ? std::max (uint32_t (0), dir.precision) : arg_as_precision (dir.precision_index);
  // format directive
  format += '%';
  if (dir.adjust_left)
    format += '-';
  if (dir.add_sign)
    format += '+';
  if (dir.add_space)
    format += ' ';
  if (dir.zero_padding && !dir.adjust_left&& strchr ("diouXx" "FfGgEeAa", dir.conversion))
    format += '0';
  if (dir.alternate_form && strchr ("oXx" "FfGgEeAa", dir.conversion))
    format += '#';
  if (dir.locale_grouping && strchr ("idu" "FfGg", dir.conversion))
    format += '\'';
  if (dir.use_width)
    format += '*';
  if (dir.use_precision && strchr ("sm" "diouXx" "FfGgEeAa", dir.conversion)) // !cp
    format += ".*";
  if (modifier)
    format += modifier;
  format += dir.conversion;
  // printf formatting
  if (dir.use_width && dir.use_precision)
    return system_string_printf (format.c_str(), field_width, field_precision, arg);
  else if (dir.use_precision)
    return system_string_printf (format.c_str(), field_precision, arg);
  else if (dir.use_width)
    return system_string_printf (format.c_str(), field_width, arg);
  else
    return system_string_printf (format.c_str(), arg);
}

std::string
StringFormatter::render_directive (const Directive &dir)
{
  switch (dir.conversion)
    {
    case 'm':
      return render_arg (dir, "", int (0)); // dummy arg to silence compiler
    case 'p':
      return render_arg (dir, "", arg_as_ptr (dir.value_index));
    case 's': // precision
      return render_arg (dir, "", arg_as_chars (dir.value_index));
    case 'c': case 'd': case 'i': case 'o': case 'u': case 'X': case 'x':
      switch (format_arg (dir.value_index).kind)
        {
        case '1':       return render_arg (dir, "hh", format_arg (dir.value_index).i1);
        case '2':       return render_arg (dir, "h", format_arg (dir.value_index).i2);
        case '4':       return render_arg (dir, "", format_arg (dir.value_index).i4);
        case '6':       return render_arg (dir, "l", format_arg (dir.value_index).i6);
        case '8':       return render_arg (dir, "ll", format_arg (dir.value_index).i8);
        default:        return render_arg (dir, "ll", arg_as_longlong (dir.value_index));
        }
    case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A':
      switch (format_arg (dir.value_index).kind)
        {
        case 'f':       return render_arg (dir, "", format_arg (dir.value_index).f);
        case 'd':
        default:        return render_arg (dir, "L", arg_as_ldouble (dir.value_index));
        }
    case '%':
      return "%";
    }
  return std::string ("%") + dir.conversion;
}

static inline size_t
upper_directive_count (const char *format)
{
  size_t n = 0;
  for (const char *p = format; *p; p++)
    if (p[0] == '%')            // count %...
      {
        n++;
        if (p[1] == '%')        // dont count %% twice
          p++;
      }
  return n;
}

std::string
StringFormatter::render_format (const size_t last, const char *format)
{
  if (last != nargs_)
    { // should never be reached
      fputs (__FILE__ ": template argument list unpacking failed\n", stderr);
      return "";
    }
  // allocate enough space to hold all directives possibly contained in format
  const size_t max_dirs = 1 + upper_directive_count (format);
  CC_DECLARE_VLA (Directive, fdirs, max_dirs); // Directive fdirs[max_dirs];
  // parse format into Directive stack
  size_t nextarg = 1, ndirs = 0;
  const char *p = format;
  while (*p)
    {
      do
        {
          if (p[0] == '%')
            break;
          p++;
        }
      while (*p);
      if (*p == 0)
        break;
      const size_t start = p - format;
      const char *err = parse_directive (&p, &nextarg, &fdirs[ndirs]);
      if (err)
        return format_error (err, format, ndirs + 1);
      fdirs[ndirs].start = start;
      fdirs[ndirs].end = p - format;
      ndirs++;
      if (!(ndirs < max_dirs))
        { // should never be reached
          fputs (__FILE__ ": invalid result from upper_directive_count()", stderr);
          return "";
        }
    }
  const size_t argcounter = nextarg - 1;
  fdirs[ndirs].end = fdirs[ndirs].start = p - format;
  // check maximum argument reference and argument count
  size_t argmaxref = argcounter;
  for (size_t i = 0; i < ndirs; i++)
    {
      const Directive &fdir = fdirs[i];
      argmaxref = std::max (argmaxref, size_t (fdir.value_index));
      argmaxref = std::max (argmaxref, size_t (fdir.width_index));
      argmaxref = std::max (argmaxref, size_t (fdir.precision_index));
    }
  if (argmaxref > last)
    return format_error ("too few arguments for format", format, 0);
  if (argmaxref < last)
    return format_error ("too many arguments for format", format, 0);
  // format pieces
  std::string result;
  p = format;
  for (size_t i = 0; i <= ndirs; i++)
    {
      const Directive &fdir = fdirs[i];
      result += std::string (p, fdir.start - (p - format));
      if (fdir.conversion)
        {
          std::string rendered_arg = render_directive (fdir);
          if (arg_transform_)
            rendered_arg = arg_transform_ (rendered_arg);
          result += rendered_arg;
        }
      p = format + fdir.end;
    }
  return result;
}

std::string
StringFormatter::locale_format (const size_t last, const char *format)
{
  if (locale_context_ == CURRENT_LOCALE)
    return render_format (last, format);
  else
    {
      ScopedPosixLocale posix_locale_scope; // pushes POSIX locale for this scope
      return render_format (last, format);
    }
}

std::string
StringFormatter::format_error (const char *err, const char *format, size_t directive)
{
  const char *cyan = "", *cred = "", *cyel = "", *crst = "";
  if (isatty (fileno (stderr)))
    {
      const char *term = getenv ("TERM");
      if (term && strcmp (term, "dumb") != 0)
        {
          cyan = "\033[36m";
          cred = "\033[31m\033[1m";
          cyel = "\033[33m";
          crst = "\033[39m\033[22m";
        }
    }
  if (directive)
    fprintf (stderr, "%sStringFormatter: %sWARNING:%s%s %s in directive %zu:%s %s\n", cyan, cred, crst, cyel, err, directive, crst, format);
  else
    fprintf (stderr, "%sStringFormatter: %sWARNING:%s%s %s:%s %s\n", cyan, cred, crst, cyel, err, crst, format);
  return format;
}

ScopedLocale::ScopedLocale (locale_t scope_locale) :
  locale_ (NULL)
{
  if (!scope_locale)
    locale_ = uselocale (LC_GLOBAL_LOCALE);     // use process locale
  else
    locale_ = uselocale (scope_locale);         // use custom locale
  if (locale_ == NULL)
    fprintf (stderr, "%s: WARNING: uselocale() returned NULL\n", __FILE__);
}

ScopedLocale::~ScopedLocale ()
{
  uselocale (locale_);                          // restore locale
}

#if 0
ScopedLocale::ScopedLocale (const String &locale_name = "")
{
  /* this constructor should:
   * - uselocale (LC_GLOBAL_LOCALE) if locale_name == "",
   * - create newlocale from locale_name, use it and later delete it, but:
   * - freelocale(newlocale()) seems buggy on glibc-2.7 (crashes)
   */
}
#endif

ScopedPosixLocale::ScopedPosixLocale () :
  ScopedLocale (posix_locale())
{}

locale_t
ScopedPosixLocale::posix_locale ()
{
  static locale_t cached_posix_locale = [] () {
    locale_t posix_locale = NULL;
    if (!posix_locale)
      posix_locale = newlocale (LC_ALL_MASK, "POSIX.UTF-8", NULL);
    if (!posix_locale)
      posix_locale = newlocale (LC_ALL_MASK, "C.UTF-8", NULL);
    if (!posix_locale)
      posix_locale = newlocale (LC_ALL_MASK, "POSIX", NULL);
    if (!posix_locale)
      posix_locale = newlocale (LC_ALL_MASK, "C", NULL);
    if (!posix_locale)
      posix_locale = newlocale (LC_ALL_MASK, NULL, NULL);
    if (posix_locale == NULL)
      fprintf (stderr, "%s: WARNING: newlocale() returned NULL\n", __FILE__);
    return posix_locale;
  } ();
  return cached_posix_locale;
}

} // Lib
} // Bse

#include "testing.hh"

namespace { // Anon
struct UncopyablePoint {
  double x, y;
  friend inline std::ostream&
  operator<< (std::ostream &s, const UncopyablePoint &p) { return s << "{" << p.x << ";" << p.y << "}"; }
  UncopyablePoint (double _x, double _y) : x (_x), y (_y) {}
  BSE_CLASS_NON_COPYABLE (UncopyablePoint);
};

BSE_INTEGRITY_TEST (bse_string_format);
static void
bse_string_format()
{
  using namespace Bse;
  // string_format
  enum { TEST17 = 17 };
  TCMP (string_format ("%d %s", -9223372036854775808uLL, "FOO"), ==, "-9223372036854775808 FOO");
  TCMP (string_format ("%g %d", 0.5, TEST17), ==, "0.5 17");
  TCMP (string_format ("0x%08x", 0xc0ffee), ==, "0x00c0ffee");
  static_assert (TEST17 == 17, "!");
  TCMP (string_format ("Only %c%%", '3'), ==, "Only 3%");
  // ostream tests
  UncopyablePoint point { 1, 2 };
  TCMP (string_format ("%s", point), ==, "{1;2}");
  TCMP (string_format ("%s+%s+%s", point, point, point), ==, "{1;2}+{1;2}+{1;2}");
  String sfoo ("foo");
  typedef char MutableChar;
  MutableChar *foo = &sfoo[0];
  TCMP (string_format ("%s", foo), ==, "foo");
  // test robustness for arcane/rarely-used width modifiers
  const char *arcane_format = "| %qd %Zd %LF |";
  TCMP (string_format (arcane_format, (long long) 1234, size_t (4321), (long double) 1234.), ==, "| 1234 4321 1234.000000 |");
  TCMP (string_format ("- %C - %lc -", long ('X'), long ('x')), ==, "- X - x -");
  // TCMP (string_format ("+ %S +", (wchar_t*) "\1\1\1\1\0\0\0\0"), ==, "+ \1\1\1\1 +");
}

} // Anon
