// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsestandardsynths.hh"

#include "bsesnet.hh"
#include "bsestandardosc.hh"
#include <string.h>
#include "bse/res/bse-resources.cc" // compile standard synth resources

/* --- functions --- */
gchar*
bse_standard_synth_inflate (const gchar *synth_name,
			    guint       *text_len)
{
  assert_return (synth_name != NULL, NULL);

  const Bse::String synth_res = Bse::String ("@res ") + synth_name + ".bse";
  Bse::Blob blob = Bse::Res (synth_res);
  if (blob.size())
    {
      gchar *result = (gchar*) g_malloc (blob.size() + 1);
      memcpy (result, blob.data(), blob.size());
      result[blob.size()] = 0;
      return result;
    }
  Bse::warning ("unknown standard synth: %s", synth_name);
  return NULL;
}
