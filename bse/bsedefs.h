/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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
 *
 * bsedefs.h: type definitions and forward definitions libbse
 */
#ifndef __BSE_DEFS_H__
#define __BSE_DEFS_H__

#undef		G_DISABLE_ASSERT
#undef		G_DISABLE_CHECKS
#include	<glib.h>
#include	<bse/glib-extra.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- some globally used defines --- */
#define FIXME(msg)  g_message ("%s:%d:FIXME(%s): " # msg, __FILE__, __LINE__, \
			       G_GNUC_PRETTY_FUNCTION)
#define FIXME_SKIP(code)  g_message ("%s:%d:FIXME(%s): code portion skipped", \
				     __FILE__, __LINE__, G_GNUC_PRETTY_FUNCTION)
#ifdef G_ENABLE_DEBUG
#define BSE_DEBUG(type, code)	G_STMT_START { \
  if (bse_debug_flags & BSE_DEBUG_##type) \
    { code ; } \
} G_STMT_END
#else  /* !G_ENABLE_DEBUG */
#define BSE_DEBUG(type, code)	/* do nothing */
#endif /* !G_ENABLE_DEBUG */


/* --- BSE basic typedefs --- */
typedef gint64				BseIndex;
typedef	gint32				BseMixValue;
typedef	gint16				BseSampleValue;
typedef gulong				BseTime;
typedef guint				BseIndex2D;
typedef guint				BseType;
typedef struct  _BseTypeClass		BseTypeClass;
typedef struct  _BseTypeInterface     	BseTypeInterface;
typedef struct  _BseTypeStruct		BseTypeStruct;


/* --- BSE parameters (and values) --- */
typedef struct  _BseEnumValue		BseEnumValue;
typedef struct  _BseFlagsValue		BseFlagsValue;
typedef struct  _BseParam		BseParam;
typedef union   _BseParamValue		BseParamValue;
typedef union   _BseParamSpec		BseParamSpec;
typedef struct  _BseParamSpecAny	BseParamSpecAny;
typedef struct  _BseParamSpecBool	BseParamSpecBool;
typedef struct  _BseParamSpecIndex2D	BseParamSpecIndex2D;
typedef struct  _BseParamSpecInt	BseParamSpecInt;
typedef struct  _BseParamSpecUInt	BseParamSpecUInt;
typedef struct  _BseParamSpecEnum	BseParamSpecEnum;
typedef struct  _BseParamSpecFlags	BseParamSpecFlags;
typedef struct  _BseParamSpecFloat	BseParamSpecFloat;
typedef struct  _BseParamSpecDouble	BseParamSpecDouble;
typedef struct  _BseParamSpecTime	BseParamSpecTime;
typedef struct  _BseParamSpecNote	BseParamSpecNote;
typedef struct  _BseParamSpecString	BseParamSpecString;
typedef struct  _BseParamSpecDots	BseParamSpecDots;
typedef struct  _BseParamSpecItem	BseParamSpecItem;


/* --- BSE objects, classes & interfaces --- */
typedef struct	_BseBinData		BseBinData;
typedef struct	_BseBinDataClass	BseBinDataClass;
typedef struct	_BseContainer		BseContainer;
typedef struct	_BseContainerClass	BseContainerClass;
typedef struct	_BseEnumClass		BseEnumClass;
typedef struct	_BseEffect		BseEffect;
typedef struct	_BseEffectClass		BseEffectClass;
typedef struct	_BseFlagsClass		BseFlagsClass;
typedef	struct	_BseFileStream		BseFileStream;
typedef	struct	_BseFileStreamClass	BseFileStreamClass;
typedef struct	_BseInstrument		BseInstrument;
typedef struct	_BseInstrumentClass	BseInstrumentClass;
typedef struct	_BseItem		BseItem;
typedef struct	_BseItemClass		BseItemClass;
typedef	struct	_BseNullStream		BseNullStream;
typedef	struct	_BseNullStreamClass	BseNullStreamClass;
typedef struct	_BseObject		BseObject;
typedef struct	_BseObjectClass		BseObjectClass;
typedef struct	_BsePattern		BsePattern;
typedef struct	_BsePatternClass	BsePatternClass;
typedef	struct	_BsePcmStream		BsePcmStream;
typedef	struct	_BsePcmStreamClass	BsePcmStreamClass;
typedef struct  _BseProcedureClass      BseProcedureClass;
typedef	struct	_BseProject		BseProject;
typedef	struct	_BseProjectClass	BseProjectClass;
typedef struct	_BseSample		BseSample;
typedef struct	_BseSampleClass		BseSampleClass;
typedef struct	_BseSNet		BseSNet;
typedef struct	_BseSNetClass		BseSNetClass;
typedef struct	_BseSong		BseSong;
typedef struct	_BseSongClass		BseSongClass;
typedef struct	_BseSource		BseSource;
typedef struct	_BseSourceClass		BseSourceClass;
typedef	struct	_BseStream		BseStream;
typedef	struct	_BseStreamClass		BseStreamClass;
typedef struct	_BseSuper		BseSuper;
typedef struct	_BseSuperClass		BseSuperClass;
typedef struct	_BseText		BseText;
typedef struct	_BseTextInterface	BseTextInterface;


/* --- BSE aux structures --- */
typedef struct	_BseCategory		BseCategory;
typedef struct	_BseChunk		BseChunk;
typedef struct	_BseDot			BseDot;
typedef struct	_BseGlobals		BseGlobals;
typedef struct	_BseIcon		BseIcon;
typedef struct	_BseLfo			BseLfo;
typedef struct	_BseParserData		BseParserData;
typedef struct  _BsePixdata             BsePixdata;
typedef struct	_BseMunk		BseMunk;
typedef struct	_BseNote		BseNote;
typedef struct	_BsePcmConfig		BsePcmConfig;
typedef struct	_BsePlugin		BsePlugin;
typedef struct	_BseSampleHashEntry	BseSampleHashEntry;
typedef	struct	_BseSongSequencer	BseSongSequencer;
typedef struct	_BseStorage		BseStorage;
typedef	struct	_BseTypeInfo		BseTypeInfo;
typedef	struct	_BseInterfaceInfo	BseInterfaceInfo;
typedef struct	_BseVoice		BseVoice;
typedef struct	_BseVoiceAllocator	BseVoiceAllocator;
typedef struct	_BseNotifyHook		BseNotifyHook;


/* --- anticipated enums --- */
typedef enum			/* <skip> */
{ /* keep in sync with bsemain.c */
  BSE_DEBUG_TABLES		= (1 << 0),
  BSE_DEBUG_CLASSES		= (1 << 1),
  BSE_DEBUG_OBJECTS		= (1 << 2),
  BSE_DEBUG_NOTIFY		= (1 << 3),
  BSE_DEBUG_PLUGINS		= (1 << 4),
  BSE_DEBUG_REGS		= (1 << 5),
  BSE_DEBUG_CHUNKS		= (1 << 6)
} BseDebugFlags;
typedef enum			/* <skip> */
{
  BSE_TOKEN_UNMATCHED           = G_TOKEN_LAST + 1,
  BSE_TOKEN_NIL			= G_TOKEN_LAST + 2
} BseTokenType;
typedef enum                    /* <skip> */
{
  BSE_PIXDATA_RGB               = 3,
  BSE_PIXDATA_RGBA              = 4,
  BSE_PIXDATA_RGB_MASK          = 0x07,
  BSE_PIXDATA_1BYTE_RLE         = (1 << 3),
  BSE_PIXDATA_ENCODING_MASK     = 0x08
} BsePixdataType;


/* --- anticipated structures --- */
struct _BsePixdata
{
  BsePixdataType type : 8;
  guint          width : 12;
  guint          height : 12;
  const guint8  *encoded_pix_data;
};
struct _BseIcon
{
  guint   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  guint   ref_count;       /* &(1<<31) indicates permanent ref counts */
  guint   width;
  guint   height;
  guint8 *pixels;
};


/* --- anticipated variables --- */
extern BseDebugFlags bse_debug_flags;


/* --- required globals --- */
extern const gchar *bse_log_domain_bse;


/* --- BSE function types --- */
typedef	void		(*BseFunc)		(void);
typedef BseTokenType	(*BseTryStatement)	(gpointer        func_data,
						 BseStorage     *storage,
						 gpointer        user_data);
typedef BseObject*	(*BsePathResolver)	(gpointer        func_data,
						 BseStorage     *storage,
						 BseType         required_type,
						 const gchar    *path);
typedef gboolean	(*BseProcedureShare)	(gpointer	 func_data,
						 const gchar	*proc_name,
						 gfloat		 progress);
typedef gboolean	(*BseCategoryForeach)	(const gchar	*category_path,
						 BseType	 type,
						 gpointer	 user_data);


/* --- notification (object callbacks) --- */
/* notifier signatures, these will get postprocessed to generate
 * bsenotifier_array.c
 */
typedef	void	(*BseNotify_destroy)		(BseObject	*object,
						 gpointer	 data);
typedef	void	(*BseNotify_name_set)		(BseObject	*object,
						 gpointer	 data);
typedef	void	(*BseNotify_param_changed)	(BseObject	*object,
						 BseParamSpec	*pspec,
						 gpointer	 data);
typedef void	(*BseNotify_write_chars)	(BseText	*text,
						 guint		 n_chars,
						 const gchar	*chars,
						 gpointer	 user_data);
typedef	void	(*BseNotify_item_added)		(BseContainer	*container,
						 BseItem	*item,
						 gpointer	 data);
typedef	void	(*BseNotify_item_removed)	(BseContainer	*container,
						 BseItem	*item,
						 gpointer	 data);
typedef void	(*BseNotify_seqid_changed)	(BseItem	*item,
						 gpointer	 data);
typedef void	(*BseNotify_set_container)	(BseItem	*item,
						 BseItem	*container,
						 gpointer	 data);
typedef void	(*BseNotify_size_changed)	(BsePattern	*pattern,
						 gpointer	 data);
typedef void	(*BseNotify_note_changed)	(BsePattern	*pattern,
						 guint		 channel,
						 guint		 row,
						 gpointer	 data);
typedef void	(*BseNotify_note_selected)	(BsePattern	*pattern,
						 guint		 channel,
						 guint		 row,
						 gpointer	 data);
typedef void	(*BseNotify_note_unselected)	(BsePattern	*pattern,
						 guint		 channel,
						 guint		 row,
						 gpointer	 data);
typedef void	(*BseNotify_sequencer_step)	(BseSong	*song,
						 gpointer	 data);






#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_DEFS_H__ */
