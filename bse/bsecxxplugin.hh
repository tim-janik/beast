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

#define BSE_CXX_REGISTER_EFFECT(NameSpace, Effect)                              \
  BSE_CXX_DEFINE_INSTANCE_INIT (Effect);                                        \
  BSE_CXX_DEFINE_SET_PROPERTY (Effect);                                         \
  BSE_CXX_DEFINE_GET_PROPERTY (Effect);                                         \
  BSE_CXX_DEFINE_CREATE_MODULE (Effect);                                        \
  BSE_CXX_DEFINE_MODULE_CONFIGURATOR (Effect);                                  \
  BSE_CXX_DEFINE_CLASS_INIT (Effect, BSE_CXX_SYM(Effect,set_property),          \
                                     BSE_CXX_SYM(Effect,get_property));         \
  ::BseExportNodeClass Effect::export_node = {                                  \
    { NULL, ::BSE_EXPORT_NODE_CLASS, #NameSpace #Effect, },                     \
    "BseEffect", BSE_CXX_COMMON_CLASS_SIZE,                                     \
    (GClassInitFunc) BSE_CXX_SYM (Effect, class_init), NULL,                    \
    BSE_CXX_INSTANCE_OFFSET + sizeof (Effect),                                  \
    BSE_CXX_SYM (Effect, instance_init),                                        \
  };  Effect::TypeInit __init_ ## Effect (&__export_identity)

#define BSE_CXX_DEFINE_EXPORTS()                                                \
  static ::BseExportNode __export_chain_head = { NULL, BSE_EXPORT_NODE_LINK, }; \
  static ::BseExportIdentity __export_identity =                                \
                    BSE_EXPORT_IDENTITY (BSE_PLUGIN_NAME, __export_chain_head); \
  extern "C" {                                                                  \
    extern ::BseExportIdentity *const BSE_EXPORT_IDENTITY_SYMBOL;               \
    ::BseExportIdentity *const BSE_EXPORT_IDENTITY_SYMBOL = &__export_identity; \
  }

#define BSE_CXX_DEFINE_CREATE_MODULE(ObjectType)                                \
  Bse::Module*                                                                  \
  ObjectType::create_module (unsigned int context_handle, GslTrans *trans)      \
  { /* create a synthesis module */                                             \
    return new ObjectType ## Module();                                          \
  }
#define BSE_CXX_DEFINE_MODULE_CONFIGURATOR(ObjectType)                          \
Bse::Module::Accessor*                                                          \
ObjectType::module_configurator()                                               \
{                                                                               \
  return Module::accessor (&ObjectType ## Module::config, Parameters (this));   \
}

struct EnumValue {
  gint   value;
  gchar *value_name;
  gchar *value_nick;
  EnumValue (gint v, gchar *vna, gchar *vni)
    : value (v), value_name (vna), value_nick (vni)
  {
  }
  operator GEnumValue ()
  {
    GEnumValue v;
    v.value = value;
    v.value_name = value_name;
    v.value_nick = value_nick;
    return v;
  }
};
#define BSE_CXX_ENUM_TYPE_KEEPER(EType,EName,N,ICode) struct EType ## __TypeKeeper \
{                                                                       \
  static void enter_type_chain (BseExportIdentity *export_identity)     \
  {                                                                     \
    static ::BseExportNodeEnum en = {                                   \
      { NULL, BSE_EXPORT_NODE_ENUM, EName, },                           \
    };                                                                  \
    if (!en.node.next) {                                                \
      static GEnumValue values[N];                                      \
      GEnumValue *v = values;                                           \
      ICode; /* initializes values via *v++ = ...; */                   \
      en.node.next = export_identity->type_chain;                       \
      export_identity->type_chain = &en.node;                           \
    }                                                                   \
  }                                                                     \
  static const GType get_type()                                         \
  {                                                                     \
    static GType t = 0;                                                 \
    if (!t)                                                             \
      t = g_type_from_name (EName);                                     \
    return t;                                                           \
  }                                                                     \
}


} // Bse

#endif /* __BSE_CXX_PLUGIN_H__ */
