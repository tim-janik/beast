/* BirnetMsg
 * Copyright (C) 2002-2006 Tim Janik
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
#ifndef __BIRNET_MSG_H__
#define __BIRNET_MSG_H__

#include <birnet/birnetthread.h>

G_BEGIN_DECLS

typedef enum {
  BIRNET_MSG_NONE = 0,     /* always off */
  BIRNET_MSG_FATAL,        /* always on */
  BIRNET_MSG_ERROR,
  BIRNET_MSG_WARNING,
  BIRNET_MSG_SCRIPT,
  BIRNET_MSG_INFO,
  BIRNET_MSG_DIAG,
  BIRNET_MSG_DEBUG,
  BIRNET_MSG_LAST
} BirnetMsgType;

/* --- standard logging --- */
#define birnet_fatal(        ...)          birnet_msg_checked (BIRNET_LOG_DOMAIN, BIRNET_MSG_FATAL, __VA_ARGS__)
#define birnet_error(        ...)          birnet_msg_checked (BIRNET_LOG_DOMAIN, BIRNET_MSG_ERROR,   __VA_ARGS__)
#define birnet_warning(      ...)          birnet_msg_checked (BIRNET_LOG_DOMAIN, BIRNET_MSG_WARNING, __VA_ARGS__)
#define birnet_info(         ...)          birnet_msg_checked (BIRNET_LOG_DOMAIN, BIRNET_MSG_INFO, __VA_ARGS__)
#define birnet_diag(         ...)          birnet_msg_checked (BIRNET_LOG_DOMAIN, BIRNET_MSG_DIAG, __VA_ARGS__)
#define birnet_debug(   lvl, ...)          birnet_msg_checked (BIRNET_LOG_DOMAIN, lvl, __VA_ARGS__)
#define birnet_nodebug( lvl, ...)          do { /* nothing */ } while (0)

/* --- user interface messages --- */
#define birnet_msg_log(level, ...)         birnet_msg_log_elist (BIRNET_LOG_DOMAIN, level, __VA_ARGS__, NULL)
#define BIRNET_MSG_TEXT0(...)              birnet_msg_bit_printf ('0', __VA_ARGS__) /* message title */
#define BIRNET_MSG_TEXT1(...)              birnet_msg_bit_printf ('1', __VA_ARGS__) /* primary message */
#define BIRNET_MSG_TEXT2(...)              birnet_msg_bit_printf ('2', __VA_ARGS__) /* secondary message */
#define BIRNET_MSG_TEXT3(...)              birnet_msg_bit_printf ('3', __VA_ARGS__) /* message details */
#define BIRNET_MSG_CHECK(...)              birnet_msg_bit_printf ('c', __VA_ARGS__) /* user switch */
#define BIRNET_MSG_TITLE                   BIRNET_MSG_TEXT0 /* alias */
#define BIRNET_MSG_PRIMARY                 BIRNET_MSG_TEXT1 /* alias */
#define BIRNET_MSG_SECONDARY               BIRNET_MSG_TEXT2 /* alias */
#define BIRNET_MSG_DETAIL                  BIRNET_MSG_TEXT3 /* alias */

/* --- config and handler API --- */
typedef enum {
  BIRNET_MSG_TO_STDERR     = 1,
  BIRNET_MSG_TO_STDLOG     = 2,
  BIRNET_MSG_TO_HANDLER    = 4,
} BirnetMsgLogFlags;
typedef struct  BirnetMessage                       BirnetMessage;
typedef void  (*BirnetMsgHandler)                  (const BirnetMessage *message);
static inline
bool            birnet_msg_check                   (BirnetMsgType        mtype);
void            birnet_msg_enable                  (BirnetMsgType        mtype);
void            birnet_msg_disable                 (BirnetMsgType        mtype);
void            birnet_msg_allow                   (const gchar      	*ident_list);
void            birnet_msg_deny                    (const gchar      	*ident_list);
void            birnet_msg_set_thread_handler      (BirnetMsgHandler     handler);
void            birnet_msg_default_handler         (const BirnetMessage *message);
BirnetMsgType   birnet_msg_type_lookup             (const gchar      	*ident);
const gchar*    birnet_msg_type_ident              (BirnetMsgType        mtype);
const gchar*    birnet_msg_type_label              (BirnetMsgType        mtype);
void            birnet_msg_type_configure          (BirnetMsgType        mtype,
						    BirnetMsgLogFlags    channel_mask,
						    const gchar      	*dummy_filename);
void            birnet_msg_configure_stdlog        (bool             	 stdlog_to_stderr,
						    const char       	*stdlog_filename,
						    guint            	 syslog_priority); /* if != 0, stdlog to syslog */
BirnetMsgType   birnet_msg_type_register           (const gchar      	*ident,
						    BirnetMsgType        default_ouput, /* FALSE, TRUE, ... */
						    const gchar      	*label);
/* automatic registration */
#define BIRNET_MSG_TYPE_DEFINE(variable, ident, default_ouput, label) BIRNET_MSG_TYPE__DEF (variable, ident, default_ouput, label)

/* --- logging internals --- */
typedef struct BirnetMsgBit BirnetMsgBit;
struct BirnetMessage {
  gchar         *log_domain;
  BirnetMsgType  type;
  char          *title;         /* translated */
  char          *primary;       /* translated */
  char          *secondary;     /* translated */
  char          *details;       /* translated */
  char          *config_check;  /* translated */
  guint          n_msg_bits;
  BirnetMsgBit **msg_bits;
};
struct BirnetMsgBit {
  gconstpointer  owner;
  gpointer       data;
};

void            birnet_msg_log_printf              (const char       *log_domain,
						    BirnetMsgType     mtype,
						    const char       *format,
						    ...) G_GNUC_PRINTF (3, 4);
void            birnet_msg_log_elist               (const char       *log_domain,
						    BirnetMsgType     mtype,
						    BirnetMsgBit     *lbit1,
						    BirnetMsgBit     *lbit2,
						    ...);
void            birnet_msg_log_trampoline          (const char       *log_domain,
						    BirnetMsgType     mtype,
						    BirnetMsgBit    **lbits,
						    BirnetMsgHandler  handler);
BirnetMsgBit*   birnet_msg_bit_appoint             (gconstpointer     owner,
						    gpointer          data,
						    void            (*data_free) (void*));
BirnetMsgBit*   birnet_msg_bit_printf              (guint8            msg_bit_type,
						    const char       *format,
						    ...) G_GNUC_PRINTF (2, 3);
void            _birnet_init_logging               (void);
static inline bool    
birnet_msg_check (BirnetMsgType mtype)
{
  extern guint8 * volatile birnet_msg_flags;
  extern volatile guint    birnet_msg_flags_max;
  if (mtype >= 0 && (unsigned) mtype <= birnet_msg_flags_max)
    return 0 != (birnet_msg_flags[mtype / 8] & (1 << mtype % 8));
  return 0;
}

/* --- internal macros --- */
#ifndef BIRNET_LOG_DOMAIN
#define BIRNET_LOG_DOMAIN  G_LOG_DOMAIN
#endif
#define birnet_msg_checked( dom, lvl, ...)  \
  do { BirnetMsgType __mt = lvl; if (birnet_msg_check (__mt)) birnet_msg_log_printf (dom, __mt, __VA_ARGS__); } while (0)
#define BIRNET_MSG_TYPE__DEF(variable, identifier, default_ouput, label) \
  BirnetMsgType variable = (BirnetMsgType) 0; \
  static void BIRNET_CONSTRUCTOR \
  BIRNET_CPP_PASTE4 (__birnet_msg_type__init, __LINE__, __, variable) (void) \
  { variable = birnet_msg_type_register (identifier, default_ouput, label); }

G_END_DECLS

#endif /* __BIRNET_MSG_H__ */

/* vim:set ts=8 sts=2 sw=2: */
