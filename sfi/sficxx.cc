/* SFI - Synthesis Fusion Kit Interface
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
#include "sficxx.hh"
#include "sfi.h"

namespace Sfi {

static void
sfi_init_cxx (void)
{
  g_type_init ();       /* just in case this hasn't been called already */

  _sfi_init_values ();
  _sfi_init_params ();
  _sfi_init_time ();
  _sfi_init_glue ();
  _sfi_init_file_crawler ();
}

static Birnet::InitHook sfi_init_hook (sfi_init_cxx);

} // Sfi

/* vim:set ts=8 sts=2 sw=2: */
