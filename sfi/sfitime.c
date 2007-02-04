/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "topconfig.h"
#include "sfitime.h"
#include "sfiring.h"
#include "sfiprimitives.h"
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>


#define	SFI_ERROR_DOMAIN	g_quark_from_static_string ("sfi-error-domain")
enum {
  ERROR_DATE_INVALID	= 1,
  ERROR_DATE_CLUTTERED,
  ERROR_DATE_YEAR_BOUNDS,
  ERROR_DATE_MONTH_BOUNDS,
  ERROR_DATE_DAY_BOUNDS,
  ERROR_DATE_HOUR_BOUNDS,
  ERROR_DATE_MINUTE_BOUNDS,
  ERROR_DATE_SECOND_BOUNDS,
};


/* --- variables --- */
static SfiTime	 gmt_diff = 0;


/* --- functions --- */
void
_sfi_init_time (void)
{
  static gboolean initialized = FALSE;
  struct timeval tv = { 0, };
  time_t t;
  gint error;

  g_assert (initialized++ == FALSE);

  tzset ();
  error = gettimeofday (&tv, NULL);
  if (error)
    g_error ("gettimeofday() failed: %s", g_strerror (errno));
  t = tv.tv_sec + tv.tv_usec / 1000000;

  /* we need to find out the timezone offset relative to GMT here */
#if 0
  { /* aparently FreeBSD/BSD4.3 doesn't have an extern long timezone; set by
     * localtime(). if present, timezone contains # of seconds west of GMT.
     */
    localtime (&t);
    gmt_diff = timezone;
  }
#else
  { /* however, struct tm { ... long tm_gmtoff; }; is hopefully available on
     * all recent glibc versions and BSDs. tm_gmtoff contains # of seconds east of UTC.
     */
    struct tm tmdata;
    localtime_r (&t, &tmdata);
    gmt_diff = -tmdata.tm_gmtoff;
  }
#endif

  gmt_diff *= SFI_USEC_FACTOR;
}

/**
 * @return		Current system time in micro seconds
 *
 * Get the current system time in micro seconds.
 * Subsequent calls to this function do not necessarily
 * return greater values. In fact, a second call may return
 * a value smaller than the first call under certain system
 * conditions. The time returned is UTC, refer to
 * sfi_time_from_utc() in order to retrieve the local
 * standard time.
 * This function is MT-safe and may be called from any thread.
 */
SfiTime
sfi_time_system (void)
{
  struct timeval tv;
  SfiTime ustime;

  gettimeofday (&tv, NULL);
  ustime = tv.tv_sec;
  ustime = ustime * SFI_USEC_FACTOR + tv.tv_usec;

  return ustime;
}

/**
 * @param ustime	local standard time in micro seconds
 * @return		UTC relative time in micro seconds
 *
 * Convert the local standard time @a ustime into
 * Coordinated Universal Time (UTC).
 * This function is MT-safe and may be called from any thread.
 */
SfiTime
sfi_time_to_utc (SfiTime ustime)
{
  return ustime + gmt_diff;
}

/**
 * @param ustime	UTC relative time in micro seconds
 * @return		local standard time in micro seconds
 *
 * Convert the Coordinated Universal Time (UTC)
 * @a ustime into local standard time.
 * This function is MT-safe and may be called from any thread.
 */
SfiTime
sfi_time_from_utc (SfiTime ustime)
{
  return ustime - gmt_diff;
}

/**
 * @param ustime	time in micro seconds
 * @return		newly allocated string
 *
 * Retrieve the time @a ustime in human readable form.
 * The returned time string describes UTC time and
 * thus contains no time zone or UTC offset information.
 */
gchar*
sfi_time_to_string (SfiTime ustime)
{
  time_t t = CLAMP (ustime, SFI_MIN_TIME, SFI_MAX_TIME) / SFI_USEC_FACTOR;
  struct tm bt;
  
  bt = *gmtime (&t);	/* FIXME: not thread safe */
  
  return g_strdup_printf ("%04d-%02d-%02d %02d:%02d:%02d",
			  bt.tm_year + 1900,
			  bt.tm_mon + 1,
			  bt.tm_mday,
			  bt.tm_hour,
			  bt.tm_min,
			  bt.tm_sec);
}

/**
   @param ustime	time in micro seconds
   @param elements      string identifying time elements
   @return		newly allocated string
   
   Retrieve the time @a ustime in human readable form.
   Within the rnage of date and time formats parsable by
   sfi_time_from_string(), the nicest display is selected
   according to the current locale and other user settings.
   By means of the @a elements argument, various elemtns of
   a full date string can be selected:
   @itemize
   @item H - display hours
   @item M - display minutes
   @item S - display seconds
   @item d - display day
   @item m - display month
   @item y - display year
   @done
   The returned time string describes UTC time and
   thus contains no time zone or UTC offset information.
*/
gchar*
sfi_time_to_nice_string (SfiTime      ustime,
                         const gchar *elements)
{
  time_t t = CLAMP (ustime, SFI_MIN_TIME, SFI_MAX_TIME) / SFI_USEC_FACTOR;
  struct tm bt;
  if (!elements)
    elements = "";

  bt = *gmtime (&t);	/* FIXME: not thread safe */

  const bool wtime = strchr (elements, 'H') || strchr (elements, 'M') || strchr (elements, 'S');
  const bool wdate = strchr (elements, 'd') || strchr (elements, 'm') || strchr (elements, 'y');

  if (wdate && !wtime)
    return g_strdup_printf ("%04d-%02d-%02d",
                            bt.tm_year + 1900,
                            bt.tm_mon + 1,
                            bt.tm_mday);
  if (!wdate && wtime)
    return g_strdup_printf ("%02d:%02d:%02d",
                            bt.tm_hour,
                            bt.tm_min,
                            bt.tm_sec);
  else
    return g_strdup_printf ("%02d:%02d:%02d %04d-%02d-%02d",
                            bt.tm_hour,
                            bt.tm_min,
                            bt.tm_sec,
                            bt.tm_year + 1900,
                            bt.tm_mon + 1,
                            bt.tm_mday);
}

/**
 * @param time_string	string containing human readable date and time
 * @return		parsed time in micro seconds or 0 on error
 *
 * Simple variant of sfi_time_from_string_err().
 */
SfiTime
sfi_time_from_string (const gchar *time_string)
{
  return sfi_time_from_string_err (time_string, NULL);
}

/**
 * @param time_string	string containing human readable date and time
 * @param error_p	location for newly allocated string containing conversion errors
 * @return		parsed time in micro seconds, may be 0 on error
 *
 * Parse date and time from a string of characters and indicate possible errors.
 * Several attempts are made to reconstruct a valid date and time despite possible
 * errors. However, if all attempts fail, the returned time is 0. The time returned
 * is UTC, refer to sfi_time_from_utc() in order to retrieve the local standard time.
 */
SfiTime
sfi_time_from_string_err (const gchar *time_string,
			  gchar      **error_p)
{
  const guint n_formats = 15;
  guint year[n_formats];
  guint month[n_formats];
  guint day[n_formats];
  guint hour[n_formats];
  guint minute[n_formats];
  guint second[n_formats];
  gboolean success[n_formats];
  gboolean garbage[n_formats];
  gboolean finished;
  gchar *string;
  SfiTime ustime;
  SfiRing *ring, *warnings = NULL;
  guint i;
  
  g_return_val_if_fail (time_string != NULL, 0);
  
  /* here, we support several date formats by making several attempts
   * to match a string and pick the best one. if we acquire a full match
   * before all match possibilities have been tryed, we skip outstanding
   * match attempts. we do not use strptime(3), since it carries the locale(7)
   * junk that doesn't do anything useful for the purpose of generic file
   * parsing and it doesn't give the smallest clue whether the source
   * string was (not) valid in any meaningfull sense.
   * rules:
   * - years need to be specified by 4 digits
   * - date _and_ time need to be specified
   * - seconds are optional
   *
   * the following formats are currently implemented:
   * "yyyy-mm-dd hh:mm:ss"
   * "yyyy-mm-dd hh:mm"
   * "yyyy-mm-dd"
   * "mm/dd/yyyy hh:mm:ss"
   * "mm/dd/yyyy hh:mm"
   * "mm/dd/yyyy"
   * "dd.mm.yyyy hh:mm:ss"
   * "dd.mm.yyyy hh:mm"
   * "dd.mm.yyyy"
   * "hh:mm:ss yyyy-mm-dd"
   * "hh:mm yyyy-mm-dd"
   * "hh:mm:ss mm/dd/yyyy"
   * "hh:mm mm/dd/yyyy"
   * "hh:mm:ss dd.mm.yyyy"
   * "hh:mm dd.mm.yyyy"
   *
   * more on time formats (ISO 8601) can be found at:
   *   http://www.cl.cam.ac.uk/~mgk25/iso-time.html
   */
  
  string = g_strdup (time_string);
  
  for (i = 0; i < n_formats; i++)
    {
      year[i] = month[i] = day[i] = 0;
      hour[i] = minute[i] = second[i] = 0;
      success[i] = garbage[i] = FALSE;
    }
  
  finished = FALSE;
  i = 0;
  
#define DATE_CHECK(index)	(year[(index)] >= 1990 &&	\
				 month[(index)] >= 1 &&		\
				 month[(index)] <= 12 &&	\
				 day[(index)] >= 1 &&		\
				 day[(index)] <= 31 &&		\
				 hour[(index)] >= 0 &&		\
				 hour[(index)] <= 23 &&		\
				 minute[(index)] >= 0 &&	\
				 minute[(index)] <= 59 &&	\
				 second[(index)] >= 0 &&	\
				 second[(index)] <= 61)
  /* g_print ("DEBUG: \"%s\" -> y%u m%u d%u h%u m%u s%u\n", string,
   *          year[i], month[i], day[i], hour[i], minute[i], second[i]);
   */
  if (!finished) /* parse "yyyy-mm-dd hh:mm:ss" e.g. "1998-04-16 23:59:59" */
    {
      gint n_values;
      gchar end_char = 0;
      
      n_values = sscanf (string,
			 "%u-%u-%u %u:%u:%u%c",
			 &year[i], &month[i], &day[i],
			 &hour[i], &minute[i], &second[i],
			 &end_char);
      success[i] = n_values >= 6;
      garbage[i] = n_values > 6;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "yyyy-mm-dd hh:mm" e.g. "1998-04-16 23:59" */
    {
      gint n_values;
      gchar end_char = 0;
      
      second[i] = 0;
      n_values = sscanf (string,
			 "%u-%u-%u %u:%u%c",
			 &year[i], &month[i], &day[i],
			 &hour[i], &minute[i],
			 &end_char);
      success[i] = n_values >= 5;
      garbage[i] = n_values > 5;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "yyyy-mm-dd" e.g. "1998-04-16" */
    {
      gint n_values;
      gchar end_char = 0;
      
      second[i] = 0;
      n_values = sscanf (string,
                         "%u-%u-%u%c",
                         &year[i], &month[i], &day[i],
                         &end_char);
      success[i] = n_values >= 3;
      garbage[i] = n_values > 3;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "mm/dd/yyyy hh:mm:ss" e.g. "04/16/1998 23:59:59" */
    
    {
      gint n_values;
      gchar end_char = 0;
      
      n_values = sscanf (string,
			 "%u/%u/%u %u:%u:%u%c",
			 &month[i], &day[i], &year[i],
			 &hour[i], &minute[i], &second[i],
			 &end_char);
      success[i] = n_values >= 6;
      garbage[i] = n_values > 6;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "mm/dd/yyyy hh:mm" e.g. "04/16/1998 23:59" */
    {
      gint n_values;
      gchar end_char = 0;
      
      second[i] = 0;
      n_values = sscanf (string,
			 "%u/%u/%u %u:%u%c",
			 &month[i], &day[i], &year[i],
			 &hour[i], &minute[i],
			 &end_char);
      success[i] = n_values >= 5;
      garbage[i] = n_values > 5;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "mm/dd/yyyy" e.g. "04/16/1998" */
    {
      gint n_values;
      gchar end_char = 0;
      
      second[i] = 0;
      n_values = sscanf (string,
                         "%u/%u/%u%c",
                         &month[i], &day[i], &year[i],
                         &end_char);
      success[i] = n_values >= 3;
      garbage[i] = n_values > 3;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "dd.mm.yyyy hh:mm:ss" e.g. "16.4.1998 23:59:59" */
    {
      gint n_values;
      gchar end_char = 0;
      
      n_values = sscanf (string,
			 "%u.%u.%u %u:%u:%u%c",
			 &day[i], &month[i], &year[i],
			 &hour[i], &minute[i], &second[i],
			 &end_char);
      success[i] = n_values >= 6;
      garbage[i] = n_values > 6;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "dd.mm.yyyy hh:mm" e.g. "16.4.1998 23:59" */
    {
      gint n_values;
      gchar end_char = 0;
      
      second[i] = 0;
      n_values = sscanf (string,
			 "%u.%u.%u %u:%u%c",
			 &day[i], &month[i], &year[i],
			 &hour[i], &minute[i],
			 &end_char);
      success[i] = n_values >= 5;
      garbage[i] = n_values > 5;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "dd.mm.yyyy" e.g. "16.4.1998" */
    {
      gint n_values;
      gchar end_char = 0;
      
      second[i] = 0;
      n_values = sscanf (string,
                         "%u.%u.%u%c",
                         &day[i], &month[i], &year[i],
                         &end_char);
      success[i] = n_values >= 3;
      garbage[i] = n_values > 3;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "hh:mm:ss yyyy-mm-dd" e.g. "23:59:59 1998-04-16" */
    {
      gint n_values;
      gchar end_char = 0;
      
      n_values = sscanf (string,
			 "%u:%u:%u %u-%u-%u%c",
			 &hour[i], &minute[i], &second[i],
			 &year[i], &month[i], &day[i],
			 &end_char);
      success[i] = n_values >= 6;
      garbage[i] = n_values > 6;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "hh:mm yyyy-mm-dd" e.g. "23:59 1998-04-16" */
    {
      gint n_values;
      gchar end_char = 0;
      
      second[i] = 0;
      n_values = sscanf (string,
			 "%u:%u %u-%u-%u%c",
			 &hour[i], &minute[i],
			 &year[i], &month[i], &day[i],
			 &end_char);
      success[i] = n_values >= 5;
      garbage[i] = n_values > 5;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "hh:mm:ss mm/dd/yyyy" e.g. "23:59:59 04/16/1998" */
    {
      gint n_values;
      gchar end_char = 0;
      
      n_values = sscanf (string,
			 "%u:%u:%u %u/%u/%u%c",
			 &hour[i], &minute[i], &second[i],
			 &month[i], &day[i], &year[i],
			 &end_char);
      success[i] = n_values >= 6;
      garbage[i] = n_values > 6;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "hh:mm mm/dd/yyyy" e.g. "23:59 04/16/1998" */
    {
      gint n_values;
      gchar end_char = 0;
      
      second[i] = 0;
      n_values = sscanf (string,
			 "%u:%u %u/%u/%u%c",
			 &hour[i], &minute[i],
			 &month[i], &day[i], &year[i],
			 &end_char);
      success[i] = n_values >= 5;
      garbage[i] = n_values > 5;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "hh:mm:ss dd.mm.yyyy" e.g. "23:59:59 16.4.1998" */
    {
      gint n_values;
      gchar end_char = 0;
      
      n_values = sscanf (string,
			 "%u:%u:%u %u.%u.%u%c",
			 &hour[i], &minute[i], &second[i],
			 &day[i], &month[i], &year[i],
			 &end_char);
      success[i] = n_values >= 6;
      garbage[i] = n_values > 6;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
  if (!finished) /* parse "hh:mm dd.mm.yyyy" e.g. "23:59:59 16.4.1998" */
    {
      gint n_values;
      gchar end_char = 0;
      
      second[i] = 0;
      n_values = sscanf (string,
			 "%u:%u %u.%u.%u%c",
			 &hour[i], &minute[i],
			 &day[i], &month[i], &year[i],
			 &end_char);
      success[i] = n_values >= 5;
      garbage[i] = n_values > 5;
      finished = success[i] && !garbage[i] && DATE_CHECK (i);
      i++;
    }
#undef	DATE_CHECK
  
  /* try to find out the best/first match if any */
  if (finished)
    i--;
  else
    {
      for (i = 0; i < n_formats - 1; i++)
	if (success[i])
	  break;
    }
  
  if (!success[i])
    {
      warnings = sfi_ring_append (warnings, g_strdup ("invalid date specification"));
      ustime = 0;
    }
  else
    {
      struct tm tm_data = { 0 };
      time_t ttime;

      if (garbage[i])
	warnings = sfi_ring_append (warnings, g_strdup ("junk characters at end of date"));
      if (year[i] < 1990)
	{
	  warnings = sfi_ring_append (warnings, g_strdup_printf ("%s out of bounds", "year"));
	  year[i] = 1990;
	}
      if (month[i] < 1 || month[i] > 12)
	{
	  warnings = sfi_ring_append (warnings, g_strdup_printf ("%s out of bounds", "month"));
	  month[i] = CLAMP (month[i], 1, 12);
	}
      if (day[i] < 1 || day[i] > 31)
	{
	  warnings = sfi_ring_append (warnings, g_strdup_printf ("%s out of bounds", "day"));
	  month[i] = CLAMP (day[i], 1, 31);
	}
      if (hour[i] < 0 || hour[i] > 23)
	{
	  warnings = sfi_ring_append (warnings, g_strdup_printf ("%s out of bounds", "hour"));
	  hour[i] = CLAMP (hour[i], 0, 23);
	}
      if (minute[i] < 0 || minute[i] > 59)
	{
	  warnings = sfi_ring_append (warnings, g_strdup_printf ("%s out of bounds", "minute"));
	  minute[i] = CLAMP (minute[i], 0, 59);
	}
      if (second[i] < 0 || second[i] > 61)
	{
	  warnings = sfi_ring_append (warnings, g_strdup_printf ("%s out of bounds", "second"));
	  second[i] = CLAMP (second[i], 0, 61);
	}
      
      tm_data.tm_sec = second[i];
      tm_data.tm_min = minute[i];
      tm_data.tm_hour = hour[i];
      tm_data.tm_mday = day[i];
      tm_data.tm_mon = month[i] - 1;
      tm_data.tm_year = year[i] - 1900;
      tm_data.tm_wday = 0;
      tm_data.tm_yday = 0;
      tm_data.tm_isdst = 1;
      
#if HAVE_TIMEGM
      ttime = timegm (&tm_data);			/* returns -1 on error */
#else
      {
        char *tz = g_strdup (g_getenv ("TZ"));
        g_setenv ("TZ", "", 1);
        tzset();
        ttime = mktime (&tm_data);
        if (tz)
          g_setenv ("TZ", tz, 1);
        else
          g_unsetenv ("TZ");
        tzset();
        g_free (tz);
      }
#endif
      
      ustime = ttime;
      ustime *= SFI_USEC_FACTOR;
      ustime = MAX (ustime, 0);
      
      /* g_print ("mktime(): year(%u) month(%u) day(%u) hour(%u) minute(%u) second(%u)\n",
       *           year[i], month[i], day[i], hour[i], minute[i], second[i]);
       */
      
      if (ustime < SFI_MIN_TIME)	/* limit ustime to 1.1.1990 */
	{
	  warnings = sfi_ring_append (warnings, g_strdup_printf ("invalid date specification (%lld < %lld, gmt-diff=%lld)",
								 ustime, SFI_MIN_TIME, gmt_diff));
	  ustime = SFI_MIN_TIME;
	}
    }
  
  /* general cleanup and error return */
  g_free (string);
  if (error_p && warnings)
    {
      GString *gstring = g_string_new (NULL);
      for (ring = warnings; ring; ring = sfi_ring_walk (ring, warnings))
	{
	  if (gstring->len)
	    g_string_append (gstring, ", ");
	  g_string_append (gstring, ring->data);
	}
      g_string_aprintf (gstring, " in date: \"%s\"", time_string);
      *error_p = g_string_free (gstring, FALSE);
    }
  else if (error_p)
    *error_p = NULL;
  for (ring = warnings; ring; ring = sfi_ring_walk (ring, warnings))
    g_free (ring->data);
  sfi_ring_free (warnings);
  return ustime;
}
