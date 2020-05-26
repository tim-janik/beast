// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "serializable.hh"
#include "pugixml.hh"
#include "internal.hh"

namespace Bse {
namespace Xms {

using PugiDoc = pugi::xml_document;
using PugiNode = pugi::xml_node;
using PugiAttr = pugi::xml_attribute;

// XNode
class SerializationNode::XNode : public PugiNode {
public:
  XNode()
  {}
  XNode (PugiNode pn)
  {
    *this = pn;
  }
  void
  operator= (PugiNode &pn)
  {
    static_assert (sizeof (PugiNode) == sizeof (void*));
    memcpy (this, &pn, sizeof (pn));
  }
};

// Context
class SerializationNode::Context {
  Phase phase_ = INACTIVE;
public:
  std::vector<std::pair<SerializableInterface*, PugiNode>> idnodes_;
  std::map<String,SerializableInterface*> nodeids_;
  std::list<Reflink> reflinks_; // need stable references after append
  SerializationArgs sargs_;
  PugiDoc doc_;
  size_t id_counter_ = 101;
  bool with_default_ = false;
  bool need_resolve_ = true;
  void
  enter_phase (Phase phase)
  {
    switch (phase)
      {
      case INACTIVE:            assert_return (phase_ != LOAD_SERIALIZE && phase_ != SAVE_SERIALIZE); break;
      case LOAD_SERIALIZE:      assert_return (phase_ == INACTIVE); break;
      case LOAD_REFLINK:        assert_return (phase_ == LOAD_SERIALIZE); break;
      case SAVE_SERIALIZE:      assert_return (phase_ == INACTIVE); break;
      case SAVE_REFLINK:        assert_return (phase_ == SAVE_SERIALIZE); break;
      }
    phase_ = phase;
    if (phase_ == INACTIVE)
      {
        idnodes_.clear();
        nodeids_.clear();
        reflinks_.clear();
        sargs_.clear();
        with_default_ = false;
        need_resolve_ = true;
      }
  }
  bool is_inactive () const { return phase_ == INACTIVE; }
  bool in_load     () const { return phase_ == LOAD_SERIALIZE || phase_ == LOAD_REFLINK; }
  bool in_save     () const { return phase_ == SAVE_SERIALIZE || phase_ == SAVE_REFLINK; }
  PugiNode
  make_child (const String &name, PugiNode &parent)
  {
    // assert_return (name.empty() == false, XNode());
    PugiNode child = parent.append_child (name.c_str());
    return child;
  }
};

// SerializationNode
SerializationNode::SerializationNode (std::shared_ptr<Context> context, XNode &xnode) :
  context_ (context)
{
  xnode_() = xnode;
}

SerializationNode::SerializationNode () :
  context_ (std::make_shared<Context> ())
{
  xnode_() = context_->make_child ("", context_->doc_);
}

SerializationNode::XNode&
SerializationNode::xnode_ () const
{
  static_assert (sizeof (xnodemem_) == sizeof (PugiNode));
  return *(XNode*) &xnodemem_;
}

SerializationNode::operator bool () const noexcept
{
  PugiNode xmlnode = xnode_();
  return context_ && !xmlnode.empty();
}

String
SerializationNode::name () const
{
  return xnode_().name();
}

void
SerializationNode::enter_phase (Phase phase)
{
  context_->enter_phase (phase);
}

SerializationNode::SerializationArgs&
SerializationNode::queued_reflinks () const
{
  return context_->sargs_;
}

void
SerializationNode::save_string (const String &attrib, const String &value, bool force_node)
{
  if (!in_save())
    return;
  PugiNode xmlnode = xnode_();
  if (!force_node)
    xmlnode
      .append_attribute (attrib.c_str())
      .set_value (value.c_str());
  else
    {
      PugiNode txtnode = xmlnode.append_child (attrib.c_str());
      txtnode
        .append_child (pugi::node_pcdata)
        .set_value (value.c_str());
    }
}

bool
SerializationNode::load_string (const String &attrib, String &value, bool force_node)
{
  if (!in_load())
    return false;
  PugiNode xmlnode = xnode_();
  // retrieve the value of an attribute attrib="..."
  PugiAttr attr;
  if (!force_node)
    attr = xmlnode.attribute (attrib.c_str());
  if (!attr.empty())
    {
      value = attr.value();
      return true;
    }
  // fall back to retrieving the text of a child <attrib>...</attrib>
  PugiNode child = xmlnode.child (attrib.c_str());
  if (!child.empty())
    {
      value = child.text().get();
      return true;
    }
  return false;
}

bool
SerializationNode::has (const String &attrib) const
{
  PugiNode xmlnode = xnode_();
  return (!xmlnode.attribute (attrib.c_str()).empty() ||
          !xmlnode.child (attrib.c_str()).empty());
}

bool
SerializationNode::loading (const String &attrib) const
{
  return in_load() && has (attrib);
}

bool
SerializationNode::in_load () const
{
  return context_->in_load();
}

bool
SerializationNode::in_save () const
{
  return context_->in_save();
}

bool
SerializationNode::with_default () const
{
  return context_->with_default_ || !context_->in_save();
}

void
SerializationNode::with_default (bool dflt)
{
  context_->with_default_ = dflt;
}

size_t
SerializationNode::count_children ()
{
  PugiNode xmlnode = xnode_();
  size_t i = 0;
  for (XNode child = xmlnode.first_child(); child; child = child.next_sibling())
    i += 1;
  return i;
}

std::string
SerializationNode::first_child_name ()
{
  PugiNode xmlnode = xnode_();
  return xmlnode.first_child().name();
}

void
SerializationNode::rotate_children ()
{
  PugiNode xmlnode = xnode_();
  xmlnode.append_move (xmlnode.first_child());
}

Reflink&
SerializationNode::manage_reflink (const Reflink &rl)
{
  // this function is only needed to turn temporaries (const&) into lvalues (to support operator&)
  return context_->reflinks_.emplace_back (rl);
}

String
SerializationNode::retrieve_id (SerializableInterface &s)
{
  if (!in_save())
    return "";
  // find PugiNode for SerializableInterface
  auto it = std::find_if (context_->idnodes_.begin(), context_->idnodes_.end(), [&s] (const auto &p) {
      return p.first == &s;
    });
  const bool serializable_visited = it != context_->idnodes_.end();
  if (!serializable_visited)
    {
      assert_return (serializable_visited == true, null_id); // ERROR: link to `s` without prior serialization
      return null_id;
    }
  PugiNode xmlnode = it->second;
  // find Xms.ID attribute
  PugiAttr attr = xmlnode.attribute ("Xms.ID");
  if (attr.empty())
    {
      attr = xmlnode.prepend_attribute ("Xms.ID");
      attr.set_value (string_format ("id%u", context_->id_counter_++).c_str());
    }
  return attr.value();
}

void
SerializationNode::register_idnode (SerializableInterface &s)
{
  PugiNode xmlnode = xnode_();
  context_->idnodes_.push_back ({ &s, xmlnode });
}

SerializableInterface*
SerializationNode::resolve_id (const String &idstring)
{
  if (!in_load() || !string_startswith (idstring, "id"))
    return nullptr;
  if (context_->need_resolve_)
    {
      for (const auto &pair : context_->idnodes_)
        {
          PugiNode xmlnode = pair.second;
          PugiAttr attr = xmlnode.attribute ("Xms.ID");
          if (!attr.empty())    // map Xms.ID="..." to SerializableInterface*
            context_->nodeids_[attr.value()] = pair.first;
        }
      context_->need_resolve_ = false;
    }
  auto it = context_->nodeids_.find (idstring);
  return it != context_->nodeids_.end() ? it->second : nullptr;
}

SerializationNode
SerializationNode::create_child (const String &childname)
{
  PugiNode xmlnode = xnode_();
  PugiNode childnode = xmlnode.append_child (childname.empty() ? "?" : childname.c_str());
  XNode xchildnode = childnode;
  SerializationNode child { context_, xchildnode };
  assert_return (childname.empty() == false, child);
  return child;
}

SerializationNode
SerializationNode::first_child (const String &childname)
{
  if (in_load())
    {
      PugiNode xmlnode = xnode_();
      for (XNode child = xmlnode.first_child(); !child.empty(); child = child.next_sibling())
        if (child.name() == childname)
          return SerializationNode { context_, child };
    }
  XNode empty;
  return SerializationNode { context_, empty };
}

SerializationNode::Children
SerializationNode::children (const String &childname)
{
  Children cvector;
  if (in_load())
    {
      PugiNode xmlnode = xnode_();
      for (XNode child = xmlnode.first_child(); !child.empty(); child = child.next_sibling())
        if (child.name() == childname)
          cvector.push_back (SerializationNode { context_, child });
    }
  return cvector;
}

SerializationField
SerializationNode::get (const String &attrib)
{
  return SerializationField (*this, attrib);
}

SerializationField
SerializationNode::operator[] (const String &attrib)
{
  return get (attrib);
}

String
SerializationNode::write_xml (const String &root)
{
  std::stringstream serialization_stream;
  PugiNode xmlnode = xnode_();
  xmlnode.set_name (root.c_str());
  const uint flags = pugi::format_no_declaration | pugi::format_indent | pugi::format_indent_attributes;
  context_->doc_.save (serialization_stream, "  ", flags);
  String result = serialization_stream.str();
  // regex to make simple nodes one-liner
  const std::string pat = "(\\n *)<([^<>\"]+)\\n *([^<>\"=]+)=\"([^<>\"]+)\" *(/?)>",
                    rpl = "$1<$2 $3=\"$4\"$5>";
  result = Re::sub (pat, rpl, result);
  return result;
}

static Bse::Error
pugi_error_to_bse (pugi::xml_parse_status pugierror)
{
  switch (pugierror)
    {
    case pugi::status_no_document_element:      return Bse::Error::NO_DATA_AVAILABLE;
    case pugi::status_unrecognized_tag:         return Bse::Error::DATA_CORRUPT;
    case pugi::status_bad_pcdata:               return Bse::Error::DATA_CORRUPT;
    case pugi::status_bad_pi:
    case pugi::status_bad_cdata:
    case pugi::status_bad_comment:
    case pugi::status_bad_doctype:
    case pugi::status_bad_attribute:
    case pugi::status_bad_end_element:
    case pugi::status_end_element_mismatch:     return Bse::Error::FORMAT_INVALID;
    case pugi::status_bad_start_element:        return Bse::Error::DATA_UNMATCHED;
    default:                                    return Bse::Error::IO;
    }
}

Bse::Error
SerializationNode::parse_xml (const String &root, const String &xmltext, String *ep)
{
  assert_return (context_->is_inactive(), Bse::Error::INTERNAL);
  std::stringstream inputstream (xmltext);
  context_->doc_.reset();
  pugi::xml_parse_result result = context_->doc_.load (inputstream, pugi::parse_default, pugi::encoding_utf8);
  xnode_() = context_->doc_.first_child();
  if (!root.empty() && result.status == pugi::status_ok && root != xnode_().name())
    {
      result.status = pugi::status_bad_start_element;
      result.offset = 0;
    }
  if (result.status != pugi::status_ok)
    {
      const size_t l = 1 + std::count (xmltext.begin(), xmltext.begin() + result.offset, '\n');
      const String errmsg = string_format ("%u: %s", l, result.description());
      if (ep)
        *ep = errmsg;
      else
        warning ("%s\n", errmsg);
      return pugi_error_to_bse (result.status);
    }
  return Bse::Error::NONE;
}

// SerializableInterface
void
SerializableInterface::xml_serialize (SerializationNode &xs)
{}

void
SerializableInterface::xml_reflink (SerializationNode &xs)
{}

// SerializationField
SerializationField::SerializationField (SerializationNode &xs, const String &attrib) :
  xs_ (xs), attrib_ (attrib)
{}

SerializationField&
SerializationField::node ()
{
  force_node_ = true;
  return *this;
}

bool
SerializationField::as_node () const
{
  return force_node_;
}

SerializationField&
SerializationField::hex ()
{
  hex_ = true;
  return *this;
}

bool
SerializationField::as_hex () const
{
  return hex_;
}

String
SerializationField::attribute () const
{
  return attrib_;
}

SerializationNode&
SerializationField::serialization_node ()
{
  return xs_;
}

SerializationField::operator std::string ()
{
  String result;
  if (xs_.in_load())
    *this & result;
  return result;
}

bool
SerializationField::serialize (Reflink &reflink, const StringVector &typedata, const std::string &fieldname)
{
  if (xs_.in_save())
    {
      reflink.save_xml (xs_, attrib_);
      return true;
    }
  if (xs_.in_load())
    return reflink.load_xml (xs_, attrib_);
  return false;
}

// Reflink
void
Reflink::save_xml (SerializationNode &xs, const String &attrib)
{
  String id = serializable_ ? xs.retrieve_id (*serializable_) : xs.null_id;
  xs[attrib] & id;
}

bool
Reflink::load_xml (SerializationNode &xs, const String &attrib)
{
  assign_ (xs, attrib);
  return true;
}

// == Helpers ==
bool
typedata_is_loadable (const StringVector &typedata, const std::string &field)
{
  return_unless (!typedata.empty() && !field.empty(), true); // avoid constraining unknown fields
  const String hints = Aida::aux_vector_find (typedata, field, "hints", "S:w");
  const bool storage = string_option_check (hints, "S");
  const bool writable = string_option_check (hints, "w");
  return storage && writable;
}

bool
typedata_is_storable (const StringVector &typedata, const std::string &field)
{
  return_unless (!typedata.empty() && !field.empty(), true); // avoid constraining unknown fields
  const String hints = Aida::aux_vector_find (typedata, field, "hints", "S:r");
  const bool storage = string_option_check (hints, "S");
  const bool readable = string_option_check (hints, "r");
  return storage && readable;
}

bool
typedata_find_minimum (const StringVector &typedata, const std::string &field, long double *limit)
{
  if (typedata.empty() || field.empty()) // avoid constraining unknown fields
    return false;
  const String str = Aida::aux_vector_find (typedata, field, "min", "");
  if (!str.empty())
    {
      if (limit)
        *limit = string_to_type<long double> (str);
      return true;
    }
  return false;
}

bool
typedata_find_maximum (const StringVector &typedata, const std::string &field, long double *limit)
{
  if (typedata.empty() || field.empty()) // avoid constraining unknown fields
    return false;
  const String str = Aida::aux_vector_find (typedata, field, "max", "");
  if (!str.empty())
    {
      if (limit)
        *limit = string_to_type<long double> (str);
      return true;
    }
  return false;
}

} // Xms
} // Bse
