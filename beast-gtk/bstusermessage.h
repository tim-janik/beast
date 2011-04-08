/* BEAST - Better Audio System
 * Copyright (C) 2002-2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BST_USER_MESSAGE_H__
#define __BST_USER_MESSAGE_H__

#include	"bstutils.h"

G_BEGIN_DECLS

/* --- structures --- */
typedef enum {
  BST_MSG_NONE          = SFI_MSG_NONE,
  BST_MSG_ALWAYS        = SFI_MSG_ALWAYS,
  BST_MSG_ERROR         = SFI_MSG_ERROR,
  BST_MSG_WARNING       = SFI_MSG_WARNING,
  BST_MSG_SCRIPT        = SFI_MSG_SCRIPT,
  BST_MSG_INFO          = SFI_MSG_INFO,
  BST_MSG_DIAG          = SFI_MSG_DIAG,
  BST_MSG_DEBUG         = SFI_MSG_DEBUG,
} BstMsgType;

typedef struct {
  guint             id;
  gchar            *text;
  gchar            *stock_icon;
  gchar            *options;
} BstMsgBit;

typedef struct {
  const char    *log_domain;
  BstMsgType     type;
  const char    *ident;         /* type identifier */
  const char    *label;         /* type label (translated) */
  const char    *title;
  const char    *primary;
  const char    *secondary;
  const char    *details;
  const char    *config_check;
  SfiProxy       janitor;
  const char    *process;
  guint          pid;
  guint          n_msg_bits;
  BstMsgBit    **msg_bits;
} BstMessage;

typedef struct {
  guint        type;
  const gchar *ident;
  const gchar *label; /* maybe NULL */
} BstMsgID;

/* --- prototypes --- */
void              bst_message_connect_to_server	(void);
void              bst_message_dialogs_popdown	(void);
guint             bst_message_handler           (const BstMessage       *message);
void              bst_message_synth_msg_handler (const BseMessage       *umsg);
const BstMsgID*   bst_message_list_types        (guint                  *n_types);
guint             bst_message_dialog_display    (const char             *log_domain,
                                                 BstMsgType              type,
                                                 guint                   n_bits,
                                                 BstMsgBit             **bits);
void              bst_msg_bit_free              (BstMsgBit              *mbit);
BstMsgBit*        bst_msg_bit_printf            (guint8                  msg_part_id,
                                                 const char             *format,
                                                 ...) G_GNUC_PRINTF (2, 3);
BstMsgBit*        bst_msg_bit_create_choice     (guint                   id,
                                                 const gchar            *name,
                                                 const gchar            *stock_icon,
                                                 const gchar            *options);
#define           bst_msg_dialog(level, ...)    BST_MSG_DIALOG (level, __VA_ARGS__)
/* SFI message bit equivalents */
#define BST_MSG_TEXT0(...)                      bst_msg_bit_printf ('0', __VA_ARGS__)
#define BST_MSG_TEXT1(...)                      bst_msg_bit_printf ('1', __VA_ARGS__)
#define BST_MSG_TEXT2(...)                      bst_msg_bit_printf ('2', __VA_ARGS__)
#define BST_MSG_TEXT3(...)                      bst_msg_bit_printf ('3', __VA_ARGS__)
#define BST_MSG_CHECK(...)                      bst_msg_bit_printf ('c', __VA_ARGS__)
#define BST_MSG_TITLE                           BST_MSG_TEXT0 /* alias */
#define BST_MSG_PRIMARY                         BST_MSG_TEXT1 /* alias */
#define BST_MSG_SECONDARY                       BST_MSG_TEXT2 /* alias */
#define BST_MSG_DETAIL                          BST_MSG_TEXT3 /* alias */
/* BST specific message bits */
#define BST_MSG_CHOICE(id, name, stock_icon)    bst_msg_bit_create_choice (id, name, stock_icon, "C")          /* choice */
#define BST_MSG_CHOICE_D(id, name, stock_icon)  bst_msg_bit_create_choice (id, name, stock_icon, "D")          /* default */
#define BST_MSG_CHOICE_S(id, name, sticn, sens) bst_msg_bit_create_choice (id, name, sticn, (sens) ? "" : "I") /* insensitive */
#define BST_MSG_DIALOG(lvl, ...)                ({ BstMsgType __mt = lvl; guint __result = 0;                   \
                                                   if (sfi_msg_check (__mt)) {                                  \
                                                     BstMsgBit *__ba[] = { __VA_ARGS__ };                       \
                                                     __result = bst_message_dialog_display (BIRNET_LOG_DOMAIN,  \
                                                                 __mt, BIRNET_ARRAY_SIZE (__ba), __ba); }       \
                                                   __result; })

G_END_DECLS

#endif	/* __BST_USER_MESSAGE_H__ */
