/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __SFI_LOG_H__
#define __SFI_LOG_H__

#include <sfi/sfivalues.h>

G_BEGIN_DECLS


/* --- debugging --- */
#define sfi_debug(       key, ...)      sfi_log (SFI_LOG_DEBUG, SfiLogger (key, 0, 0), __VA_ARGS__)
#define sfi_nodebug(     key, ...)      do { /* nothing */ } while (0)
int     sfi_debug_check (const char *key);
void    sfi_debug_allow (const char *key);
void    sfi_debug_deny  (const char *key);


/* --- logging --- */
#define SfiLogger(key, cblurb, ablurb)  sfi_log_context (key, cblurb /* preferences */, ablurb /* dialogs */)
#define sfi_error(logger, ...)          sfi_log (SFI_LOG_ERROR, logger, __VA_ARGS__)
#define sfi_warn(logger, ...)           sfi_log (SFI_LOG_WARN, logger, __VA_ARGS__)
#define sfi_info(logger, ...)           sfi_log (SFI_LOG_INFO, logger, __VA_ARGS__)
#define sfi_diag(...)                   sfi_log (SFI_LOG_DIAG, SfiLogger (0, 0, 0), __VA_ARGS__)
#define sfi_msg(...)                    sfi_log_printf (NULL, SFI_LOG_INFO, SfiLogger (NULL, NULL, NULL), __VA_ARGS__)


typedef enum /*< skip >*/
{
  SFI_LOG_TO_STDERR     = 1,
  SFI_LOG_TO_STDLOG     = 2,
  SFI_LOG_TO_HANDLER    = 4,
} SfiLogActions;
typedef struct _SfiLogContext        SfiLogContext;
typedef void  (*SfiLogHandler)      (const char          *log_domain,
                                     unsigned char        level,
                                     const SfiLogContext *lcontext,
                                     const char          *message);

void sfi_log_set_handler            (SfiLogHandler        handler);
void sfi_log_default_handler        (const char          *log_domain,
                                     unsigned char        level,
                                     const SfiLogContext *lcontext,
                                     const char          *message);
void sfi_log_configure_level        (unsigned char        level,
                                     SfiLogActions        actions);
void sfi_log_configure_stdlog       (gboolean             stdlog_to_err,
                                     const char          *stdlog_filename,
                                     gint                 syslog_priority); /* if != 0, stdlog to syslog */
struct _SfiLogContext {
  const char *key;
  const char *config_blurb;
  const char *alert_blurb;
};
static inline
const SfiLogContext sfi_log_context (const char          *key,
                                     const char          *config_blurb,     /* toggle in preferences */
                                     const char          *alert_blurb);     /* toggle in dialog */
void                sfi_log_printf  (const char          *log_domain,
                                     unsigned char        level,
                                     const SfiLogContext  lcontext,
                                     const char          *format,
                                     ...) G_GNUC_PRINTF (4,5);
void                sfi_log_valist  (const char          *log_domain,
                                     unsigned char        level,
                                     const SfiLogContext  lcontext,
                                     const char          *format,
                                     va_list              args);
#ifndef SFI_LOG_DOMAIN
#define SFI_LOG_DOMAIN  G_LOG_DOMAIN
#endif
#define sfi_log(level, logger, ...)     sfi_log_printf (SFI_LOG_DOMAIN, level, logger, __VA_ARGS__)


/* --- internal/implementation --- */
#define	SFI_LOG_ERROR	('E')
#define	SFI_LOG_WARN	('W')
#define	SFI_LOG_INFO	('I')
#define	SFI_LOG_DIAG	('A')
#define	SFI_LOG_DEBUG	('D')

static inline const SfiLogContext
sfi_log_context (const char *key,
                 const char *config_blurb,
                 const char *alert_blurb)
{
  SfiLogContext lcontext;
  lcontext.key = key;
  lcontext.config_blurb = config_blurb;
  lcontext.alert_blurb = alert_blurb;
  return lcontext;
}


G_END_DECLS

#endif /* __SFI_LOG_H__ */

/* vim:set ts=8 sts=2 sw=2: */
