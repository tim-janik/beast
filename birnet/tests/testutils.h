/* Birnet
 * Copyright (C) 2006 Tim Janik
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
#include <glib.h>
#include <stdio.h>

#define _(x) 		(x)
#define TMSG(what)      do { g_print ("%s [", what); fflush (stdout); } while (0)
#define TOK()           do { g_print ("-"); fflush (stdout); } while (0)	/* OK */
#define TICK()          TOK()
#define TACK()          do { g_print ("+"); fflush (stdout); } while (0)	/* glitch */
#define TFAIL()         do { g_print ("X"); fflush (stdout); } while (0)	/* slight FAIL */
#define TDONE()         do { g_print ("]\n"); fflush (stdout); } while (0)
#define TASSERT(code)	do { if (code) TOK (); else g_error ("(line:%u) failed to assert: %s", __LINE__, #code); } while (0)
