// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecompat.hh"
#include "bsestorage.hh"
#include <string.h>


/* --- functions --- */
gchar*
bse_compat_rewrite_type_name (BseStorage    *storage,
                              const gchar   *type_name)
{
  const guint vmajor = storage->major_version;
  const guint vminor = storage->minor_version;
  const guint vmicro = storage->micro_version;
  struct { guint vmajor, vminor, vmicro; const gchar *old_type, *new_type; } type_changes[] = {
    { 0, 5, 1,  "BseSNet",              "BseCSynth"             },
    { 0, 5, 4,  "BseMonoKeyboard",      "BseMidiInput"          },
    { 0, 5, 4,  "BseMidiIController",   "BseMidiController"     },
    { 0, 5, 4,  "BseSubKeyboard",       "BseInstrumentInput"    },
    { 0, 5, 4,  "BseSubInstrument",     "BseInstrumentOutput"   },
    { 0, 6, 2,  "ArtsCompressor",       "BseArtsCompressor"     },
    { 0, 6, 2,  "ArtsStereoCompressor", "BseArtsCompressor"     },
    { 0, 6, 2,  "DavBassFilter",        "BseDavBassFilter"      },
    { 0, 6, 2,  "DavChorus",            "BseDavChorus"          },
    { 0, 7, 5,  "DavOrgan",             "BseDavOrgan"           },
  };
  guint i;
  for (i = 0; i < G_N_ELEMENTS (type_changes); i++)
    if (BSE_VERSION_CMP (vmajor, vminor, vmicro,
                         type_changes[i].vmajor,
                         type_changes[i].vminor,
                         type_changes[i].vmicro) <= 0 &&
        strcmp (type_name, type_changes[i].old_type) == 0)
      return g_strdup (type_changes[i].new_type);
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
  struct { guint vmajor, vminor, vmicro; const gchar *type, *old_channel, *new_channel; } ichannel_changes[] = {
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
        strcmp (ichannel_ident, ichannel_changes[i].old_channel) == 0)
      return g_strdup (ichannel_changes[i].new_channel);
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
  struct { guint vmajor, vminor, vmicro; const gchar *type, *old_channel, *new_channel; } ochannel_changes[] = {
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
        strcmp (ochannel_ident, ochannel_changes[i].old_channel) == 0)
      return g_strdup (ochannel_changes[i].new_channel);
  return NULL;
}
