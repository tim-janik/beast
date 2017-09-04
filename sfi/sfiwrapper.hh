// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_WRAPPER_H__
#define __SFI_WRAPPER_H__

#include <sfi/bcore.hh>
#include <sfi/path.hh>
#include <mutex>

#ifdef  BSE_CONVENIENCE
using Bse::uint8;
using Bse::uint16;
using Bse::uint32;
using Bse::uint64;
using Bse::int8;
using Bse::int16;
using Bse::int32;
using Bse::int64;
using Bse::unichar;
#endif // BSE_CONVENIENCE

/* --- initialization --- */
typedef struct
{
  const char *value_name;       /* value list ends with value_name == NULL */
  const char *value_string;
  long double value_num;        /* valid if value_string == NULL */
} SfiInitValue;
void sfi_init (int *argcp, char **argv, const Bse::StringVector &args = Bse::StringVector());

/* --- url handling --- */
void sfi_url_show                   	(const char           *url);

#endif /* __SFI_WRAPPER_H__ */
/* vim:set ts=8 sts=2 sw=2: */
