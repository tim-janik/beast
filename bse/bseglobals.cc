/* BSE - Better Sound Engine
 * Copyright (C) 1997-2004 Tim Janik
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
#include	"bseglobals.h"
#include	"bsemain.h"


/* --- functions --- */
void
bse_globals_init (void) { /* FIXME: remove */ }

double
bse_db_to_factor (double dB)
{
  double factor = dB / 20; /* Bell */
  return pow (10, factor);
}

double
bse_db_from_factor (double factor,
		    double min_dB)
{
  if (factor > 0)
    {
      double dB = log10 (factor); /* Bell */
      dB *= 20;
      return dB;
    }
  else
    return min_dB;
}

long
bse_time_range_to_ms (BseTimeRangeType time_range)
{
  g_return_val_if_fail (time_range >= BSE_TIME_RANGE_SHORT, 0);
  g_return_val_if_fail (time_range <= BSE_TIME_RANGE_LONG, 0);
  
  switch (time_range)
    {
    case BSE_TIME_RANGE_SHORT:		return BSE_TIME_RANGE_SHORT_ms;
    case BSE_TIME_RANGE_MEDIUM:		return BSE_TIME_RANGE_MEDIUM_ms;
    case BSE_TIME_RANGE_LONG:		return BSE_TIME_RANGE_LONG_ms;
    }
  return 0;	/* can't be triggered */
}


/* --- idle handlers --- */
/* important ordering constrains:
 * BSE_PRIORITY_NOW             = -G_MAXINT / 2
 * BSE_PRIORITY_HIGH		= G_PRIORITY_HIGH - 10
 * BSE_PRIORITY_NEXT		= G_PRIORITY_HIGH - 5
 * G_PRIORITY_HIGH		(-100)
 * BSE_PRIORITY_NOTIFY		= G_PRIORITY_DEFAULT - 1
 * G_PRIORITY_DEFAULT		(0)
 * GDK_PRIORITY_EVENTS		= G_PRIORITY_DEFAULT
 * BSE_PRIORITY_PROG_IFACE	= G_PRIORITY_DEFAULT
 * G_PRIORITY_HIGH_IDLE		(100)
 * BSE_PRIORITY_UPDATE		= G_PRIORITY_HIGH_IDLE + 5
 * GTK_PRIORITY_RESIZE		= G_PRIORITY_HIGH_IDLE + 10
 * GDK_PRIORITY_REDRAW		= G_PRIORITY_HIGH_IDLE + 20
 * G_PRIORITY_DEFAULT_IDLE	(200)
 * G_PRIORITY_LOW		(300)
 * BSE_PRIORITY_BACKGROUND	= G_PRIORITY_LOW + 500
 */

/**
 * @param function	user function
 * @param data	        user data
 * @return		idle handler id, suitable for bse_idle_remove()
 * Execute @a function (@a data) inside the main BSE thread as soon as possible.
 * Usually this function should not be used but bse_idle_next() should be used instead.
 * Only callbacks that have hard dependencies on immediate asyncronous execution,
 * preceeding even realtime synthesis job handling should be executed this way.
 * This function is MT-safe and may be called from any thread.
 */
unsigned int
bse_idle_now (GSourceFunc function,
	      void       *data)
{
  GSource *source = g_idle_source_new ();
  unsigned int id;
  g_source_set_priority (source, BSE_PRIORITY_NOW);
  g_source_set_callback (source, function, data, NULL);
  id = g_source_attach (source, bse_main_context);
  g_source_unref (source);
  return id;
}

/**
 * @param function	user function
 * @param data	        user data
 * @return		idle handler id, suitable for bse_idle_remove()
 * Execute @a function (@a data) inside the main BSE thread as soon as resonably possible.
 * This function is intended to be used by code which needs to execute some portions
 * asyncronously as soon as the BSE core isn't occupied by realtime job handling.
 * This function is MT-safe and may be called from any thread.
 */
unsigned int
bse_idle_next (GSourceFunc function,
	       void       *data)
{
  GSource *source = g_idle_source_new ();
  unsigned int id;
  g_source_set_priority (source, BSE_PRIORITY_NEXT);
  g_source_set_callback (source, function, data, NULL);
  id = g_source_attach (source, bse_main_context);
  g_source_unref (source);
  return id;
}

/**
 * @param function	user function
 * @param data	user data
 * @return		idle handler id, suitable for bse_idle_remove()
 * Queue @a function (@a data) for execution inside the main BSE thread,
 * similar to bse_idle_now(), albeit with a lower priority.
 * This function is intended to be used by code which emits
 * asyncronous notifications.
 * This function is MT-safe and may be called from any thread.
 */
unsigned int
bse_idle_notify (GSourceFunc function,
		 void       *data)
{
  GSource *source = g_idle_source_new ();
  unsigned int id;
  g_source_set_priority (source, BSE_PRIORITY_NOTIFY);
  g_source_set_callback (source, function, data, NULL);
  id = g_source_attach (source, bse_main_context);
  g_source_unref (source);
  return id;
}

unsigned int
bse_idle_normal (GSourceFunc function,
		 void       *data)
{
  GSource *source = g_idle_source_new ();
  unsigned int id;
  g_source_set_priority (source, BSE_PRIORITY_NORMAL);
  g_source_set_callback (source, function, data, NULL);
  id = g_source_attach (source, bse_main_context);
  g_source_unref (source);
  return id;
}

unsigned int
bse_idle_update (GSourceFunc function,
		 void       *data)
{
  GSource *source = g_idle_source_new ();
  unsigned int id;
  g_source_set_priority (source, BSE_PRIORITY_UPDATE);
  g_source_set_callback (source, function, data, NULL);
  id = g_source_attach (source, bse_main_context);
  g_source_unref (source);
  return id;
}

unsigned int
bse_idle_background (GSourceFunc function,
		     void       *data)
{
  GSource *source = g_idle_source_new ();
  unsigned int id;
  g_source_set_priority (source, BSE_PRIORITY_BACKGROUND);
  g_source_set_callback (source, function, data, NULL);
  id = g_source_attach (source, bse_main_context);
  g_source_unref (source);
  return id;
}

/**
 * @param usec_delay	microsecond delay
 * @param function	user function
 * @param data	user data
 * @return		idle handler id, suitable for bse_idle_remove()
 * Execute @a function (@a data) with the main BSE thread, similar to
 * bse_idle_now(), after a delay period of @a usec_delay has passed.
 * This function is MT-safe and may be called from any thread.
 */
unsigned int
bse_idle_timed (guint64     usec_delay,
		GSourceFunc function,
		void       *data)
{
  GSource *source = g_timeout_source_new (CLAMP (usec_delay / 1000, 0, G_MAXUINT));
  unsigned int id;
  g_source_set_priority (source, BSE_PRIORITY_NEXT);
  g_source_set_callback (source, function, data, NULL);
  id = g_source_attach (source, bse_main_context);
  g_source_unref (source);
  return id;
}

/**
 * @param id	idle handler id
 * Remove or unqueue an idle handler queued by bse_idle_now()
 * or one of its variants.
 * This function is MT-safe and may be called from any thread.
 */
gboolean
bse_idle_remove (unsigned int id)
{
  GSource *source;

  g_return_val_if_fail (id > 0, FALSE);

  source = g_main_context_find_source_by_id (bse_main_context, id);
  if (source)
    g_source_destroy (source);
  return source != NULL;
}
