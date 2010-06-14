/* BSE - Better Sound Engine
 * Copyright (C) 2002, 2003 Tim Janik
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
#include "bsecompat.h"
#include "bsestorage.h"
#include <string.h>


/* --- functions --- */
gchar*
bse_compat_rewrite_type_name (BseStorage    *storage,
                              const gchar   *type_name)
{
  const guint vmajor = storage->major_version;
  const guint vminor = storage->minor_version;
  const guint vmicro = storage->micro_version;
  struct { guint vmajor, vminor, vmicro; gchar *old, *new; } type_changes[] = {
    { 0, 5, 1,  "BseSNet",              "BseCSynth"             },
    { 0, 5, 4,  "BseMonoKeyboard",      "BseMidiInput"          },
    { 0, 5, 4,  "BseMidiIController",   "BseMidiController"     },
    { 0, 5, 4,  "BseSubKeyboard",       "BseInstrumentInput"    },
    { 0, 5, 4,  "BseSubInstrument",     "BseInstrumentOutput"   },
    { 0, 6, 2,  "ArtsCompressor",       "BseArtsCompressor"     },
    { 0, 6, 2,  "ArtsStereoCompressor", "BseArtsCompressor"     },
    { 0, 6, 2,  "DavBassFilter",        "BseDavBassFilter"      },
    { 0, 6, 2,  "DavChorus",            "BseDavChorus"          },
    { 0, 7, 1,  "DavOrgan",             "BseDavOrgan"           },
  };
  guint i;
  for (i = 0; i < G_N_ELEMENTS (type_changes); i++)
    if (BSE_VERSION_CMP (vmajor, vminor, vmicro,
                         type_changes[i].vmajor,
                         type_changes[i].vminor,
                         type_changes[i].vmicro) <= 0 &&
        strcmp (type_name, type_changes[i].old) == 0)
      return g_strdup (type_changes[i].new);
  return NULL;
}

gchar*
bse_compat_rewrite_ichannel_ident (BseStorage    *storage,
                                   const gchar   *type_name,
                                   const gchar   *ichannel_ident)
{
  const guint vmajor = storage->major_version;
  const guint vminor = storage->minor_version;
  const guint vmicro = storage->micro_version;
  struct { guint vmajor, vminor, vmicro; gchar *type, *old, *new; } ichannel_changes[] = {
    { 0, 6, 2,  "ArtsStereoCompressor", "left-audio-in",        "audio-in1" },
    { 0, 6, 2,  "ArtsStereoCompressor", "right-audio-in",       "audio-in2" },
    { 0, 6, 2,  "ArtsCompressor",       "audio-in",             "audio-in1" },
  };
  guint i;
  for (i = 0; i < G_N_ELEMENTS (ichannel_changes); i++)
    if (BSE_VERSION_CMP (vmajor, vminor, vmicro,
                         ichannel_changes[i].vmajor,
                         ichannel_changes[i].vminor,
                         ichannel_changes[i].vmicro) <= 0 &&
        strcmp (type_name, ichannel_changes[i].type) == 0 &&
        strcmp (ichannel_ident, ichannel_changes[i].old) == 0)
      return g_strdup (ichannel_changes[i].new);
  return NULL;
}

gchar*
bse_compat_rewrite_ochannel_ident (BseStorage    *storage,
                                   const gchar   *type_name,
                                   const gchar   *ochannel_ident)
{
  const guint vmajor = storage->major_version;
  const guint vminor = storage->minor_version;
  const guint vmicro = storage->micro_version;
  struct { guint vmajor, vminor, vmicro; gchar *type, *old, *new; } ochannel_changes[] = {
    { 0, 6, 2,  "ArtsStereoCompressor", "left-audio-out",       "audio-out1" },
    { 0, 6, 2,  "ArtsStereoCompressor", "right-audio-out",      "audio-out2" },
    { 0, 6, 2,  "ArtsCompressor",       "audio-out",            "audio-out1" },
  };
  guint i;
  for (i = 0; i < G_N_ELEMENTS (ochannel_changes); i++)
    if (BSE_VERSION_CMP (vmajor, vminor, vmicro,
                         ochannel_changes[i].vmajor,
                         ochannel_changes[i].vminor,
                         ochannel_changes[i].vmicro) <= 0 &&
        strcmp (type_name, ochannel_changes[i].type) == 0 &&
        strcmp (ochannel_ident, ochannel_changes[i].old) == 0)
      return g_strdup (ochannel_changes[i].new);
  return NULL;
}
