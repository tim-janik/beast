// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MAIN_H__
#define __BSE_MAIN_H__

#include <bse/bseenums.hh>

// == BSE Initialization ==
void _bse_init_async	(const char *app_name, const Bse::StringVector &args);
bool _bse_initialized	();
int  bse_init_and_test 	(const Bse::StringVector &args, const std::function<int()> &bsetester);
void bse_main_wakeup    ();

namespace Bse {

// == Bse::GlobalPreferences ==
struct GlobalPreferences : Preferences {
  static Preferences   defaults ();
  static void          assign   (const Preferences &preferences);
  static void          flush    ();
  static void          lock     ();
  static void          unlock   ();
  static bool          locked   ();
};
struct GlobalPreferencesPtr {
  const GlobalPreferences* operator-> () const;
  const GlobalPreferences& operator*  () const { return *operator->(); }
};
inline GlobalPreferencesPtr global_prefs;

// == Bse Configuration ==
String          config_string       (const String &key, const String &fallback = "");
bool            config_bool         (const String &key, bool fallback = false);
int64           config_int          (const String &key, int64 fallback = 0);

bool* register_driver_loader (const char *staticwhat, Error (*loader) ());

class JobQueue {
  static void call_remote (const std::function<void()>&);
  template<typename T, typename = void>
  struct LambdaTraits;
  template<typename R> struct LambdaTraits<R (*)()> { using ReturnType = R; };
  template<typename R, typename C>
  struct LambdaTraits<R (C::*)() const> { using ReturnType = R; };
  template<typename T> struct LambdaTraits<T, void_t< decltype (&T::operator()) > > :
    public LambdaTraits<decltype (&T::operator())> {};
public:
  template<typename F> typename LambdaTraits<F>::ReturnType
  operator+= (const F &job)
  {
    using R = typename LambdaTraits<F>::ReturnType;
    if constexpr (std::is_same<R, void>::value)
      {
        call_remote (job);
        return;
      }
    else // R != void
      {
        R result;
        call_remote ([&job, &result] () {
            result = job();
          });
        return result;
      }
  }
};
/// Execute a lambda job in the Bse main loop and wait for its result.
extern JobQueue jobs;

} // Bse


/* --- global macros --- */
#define	BSE_THREADS_ENTER()			// bse_main_global_lock ()
#define	BSE_THREADS_LEAVE()			// bse_main_global_unlock ()

/* --- internal --- */
void    _bse_init_c_wrappers    ();

#endif /* __BSE_MAIN_H__ */
