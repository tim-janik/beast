// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_DEFS_H__
#define __BSE_DEFS_H__

#undef   G_DISABLE_ASSERT
#undef   G_DISABLE_CHECKS
#include <bse/sfi.hh>
#include <bse/sfistore.hh>	// FIXME


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
struct BseMidiDecoder;
struct BseMidiNotifier;
struct BseMidiNotifierClass;
struct BseMidiReceiver;
struct BseMidiSynth;
struct BseMidiSynthClass;
struct BseMidiContext;
struct BseObject;
struct BseObjectClass;
struct BsePart;
struct BsePartClass;
struct BsePcmWriter;
struct BsePcmWriterClass;
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
struct BseSoundFont;
struct BseSoundFontClass;
struct BseSoundFontPreset;
struct BseSoundFontPresetClass;
struct BseSoundFontRepo;
struct BseSoundFontRepoClass;
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
typedef struct  _BseUndoStack		   BseUndoStack;
typedef struct  _BseUndoStep               BseUndoStep;
typedef struct  _BseVirtualThroughput      BseVirtualThroughput;
typedef struct  _BseVirtualThroughputClass BseVirtualThroughputClass;
typedef struct  _BseVoice		   BseVoice;
struct BseWave;
struct BseWaveRepo;
struct BseWaveRepoClass;
// == Bse Engine Module Types ==
struct BseModuleClass;
namespace Bse {
class Module;
struct IStream;
struct JStream;
struct OStream;
struct Job;
struct Trans;
} // Bse
typedef Bse::Module  BseModule;
typedef Bse::JStream BseJStream;
typedef Bse::IStream BseIStream;
typedef Bse::OStream BseOStream;
typedef Bse::Job     BseJob;
typedef Bse::Trans   BseTrans;
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
typedef gboolean      (*BseCategoryForeach)  (const gchar       *category_path,
                                              GType              type,
                                              gpointer           user_data);
typedef void          (*BseEngineAccessFunc) (BseModule         *module,
                                              gpointer           data);

namespace Bse {

class ObjectImpl;
class ItemImpl;
class SourceImpl;
class ContainerImpl;
class SuperImpl;
class PartImpl;
class SNetImpl;
class ProjectImpl;
class ServerImpl;

class ResourceCrawlerImpl;
using ResourceCrawlerImplP = std::shared_ptr<ResourceCrawlerImpl>;

namespace AudioSignal {
struct AudioTiming;
class Engine;
class Processor;
using ProcessorP = std::shared_ptr<Processor>;
} // AudioSignal

} // Bse

#endif /* __BSE_DEFS_H__ */
