/* BSE - Bedevilled Sound Engine                        -*-mode: c++;-*-
 * Copyright (C) 2003 Tim Janik
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
 */
#ifndef __BSE_CXX_PLUGIN_H__
#define __BSE_CXX_PLUGIN_H__

/* default plugin name specification (if omitted in plugin) */
#ifndef BSE_PLUGIN_NAME
#  ifdef BSE_PLUGIN_FALLBACK
#    define BSE_PLUGIN_NAME BSE_PLUGIN_FALLBACK
#  else /* !BSE_PLUGIN_NAME && !BSE_PLUGIN_FALLBACK */
#    define BSE_PLUGIN_NAME __FILE__
#  endif /* !BSE_PLUGIN_NAME && !BSE_PLUGIN_FALLBACK */
#endif /* !BSE_PLUGIN_NAME */

#include <bse/bsecxxmodule.h>
#include <bse/bseexports.h>
#include <bse/bseparam.h>

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
#define BSE_CXX_DEFINE_EXPORTS()                                                \
  static ::BseExportNode __export_chain_head = { NULL, BSE_EXPORT_NODE_LINK, }; \
  static ::BseExportIdentity __export_identity =                                \
                    BSE_EXPORT_IDENTITY (BSE_PLUGIN_NAME, __export_chain_head); \
  extern "C" {                                                                  \
    extern ::BseExportIdentity *const BSE_EXPORT_IDENTITY_SYMBOL;               \
    ::BseExportIdentity *const BSE_EXPORT_IDENTITY_SYMBOL = &__export_identity; \
  }
#define BSE_CXX_EXPORT_IDENTITY    __export_identity
#else   /* BSE internal "Plugins" */
#define BSE_CXX_DEFINE_EXPORTS()
#define BSE_CXX_EXPORT_IDENTITY    bse_builtin_export_identity
extern "C" {
extern ::BseExportIdentity bse_builtin_export_identity; /* sync with bseplugin.h */
};
#endif


/* --- enum registration --- */
/* enum registration is based on a static ExportTypeKeeper
 * object, which provides the enum's get_type() implementation and
 * auto-registers the enum's export node with the export_identity.
 */
#define BSE_CXX_DECLARED_ENUM_TYPE(NameSpace,EnumType)                  \
  (::NameSpace::bse_type_keeper__3##EnumType.get_type ())
#define BSE_CXX_DECLARE_ENUM(EnumType,EnumName,N,ICode)                 \
  template<class E> static BseExportNode* bse_export_node ();           \
  template<> static BseExportNode*                                      \
  bse_export_node<EnumType> ()                                          \
  {                                                                     \
    static BseExportNodeEnum enode = {                                  \
      { NULL, BSE_EXPORT_NODE_ENUM, EnumName, },                        \
    };                                                                  \
    struct Sub {                                                        \
      static GEnumValue* get_values (void)                              \
      {                                                                 \
        static GEnumValue values[N + 1];                                \
        if (!values[0].value_name) {                                    \
          GEnumValue *v = values;                                       \
          ICode; /* initializes values via *v++ = ...; */               \
          g_assert (v == values + N);                                   \
          *v++ = ::Bse::EnumValue (0, 0, 0); /* NULL termination */     \
        }                                                               \
        return values;                                                  \
      }                                                                 \
    };                                                                  \
    if (!enode.get_values)                                              \
      enode.get_values = Sub::get_values;                               \
    return &enode.node;                                                 \
  }                                                                     \
  extern ::Bse::ExportTypeKeeper bse_type_keeper__3##EnumType;
#define BSE_CXX_REGISTER_ENUM(EnumType)                                 \
  ::Bse::ExportTypeKeeper                                               \
         bse_type_keeper__3##EnumType (bse_export_node<EnumType>,       \
                                       &BSE_CXX_EXPORT_IDENTITY);
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
  template<> static BseExportNode*                                      \
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
                                         &BSE_CXX_EXPORT_IDENTITY);


/* --- sequence registration --- */
/* sequence registration works similar to record registration */
#define BSE_CXX_DECLARED_SEQUENCE_TYPE(NameSpace,SequenceType)                  \
  (::NameSpace::bse_type_keeper__2##SequenceType.get_type ())
#define BSE_CXX_DECLARE_SEQUENCE(SequenceType)                                  \
  template<class E> static BseExportNode* bse_export_node ();                   \
  template<> static BseExportNode*                                              \
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
                                           &BSE_CXX_EXPORT_IDENTITY);


/* --- procedure registration --- */
/* procedure registration works similar to enum registration. */
#define BSE_CXX_DECLARED_PROC_TYPE(NameSpace,ProcType)                          \
  (::NameSpace::bse_type_keeper__9##ProcType.get_type ())
#define BSE_CXX_DECLARE_PROC(ProcType)                                          \
  extern ::Bse::ExportTypeKeeper bse_type_keeper__9##ProcType;
#define BSE_CXX_REGISTER_PROCEDURE(ProcType)                                    \
  template<class C> static ::BseExportNode* bse_export_node ();                 \
  template<> static ::BseExportNode*                                            \
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
                                   &BSE_CXX_EXPORT_IDENTITY);


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
  template<> static ::BseExportNode*                                            \
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
                                     &BSE_CXX_EXPORT_IDENTITY);
/* implement static_data portions used by auto-generated classes */
#define BSE_CXX_DEFINE_STATIC_DATA(ObjectType)                                  \
  ObjectType::StaticData ObjectType::static_data;


/* --- type keeper for export nodes --- */
class ExportTypeKeeper
{
  BseExportNode    *enode;
  explicit          ExportTypeKeeper (const ExportTypeKeeper&);
  ExportTypeKeeper& operator=        (const ExportTypeKeeper&);
public:
  explicit          ExportTypeKeeper (::BseExportNode*   (*export_node) (),
                                      ::BseExportIdentity *export_identity)
  {
    enode = export_node ();
    enode->next = export_identity->export_chain;
    export_identity->export_chain = enode;
  }
  const GType get_type()
  {
    return enode->type;
  }
};

} // Bse

/* include generated C++ core types */
#include <bse/bsecore.gen-idl.h>        /* includes bsecxxplugin.h itself */

/* define types dependant on bsecore.idl */
namespace Bse {

/* --- trampoline templates --- */
template<class ObjectType, typename PropertyID> static void
cxx_get_candidates_trampoline (BseItem               *item,
                               guint                  prop_id,
                               BsePropertyCandidates *pc,
                               GParamSpec            *pspec)
{
  CxxBase *cbase = cast (item);
  ObjectType *instance = static_cast<ObjectType*> (cbase);
  if (0)        // check ObjectType::get_candidates() member and prototype
    (void) static_cast<void (ObjectType::*) (PropertyID, ::Bse::PropertyCandidatesHandle&, GParamSpec*)> (&ObjectType::get_candidates);
  ::Bse::PropertyCandidatesHandle pch (::Sfi::INIT_NULL);
  ::Bse::PropertyCandidates *cxxpc = (::Bse::PropertyCandidates*) pc;
  if (cxxpc)
    pch.take (cxxpc);   /* take as pointer, not via CopyConstructor */
  instance->get_candidates (static_cast<PropertyID> (prop_id), pch, pspec);
  if (cxxpc)
    pch.steal();        /* steal to avoid destruction */
}

} // Bse

#endif /* __BSE_CXX_PLUGIN_H__ */
