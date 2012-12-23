// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_SERIAL_H__
#define __SFI_SERIAL_H__
#include <sfi/sfivalues.hh>
G_BEGIN_DECLS
/* --- functions --- */
GTokenType	sfi_value_parse_typed		(GValue		*value,
						 GScanner	*scanner);
GTokenType	sfi_value_parse_param_rest	(GValue		*value,
						 GScanner	*scanner,
						 GParamSpec	*pspec);
void		sfi_value_store_typed		(const GValue	*value,
						 GString	*gstring);
void		sfi_value_store_param	        (const GValue	*value,
						 GString	*gstring,
						 GParamSpec	*pspec,
						 guint		 indent);
void            sfi_value_store_stderr          (const GValue   *value);
/* --- NULL (nil) token handling --- */
#define  SFI_SERIAL_NULL_TOKEN	"#f"
/* parse NULL token if possible and return TRUE, otherwise
 * return FALSE and don't advance scanner
 */
gboolean sfi_serial_check_parse_null_token	(GScanner	*scanner);
/* --- GScanner config --- */
extern const GScannerConfig *sfi_storage_scanner_config;
G_END_DECLS
#endif /* __SFI_SERIAL_H__ */
/* vim:set ts=8 sts=2 sw=2: */
