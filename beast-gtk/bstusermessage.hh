// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_USER_MESSAGE_H__
#define __BST_USER_MESSAGE_H__
#include	"bstutils.hh"
G_BEGIN_DECLS

/* --- structures --- */
typedef enum {
  BST_MSG_ERROR         = Bse::ERROR,
  BST_MSG_WARNING       = Bse::WARNING,
  BST_MSG_INFO          = Bse::INFO,
  BST_MSG_DEBUG         = Bse::DEBUG,
  BST_MSG_SCRIPT,
} BstMsgType;
const char* bst_msg_type_ident (BstMsgType);

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
guint             bst_message_dialog_display    (const char             *log_domain,
                                                 BstMsgType              type,
                                                 guint                   n_bits,
                                                 BstMsgBit             **bits);
void              bst_msg_bit_free              (BstMsgBit              *mbit);
#define           bst_msg_bit_printf(msg_part_id, ...)  bst_msg_bit_create (msg_part_id, Rapicorn::string_format (__VA_ARGS__))
BstMsgBit*        bst_msg_bit_create            (guint8 msg_part_id, const std::string &text);
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
#define BST_MSG_DIALOG(lvl, ...)                ({ BstMsgType __mt = lvl; uint __result = 0;                  \
                                                   BstMsgBit *__ba[] = { __VA_ARGS__ };                       \
                                                   __result = bst_message_dialog_display ("BEAST",  \
                                                               __mt, RAPICORN_ARRAY_SIZE (__ba), __ba);         \
                                                   __result; })

G_END_DECLS

#endif	/* __BST_USER_MESSAGE_H__ */
