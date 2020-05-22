// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "explore.hh"
#include <bse/testing.hh>
#include <bse/serializable.hh>
#include <bse/bsemain.hh>

namespace { // Anon
using namespace Bse;

// test_serializable_configuration
struct SerializableConfiguration : public virtual Xms::SerializableInterface {
  Preferences config_;
protected:
  void
  xml_serialize (Xms::SerializationNode &xs) override
  {
    xs["Configuration"] & config_;
  }
public:
  SerializableConfiguration() = default;
  Preferences& config() { return config_; }
  bool operator== (const SerializableConfiguration &o) { return config_ == o.config_; }
  bool operator!= (const SerializableConfiguration &o) { return !operator== (o); }
};

static void
test_serializable_configuration()
{
  SerializableConfiguration cfg1, cfg2;
  const int L = 6;
  const char *const S = "NONE-0123456789";
  String xmltext;
  {
    cfg1.config().synth_latency = L;
    cfg1.config().license_default = S;
    Xms::SerializationNode xs;
    xs.save (cfg1);
    xmltext = xs.write_xml ("Root");
  }
  TASSERT (cfg1 != cfg2);
  {
    Xms::SerializationNode xs;
    if (Bse::Error::NONE == xs.parse_xml ("Root", xmltext, nullptr))
      xs.load (cfg2);
    TASSERT (S == cfg2.config().license_default);
    TASSERT (L == cfg2.config().synth_latency);
  }
  TASSERT (cfg1 == cfg2);
  // printout ("%s", xmltext);
}
TEST_ADD (test_serializable_configuration);

// test_serializable_hierarchy
class FrobnicatorBase : public virtual Xms::SerializableInterface {
protected:
  void
  xml_serialize (Xms::SerializationNode &xs) override
  {
    xs["flags"].hex() & flags_;
  }
public:
  uint32_t flags_ = 0x01020304;
};

class FrobnicatorSpecial : public FrobnicatorBase {
  String                    type_;
protected:
  void
  xml_serialize (Xms::SerializationNode &xs) override
  {
    FrobnicatorBase::xml_serialize (xs);
    xs["type"] & type_;
    xs["max"] & max_;
  }
public:
  explicit     FrobnicatorSpecial (const String &t = "") : type_ (t) {}
  double       max_ = 0;
};

class FrobnicatorImpl : public FrobnicatorBase {
  String                    type_;
  int64                     state_ = 0;
  bool                      yesno_ = 0;
  float                     factor_ = 0;
  bool                      toggle_ = 0;
  std::vector<int>          nums_;
  std::vector<std::vector<float>> matrix_;
  Bse::Error                error_ = Bse::Error (0);
  std::vector<Xms::SerializableInterface*> children_;
public:
  std::vector<Bse::Preferences> configs_;
  FrobnicatorImpl              *link_ = nullptr;
protected:
  void         xml_serialize (Xms::SerializationNode &xs) override;
  void         xml_reflink   (Xms::SerializationNode &xs) override;
public:
  explicit     FrobnicatorImpl   (const String &t = "") : type_ (t) {}
  void         load          ()                         { Xms::SerializationNode xnode; xnode.load (*this); } // from parser
  void         save          ()                         { Xms::SerializationNode xnode; xnode.save (*this); }
  Xms::SerializableInterface&
  create_track  (const String &type)
  {
    Xms::SerializableInterface *c;
    if (type == "Master")
      c = new FrobnicatorSpecial (type);
    else
      c = new FrobnicatorImpl (type);
    return *children_.emplace_back (c);
  }
  void
  populate (bool matrix = false)
  {
    state_ = +32768;
    yesno_ = true;
    factor_ = -7.7;
    toggle_ = true;
    nums_ = { 1, 2, 3 };
    if (matrix)
      matrix_ = { { 9, 8, 7 }, { 6, 5, 4 }, };
    error_ = Bse::Error::DEVICE_NOT_AVAILABLE;
  }
};

void
FrobnicatorImpl::xml_serialize (Xms::SerializationNode &xs)
{
  if (xs.in_load() || !type_.empty()) // skip empty default
    xs["type"] & type_;
  FrobnicatorBase::xml_serialize (xs);
  if (xs.with_default() || state_)
    xs["state"]  & this->state_;
  if (type_ == "")
    xs["matrix"] & matrix_;
  if (type_ == "")
    xs["nums"] & nums_;
  if (xs.with_default() || factor_ != 0)
    xs["factor"] & factor_;
  xs["toggle"] & this->toggle_;
  xs["error"]  & error_;
  xs["configs"] & configs_;
  for (auto &childp : children_)                // if (xs.in_save()) ...
    xs.save_under ("Track", *childp);           // creates <Track attrib="value">...</> via childp->xml_serialize()
  for (auto &xc : xs.children ("Track"))        // filters on children of type <Track />
    xc.load (create_track (xc.get ("type")));
  // custom stringify
  String yn = yesno_ ? "yes" : "no";
  xs["question"] & yn;
  if (xs.loading ("question"))
    yesno_ = yn == "yes";
}

void
FrobnicatorImpl::xml_reflink (Xms::SerializationNode &xs)
{
  xs["link"] & xs.reflink (link_);
}

static void
test_serializable_hierarchy()
{
  const bool V = false;
  String xmltext1;
  if (V)
    printerr ("XML:\n");
  {
    FrobnicatorImpl project;
    project.flags_ = 0x01010101;
    project.populate (true);
    auto &midi = project.create_track ("Midi");
    dynamic_cast<FrobnicatorBase*> (&midi)->flags_ = 0x02020202;
    project.link_ = dynamic_cast<FrobnicatorImpl*> (&project.create_track ("Audio"));
    project.link_->populate (false);
    project.link_->configs_.push_back (*Bse::global_config);
    FrobnicatorSpecial *master = dynamic_cast<FrobnicatorSpecial*> (&project.create_track ("Master"));
    master->max_ = -9e-309;
    master->flags_ = 0xeaeaeaea;
    Xms::SerializationNode xs;
    xs.save (project);
    xmltext1 = xs.write_xml ("Project");
    TASSERT (xs.name() == "Project");
#if 0
    xmltext1 = xs.write_xml ("Test55");
    TASSERT (xs.name() == "Test55");
#endif
  }
  if (V)
    printerr ("%s\n", xmltext1);
  if (V)
    printerr ("DUP:\n");
  String xmltext2;
  {
    FrobnicatorImpl project;
    {
      Xms::SerializationNode xs;
      if (Bse::Error::NONE == xs.parse_xml ("Project", xmltext1))
        {
          TASSERT (xs.name() == "Project");
          xs.load (project);
        }
    }
    Xms::SerializationNode xs;
    xs.save (project);
    xmltext2 = xs.write_xml ("Project");
  }
  if (V)
    printerr ("%s\n", xmltext2);
  TASSERT (xmltext1 == xmltext2);
}
TEST_ADD (test_serializable_hierarchy);
} // Anon
