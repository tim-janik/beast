// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "serialize.hh"
#include "bse/internal.hh"
#include <sstream>
#include <algorithm>

#include "pugixml.hh"

namespace Bse {

// == Helpers for pugixml ==
typedef pugi::xml_document      XmlDoc;
typedef pugi::xml_node          XmlNode; // A convenient XML node pointer
typedef pugi::xml_attribute     XmlAttr; // A convenient XML attribute pointer
#define APPEND_ATTRIBUTE(xmlnode, attr, ...)                    \
  ({ xmlnode                                                    \
      .append_attribute (Bse::String (attr).c_str())            \
      .set_value (Bse::string_format (__VA_ARGS__).c_str());    \
  })
#define PREPEND_ATTRIBUTE(xmlnode, attr, ...)                    \
  ({ xmlnode                                                    \
      .prepend_attribute (Bse::String (attr).c_str())            \
      .set_value (Bse::string_format (__VA_ARGS__).c_str());    \
  })
#define add_textnode(xmlnode, txtnode, ...)     ({      \
      XmlNode __tmp_node_ = xmlnode.append_child (Bse::String (txtnode).c_str()); \
      const Bse::String __tmp_str_ = Bse::string_format (__VA_ARGS__).c_str(); \
      if (!__tmp_str_.empty())                                          \
        __tmp_node_.append_child (pugi::node_pcdata)                    \
          .set_value (__tmp_str_.c_str());                              \
    })

// == SerializeContext::Private ==
struct SerializeContextCommon {
  XmlDoc                              doc;
  XmlNode                             root;
  int                                 in_load, in_save;
  String                              error;
  std::map<void*, String>             uid_map;
  std::set<String>                    uid_set;
  std::map<String,Aux::ErasedPtr>     refmap;
  std::vector<Aux::ErasedPtr>         trashpointers;
  SerializeContextCommon() :
    in_load (0), in_save (0)
  {}
};
typedef std::shared_ptr<SerializeContextCommon> SerializeContextCommonP;
struct SerializeContext::Private {
  XmlNode                 node;
  SerializeContextCommonP common;
  Private (SerializeContextCommonP c, XmlNode n = XmlNode()) :
    node (n), common (c)
  {}
};

// == SerializeFromXML ==
SerializeFromXML::SerializeFromXML (const std::string &input)
{
  std::stringstream inputstream (input);
  SerializeContextCommonP common = std::make_shared<SerializeContextCommon> ();
  pugi::xml_parse_result result = common->doc.load (inputstream, pugi::parse_default, pugi::encoding_utf8);
  common->root = common->doc.first_child();
  SerializeContext::Private priv (common);
  priv.node = priv.common->root;
  sc_ = FriendAllocator<SerializeContext>::make_shared (priv);
  if (!result)
    {
      const size_t l = 1 + std::count (input.begin(), input.begin() + result.offset, '\n');
      sc_->set_error (string_format ("%u: %s", l, result.description()));
    }
  assert_return (sc_->is_root());
}

SerializeFromXML::~SerializeFromXML()
{}

size_t
SerializeFromXML::copy_all_pointers (VoidPtrVector &vv)
{
  return sc_->copy_all_pointers (vv);
}

// == SerializeToXML ==
SerializeToXML::SerializeToXML (const String &roottag, const String &version)
{
  SerializeContextCommonP common = std::make_shared<SerializeContextCommon> ();
  common->root = common->doc.append_child (String (roottag.empty() ? "data" : roottag).c_str());
  if (!version.empty())
    APPEND_ATTRIBUTE (common->root, "version", "%s", version);
  SerializeContext::Private priv (common);
  priv.node = priv.common->root;
  sc_ = FriendAllocator<SerializeContext>::make_shared (priv);
  assert_return (sc_->is_root());
}

std::string
SerializeToXML::to_xml ()
{
  if (result_.empty())
    {
      std::stringstream serialization_stream;
      PREPEND_ATTRIBUTE (sc_->p_.common->root, "xmlns:xo", "https://testbit.eu/d/xmlns-xml-serialize");
      uint flags = pugi::format_no_declaration | pugi::format_indent | pugi::format_indent_attributes;
      sc_->p_.common->doc.save (serialization_stream, "  ", flags);
      result_ = serialization_stream.str();
      // regex to make simple nodes one-liner
      const std::string pat = "(\\n *)<([^<>\"]+)\\n *([^<>\"=]+)=\"([^<>\"]+)\" *(/?)>",
                        rpl = "$1<$2 $3=\"$4\"$5>";
      result_ = Re::sub (pat, rpl, result_);
    }
  return result_;
}

SerializeToXML::~SerializeToXML ()
{}

// == SerializeContext ==
const char *SerializeContext::attr_main   = "xo:main";
const char *SerializeContext::attr_this   = "xo:this";
const char *SerializeContext::attr_item   = "item";
const char *SerializeContext::attr_link   = "xo:link";
const char *SerializeContext::attr_object = "object";
const char *SerializeContext::attr_record = "record";
const char *SerializeContext::attr_typeid = "xo:typeid";

SerializeContext::SerializeContext (Private &priv) :
  p_ (*new (privatemem_) Private (priv)),
  is_range_ (false)
{
  static_assert (sizeof (Private) <= sizeof (privatemem_), "");
}

SerializeContext::~SerializeContext()
{
  if (is_root())
    assert_return (!in_load() && !in_save());
  p_.~Private();
}

String
SerializeContext::version() const
{
  if (!in_load())
    return "";
  XmlAttr version_attr = p_.common->root.attribute ("version");
  return version_attr ? version_attr.name() : "";
}

bool
SerializeContext::version_after (const std::string &old) const
{
  return version_after (version(), old);
}

bool
SerializeContext::version_after (const std::string &newer, const std::string &older)
{
  return strverscmp (newer.c_str(), older.c_str()) >= 0;
}

String
SerializeContext::debug_name()
{
  return p_.node ? p_.node.name() : "";
}

bool
SerializeContext::is_root () const
{
  return p_.node == p_.common->root;
}

void
SerializeContext::set_error (const String &error)
{
  if (p_.common->error.empty())
    p_.common->error = error;
}

std::string
SerializeContext::error () const
{
  return p_.common->error;
}

bool
SerializeContext::in_error () const
{
  return !error().empty();
}

bool
SerializeContext::in_load () const
{
  return p_.common->in_load;
}

bool
SerializeContext::in_save () const
{
  return p_.common->in_save;
}

void
SerializeContext::enter_load ()
{
  assert_return (!in_save());
  p_.common->in_load++;
}

void
SerializeContext::leave_load ()
{
  assert_return (in_load() && !in_save());
  p_.common->in_load--;
}

void
SerializeContext::enter_save ()
{
  assert_return (!in_load());
  p_.common->in_save++;
}

void
SerializeContext::leave_save ()
{
  assert_return (!in_load() && in_save());
  p_.common->in_save--;
}

void
SerializeContext::refmap_add (const String &refid, const Aux::ErasedPtr &eptr)
{
  auto it = p_.common->refmap.find (refid);
  const bool entry_exists = it != p_.common->refmap.end();
  assert_return (entry_exists == false);
  p_.common->refmap[refid] = eptr;
}

Aux::ErasedPtr
SerializeContext::refmap_fetch (const String &refid)
{
  auto it = p_.common->refmap.find (refid);
  const bool entry_exists = it != p_.common->refmap.end();
  return entry_exists ? it->second : Aux::ErasedPtr();
}

void
SerializeContext::clear_maps()
{
  p_.common->trashpointers.reserve (p_.common->refmap.size());
  for (const auto pair : p_.common->refmap)
    p_.common->trashpointers.push_back (pair.second);
  p_.common->refmap.clear();
  p_.common->uid_map.clear();
  p_.common->uid_set.clear();
  // trashpointers keeps references until destructor
}

size_t
SerializeContext::copy_all_pointers (VoidPtrVector &vv)
{
  const size_t l = vv.size();
  vv.reserve (vv.size() + p_.common->refmap.size() + p_.common->trashpointers.size());
  for (const auto pair : p_.common->refmap)
    vv.push_back (pair.second);
  for (auto p : p_.common->trashpointers)
    vv.push_back (p);
  return vv.size() - l;
}

String
SerializeContext::find_uid (void *address, const std::type_info &type, bool isroot, bool *unsaved)
{
  String uid = p_.common->uid_map[address];
  if (uid.empty() && isroot)
    {
      uid = attr_main;
      const bool main_saved = p_.common->uid_set.count (uid) != 0;
      assert_return (main_saved == false, uid); // save() can only be used once
      p_.common->uid_map[address] = uid;
      p_.common->uid_set.insert (uid);
      *unsaved = true;
    }
  else if (uid.empty())
    {
      const String tname = Aida::string_demangle_cxx (type.name());
      const char *part = strrchr (tname.c_str(), ':');
      part = part ? part + 1 : tname.c_str();
      const String name = string_tolower (part);
      for (size_t i = 1; ; i++)
        {
          uid = string_format ("%s%u", name, i);
          if (p_.common->uid_set.count (uid) == 0)
            {
              p_.common->uid_map[address] = uid;
              p_.common->uid_set.insert (uid);
              *unsaved = true;
              break;
            }
        }
    }
  else
    *unsaved = false;
  return uid;
}

SerializeContextP
SerializeContext::find_nested_by_uid (const String &objectid)
{
  SerializeContextP nested;
  assert_return (in_load(), nested);
  assert_return (objectid.empty() == false, nested);
  const char hint = 'S';
  for (XmlNode child = p_.common->root.first_child(); child; child = child.next_sibling())
    {
      XmlAttr attr = child.attribute (attr_this);
      if (!attr.empty() && objectid == attr.value())
        {
          Private priv (p_);
          priv.node = child;
          nested = FriendAllocator<SerializeContext>::make_shared (priv);
          nested->is_range_ = hint == 'R';
          break;
        }
    }
  return nested;
}

SerializeContextP
SerializeContext::load_nested (const String &attribute, char hint)
{
  SerializeContextP nested;
  assert_return (in_load(), nested);
  assert_return (attribute.empty() == false, nested);
  XmlNode child = p_.node.child (attribute.c_str());
  if (!child.empty())
    {
      Private priv (p_);
      priv.node = child;
      nested = FriendAllocator<SerializeContext>::make_shared (priv);
      nested->is_range_ = hint == 'S';  // Sequence
    }
  return nested;
}

SerializeContextP
SerializeContext::save_nested (const String &attribute, char hint)
{
  // Hints:
  // R - Record
  // S - Sequence
  // O - Object
  // ^ - Root Object
  assert_return (in_save(), NULL);
  XmlNode parent = hint == '^' ? p_.common->root : p_.node;
  Private priv (p_);
  priv.node = parent.append_child (attribute.c_str());
  SerializeContextP nested = FriendAllocator<SerializeContext>::make_shared (priv);
  nested->is_range_ = hint == 'S';
  return nested;
}

std::string
SerializeContext::first_child_name()
{
  return p_.node.first_child().name();
}

size_t
SerializeContext::count_children()
{
  size_t i = 0;
  for (XmlNode child = p_.node.first_child(); child; child = child.next_sibling())
    i += 1;
  return i;
}

void
SerializeContext::rotate_children()
{
  p_.node.append_move (p_.node.first_child());
}

void
SerializeContext::save_comment (const String &comment)
{
  assert_return (in_save());
  p_.node.append_child (pugi::node_comment).set_value (comment.c_str());
}

static String
find_attribute_child (XmlNode node, const String &key, bool *found)
{
  String value;
  // retrieve the value of an attribute named 'key'
  XmlAttr attr = node.attribute (key.c_str());
  if (!attr.empty())
    value = attr.value();
  else
    {
      // fall back to retrieving the text of a child named 'key'
      XmlNode child = node.child (key.c_str());
      if (!child.empty())
        value = child.text().get();
      else
        {
          if (found)
            *found = false;
          return "";
        }
    }
  if (found)
    *found = true;
  return value;
}

static bool
is_simple_string (const std::string &s)
{
  if (s.size() > 80)
    return false;
  for (auto c : s)
    if (c == '<' || c == '&' || c == '>' || c == '"' ||
        unicode_is_noncharacter (c) ||
        unicode_is_control_code (c) ||
        unicode_is_private (c))
      return false;
  return true;
}

int64
SerializeContext::load_integral (const String &attribute, char hint, bool *found)
{
  assert_return (in_load(), 0);
  assert_return (attribute.empty() == false, 0);
  const String value = find_attribute_child (p_.node, attribute, found);
  int64 i64;
  switch (hint)
    {
    case 'B':   i64 = string_to_bool (value);           break;
    case 'U':   i64 = string_to_uint (value);           break;
    default:    i64 = string_to_int (value);            break;
    }
  return i64;
}

void
SerializeContext::save_integral (const String &attribute, int64 val, char hint)
{
  assert_return (in_save());
  assert_return (attribute.empty() == false);
  String string;
  switch (hint)
    {
    case 'B':   string = val ? "true" : "false";                break;
    case 'U':   string = string_format ("%u", val);             break;
    case 'E': // fallthrough
    default:    string = string_format ("%d", val);             break;
    }
  if (!is_range_ && is_simple_string (string))
    APPEND_ATTRIBUTE (p_.node, attribute, "%s", string);
  else
    add_textnode (p_.node, attribute, "%s", string);
}

long double
SerializeContext::load_float (const String &attribute, char hint, bool *found)
{
  assert_return (in_load(), 0);
  assert_return (attribute.empty() == false, 0);
  const String value = find_attribute_child (p_.node, attribute, found);
  return string_to_double (value);
}

void
SerializeContext::save_float (const String &attribute, long double val, char hint)
{
  assert_return (in_save());
  assert_return (attribute.empty() == false);
  String string;
  switch (hint)
    {
      // FLOAT:  "%1.8e"  or "%.9g"
      // DOUBLE: "%1.16e" or "%.17g"
    default:
      string = string_format ("%.17g", val);
      break;
    }
  if (!is_range_ && is_simple_string (string))
    APPEND_ATTRIBUTE (p_.node, attribute, "%s", string);
  else
    add_textnode (p_.node, attribute, "%s", string);
}

std::string
SerializeContext::load_string (const String &attribute, bool *found)
{
  assert_return (in_load(), "");
  assert_return (attribute.empty() == false, "");
  const String value = find_attribute_child (p_.node, attribute, found);
  return value;
}

void
SerializeContext::save_string (const String &attribute, const std::string &val, char hint)
{
  assert_return (in_save());
  assert_return (attribute.empty() == false);
  switch (is_range_ ? 0 : hint)
    {
    case '^':
      PREPEND_ATTRIBUTE (p_.node, attribute, "%s", val);
      break;
    default:
      if (!is_range_ && is_simple_string (val))
        APPEND_ATTRIBUTE (p_.node, attribute, "%s", val);
      else
        add_textnode (p_.node, attribute, "%s", val);
      break;
    }
}

// == SerializeContext::Factory ==
SerializeContext::Factory *SerializeContext::Factory::factories_ = NULL;

SerializeContext::Factory::Factory (const std::type_info &typeinfo) :
  next_ (NULL), typeinfo_ (typeinfo), factory_type_ (canonify_mangled (typeinfo_.name()))
{
  next_ = factories_;
  factories_ = this;
}

SerializeContext::Factory::~Factory()
{
  for (Factory **f = &factories_; *f; f = &(*f)->next_)
    if (*f == this)
      {
        *f = this->next_;
        this->next_ = NULL;
        break;
      }
}

SerializeContext::Factory*
SerializeContext::Factory::find_factory (const std::string &factorytype)
{
  for (Factory *f = factories_; f; f = f->next_)
    if (f->factory_type_ == factorytype)
      return f;
  return NULL;
}

SerializeContext::Factory*
SerializeContext::Factory::find_factory (const std::type_info &typeinfo)
{
  for (Factory *f = factories_; f; f = f->next_)
    if (f->typeinfo_ == typeinfo)
      return f;
  return NULL;
}

SerializeContext::Factory*
SerializeContext::Factory::lookup_factory (const std::type_info &typeinfo)
{
  Factory *factory = find_factory (typeinfo);
  if (!factory)
    warning ("SerializeContext: unexported type, need: BSE_SERIALIZATION_EXPORT (%s);", Aida::string_demangle_cxx (typeinfo.name()));
  return factory;
}

String
SerializeContext::Factory::canonify_mangled (const char *mangled)
{
  return Aida::string_demangle_cxx (mangled);
}

// == SerializableBase ==
SerializableBase::SerializableBase ()
{}

SerializableBase::~SerializableBase ()
{}

} // Bse
