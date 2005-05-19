/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2005 Tim Janik
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

typedef enum {
  SFI_MSG_NONE = 0,     /* always off */
  SFI_MSG_FATAL,        /* always on */
  SFI_MSG_ERROR,
  SFI_MSG_WARNING,
  SFI_MSG_SCRIPT,
  SFI_MSG_INFO,
  SFI_MSG_DIAG,
  SFI_MSG_DEBUG,
  SFI_MSG_LAST,
  SFI_MSG__UINT32TAG = 0xffffffff
} SfiMsgType;

/* --- standard logging --- */
#define sfi_fatal(        ...)          sfi_msg_checked (SFI_LOG_DOMAIN, SFI_MSG_FATAL, __VA_ARGS__)
#define sfi_error(        ...)          sfi_msg_checked (SFI_LOG_DOMAIN, SFI_MSG_ERROR,   __VA_ARGS__)
#define sfi_warning(      ...)          sfi_msg_checked (SFI_LOG_DOMAIN, SFI_MSG_WARNING, __VA_ARGS__)
#define sfi_info(         ...)          sfi_msg_checked (SFI_LOG_DOMAIN, SFI_MSG_INFO, __VA_ARGS__)
#define sfi_diag(         ...)          sfi_msg_checked (SFI_LOG_DOMAIN, SFI_MSG_DIAG, __VA_ARGS__)
#define sfi_debug(   lvl, ...)          sfi_msg_checked (SFI_LOG_DOMAIN, lvl, __VA_ARGS__)
#define sfi_nodebug( lvl, ...)          do { /* nothing */ } while (0)

/* --- user interface messages --- */
#define sfi_msg_log(level, ...)         sfi_msg_log_elist (SFI_LOG_DOMAIN, level, __VA_ARGS__, NULL)
#define SFI_MSG_TEXT0(...)              sfi_msg_bit_printf ('0', __VA_ARGS__) /* message title */
#define SFI_MSG_TEXT1(...)              sfi_msg_bit_printf ('1', __VA_ARGS__) /* primary message */
#define SFI_MSG_TEXT2(...)              sfi_msg_bit_printf ('2', __VA_ARGS__) /* secondary message */
#define SFI_MSG_TEXT3(...)              sfi_msg_bit_printf ('3', __VA_ARGS__) /* message details */
#define SFI_MSG_CHECK(...)              sfi_msg_bit_printf ('c', __VA_ARGS__) /* user switch */
#define SFI_MSG_TITLE                   SFI_MSG_TEXT0 /* alias */
#define SFI_MSG_PRIMARY                 SFI_MSG_TEXT1 /* alias */
#define SFI_MSG_SECONDARY               SFI_MSG_TEXT2 /* alias */
#define SFI_MSG_DETAIL                  SFI_MSG_TEXT3 /* alias */

/* --- config and handler API --- */
typedef enum {
  SFI_MSG_TO_STDERR     = 1,
  SFI_MSG_TO_STDLOG     = 2,
  SFI_MSG_TO_HANDLER    = 4,
} SfiMsgLogFlags;
typedef struct  SfiMessage                       SfiMessage;
typedef void  (*SfiMsgHandler)                  (const SfiMessage *message);
static inline
gboolean        sfi_msg_check                   (SfiMsgType        mtype);
void            sfi_msg_enable                  (SfiMsgType        mtype);
void            sfi_msg_disable                 (SfiMsgType        mtype);
void            sfi_msg_allow                   (const gchar      *ident_list);
void            sfi_msg_deny                    (const gchar      *ident_list);
void            sfi_msg_set_thread_handler      (SfiMsgHandler     handler);
void            sfi_msg_default_handler         (const SfiMessage *message);
SfiMsgType      sfi_msg_type_lookup             (const gchar      *ident);
const gchar*    sfi_msg_type_ident              (SfiMsgType        mtype);
const gchar*    sfi_msg_type_label              (SfiMsgType        mtype);
void            sfi_msg_type_configure          (SfiMsgType        mtype,
                                                 SfiMsgLogFlags    channel_mask,
                                                 const gchar      *dummy_filename);
void            sfi_msg_configure_stdlog        (gboolean          stdlog_to_stderr,
                                                 const char       *stdlog_filename,
                                                 guint             syslog_priority); /* if != 0, stdlog to syslog */
SfiMsgType      sfi_msg_type_register           (const gchar      *ident,
                                                 SfiMsgType        default_ouput, /* FALSE, TRUE, ... */
                                                 const gchar      *label);
/* automatic registration */
#define SFI_MSG_TYPE_DEFINE(variable, ident, default_ouput, label) SFI_MSG_TYPE__DEF (variable, ident, default_ouput, label)

/* --- logging internals --- */
typedef struct SfiMsgBit SfiMsgBit;
struct SfiMessage {
  gchar         *log_domain;
  SfiMsgType     type;
  char          *title;         /* translated */
  char          *primary;       /* translated */
  char          *secondary;     /* translated */
  char          *details;       /* translated */
  char          *config_check;  /* translated */
  guint          n_msg_bits;
  SfiMsgBit    **msg_bits;
};
struct SfiMsgBit {
  gconstpointer  owner;
  gpointer       data;
};

void            sfi_msg_log_printf              (const char       *log_domain,
                                                 SfiMsgType        mtype,
                                                 const char       *format,
                                                 ...) G_GNUC_PRINTF (3, 4);
void            sfi_msg_log_elist               (const char       *log_domain,
                                                 SfiMsgType        mtype,
                                                 SfiMsgBit        *lbit1,
                                                 SfiMsgBit        *lbit2,
                                                 ...);
void            sfi_msg_log_trampoline          (const char       *log_domain,
                                                 SfiMsgType        mtype,
                                                 SfiMsgBit       **lbits,
                                                 SfiMsgHandler     handler);
SfiMsgBit*      sfi_msg_bit_appoint             (gconstpointer     owner,
                                                 gpointer          data,
                                                 void            (*data_free) (void*));
SfiMsgBit*      sfi_msg_bit_printf              (guint8            msg_bit_type,
                                                 const char       *format,
                                                 ...) G_GNUC_PRINTF (2, 3);
void            _sfi_init_logging               (void);
static inline gboolean
sfi_msg_check (SfiMsgType mtype)
{
  extern guint8 * volatile sfi_msg_flags;
  extern volatile guint    sfi_msg_flags_max;
  if (mtype >= 0 && mtype <= sfi_msg_flags_max)
    return 0 != (sfi_msg_flags[mtype / 8] & (1 << mtype % 8));
  return 0;
}

/* --- internal macros --- */
#ifndef SFI_LOG_DOMAIN
#define SFI_LOG_DOMAIN  G_LOG_DOMAIN
#endif
#define sfi_msg_checked( dom, lvl, ...)  \
  do { SfiMsgType __mt = lvl; if (sfi_msg_check (__mt)) sfi_msg_log_printf (dom, __mt, __VA_ARGS__); } while (0)
#define SFI_MSG_TYPE__CONCAT2(a,b,c,d)  a ## b ## c ## d /* twofold indirection is required to expand __LINE__ */
#define SFI_MSG_TYPE__CONCAT(a,b,c,d)   SFI_MSG_TYPE__CONCAT2 (a, b, c, d)
#define SFI_MSG_TYPE__DEF(variable, identifier, default_ouput, label) \
  SfiMsgType variable = (SfiMsgType) 0; \
  static void __attribute__ ((constructor)) \
  SFI_MSG_TYPE__CONCAT (__sfi_msg_type__init, __LINE__, __, variable) (void) \
  { variable = sfi_msg_type_register (identifier, default_ouput, label); }

G_END_DECLS

#endif /* __SFI_LOG_H__ */

/* vim:set ts=8 sts=2 sw=2: */
