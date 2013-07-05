// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_DEFS_H__
#define __BSE_DEFS_H__

#undef   G_DISABLE_ASSERT
#undef   G_DISABLE_CHECKS
#include <libintl.h>
#include <sfi/sfi.hh>
#include <sfi/sfistore.hh>	// FIXME
#include <sfi/sficomwire.hh>	// FIXME
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
struct BseBus;
struct BseBusClass;
typedef struct  _BseCapture                BseCapture;
typedef struct  _BseCaptureClass           BseCaptureClass;
struct BseContainer;
struct BseContainerClass;
struct BseContextMerger;
struct BseContextMergerClass;
struct BseCSynth;
struct BseCSynthClass;
struct BseEditableSample;
struct BseEditableSampleClass;
struct BseItem;
struct BseItemClass;
struct BseJanitor;
struct BseJanitorClass;
struct BseMidiDecoder;
struct BseMidiNotifier;
struct BseMidiNotifierClass;
struct BseMidiReceiver;
struct BseMidiSynth;
struct BseMidiSynthClass;
struct BseMidiContext;
struct BseObject;
struct BseObjectClass;
struct BseParasite;
struct BsePart;
struct BsePartClass;
struct BsePcmWriter;
struct BsePcmWriterClass;
typedef struct  _BseProcedureClass         BseProcedureClass;
struct BseProject;
struct BseProjectClass;
typedef struct  _BseScriptControl          BseScriptControl;
typedef struct  _BseScriptControlClass     BseScriptControlClass;
struct BseServer;
struct BseServerClass;
struct BseSNet;
struct BseSNetClass;
struct BseSong;
struct BseSongClass;
typedef struct  _BseSongSequencer          BseSongSequencer;
struct BseSource;
struct BseSourceClass;
struct BseStorage;
struct BseStorageClass;
struct BseSubSynth;
struct BseSubSynthClass;
struct BseSuper;
struct BseSuperClass;
struct BseTrack;
struct BseTrackClass;
typedef struct  _BseTrans                  BseTrans;
typedef struct  _BseUndoStack		   BseUndoStack;
typedef struct  _BseUndoStep               BseUndoStep;
typedef struct  _BseVirtualThroughput      BseVirtualThroughput;
typedef struct  _BseVirtualThroughputClass BseVirtualThroughputClass;
typedef struct  _BseVoice		   BseVoice;
struct BseWave;
struct BseWaveRepo;
struct BseWaveRepoClass;
/* --- BseModule special handling --- */
typedef struct _BseModule                  BseModule;
typedef struct _BseModuleClass             BseModuleClass;
typedef struct _BseIStream                 BseIStream;
typedef struct _BseJStream                 BseJStream;
typedef struct _BseOStream                 BseOStream;
/* dereference some BseModule members without including bseengine.hh */
#define	BSE_MODULE_GET_USER_DATA(bsemodule)	(((gpointer*) bsemodule)[1])
#define	BSE_MODULE_GET_ISTREAMSP(bsemodule)	(((gpointer*) bsemodule)[2])
#define	BSE_MODULE_GET_JSTREAMSP(bsemodule)	(((gpointer*) bsemodule)[3])
#define	BSE_MODULE_GET_OSTREAMSP(bsemodule)	(((gpointer*) bsemodule)[4])
/* --- Bse Loader --- */
struct BseLoader;
typedef struct _BseWaveDsc              BseWaveDsc;
typedef struct _BseWaveChunkDsc         BseWaveChunkDsc;
/* --- BSE aux structures --- */
typedef struct  _BseExportNode          BseExportNode;
typedef struct  _BseExportNodeBoxed	BseExportNodeBoxed;
typedef struct  _BseGlobals             BseGlobals;
struct BsePlugin;
struct BsePluginClass;
/* --- BSE function types --- */
typedef void          (*BseFunc)             (void);
typedef void          (*BseFreeFunc)         (gpointer           data);
typedef gboolean      (*BseIOWatch)	     (gpointer		 data,
                                              guint              n_pfds,
					      GPollFD		*pfd);
typedef GTokenType    (*BseTryStatement)     (gpointer           context_data,
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
