/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef	__BSE_STORAGE_H__
#define	__BSE_STORAGE_H__

#include <bse/bseobject.h>
#include <bse/gsldefs.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_STORAGE               (BSE_TYPE_ID (BseStorage))
#define BSE_STORAGE(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_STORAGE, BseStorage))
#define BSE_STORAGE_CLASS(class)       (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_STORAGE, BseStorageClass))
#define BSE_IS_STORAGE(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_STORAGE))
#define BSE_IS_STORAGE_CLASS(class)    (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_STORAGE))
#define BSE_STORAGE_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_STORAGE, BseStorageClass))


/* --- macros --- */
#define	BSE_STORAGE_READABLE(st)	((BSE_OBJECT_FLAGS (st) & BSE_STORAGE_FLAG_READABLE) != 0)
#define	BSE_STORAGE_WRITABLE(st)	((BSE_OBJECT_FLAGS (st) & BSE_STORAGE_FLAG_WRITABLE) != 0)
#define	BSE_STORAGE_NEEDS_BREAK(st)	((BSE_OBJECT_FLAGS (st) & BSE_STORAGE_FLAG_NEEDS_BREAK) != 0)
#define	BSE_STORAGE_AT_BOL(st)		((BSE_OBJECT_FLAGS (st) & BSE_STORAGE_FLAG_AT_BOL) != 0)
#define	BSE_STORAGE_PUT_DEFAULTS(st)	((BSE_OBJECT_FLAGS (st) & BSE_STORAGE_FLAG_PUT_DEFAULTS) != 0)
#define	BSE_STORAGE_SELF_CONTAINED(st)	((BSE_OBJECT_FLAGS (st) & BSE_STORAGE_FLAG_SELF_CONTAINED) != 0)
#define	BSE_STORAGE_PROXIES_ENABLED(st)	((BSE_OBJECT_FLAGS (st) & BSE_STORAGE_FLAG_PROXIES_ENABLED) != 0)


/* --- BseStorage flags --- */
typedef enum	/*< skip >*/
{
  BSE_STORAGE_FLAG_READABLE	   = 1 << (BSE_OBJECT_FLAGS_USHIFT + 0),
  BSE_STORAGE_FLAG_WRITABLE	   = 1 << (BSE_OBJECT_FLAGS_USHIFT + 1),
  BSE_STORAGE_FLAG_NEEDS_BREAK	   = 1 << (BSE_OBJECT_FLAGS_USHIFT + 2),
  BSE_STORAGE_FLAG_AT_BOL	   = 1 << (BSE_OBJECT_FLAGS_USHIFT + 3),
  BSE_STORAGE_FLAG_PUT_DEFAULTS	   = 1 << (BSE_OBJECT_FLAGS_USHIFT + 4),
  BSE_STORAGE_FLAG_SELF_CONTAINED  = 1 << (BSE_OBJECT_FLAGS_USHIFT + 5),
  BSE_STORAGE_FLAG_PROXIES_ENABLED = 1 << (BSE_OBJECT_FLAGS_USHIFT + 6)
} BseStorageFlags;
#define BSE_STORAGE_FLAGS_USHIFT	  (BSE_OBJECT_FLAGS_USHIFT + 7)
typedef enum	/*< skip >*/
{
  BSE_STORAGE_SKIP_DEFAULTS	= 1 << 0,
  BSE_STORAGE_SKIP_COMPAT	= 1 << 1
} BseStorageMode;


/* --- BseStorage --- */
typedef struct _BseStorageBBlock   BseStorageBBlock;
typedef struct _BseStorageItemLink BseStorageItemLink;
typedef void (*BseStorageRestoreLink)	(gpointer	 data,
					 BseStorage	*storage,
					 BseItem	*from_item,
					 BseItem	*to_item,
					 const gchar	*error);
struct _BseStorage
{
  BseObject		 parent_instance;
  /* reading */
  GScanner		*scanner;
  gint			 fd;
  glong			 bin_offset;
  GHashTable		*path_table;
  BseStorageItemLink	*item_links;
  guint                  major_version;
  guint                  minor_version;
  guint                  micro_version;
  /* writing */
  GSList		*indent;
  BseStorageBBlock	*wblocks;	/* keeps ref */
  GString		*gstring;
};
struct _BseStorageClass
{
  BseObjectClass parent_class;
};


/* --- prototypes -- */
void		bse_storage_reset		(BseStorage	*storage);
void		bse_storage_prepare_write	(BseStorage	*storage,
						 BseStorageMode  mode);
BseErrorType	bse_storage_input_file		(BseStorage	*storage,
						 const gchar	*file_name);
BseErrorType	bse_storage_input_text		(BseStorage	*storage,
						 const gchar	*text);
GTokenType	bse_storage_restore_item	(BseStorage	*storage,
						 gpointer	 item);
GTokenType	bse_storage_parse_statement	(BseStorage	*storage,
						 gpointer	 item);
void		bse_storage_store_item		(BseStorage	*storage,
						 gpointer	 item);
void		bse_storage_store_child		(BseStorage	*storage,
						 gpointer	 item);
void		bse_storage_enable_proxies	(BseStorage	*storage);


/* --- writing --- */
void		bse_storage_push_level		(BseStorage	*storage);
void		bse_storage_pop_level		(BseStorage	*storage);
void		bse_storage_putc		(BseStorage	*storage,
						 gchar		 character);
void		bse_storage_puts		(BseStorage	*storage,
						 const gchar	*string);
void		bse_storage_putf		(BseStorage	*storage,
						 gfloat		 vfloat);
void		bse_storage_putd		(BseStorage	*storage,
						 gdouble	 vdouble);
void		bse_storage_putr		(BseStorage	*storage,
						 SfiReal	 vreal,
						 const gchar	*hints);
void		bse_storage_printf		(BseStorage	*storage,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2, 3);
void		bse_storage_handle_break	(BseStorage	*storage);
void		bse_storage_break		(BseStorage	*storage);
void		bse_storage_needs_break		(BseStorage	*storage);
void		bse_storage_put_param		(BseStorage	*storage,
						 const GValue	*value,
						 GParamSpec	*pspec);
void		bse_storage_put_value		(BseStorage	*storage,
						 const GValue	*value,
						 GParamSpec	*pspec);
void		bse_storage_put_data_handle	(BseStorage	*storage,
						 guint		 significant_bits,
						 GslDataHandle	*handle,
						 GslLong	 vlength);
void		bse_storage_flush_fd		(BseStorage	*storage,
						 gint		 fd);
gchar*		bse_storage_mem_flush		(BseStorage	*storage);
const gchar*	bse_storage_peek_text		(BseStorage	*storage,
						 guint		*length);
BseErrorType	bse_storage_store_procedure	(gpointer	   storage,
						 BseProcedureClass *proc,
						 const GValue      *ivalues,
						 GValue            *ovalues);
void		bse_storage_put_item_link	(BseStorage	*storage,
						 BseItem	*from_item,
						 BseItem	*to_item);


/* --- reading --- */
gboolean	bse_storage_input_eof		(BseStorage	*storage);
void		bse_storage_error		(BseStorage	*storage,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2,3);
void		bse_storage_unexp_token		(BseStorage	*storage,
						 GTokenType	 expected_token);
void		bse_storage_warn		(BseStorage	*storage,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2,3);
GTokenType	bse_storage_warn_skip		(BseStorage	*storage,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2,3);
/* use this instead of bse_storage_warn_skip() if the current token
 * hasn't been processed yet (might be the seeked for ')')
 */
GTokenType	bse_storage_warn_skipc		(BseStorage	*storage,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2,3);
GTokenType	bse_storage_skip_statement	(BseStorage	*storage);
GTokenType	bse_storage_parse_rest		(BseStorage     *storage,
						 BseTryStatement try_statement,
						 gpointer        func_data,
						 gpointer        user_data);
GTokenType	bse_storage_parse_note		(BseStorage	*storage,
						 gint		*note,
						 gchar           bbuffer[BSE_BBUFFER_SIZE]);
GTokenType	bse_storage_parse_data_handle	(BseStorage	*storage,
						 guint           n_channels,
						 gfloat          osc_freq,
						 gfloat          mix_freq,
						 GslDataHandle **data_handle_p);
GTokenType	bse_storage_parse_param_value	(BseStorage	*storage,
						 GValue		*value,
						 GParamSpec	*pspec);
GTokenType	bse_storage_parse_item_link	(BseStorage	*storage,
						 BseItem	*from_item,
						 BseStorageRestoreLink restore_link,
						 gpointer	  data);
void		bse_storage_resolve_item_links	(BseStorage	*storage);


/* --- helpers --- */
#define bse_storage_scanner_parse_or_return(scanner, token)  G_STMT_START{ \
  guint _t = (token); \
  if (g_scanner_get_next_token (scanner) != _t) \
    return _t; \
}G_STMT_END
#define bse_storage_scanner_peek_or_return(scanner, token)   G_STMT_START{ \
  GScanner *__s = (scanner); guint _t = (token); \
  if (g_scanner_peek_next_token (__s) != _t) { \
    g_scanner_get_next_token (__s); /* advance position for error-handler */ \
    return _t; \
  } \
}G_STMT_END


G_END_DECLS

#endif /* __BSE_STORAGE_H__ */
