/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
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
#include "sfitime.h"
#include "sfiprimitives.h"
#include "sfilog.h"
#include <sys/time.h>
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
static gchar	*time_zones[2] = { NULL, NULL };


/* --- functions --- */
void
_sfi_init_time (void)
{
  static gboolean initialized = FALSE;
  SfiTime ustime;
  time_t t;

  g_assert (initialized++ == FALSE);

  ustime = sfi_time_system ();
  t = ustime / SFI_USEC_FACTOR;
  localtime (&t);
  gmt_diff = timezone;
  gmt_diff *= SFI_USEC_FACTOR;
  time_zones[0] = g_strdup (tzname[0]);
  time_zones[1] = g_strdup (tzname[1]);
}

/**
 * sfi_time_system
 * @RETURNS: Current system time in micro seconds
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
  gint error = gettimeofday (&tv, NULL);

  if (error)
    sfi_info ("gettimeofday() failed: %s", g_strerror (errno));
  ustime = tv.tv_sec;
  ustime = ustime * SFI_USEC_FACTOR + tv.tv_usec;

  return ustime;
}

/**
 * sfi_time_to_utc
 * @ustime:  local standard time in micro seconds
 * @RETURNS: UTC relative time in micro seconds
 *
 * Convert the local standard time @ustime into
 * Coordinated Universal Time (UTC).
 * This function is MT-safe and may be called from any thread.
 */
SfiTime
sfi_time_to_utc (SfiTime ustime)
{
  return ustime + gmt_diff;
}

/**
 * sfi_time_from_utc
 * @ustime:  UTC relative time in micro seconds
 * @RETURNS: local standard time in micro seconds
 *
 * Convert the Coordinated Universal Time (UTC)
 * @ustime into local standard time.
 * This function is MT-safe and may be called from any thread.
 */
SfiTime
sfi_time_from_utc (SfiTime ustime)
{
  return ustime - gmt_diff;
}

/**
 * sfi_time_to_string
 * @ustime:  time in micro seconds
 * @RETURNS: newly allocated string
 *
 * Retrieve the time @ustime in human readable form.
 * The returned time string contains no time zone
 * or UTC offset information.
 */
gchar*
sfi_time_to_string (SfiTime ustime)
{
  time_t t = CLAMP (ustime, SFI_MIN_TIME, SFI_MAX_TIME) / SFI_USEC_FACTOR;
  struct tm bt;
  
  bt = *localtime (&t);	/* not thread safe */
  
  return g_strdup_printf ("%04d-%02d-%02d %02d:%02d:%02d",
			  bt.tm_year + 1900,
			  bt.tm_mon + 1,
			  bt.tm_mday,
			  bt.tm_hour,
			  bt.tm_min,
			  bt.tm_sec);
}

/**
 * sfi_time_from_string
 * @time_string: string containing human readable date and time
 * @RETURNS:     parsed time in micro seconds or 0 on error
 *
 * Simple variant of sfi_time_from_string_err().
 */
SfiTime
sfi_time_from_string (const gchar *time_string)
{
  return sfi_time_from_string_err (time_string, NULL);
}

/**
 * sfi_time_from_string_err
 * @time_string: string containing human readable date and time
 * @error_p:     location for newly allocated string containing conversion errors
 * @RETURNS:     parsed time in micro seconds or 0 on error
 *
 * Parse time from a string of characters and indicate possible errors.
 * Several attempts are made to reconstruct a valid time despite possible
 * errors. However, if all attempts fail, the returned time is 0.
 */
SfiTime
sfi_time_from_string_err (const gchar *time_string,
			  gchar      **error_p)
{
  const guint n_formats = 12;
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
   * junk that doesn't do anything usefull for the purpose of generic file
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
   * "mm/dd/yyyy hh:mm:ss"
   * "mm/dd/yyyy hh:mm"
   * "dd.mm.yyyy hh:mm:ss"
   * "dd.mm.yyyy hh:mm"
   * "hh:mm:ss yyyy-mm-dd"
   * "hh:mm yyyy-mm-dd"
   * "hh:mm:ss mm/dd/yyyy"
   * "hh:mm mm/dd/yyyy"
   * "hh:mm:ss dd.mm.yyyy"
   * "hh:mm dd.mm.yyyy"
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
      time_t ttime = 0;

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
      tm_data.tm_isdst = -1;
      
      ttime = mktime (&tm_data);	/* returns -1 on error */

      /* g_print ("DEBUG: year(%u) month(%u) day(%u) hour(%u) minute(%u) second(%u)\n",
       * year[i], month[i], day[i], hour[i], minute[i], second[i]);
       *
       * g_print ("timeparser: (%s) secs=%lu, <%s>\n",
       * string, ttime == -1 ? 0 : ttime, ctime (&ttime));
       */

      if (ttime < SFI_MIN_TIME / SFI_USEC_FACTOR) /* limit ttime to 1.1.1990 */
	{
	  warnings = sfi_ring_append (warnings, g_strdup ("invalid date specification"));
	  ustime = 0;
	}
      else
	{
	  ustime = ttime;
	  ustime *= SFI_USEC_FACTOR;
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
      g_string_aprintf (gstring, " in date: %s", time_string);
      *error_p = g_string_free (gstring, FALSE);
    }
  else if (error_p)
    *error_p = NULL;
  for (ring = warnings; ring; ring = sfi_ring_walk (ring, warnings))
    g_free (ring->data);
  sfi_ring_free (warnings);
  return ustime;
}
