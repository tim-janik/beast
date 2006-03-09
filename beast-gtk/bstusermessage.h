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
  BST_MSG_NONE          = BIRNET_MSG_NONE,
  BST_MSG_FATAL         = BIRNET_MSG_FATAL,
  BST_MSG_ERROR         = BIRNET_MSG_ERROR,
  BST_MSG_WARNING       = BIRNET_MSG_WARNING,
  BST_MSG_SCRIPT        = BIRNET_MSG_SCRIPT,
  BST_MSG_INFO          = BIRNET_MSG_INFO,
  BST_MSG_DIAG          = BIRNET_MSG_DIAG,
  BST_MSG_DEBUG         = BIRNET_MSG_DEBUG,
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
  BirnetMsgBit    **msg_bits;
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
void              bst_message_log_handler       (const BirnetMessage       *lmsg);
void              bst_message_synth_msg_handler (const BseMessage       *umsg);
const BstMsgID*   bst_message_list_types        (guint                  *n_types);
guint             bst_message_dialog_elist      (const char             *log_domain,
                                                 BstMsgType              type,
                                                 BirnetMsgBit              *lbit1,
                                                 BirnetMsgBit              *lbit2,
                                                 ...);
BirnetMsgBit*        bst_message_bit_appoint       (guint                   id,
                                                 const gchar            *name,
                                                 const gchar            *stock_icon,
                                                 const gchar            *options);
#define bst_msg_dialog(level, ...)              bst_message_dialog_elist (BIRNET_LOG_DOMAIN, level, __VA_ARGS__, NULL)
/* SFI message bit equivalents */
#define BST_MSG_TEXT0(...)                      BIRNET_MSG_TEXT0 (__VA_ARGS__)
#define BST_MSG_TEXT1(...)                      BIRNET_MSG_TEXT1 (__VA_ARGS__)
#define BST_MSG_TEXT2(...)                      BIRNET_MSG_TEXT2 (__VA_ARGS__)
#define BST_MSG_TEXT3(...)                      BIRNET_MSG_TEXT3 (__VA_ARGS__)
#define BST_MSG_CHECK(...)                      BIRNET_MSG_CHECK (__VA_ARGS__)
#define BST_MSG_TITLE                           BST_MSG_TEXT0 /* alias */
#define BST_MSG_PRIMARY                         BST_MSG_TEXT1 /* alias */
#define BST_MSG_SECONDARY                       BST_MSG_TEXT2 /* alias */
#define BST_MSG_DETAIL                          BST_MSG_TEXT3 /* alias */
/* BST specific message bits */
#define BST_MSG_CHOICE(id, name, stock_icon)    bst_message_bit_appoint (id, name, stock_icon, "")
#define BST_MSG_CHOICE_D(id, name, stock_icon)  bst_message_bit_appoint (id, name, stock_icon, "D")          /* default */
#define BST_MSG_CHOICE_S(id, name, sticn, sens) bst_message_bit_appoint (id, name, sticn, (sens) ? "" : "I") /* insensitive */

G_END_DECLS

#endif	/* __BST_USER_MESSAGE_H__ */
