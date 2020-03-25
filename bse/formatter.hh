// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_FORMATTER_HH__
#define __BSE_FORMATTER_HH__

#include <string>
#include <sstream>
#include <functional>
#include <list>

namespace Bse {

/// Namespace for implementation internals
namespace Lib {

/// Class to push a specific locale_t for the scope of its lifetime.
class ScopedLocale {
  locale_t      locale_;
  /*copy*/      ScopedLocale (const ScopedLocale&) = delete;
  ScopedLocale& operator=    (const ScopedLocale&) = delete;
protected:
  explicit      ScopedLocale (locale_t scope_locale);
public:
  // explicit   ScopedLocale (const String &locale_name = ""); // not supported
  /*dtor*/     ~ScopedLocale ();
};

/// Class to push the POSIX/C locale_t (UTF-8) for the scope of its lifetime.
class ScopedPosixLocale : public ScopedLocale {
public:
  explicit        ScopedPosixLocale ();
  static locale_t posix_locale      (); ///< Retrieve the (UTF-8) POSIX/C locale_t.
};

// == StringFormatter ==

/** StringFormatter - sprintf() like string formatting for C++.
 *
 * See format() for supported flags, modifiers and conversions.
 * To find source code strings with size modifiers for possible cleanups, use:
 * egrep "\"([^\"]|\\\")*%[0-9$]*[-+#0 \'I]*[*0-9$]*[.*0-9$]*[hlLqjzt]+[nSspmCcdiouXxFfGgEeAa]"
 */
class StringFormatter {
  typedef long long signed int LLong;
  typedef long long unsigned int ULLong;
  typedef long double LDouble;
  struct FormatArg {
    union { LDouble d; double f; signed char i1; short i2; int i4; long i6; LLong i8; void *p; const char *s; };
    char kind; // f d i u p s
  };
  inline void assign (FormatArg &farg, bool               arg) { farg.kind = '1'; farg.i1 = arg; }
  inline void assign (FormatArg &farg, char               arg) { farg.kind = '1'; farg.i1 = arg; }
  inline void assign (FormatArg &farg, signed char        arg) { farg.kind = '1'; farg.i1 = arg; }
  inline void assign (FormatArg &farg, unsigned char      arg) { farg.kind = '1'; farg.i1 = arg; }
#if __SIZEOF_WCHAR_T__ == 1
  inline void assign (FormatArg &farg, wchar_t            arg) { farg.kind = '1'; farg.i1 = arg; }
#endif
  inline void assign (FormatArg &farg, short              arg) { farg.kind = '2'; farg.i2 = arg; }
  inline void assign (FormatArg &farg, unsigned short     arg) { farg.kind = '2'; farg.i2 = arg; }
#if __SIZEOF_WCHAR_T__ == 2
  inline void assign (FormatArg &farg, wchar_t            arg) { farg.kind = '2'; farg.i2 = arg; }
#endif
  inline void assign (FormatArg &farg, int                arg) { farg.kind = '4'; farg.i4 = arg; }
  inline void assign (FormatArg &farg, unsigned int       arg) { farg.kind = '4'; farg.i4 = arg; }
#if __SIZEOF_WCHAR_T__ == 4
  inline void assign (FormatArg &farg, wchar_t            arg) { farg.kind = '4'; farg.i4 = arg; }
#endif
  inline void assign (FormatArg &farg, long               arg) { farg.kind = '6'; farg.i6 = arg; }
  inline void assign (FormatArg &farg, unsigned long      arg) { farg.kind = '6'; farg.i6 = arg; }
  inline void assign (FormatArg &farg, long long          arg) { farg.kind = '8'; farg.i8 = arg; }
  inline void assign (FormatArg &farg, unsigned long long arg) { farg.kind = '8'; farg.i8 = arg; }
  inline void assign (FormatArg &farg, float              arg) { farg.kind = 'f'; farg.f = arg; }
  inline void assign (FormatArg &farg, double             arg) { farg.kind = 'f'; farg.f = arg; }
  inline void assign (FormatArg &farg, long double        arg) { farg.kind = 'd'; farg.d = arg; }
  inline void assign (FormatArg &farg, char              *arg) { farg.kind = 's'; farg.s = arg; }
  inline void assign (FormatArg &farg, const char        *arg) { farg.kind = 's'; farg.s = arg; }
  inline void assign (FormatArg &farg, const std::string &arg) { assign (farg, arg.c_str()); }
  inline void assign (FormatArg &farg, void              *arg) { farg.kind = 'p'; farg.p = arg; }
  template<class T> inline void assign (FormatArg &farg, T *const &arg) { assign (farg, (void*) arg); }
  template<class T> typename std::enable_if<std::is_enum<T>::value, void>  // eliminated via SFINAE
  ::type      assign (FormatArg &farg, const T           &arg) { farg.kind = '8'; farg.i8 = LLong (arg); }
  template<class T> typename std::enable_if<std::is_class<T>::value, void> // eliminated via SFINAE
  ::type      assign (FormatArg &farg, const T           &arg)
  {
    std::ostringstream os;
    os << arg;
    temporaries_.push_back (os.str());
    assign (farg, temporaries_.back());
  }
  const FormatArg& format_arg       (size_t nth);
  uint32_t         arg_as_width     (size_t nth);
  uint32_t         arg_as_precision (size_t nth);
  LLong            arg_as_longlong  (size_t nth);
  LDouble          arg_as_ldouble   (size_t nth);
  const char*      arg_as_chars     (size_t nth);
  void*            arg_as_ptr       (size_t nth);
  struct Directive {
    char     conversion;
    uint32_t adjust_left : 1, add_sign : 1, use_width : 1, use_precision : 1;
    uint32_t alternate_form : 1, zero_padding : 1, add_space : 1, locale_grouping : 1;
    uint32_t field_width, precision, start, end, value_index, width_index, precision_index;
    Directive() :
      conversion (0), adjust_left (0), add_sign (0), use_width (0), use_precision (0),
      alternate_form (0), zero_padding (0), add_space (0), locale_grouping (0),
      field_width (0), precision (0), start (0), end (0), value_index (0), width_index (0), precision_index (0)
    {}
  };
  typedef std::function<std::string (const std::string&)> ArgTransform;
  FormatArg          *const fargs_;
  const size_t        nargs_;
  const int           locale_context_;
  const ArgTransform &arg_transform_;
  std::list<std::string> temporaries_; // must use list to keep c_str() references stable
  static std::string            format_error     (const char *err, const char *format, size_t directive);
  static const char*            parse_directive  (const char **stringp, size_t *indexp, Directive *dirp);
  std::string                   locale_format    (size_t last, const char *format);
  std::string                   render_format    (size_t last, const char *format);
  std::string                   render_directive (const Directive &dir);
  template<class A> std::string render_arg       (const Directive &dir, const char *modifier, A arg);
  template<size_t N> inline std::string
  intern_format (const char *format)
  {
    return locale_format (N, format);
  }
  template<size_t N, class A, class ...Args> inline std::string
  intern_format (const char *format, const A &arg, const Args &...args)
  {
    assign (fargs_[N], arg);
    return intern_format<N+1> (format, args...);
  }
  template<size_t N> inline constexpr
  StringFormatter (const ArgTransform &arg_transform, size_t nargs, FormatArg (&mem)[N], int lc) :
    fargs_ (mem), nargs_ (nargs), locale_context_ (lc), arg_transform_ (arg_transform) {}
public:
  enum LocaleContext {
    POSIX_LOCALE,
    CURRENT_LOCALE,
  };
  /** Format a string according to an sprintf() @a format string with @a arguments.
   * Refer to sprintf() for the format string details, this function is designed to
   * serve as an sprintf() replacement and mimick its behaviour as close as possible.
   * Supported format directive features are:
   * - Formatting flags (sign conversion, padding, alignment), i.e. the flags: [-#0+ ']
   * - Field width and precision specifications.
   * - Positional arguments for field width, precision and value.
   * - Length modifiers are tolerated: i.e. any of [hlLjztqZ].
   * - The conversion specifiers [spmcCdiouXxFfGgEeAa].
   *
   * Additionally, arguments can be transformed after conversion by passing a std::string
   * conversion function as @a arg_transform. This may e.g. be used for XML character
   * escaping of the format argument values. <br/>
   * @NOTE Format errors, e.g. missing arguments will produce a warning on stderr and
   * return the @a format string unmodified.
   * @returns A formatted string.
   */
  template<LocaleContext LC = POSIX_LOCALE, class ...Args>
  static __attribute__ ((__format__ (printf, 2, 0), noinline)) std::string
  format (const ArgTransform &arg_transform, const char *format, const Args &...arguments)
  {
    constexpr size_t N = sizeof... (Args);
    FormatArg mem[N ? N : 1];
    StringFormatter formatter (arg_transform, N, mem, LC);
    return formatter.intern_format<0> (format, arguments...);
  }
};

} // Lib
} // Bse

#endif  // __BSE_FORMATTER_HH__
