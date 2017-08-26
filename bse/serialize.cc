// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "serialize.hh"
#include <sstream>
#include <algorithm>

#include "pugixml.hh"

namespace Bse {

// == Helpers for pugixml ==
typedef pugi::xml_document      XmlDoc;
typedef pugi::xml_node          XmlNode; // A convenient XML node pointer
typedef pugi::xml_attribute     XmlAttr; // A convenient XML attribute pointer
#define add_attribute(xmlnode, attr, ...)                       \
  ({ xmlnode                                                    \
      .append_attribute (Bse::String (attr).c_str())            \
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
SerializeFromXML::SerializeFromXML (std::istream &is)
{
  const std::string xmlstring (std::istreambuf_iterator<char> (is), {});
  std::stringstream inputstream (xmlstring);
  SerializeContextCommonP common = std::make_shared<SerializeContextCommon> ();
  pugi::xml_parse_result result = common->doc.load (inputstream, pugi::parse_default, pugi::encoding_utf8);
  common->root = common->doc.first_child();
  SerializeContext::Private priv (common);
  priv.node = priv.common->root;
  sc_ = FriendAllocator<SerializeContext>::make_shared (priv);
  if (!result)
    {
      const size_t l = 1 + std::count (xmlstring.begin(), xmlstring.begin() + result.offset, '\n');
      sc_->set_error (string_format ("%u: %s", l, result.description()));
    }
  assert_return (sc_->is_root());
}

SerializeFromXML::~SerializeFromXML ()
{}

void
SerializeFromXML::disown_all_pointers ()
{
  sc_->disown_all_pointers();
}

void
SerializeFromXML::delete_all_pointers ()
{
  sc_->delete_all_pointers();
}

// == SerializeToXML ==
SerializeToXML::SerializeToXML (std::ostream &os, const String &roottag, const String &version) :
  os_ (os)
{
  SerializeContextCommonP common = std::make_shared<SerializeContextCommon> ();
  common->root = common->doc.append_child (String (roottag.empty() ? "data" : roottag).c_str());
  if (!version.empty())
    add_attribute (common->root, "version", "%s", version);
  SerializeContext::Private priv (common);
  priv.node = priv.common->root;
  sc_ = FriendAllocator<SerializeContext>::make_shared (priv);
  assert_return (sc_->is_root());
}

bool
SerializeToXML::flush ()
{
  if (!flushed_)
    {
      sc_->p_.common->doc.save (os_, "  ", pugi::format_indent); // pugi::format_indent_attributes
      flushed_ = true;
    }
  return !in_error();
}

SerializeToXML::~SerializeToXML ()
{
  flush();
}

// == SerializeContext ==
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
}

void
SerializeContext::disown_pointer (void *pointer)
{
  if (!in_load() || !pointer)
    return;     // ignore in_save() silently to simplify user code
  for (const auto pair : p_.common->refmap)
    if (pair.second.holds (pointer))
      {
        if (!pair.second.disarm_deleter())
          Bse::warning ("SerializeContext.disown_pointer: shared_ptr is not deletable: %p", pointer);
        return;
      }
  Bse::warning ("SerializeContext.disown_pointer: attempt to disown unknown pointer: %p", pointer);
}

void
SerializeContext::disown_all_pointers()
{
  assert_return (!in_load() && !in_save()); // can't work while the refmap is still being build/used
  for (auto p : p_.common->trashpointers)
    p.disarm_deleter();
}

void
SerializeContext::delete_all_pointers()
{
  assert_return (!in_load() && !in_save()); // can't work while the refmap is still being build/used
  p_.common->trashpointers.clear();
}

String
SerializeContext::find_uid (void *address, const std::type_info &type, bool isroot, bool *unsaved)
{
  String uid = p_.common->uid_map[address];
  if (uid.empty() && isroot)
    {
      uid = sc_main;
      const bool main_saved = p_.common->uid_set.count (uid) != 0;
      assert_return (main_saved == false, uid); // save() can only be used once
      p_.common->uid_map[address] = uid;
      p_.common->uid_set.insert (uid);
      *unsaved = true;
    }
  else if (uid.empty())
    {
      const String tname = cxx_demangle (type.name());
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
      XmlAttr attr = child.attribute (sc_uid.c_str());
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
      nested->is_range_ = hint == 'R';
    }
  return nested;
}

SerializeContextP
SerializeContext::save_nested (const String &attribute, char hint)
{
  assert_return (in_save(), NULL);
  XmlNode parent = hint == 'S' ? p_.common->root : p_.node;
  Private priv (p_);
  priv.node = parent.append_child (attribute.c_str());
  SerializeContextP nested = FriendAllocator<SerializeContext>::make_shared (priv);
  nested->is_range_ = hint == 'R';
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
  if (s.size() > 20)
    return false;
  for (auto c : s)
    if (strchr ("<&>\"", c) || c < ' ' || c > '~')
      return false;
  return true;
}

static bool
short_attribute_row (XmlNode node, int64 maxlen = 90)
{
  if (maxlen < 0)
    return false;
  maxlen -= strlen (node.name()) + 1;
  if (maxlen < 0)
    return false;
  for (pugi::xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute())
    {
      maxlen -= strlen (attr.name()) + 1;
      if (maxlen < 0)
        return false;
      maxlen -= strlen (attr.value()) + 1;
      if (maxlen < 0)
        return false;
    }
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
    case 'E':   string = string_format ("0x%02x", val);         break;
    default:    string = string_format ("%d", val);             break;
    }
  if (!is_range_ && is_simple_string (string) && short_attribute_row (p_.node))
    add_attribute (p_.node, attribute, "%s", string);
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
    default:    string = string_format ("%e", val);             break;
    }
  if (!is_range_ && is_simple_string (string) && short_attribute_row (p_.node))
    add_attribute (p_.node, attribute, "%s", string);
  else
    add_textnode (p_.node, attribute, "%s", string);
}

std::string
SerializeContext::load_string (const String &attribute, char hint, bool *found)
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
    case '-':
      add_attribute (p_.node, attribute, "%s", val);
      break;
    default:
      if (is_simple_string (val) && short_attribute_row (p_.node))
        add_attribute (p_.node, attribute, "%s", val);
      else
        add_textnode (p_.node, attribute, "%s", val);
      break;
    }
}

// == SerializeContext::Factory ==
SerializeContext::Factory *SerializeContext::Factory::factories_ = NULL;

SerializeContext::Factory::Factory (Factory *register_factory) :
  next_ (NULL)
{
  if (register_factory)
    {
      assert_return (register_factory == this);
      next_ = factories_;
      factories_ = this;
    }
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
SerializeContext::Factory::find_name (const std::string &type_name)
{
  for (Factory *f = factories_; f; f = f->next_)
    {
      Factory *factory = f->match_name (type_name);
      if (factory)
        return factory;
    }
  return NULL;
}

std::string
SerializeContext::Factory::find_type (const std::type_info &target, bool mandatory)
{
  for (Factory *f = factories_; f; f = f->next_)
    {
      const std::string type_name = f->match_type (target);
      if (!type_name.empty())
        return type_name;
    }
  if (mandatory)
    warning ("SerializeContext: unexported type, need: BSE_SERIALIZATION_EXPORT (%s);", cxx_demangle (target.name()));
  return "";
}


// == SerializableBase ==
SerializableBase::SerializableBase ()
{}

SerializableBase::~SerializableBase ()
{}

} // Bse
