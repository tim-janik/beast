/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_USER_MESSAGE_H__
#define __BST_USER_MESSAGE_H__

#include	"bstutils.h"

G_BEGIN_DECLS

/* --- structures --- */
typedef enum {
  BST_MSG_NONE          = SFI_MSG_NONE,
  BST_MSG_FATAL         = SFI_MSG_FATAL,
  BST_MSG_ERROR         = SFI_MSG_ERROR,
  BST_MSG_WARNING       = SFI_MSG_WARNING,
  BST_MSG_SCRIPT        = SFI_MSG_SCRIPT,
  BST_MSG_INFO          = SFI_MSG_INFO,
  BST_MSG_DIAG          = SFI_MSG_DIAG,
  BST_MSG_DEBUG         = SFI_MSG_DEBUG,
} BstMsgType;

typedef struct {
  gchar         *log_domain;
  BstMsgType     type;
  gchar         *ident;         /* type identifier */
  gchar         *label;         /* type label (translated) */
  gchar         *title;
  gchar         *primary;
  gchar         *secondary;
  gchar         *details;
  gchar         *config_check;
  SfiProxy       janitor;
  gchar         *process;
  guint          pid;
  guint          n_msg_bits;
  SfiMsgBit    **msg_bits;
} BstMessage;
typedef struct {
  guint        type;
  const gchar *ident;
  const gchar *label; /* maybe NULL */
} BstMsgID;

/* --- prototypes --- */
void              bst_message_connect_to_server	(void);
void              bst_message_dialogs_popdown	(void);
void              bst_message_handler           (const BstMessage       *message);
void              bst_message_log_handler       (const SfiMessage       *lmsg);
void              bst_message_synth_msg_handler (const BseMessage       *umsg);
const BstMsgID*   bst_message_list_types        (guint                  *n_types);
void              bst_message_dialog_elist      (const char             *log_domain,
                                                 BstMsgType              type,
                                                 SfiMsgBit              *lbit1,
                                                 SfiMsgBit              *lbit2,
                                                 ...);
#define bst_msg_dialog(level, ...)              bst_message_dialog_elist (SFI_LOG_DOMAIN, level, __VA_ARGS__, NULL)
#define BST_MSG_TEXT0(...)                      SFI_MSG_TEXT0 (__VA_ARGS__)
#define BST_MSG_TEXT1(...)                      SFI_MSG_TEXT1 (__VA_ARGS__)
#define BST_MSG_TEXT2(...)                      SFI_MSG_TEXT2 (__VA_ARGS__)
#define BST_MSG_TEXT3(...)                      SFI_MSG_TEXT3 (__VA_ARGS__)
#define BST_MSG_CHECK(...)                      SFI_MSG_CHECK (__VA_ARGS__)
#define BST_MSG_TITLE                           BST_MSG_TEXT0 /* alias */
#define BST_MSG_PRIMARY                         BST_MSG_TEXT1 /* alias */
#define BST_MSG_SECONDARY                       BST_MSG_TEXT2 /* alias */
#define BST_MSG_DETAIL                          BST_MSG_TEXT3 /* alias */

G_END_DECLS

#endif	/* __BST_USER_MESSAGE_H__ */
