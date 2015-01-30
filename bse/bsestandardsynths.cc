// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsestandardsynths.hh"

#include "bsesnet.hh"
#include "bsestandardosc.hh"
#include <string.h>

/* --- typedef & structures --- */
typedef struct {
  const gchar  *name;
  guint         text_size;
  const guint8 *cdata;
  guint         clength;
} BseZFile;

/* --- generated ZFiles --- */
#include "bse/zintern/bse-resources.cc"

/* --- variables --- */
static GSList	*zfile_names = NULL;

/* --- functions --- */
gchar*
bse_standard_synth_inflate (const gchar *synth_name,
			    guint       *text_len)
{
  g_return_val_if_fail (synth_name != NULL, NULL);

  const Rapicorn::String synth_res = Rapicorn::String ("@res ") + synth_name + ".bse";
  Rapicorn::Blob blob = Rapicorn::Res (synth_res);
  if (blob.size())
    {
      gchar *result = (gchar*) g_malloc (blob.size() + 1);
      memcpy (result, blob.data(), blob.size());
      result[blob.size()] = 0;
      return result;
    }
  g_warning ("unknown standard synth: %s", synth_name);
  return NULL;
}

GSList*
bse_standard_synth_get_list (void)
{
  guint i;
  if (!zfile_names)
    for (i = 0; i < G_N_ELEMENTS (bse_zfiles); i++)
      zfile_names = g_slist_prepend (zfile_names, (gchar*) bse_zfiles[i].name);
  return zfile_names;
}
