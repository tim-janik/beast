/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bswproxy.h"

#include <bse/bsemain.h>
#include <bse/bseglue.h>	// FIXME
#include <bse/bseserver.h>
#include <gobject/gvaluecollector.h>


/* --- functions --- */
SfiProxy
bsw_proxy_get_server (void)
{
  return 1;	// FIXME: use glue layer for server ID ?
}
