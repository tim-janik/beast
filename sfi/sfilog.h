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

/* --- standard logging --- */
#define sfi_error(        ...)          sfi_log_printf (SFI_LOG_DOMAIN, SFI_MSG_ERROR,   NULL, __VA_ARGS__)
#define sfi_warning(      ...)          sfi_log_printf (SFI_LOG_DOMAIN, SFI_MSG_WARNING, NULL, __VA_ARGS__)
#define sfi_info(         ...)          sfi_log_printf (SFI_LOG_DOMAIN, SFI_MSG_INFO,    NULL, __VA_ARGS__)
#define sfi_diag(         ...)          sfi_log_printf (SFI_LOG_DOMAIN, SFI_MSG_DIAG,    NULL, __VA_ARGS__)
#define sfi_debug(   key, ...)          do { if (sfi_debug_check (key)) sfi_log_printf (SFI_LOG_DOMAIN, SFI_MSG_DEBUG, NULL, __VA_ARGS__); } while (0)
#define sfi_nodebug( key, ...)          do { /* nothing */ } while (0)

/* --- debugging --- */
gboolean           sfi_debug_check      (const char     *key);
void               sfi_debug_allow      (const char     *key_list);
void               sfi_debug_deny       (const char     *key_list);

/* --- user interface messages --- */
#define sfi_log_msg(level, ...)         sfi_log_msg_valist (SFI_LOG_DOMAIN, level, __VA_ARGS__, NULL);
#define SFI_MSG_ERROR                   ('E')
#define SFI_MSG_WARNING                 ('W')
#define SFI_MSG_INFO                    ('I')
#define SFI_MSG_DIAG                    ('A')
#define SFI_MSG_DEBUG                   ('D')
#define SFI_MSG_TEXT0(...)              sfi_log_bit_printf ('0', __VA_ARGS__) /* message title */
#define SFI_MSG_TEXT1(...)              sfi_log_bit_printf ('1', __VA_ARGS__) /* primary message */
#define SFI_MSG_TEXT2(...)              sfi_log_bit_printf ('2', __VA_ARGS__) /* secondary message */
#define SFI_MSG_TEXT3(...)              sfi_log_bit_printf ('3', __VA_ARGS__) /* message details */
#define SFI_MSG_CHECK(...)              sfi_log_bit_printf ('c', __VA_ARGS__) /* user switch */
#define SFI_MSG_TITLE                   SFI_MSG_TEXT0 /* alias */
#define SFI_MSG_PRIMARY                 SFI_MSG_TEXT1 /* alias */
#define SFI_MSG_SECONDARY               SFI_MSG_TEXT2 /* alias */
#define SFI_MSG_DETAIL                  SFI_MSG_TEXT3 /* alias */

/* --- logging configuration --- */
typedef struct SfiLogMessage        SfiLogMessage;
typedef void (*SfiLogHandler)      (const SfiLogMessage *message);
typedef enum /*< skip >*/
{
  SFI_LOG_TO_STDERR     = 1,
  SFI_LOG_TO_STDLOG     = 2,
  SFI_LOG_TO_HANDLER    = 4,
} SfiLogFlags;
void    sfi_log_assign_level       (unsigned char        level,
                                    SfiLogFlags          channel_mask);
void    sfi_log_set_stdlog         (gboolean             stdlog_to_stderr,
                                    const char          *stdlog_filename,
                                    guint                syslog_priority); /* if != 0, stdlog to syslog */
void    sfi_log_set_thread_handler (SfiLogHandler        handler);
void    sfi_log_default_handler    (const SfiLogMessage *message);
gchar*  sfi_log_msg_level_name     (guint                level);

/* --- logging internals --- */
typedef struct SfiLogBit SfiLogBit;
struct SfiLogMessage {
  gchar         *log_domain;
  guint          level;
  char          *key;            /* maybe generated */
  char          *title;          /* translated */
  char          *primary;        /* translated */
  char          *secondary;      /* translated */
  char          *details;        /* translated */
  char          *config_check;   /* translated */
  guint          n_log_bits;
  SfiLogBit    **log_bits;
};
struct SfiLogBit {
  gconstpointer  owner;
  gpointer       data;
};

void       sfi_log_printf               (const char     *log_domain,
                                         guint           level,
                                         const char     *key,
                                         const char     *format,
                                         ...) G_GNUC_PRINTF (4, 5);
void       sfi_log_msg_valist           (const char     *log_domain,
                                         guint           level,
                                         SfiLogBit      *lbit1,
                                         SfiLogBit      *lbit2,
                                         ...);
SfiLogBit* sfi_log_bit_appoint          (gconstpointer   owner,
                                         gpointer        data,
                                         void          (*data_free) (gpointer));
SfiLogBit* sfi_log_bit_printf           (guint8          log_msg_tag,
                                         const char     *format,
                                         ...) G_GNUC_PRINTF (2, 3);
void       _sfi_init_logging            (void);
#ifndef SFI_LOG_DOMAIN
#define SFI_LOG_DOMAIN  G_LOG_DOMAIN
#endif

G_END_DECLS

#endif /* __SFI_LOG_H__ */

/* vim:set ts=8 sts=2 sw=2: */
