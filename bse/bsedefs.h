/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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

#undef   G_DISABLE_ASSERT
#undef   G_DISABLE_CHECKS
#include <libintl.h>
#include <sfi/sfi.h>
#include <sfi/sfistore.h>	// FIXME
#include <sfi/sficomwire.h>	// FIXME
#include <math.h>
#include <bse/bseconfig.h>

G_BEGIN_DECLS


/* --- some globally used macros --- */
#define BSE_VERSION_CMP(v1_major, v1_minor, v1_micro, v2_major, v2_minor, v2_micro) ( \
                                    (v1_major != v2_major) ? (v1_major > v2_major ? +1 : -1) : \
                                    (v1_minor != v2_minor) ? (v1_minor > v2_minor ? +1 : -1) : \
                                    (v1_micro < v2_micro ? -1 : v1_micro > v2_micro))


/* --- BSE objects, classes & interfaces --- */
typedef struct  _BseBinData                BseBinData;
typedef struct  _BseBinDataClass           BseBinDataClass;
typedef struct  _BseBus                    BseBus;
typedef struct  _BseBusClass               BseBusClass;
typedef struct  _BseCapture                BseCapture;
typedef struct  _BseCaptureClass           BseCaptureClass;
typedef struct  _BseContainer              BseContainer;
typedef struct  _BseContainerClass         BseContainerClass;
typedef struct  _BseContextMerger          BseContextMerger;
typedef struct  _BseContextMergerClass     BseContextMergerClass;
typedef struct  _BseCSynth                 BseCSynth;
typedef struct  _BseCSynthClass            BseCSynthClass;
typedef struct  _BseEditableSample         BseEditableSample;
typedef struct  _BseEditableSampleClass    BseEditableSampleClass;
typedef struct  _BseEffect                 BseEffect;
typedef struct  _BseEffectClass            BseEffectClass;
typedef struct  _BseItem                   BseItem;
typedef struct  _BseItemClass              BseItemClass;
typedef struct  _BseJanitor                BseJanitor;
typedef struct  _BseJanitorClass           BseJanitorClass;
typedef struct	_BseMidiDecoder		   BseMidiDecoder;
typedef struct  _BseMidiNotifier           BseMidiNotifier;
typedef struct  _BseMidiNotifierClass      BseMidiNotifierClass;
typedef struct  _BseMidiReceiver           BseMidiReceiver;
typedef struct  _BseMidiSynth              BseMidiSynth;
typedef struct  _BseMidiSynthClass         BseMidiSynthClass;
typedef struct  _BseMidiContext            BseMidiContext;
typedef struct  _BseObject                 BseObject;
typedef struct  _BseObjectClass            BseObjectClass;
typedef struct  _BseParasite		   BseParasite;
typedef struct  _BsePart		   BsePart;
typedef struct  _BsePartClass		   BsePartClass;
typedef struct  _BsePcmWriter              BsePcmWriter;
typedef struct  _BsePcmWriterClass         BsePcmWriterClass;
typedef struct  _BseProcedureClass         BseProcedureClass;
typedef struct  _BseProject                BseProject;
typedef struct  _BseProjectClass           BseProjectClass;
typedef struct  _BseScriptControl          BseScriptControl;
typedef struct  _BseScriptControlClass     BseScriptControlClass;
typedef struct  _BseServer                 BseServer;
typedef struct  _BseServerClass            BseServerClass;
typedef struct  _BseSNet                   BseSNet;
typedef struct  _BseSNetClass              BseSNetClass;
typedef struct  _BseSong                   BseSong;
typedef struct  _BseSongClass              BseSongClass;
typedef struct  _BseSongSequencer          BseSongSequencer;
typedef struct  _BseSource                 BseSource;
typedef struct  _BseSourceClass            BseSourceClass;
typedef struct  _BseStorage                BseStorage;
typedef struct  _BseStorageClass           BseStorageClass;
typedef struct  _BseSubSynth               BseSubSynth;
typedef struct  _BseSubSynthClass          BseSubSynthClass;
typedef struct  _BseSuper                  BseSuper;
typedef struct  _BseSuperClass             BseSuperClass;
typedef struct  _BseTrack                  BseTrack;
typedef struct  _BseTrackClass             BseTrackClass;
typedef struct  _BseTrans                  BseTrans;
typedef struct  _BseUndoStack		   BseUndoStack;
typedef struct  _BseUndoStep               BseUndoStep;
typedef struct  _BseVirtualThroughput      BseVirtualThroughput;
typedef struct  _BseVirtualThroughputClass BseVirtualThroughputClass;
typedef struct  _BseVoice		   BseVoice;
typedef struct  _BseWave                   BseWave;
typedef struct  _BseWaveRepo               BseWaveRepo;
typedef struct  _BseWaveRepoClass          BseWaveRepoClass;

/* --- BseModule special handling --- */
typedef struct  _BseModule                 BseModule;
typedef struct  _BseModuleClass            BseModuleClass;
/* dereference BseModule.user_data without including bseengine.h */
#define	BSE_MODULE_GET_USER_DATA(bsemodule)	(((gpointer*) bsemodule)[1])


/* --- BSE aux structures --- */
typedef struct  _BseExportNode          BseExportNode;
typedef struct  _BseExportNodeBoxed	BseExportNodeBoxed;
typedef struct  _BseGlobals             BseGlobals;
typedef struct  _BsePlugin              BsePlugin;
typedef struct  _BsePluginClass         BsePluginClass;


/* --- BSE function types --- */
typedef void          (*BseFunc)             (void);
typedef void          (*BseFreeFunc)         (gpointer           data);
typedef void          (*BseIOWatch)	     (gpointer		 data,
					      GPollFD		*pfd);
typedef SfiTokenType  (*BseTryStatement)     (gpointer           context_data,
                                              BseStorage        *storage,
                                              GScanner          *scanner,
                                              gpointer           user_data);
typedef BseObject*    (*BseUPathResolver)    (gpointer           func_data,
                                              GType              required_type,
                                              const gchar       *path,
					      gchar	       **error);
typedef gboolean      (*BseProcedureShare)   (gpointer           func_data,
                                              const gchar       *proc_name,
                                              gfloat             progress);
typedef gboolean      (*BseCategoryForeach)  (const gchar       *category_path,
                                              GType              type,
                                              gpointer           user_data);
typedef void          (*BseEngineAccessFunc) (BseModule         *module,
                                              gpointer           data); 
                                         


/* --- i18n and gettext helpers --- */
const gchar* bse_gettext (const gchar *text);
#define _(str)	bse_gettext (str)
#define N_(str) (str)

G_END_DECLS

#endif /* __BSE_DEFS_H__ */
