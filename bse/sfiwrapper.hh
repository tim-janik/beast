// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_WRAPPER_H__
#define __SFI_WRAPPER_H__

#include <bse/bcore.hh>
#include <bse/path.hh>
#include <mutex>

/* --- initialization --- */
typedef struct
{
  const char *value_name;       /* value list ends with value_name == NULL */
  const char *value_string;
  long double value_num;        /* valid if value_string == NULL */
} SfiInitValue;
void sfi_init (const Bse::StringVector &args = Bse::StringVector());

/* --- url handling --- */
void sfi_url_show                   	(const char           *url);

#endif /* __SFI_WRAPPER_H__ */
/* vim:set ts=8 sts=2 sw=2: */
