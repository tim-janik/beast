// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gslcommon.hh"

#include "gsldatacache.hh"
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/time.h>

namespace Bse {

// == TickStamp ==
static std::mutex               global_tick_stamp_mutex;
static uint64	                tick_stamp_system_time = 0;
static uint64                   tick_stamp_leaps = 0;
std::atomic<uint64>             TickStamp::global_tick_stamp { 0 }; // initialized to 1 from gsl_init(), so 0 == invalid
static std::list<TickStampWakeupP> tick_stamp_wakeups;

void
TickStamp::_init_forgsl()
{
  assert_return (global_tick_stamp == 0);    // assert we're uninitialized
  global_tick_stamp = 1;
}

void
TickStamp::_set_leap (uint64 ticks)
{
  std::lock_guard<std::mutex> locker (global_tick_stamp_mutex);
  tick_stamp_leaps = ticks;
}

void
TickStamp::_increment ()
{
  assert_return (tick_stamp_leaps > 0);
  volatile guint64 newstamp;
  uint64 systime;
  systime = sfi_time_system ();
  newstamp = global_tick_stamp + tick_stamp_leaps;
  {
    std::lock_guard<std::mutex> locker (global_tick_stamp_mutex);
    global_tick_stamp = newstamp;
    tick_stamp_system_time = systime;
  }
  struct Internal : Wakeup { using Wakeup::_emit_wakeups; };
  Internal::_emit_wakeups (newstamp);
}

#ifdef  DOXYGEN
/**
 * Retrieve BSE execution tick stamp as unsigned 64bit integer.
 *
 * Returns the global GSL tick counter stamp.
 * BSE increments its global tick stamp at certain intervals,
 * by specific amounts (refer to bse_engine_init() for further
 * details). The tick stamp is a non-wrapping, unsigned 64bit
 * integer greater than 0.
 * Threads can schedule sleep interruptions at certain tick
 * stamps with awake_after() and awake_before().
 * Tick stamp updating occours at BSE engine block processing
 * boundaries, so code that can guarantee to not run across
 * those boundaries (for instance BseProcessFunc() functions)
 * may use Bse::TickStamp::current() to retrieve the current
 * tick in a fast manner (not involving mutex locking).
 * See also bse_module_tick_stamp().
 * This function is MT-safe and may be called from any thread.
 */
uint64 TickStamp::current () { ... }
#endif

/**
 * @return Current tick stamp and system time in micro seconds
 *
 * Get the system time of the last GSL global tick stamp update.
 * This function is MT-safe and may be called from any thread.
 */
TickStamp::Update
TickStamp::get_last()
{
  Update ustamp;
  std::lock_guard<std::mutex> locker (global_tick_stamp_mutex);
  ustamp.tick_stamp = global_tick_stamp;
  ustamp.system_time = tick_stamp_system_time;
  return ustamp;
}

TickStamp::Wakeup::Wakeup (const std::function<void()> &wakeup) :
  wakeup_ (wakeup), awake_stamp_ (0)
{}

TickStampWakeupP
TickStamp::create_wakeup (const std::function<void()> &wakeup)
{
  struct WakeupImpl : TickStamp::Wakeup {
    WakeupImpl (const std::function<void()> &wakeup) :
      Wakeup (wakeup)
    {}
  };
  auto wp = std::make_shared<WakeupImpl> (wakeup);
  return wp;
}

/**
 * @param stamp stamp to trigger wakeup
 *
 * Wake the current thread up at a future tick increment which exceeds @a stamp.
 */
void
TickStamp::Wakeup::awake_after (uint64 stamp)
{
  std::lock_guard<std::mutex> locker (global_tick_stamp_mutex);
  if (!awake_stamp_ && stamp)
    {
      tick_stamp_wakeups.push_back (shared_from_this());
      awake_stamp_ = stamp;
    }
  else if (!stamp && awake_stamp_)
    {
      tick_stamp_wakeups.remove (shared_from_this());
      awake_stamp_ = 0;
    }
  else if (awake_stamp_ && stamp)
    awake_stamp_ = MIN (awake_stamp_, stamp);
}

/**
 * @param stamp tick stamp update to trigger wakeup
 *
 * Wakeup the currently running thread upon the last global tick stamp
 * update (see Bse::TickStamp::current()) that happens prior to updating the
 * global tick stamp to @a tick_stamp.
 * (If the moment of wakeup has already passed by, the thread is
 * woken up at the next global tick stamp update.)
 */
void
TickStamp::Wakeup::awake_before (uint64 stamp)
{
  assert_return (stamp > 0);
  if (stamp > tick_stamp_leaps)
    stamp -= tick_stamp_leaps;
  awake_after (stamp);
}

void
TickStamp::Wakeup::_emit_wakeups (uint64 wakeup_stamp)
{
  std::lock_guard<std::mutex> locker (global_tick_stamp_mutex);
  std::list<TickStampWakeupP> list, notifies;
  list.swap (tick_stamp_wakeups);
  for (auto it : list)
    if (it->awake_stamp_ > wakeup_stamp)
      tick_stamp_wakeups.push_back (it);
    else // awake_stamp_ <= wakeup_stamp
      notifies.push_back (it);
  for (auto it : notifies)
    if (it->wakeup_)
      {
        it->awake_stamp_ = 0;
        it->wakeup_();
      }
}

} // Bse


/* --- misc --- */
const gchar*
gsl_byte_order_to_string (guint byte_order)
{
  assert_return (byte_order == G_LITTLE_ENDIAN || byte_order == G_BIG_ENDIAN, NULL);
  if (byte_order == G_LITTLE_ENDIAN)
    return "little-endian";
  if (byte_order == G_BIG_ENDIAN)
    return "big-endian";
  return NULL;
}
guint
gsl_byte_order_from_string (const gchar *string)
{
  assert_return (string != NULL, 0);
  while (*string == ' ')
    string++;
  if (strncasecmp (string, "little", 6) == 0)
    return G_LITTLE_ENDIAN;
  if (strncasecmp (string, "big", 3) == 0)
    return G_BIG_ENDIAN;
  return 0;
}

Bse::Error
gsl_file_check (const std::string &file_name, const std::string &mode)
{
  if (Bse::Path::check (file_name, mode))
    return Bse::Error::NONE;
  return gsl_error_from_errno (errno, Bse::Error::FILE_OPEN_FAILED);
}

Bse::Error
gsl_error_from_errno (gint         sys_errno,
		      Bse::Error fallback)
{
  switch (sys_errno)
    {
    case 0:             return Bse::Error::NONE;
    case ELOOP:
    case ENAMETOOLONG:
    case ENOENT:        return Bse::Error::FILE_NOT_FOUND;
    case EISDIR:        return Bse::Error::FILE_IS_DIR;
    case EROFS:
    case EPERM:
    case EACCES:        return Bse::Error::PERMS;
#ifdef ENODATA  /* GNU/kFreeBSD lacks this */
    case ENODATA:
#endif
    case ENOMSG:        return Bse::Error::FILE_EOF;
    case ENOMEM:	return Bse::Error::NO_MEMORY;
    case ENOSPC:	return Bse::Error::NO_SPACE;
    case ENFILE:	return Bse::Error::NO_FILES;
    case EMFILE:	return Bse::Error::MANY_FILES;
    case EFBIG:
    case ESPIPE:
    case EIO:           return Bse::Error::IO;
    case EEXIST:        return Bse::Error::FILE_EXISTS;
    case ETXTBSY:
    case EBUSY:         return Bse::Error::FILE_BUSY;
    case EAGAIN:
    case EINTR:		return Bse::Error::TEMP;
    case EFAULT:        return Bse::Error::INTERNAL;
    case EBADF:
    case ENOTDIR:
    case ENODEV:
    case EINVAL:
    default:            return fallback;
    }
}

static guint
score_error (Bse::Error error)
{
  /* errors are sorted by increasing descriptiveness */
  static const Bse::Error error_score[] = {
    Bse::Error::NONE /* least descriptive, indicates 0-initialized error variable */,
    Bse::Error::UNKNOWN, Bse::Error::INTERNAL, Bse::Error::TEMP,
    Bse::Error::IO, Bse::Error::FILE_EOF,
    Bse::Error::FILE_OPEN_FAILED, Bse::Error::FILE_SEEK_FAILED,
    Bse::Error::FILE_READ_FAILED, Bse::Error::FILE_WRITE_FAILED,
    Bse::Error::FILE_NOT_FOUND, Bse::Error::WAVE_NOT_FOUND,
  };
  guint i;
  for (i = 0; i < G_N_ELEMENTS (error_score); i++)
    if (error_score[i] == error)
      return i;
  return i;
}

Bse::Error
gsl_error_select (guint           n_errors,
                  Bse::Error    first_error,
                  ...)
{
  Bse::Error *errors = g_new (Bse::Error, MAX (1, n_errors));
  va_list args;
  guint i, score;
  /* function used to select a descriptive error in
   * multi-error scenarios
   */
  va_start (args, first_error);
  for (i = 0; i < n_errors; i++)
    {
      if (i)
        first_error = (Bse::Error) va_arg (args, int); // Bse::Error
      errors[i] = first_error;
    }
  va_end (args);
  /* grab first error, unless followed by an error with higher score */
  Bse::Error e = errors[0];
  score = score_error (e);
  for (i = 1; i < n_errors; i++)
    {
      guint s = score_error (errors[i]);
      if (s > score)
        {
          score = s;
          e = errors[i];
        }
    }
  g_free (errors);
  return e;
}


/* --- progress notification --- */
GslProgressState
gsl_progress_state (gpointer        data,
                    GslProgressFunc pfunc,
                    guint           precision)
{
  GslProgressState pstate = { 0, -99, };
  pstate.pfunc = pfunc;
  pstate.pdata = data;
  pstate.precision = precision = CLAMP (precision, 0, 9);
  pstate.epsilon = 1;
  while (precision--)
    pstate.epsilon *= 0.1;
  pstate.epsilon *= 0.5;
  return pstate;
}

void
gsl_progress_notify (GslProgressState *pstate,
                     gfloat            pval,
                     const gchar      *detail_format,
                     ...)
{
  gboolean need_update;

  assert_return (pstate != NULL);

  if (pval >= 0)
    {
      pval = CLAMP (pval, 0, 100);
      need_update = ABS (pval - pstate->pval) > pstate->epsilon;
    }
  else
    {
      pval = -1;
      need_update = TRUE;
    }

  if (need_update && pstate->pfunc)
    {
      gchar *detail = NULL;
      guint l;
      if (detail_format)
        {
          va_list args;
          va_start (args, detail_format);
          detail = g_strdup_vprintf (detail_format, args);
          va_end (args);
        }
      pstate->pval = pval;
      l = pstate->pfunc (pstate->pdata, pstate->pval, detail && detail[0] ? detail : NULL, pstate);
      pstate->wipe_length = MAX (pstate->wipe_length, l);
      g_free (detail);
    }
}

void
gsl_progress_wipe (GslProgressState *pstate)
{
  assert_return (pstate != NULL);

  if (pstate->wipe_length)
    {
      char *wstr = (char*) g_malloc (pstate->wipe_length + 1 + 1);
      memset (wstr, ' ', pstate->wipe_length);
      wstr[pstate->wipe_length] = '\r';
      wstr[pstate->wipe_length + 1] = 0;
      Bse::printerr ("%s", wstr);
      g_free (wstr);
      pstate->wipe_length = 0;
    }
}

guint
gsl_progress_printerr (gpointer          message,
                       gfloat            pval,
                       const gchar      *detail,
                       GslProgressState *pstate)
{
  gchar *str, format[128] = "%s%sprocessed %5.1f%% %s%s%s";
  gchar *ppos = strchr (format, '1');
  guint l, prec = pstate->precision;
  ppos[0] = '0' + CLAMP (prec, 0, 9);
  str = g_strdup_format (format,
                         message ? (gchar*) message : "",
                         message ? ": " : "",
                         pval,
                         detail ? "(" : "",
                         detail ? detail : "",
                         detail ? ")" : "");
  l = strlen (str);
  Bse::printerr ("%s            \r", str);
  g_free (str);
  return l;
}
/* --- global initialization --- */
void
gsl_init (void)
{
  assert_return (Bse::TickStamp::current() == 0);     // assert single initialization
  struct Internal : Bse::TickStamp { using TickStamp::_init_forgsl; };
  Internal::_init_forgsl();
  /* initialize subsystems */
  _gsl_init_fd_pool ();
  _gsl_init_data_caches ();
  _gsl_init_loader_gslwave ();
  _gsl_init_loader_aiff ();
  _gsl_init_loader_wav ();
  _gsl_init_loader_oggvorbis ();
  _gsl_init_loader_mad ();
  bse_init_loader_gus_patch ();
  bse_init_loader_flac ();
}
