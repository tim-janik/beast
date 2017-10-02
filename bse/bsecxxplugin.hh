// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CXX_PLUGIN_H__
#define __BSE_CXX_PLUGIN_H__

#include <bse/bsecxxmodule.hh>
#include <bse/bseexports.hh>
#include <bse/bseparam.hh>
#include <bse/bsecategories.hh>
#include <bse/bseplugin.hh>
#include <sfi/sficxx.hh>

namespace Bse {

/* --- frequently used constants --- */
const SfiInt  KAMMER_NOTE = SFI_KAMMER_NOTE;
const SfiInt  KAMMER_OCTAVE = SFI_KAMMER_OCTAVE;
const SfiReal KAMMER_FREQ = BSE_KAMMER_FREQUENCY;
const SfiInt  MIN_FINE_TUNE = BSE_MIN_FINE_TUNE;
const SfiInt  MAX_FINE_TUNE = BSE_MAX_FINE_TUNE;


/* -- export identity --- */
/* provide plugin export identity, preceeding all type exports */
#ifndef BSE_COMPILATION
#define BSE_CXX_DEFINE_EXPORTS()               \
  static ::BseExportIdentity __staticbse_export_identity = BSE_EXPORT_IDENTITY (*(::BseExportNode*) 0);
#define BSE_CXX_EXPORT_IDENTITY    &__staticbse_export_identity
#else   /* BSE internal "Plugins" */
#define BSE_CXX_DEFINE_EXPORTS()
#define BSE_CXX_EXPORT_IDENTITY    &bse_builtin_export_identity
#endif

// == Resident Type Plugin Registration ==
#define BSE_RESIDENT_TYPE_DEF(Object, func, anc, category, blurb, icon) \
  static GType func##_get_type () {                                     \
    static const GTypeInfo type_info = {                                \
      sizeof (Object##Class),                                           \
      (GBaseInitFunc) NULL,                                             \
      (GBaseFinalizeFunc) NULL,                                         \
      (GClassInitFunc) func##_class_init,                               \
      (GClassFinalizeFunc) NULL,                                        \
      NULL /* class_data */,                                            \
      sizeof (Object),                                                  \
      0 /* n_preallocs */,                                              \
      (GInstanceInitFunc) func##_init,                                  \
    };                                                                  \
    static GType type_id = 0;                                           \
    if (!type_id)                                                       \
      {                                                                 \
        type_id = bse_type_register_static (anc,  # Object , blurb, __FILE__, __LINE__, &type_info); \
        if (category)                                                   \
          bse_categories_register_stock_module (category, type_id, icon); \
      }                                                                 \
    return type_id;                                                     \
  }                                                                     \
  static void func##__onload () {                                       \
    bse_plugin_make_resident();                                         \
    (void) (volatile GType) func##_get_type();                          \
  } static Sfi::Init func##__onload_ (func##__onload);
#define BSE_RESIDENT_SOURCE_DEF(Object, func, category, blurb, icon)    \
  BSE_RESIDENT_TYPE_DEF(Object, func, BSE_TYPE_SOURCE, category, blurb, icon)

/* --- hook registration --- */
/* hook registration is based on a static ExportTypeKeeper
 * object, which provides the hook's get_type() implementation and
 * auto-registers the hook's export node with the export_identity.
 */
#define BSE_CXX_REGISTER_HOOK(HookType)         BSE_CXX_REGISTER_HOOK_NODE (HookType, 0)
#define BSE_CXX_REGISTER_STATIC_HOOK(HookType)  BSE_CXX_REGISTER_HOOK_NODE (HookType, 1)
#define BSE_CXX_REGISTER_HOOK_NODE(HookType, __static)                  \
  template<class E> static BseExportNode* bse_export_node ();           \
  template<> inline BseExportNode*                                      \
  bse_export_node<HookType> ()                                          \
  {                                                                     \
    static BseExportNodeHook hnode = {                                  \
      { NULL, BSE_EXPORT_NODE_HOOK, "", },                              \
    };                                                                  \
    static HookType hook_instance;                                      \
    struct Sub {                                                        \
      static void                                                       \
      hook_trampoline (void *data)                                      \
      {                                                                 \
        hook_instance.run();                                            \
      }                                                                 \
    };                                                                  \
    if (!hnode.hook)                                                    \
      {                                                                 \
        hnode.hook = Sub::hook_trampoline;                              \
        hnode.make_static = __static != 0;                              \
      }                                                                 \
    return &hnode.node;                                                 \
  }                                                                     \
  extern ::Bse::ExportTypeKeeper bse_type_keeper__7##HookType;          \
  ::Bse::ExportTypeKeeper                                               \
         bse_type_keeper__7##HookType (bse_export_node<HookType>,       \
                                       BSE_CXX_EXPORT_IDENTITY);

/* --- enum registration --- */
/* enum registration is based on a static ExportTypeKeeper
 * object, which provides the enum's get_type() implementation and
 * auto-registers the enum's export node with the export_identity.
 */
#define BSE_CXX_DECLARED_ENUM_TYPE(NameSpace,EnumType)                  \
  (::NameSpace::bse_type_keeper__3##EnumType.get_type ())
#define BSE_CXX_DECLARE_ENUM(EnumType,EnumName,N,ICode)                 \
  template<class E> static BseExportNode* bse_export_node ();           \
  template<> BseExportNode*                                             \
  bse_export_node<EnumType> ()                                          \
  {                                                                     \
    static BseExportNodeEnum enode = {                                  \
      { NULL, BSE_EXPORT_NODE_ENUM, EnumName, },                        \
    };                                                                  \
    struct Sub {                                                        \
      static GEnumValue*                                                \
      get_enum_values (void)                                            \
      {                                                                 \
        static GEnumValue values[N + 1];                                \
        if (!values[0].value_name) {                                    \
          GEnumValue *v = values;                                       \
          ICode; /* initializes values via *v++ = ...; */               \
          BSE_ASSERT_RETURN (v == values + N, NULL);                    \
          *v++ = ::Bse::EnumValue (0, 0, 0); /* NULL termination */     \
        }                                                               \
        return values;                                                  \
      }                                                                 \
    };                                                                  \
    if (!enode.get_enum_values)                                         \
      {                                                                 \
        enode.get_enum_values = Sub::get_enum_values;                   \
        enode.get_choice_values = EnumType ## _choice_values;           \
      }                                                                 \
    return &enode.node;                                                 \
  }                                                                     \
  extern ::Bse::ExportTypeKeeper bse_type_keeper__3##EnumType;
#define BSE_CXX_REGISTER_ENUM(EnumType)                                 \
  ::Bse::ExportTypeKeeper                                               \
         bse_type_keeper__3##EnumType (bse_export_node<EnumType>,       \
                                       BSE_CXX_EXPORT_IDENTITY);
/* convenience creator to allow easy assignments of GEnumValue structs */
inline const GEnumValue
EnumValue (int         int_value,
           const char *value_name,
           const char *value_nick)
{
  GEnumValue value;
  value.value = int_value;
  value.value_name = const_cast<char*> (value_name);
  value.value_nick = const_cast<char*> (value_nick);
  return value;
}

/* --- record registration --- */
/* record registration is based on a static ExportTypeKeeper
 * object, which provides the record's get_type() implementation and
 * auto-registers the record's export node with the export_identity.
 */
#define BSE_CXX_DECLARED_RECORD_TYPE(NameSpace,RecordType)              \
  (::NameSpace::bse_type_keeper__1##RecordType.get_type ())
#define BSE_CXX_DECLARE_RECORD(RecordType)                              \
  template<class E> static BseExportNode* bse_export_node ();           \
  template<> BseExportNode*                                             \
  bse_export_node<RecordType> ()                                        \
  {                                                                     \
    static BseExportNodeBoxed bnode = {                                 \
      { NULL, BSE_EXPORT_NODE_RECORD, NULL, },                          \
    };                                                                  \
    if (!bnode.node.name) {                                             \
      struct Sub { static void fill_strings (BseExportStrings *es) {    \
        es->blurb = RecordType::blurb();                                \
        es->authors = RecordType::authors();                            \
        es->license = RecordType::license();                            \
      } };                                                              \
      bnode.node.name = RecordType::type_name();                        \
      bnode.node.options = RecordType::options();                       \
      bnode.node.fill_strings = Sub::fill_strings;                      \
      bnode.copy = Sfi::RecordHandle< RecordType >::boxed_copy;         \
      bnode.free = Sfi::RecordHandle< RecordType >::boxed_free;         \
      bnode.seqrec2boxed = ::Sfi::cxx_boxed_from_rec<RecordType>;       \
      bnode.boxed2recseq = ::Sfi::cxx_boxed_to_rec<RecordType>;         \
      bnode.func.get_fields = RecordType::get_fields;                   \
    }                                                                   \
    return &bnode.node;                                                 \
  }                                                                     \
  extern ::Bse::ExportTypeKeeper bse_type_keeper__1##RecordType;
#define BSE_CXX_REGISTER_RECORD(RecordType)                             \
  ::Bse::ExportTypeKeeper                                               \
         bse_type_keeper__1##RecordType (bse_export_node<RecordType>,   \
                                         BSE_CXX_EXPORT_IDENTITY);


/* --- sequence registration --- */
/* sequence registration works similar to record registration */
#define BSE_CXX_DECLARED_SEQUENCE_TYPE(NameSpace,SequenceType)                  \
  (::NameSpace::bse_type_keeper__2##SequenceType.get_type ())
#define BSE_CXX_DECLARE_SEQUENCE(SequenceType)                                  \
  template<class E> static BseExportNode* bse_export_node ();                   \
  template<> BseExportNode*                                                     \
  bse_export_node<SequenceType> ()                                              \
  {                                                                             \
    static BseExportNodeBoxed bnode = {                                         \
      { NULL, BSE_EXPORT_NODE_SEQUENCE, NULL, },                                \
    };                                                                          \
    if (!bnode.node.name) {                                                     \
      struct Sub { static void fill_strings (BseExportStrings *es) {            \
        es->blurb = SequenceType::blurb();                                      \
        es->authors = SequenceType::authors();                                  \
        es->license = SequenceType::license();                                  \
      } };                                                                      \
      bnode.node.name = SequenceType::type_name();                              \
      bnode.node.options = SequenceType::options();                             \
      bnode.node.fill_strings = Sub::fill_strings;                              \
      bnode.copy = SequenceType::boxed_copy;                                    \
      bnode.free = SequenceType::boxed_free;                                    \
      bnode.seqrec2boxed = ::Sfi::cxx_boxed_from_seq<SequenceType>;             \
      bnode.boxed2recseq = ::Sfi::cxx_boxed_to_seq<SequenceType>;               \
      bnode.func.get_element = SequenceType::get_element;                       \
    }                                                                           \
    return &bnode.node;                                                         \
  }                                                                             \
  extern ::Bse::ExportTypeKeeper bse_type_keeper__2##SequenceType;
#define BSE_CXX_REGISTER_SEQUENCE(SequenceType)                                 \
  ::Bse::ExportTypeKeeper                                                       \
         bse_type_keeper__2##SequenceType (bse_export_node<SequenceType>,       \
                                           BSE_CXX_EXPORT_IDENTITY);


/* --- procedure registration --- */
/* procedure registration works similar to enum registration. */
#define BSE_CXX_DECLARED_PROC_TYPE(NameSpace,ProcType)                          \
  (::NameSpace::bse_type_keeper__9##ProcType.get_type ())
#define BSE_CXX_DECLARE_PROC(ProcType)                                          \
  extern ::Bse::ExportTypeKeeper bse_type_keeper__9##ProcType;
#define BSE_CXX_REGISTER_PROCEDURE(ProcType)                                    \
  template<class C> static ::BseExportNode* bse_export_node ();                 \
  template<> ::BseExportNode*                                                   \
  bse_export_node<Procedure::ProcType> ()                                       \
  {                                                                             \
    static ::BseExportNodeProc pnode = {                                        \
      { NULL, ::BSE_EXPORT_NODE_PROC, NULL, },                                  \
      0, Procedure::ProcType::init, Procedure::ProcType::marshal,               \
    };                                                                          \
    if (!pnode.node.name) {                                                     \
      struct Sub { static void fill_strings (BseExportStrings *es) {            \
        es->blurb = Procedure::ProcType::blurb();                               \
        es->authors = Procedure::ProcType::authors();                           \
        es->license = Procedure::ProcType::license();                           \
        es->i18n_category = Procedure::ProcType::i18n_category();               \
      } };                                                                      \
      pnode.node.name = Procedure::ProcType::type_name();                       \
      pnode.node.options = Procedure::ProcType::options();                      \
      pnode.node.category = Procedure::ProcType::category();                    \
      pnode.node.pixstream = Procedure::ProcType::pixstream();                  \
      pnode.node.fill_strings = Sub::fill_strings;                              \
    }                                                                           \
    return &pnode.node;                                                         \
  }                                                                             \
  ::Bse::ExportTypeKeeper                                                       \
         bse_type_keeper__9##ProcType (bse_export_node<Procedure::ProcType>,    \
                                   BSE_CXX_EXPORT_IDENTITY);


/* --- class registration --- */
/* class registration works similar to enum registration.
 * in addition, we need to define a couple trampoline functions to make
 * C++ methods callable, and for effects, we're providing some basic
 * method implementations to interface with the synmthesis Module.
 */
#define BSE_CXX_DECLARED_CLASS_TYPE(NameSpace,ClassType)                        \
  (::NameSpace::bse_type_keeper__0##ClassType.get_type ())
#define BSE_CXX_DECLARE_CLASS(ClassType)                                        \
  extern ::Bse::ExportTypeKeeper bse_type_keeper__0##ClassType;
#define BSE_CXX_REGISTER_EFFECT(Effect)                                         \
  BSE_CXX_DEFINE_STATIC_DATA (Effect##Base);                                    \
  template<class C> static ::BseExportNode* bse_export_node ();                 \
  template<> ::BseExportNode*                                                   \
  bse_export_node<Effect> ()                                                    \
  {                                                                             \
    static ::BseExportNodeClass cnode = {                                       \
      { NULL, ::BSE_EXPORT_NODE_CLASS, NULL, },                                 \
      "BseEffect", BSE_CXX_COMMON_CLASS_SIZE,                                   \
      (GClassInitFunc) static_cast <void (*) (::Bse::CxxBaseClass*)>            \
                                   (::Bse::cxx_class_init_trampoline<Effect>),  \
      NULL,                                                                     \
      BSE_CXX_INSTANCE_OFFSET + sizeof (Effect),                                \
      ::Bse::cxx_instance_init_trampoline<Effect>,                              \
    };                                                                          \
    if (!cnode.node.name) {                                                     \
      struct Sub { static void fill_strings (BseExportStrings *es) {            \
        es->blurb = Effect::blurb();                                            \
        es->authors = Effect::authors();                                        \
        es->license = Effect::license();                                        \
        es->i18n_category = Effect::i18n_category();                            \
      } };                                                                      \
      cnode.node.name = Effect::type_name();                                    \
      cnode.node.options = Effect::options();                                   \
      cnode.node.category = Effect::category();                                 \
      cnode.node.pixstream = Effect::pixstream();                               \
      cnode.node.fill_strings = Sub::fill_strings;                              \
    }                                                                           \
    return &cnode.node;                                                         \
  }                                                                             \
  ::Bse::ExportTypeKeeper                                                       \
         bse_type_keeper__0##Effect (bse_export_node<Effect>,                   \
                                     BSE_CXX_EXPORT_IDENTITY);
/* implement static_data portions used by auto-generated classes */
#define BSE_CXX_DEFINE_STATIC_DATA(ObjectType)                                  \
  ObjectType::StaticData ObjectType::static_data;


/* --- type keeper for export nodes --- */
class ExportTypeKeeper
{
  BseExportNode    *enode;
  BsePlugin        *plugin;
  static BsePlugin* plugin_export_node (const ::BseExportIdentity *plugin_identity,
                                        ::BseExportNode           *enode);
  static void       plugin_cleanup     (BsePlugin                 *plugin,
                                        ::BseExportNode           *enode);
  BSE_CLASS_NON_COPYABLE (ExportTypeKeeper);
public:
  ExportTypeKeeper (::BseExportNode* (*export_node) (),
                    ::BseExportIdentity *export_identity)
  {
    enode = export_node();
    plugin = plugin_export_node (export_identity, enode);
  }
  ~ExportTypeKeeper()
  {
    if (plugin)
      plugin_cleanup (plugin, enode);
  }
  const GType get_type()        { return enode->type; }
};
} // Bse
/* include generated C++ core types */
#include <bse/bsebasics.genidl.hh>        /* includes bsecxxplugin.hh itself */
/* define types dependant on bsebasics.idl */
namespace Bse {
/* --- trampoline templates --- */
template<class ObjectType, typename PropertyID> static void
cxx_get_candidates_trampoline (BseItem               *item,
                               guint                  prop_id,
                               Bse::PropertyCandidates &pc,
                               GParamSpec            *pspec)
{
  CxxBase *cbase = cast (item);
  ObjectType *instance = static_cast<ObjectType*> (cbase);
  if (0)        // check ObjectType::get_candidates() member and prototype
    (void) static_cast<void (ObjectType::*) (PropertyID, ::Bse::PropertyCandidates&, GParamSpec*)> (&ObjectType::get_candidates);
  ::Bse::PropertyCandidates pch;
  instance->get_candidates (static_cast<PropertyID> (prop_id), pch, pspec);
}

} // Bse

#endif /* __BSE_CXX_PLUGIN_H__ */
