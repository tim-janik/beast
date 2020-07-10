// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PROPERTY_HH__
#define __BSE_PROPERTY_HH__

#include <bse/object.hh>

namespace Bse {

class PropertyImpl;
using PropertyImplP = std::shared_ptr<PropertyImpl>;

class PropertyWrapper {
  friend std::unique_ptr<PropertyWrapper>::deleter_type;
protected:
  enum Tag {
    IDENTIFIER = 1,
    LABEL,
    NICK,
    UNIT,
    HINTS,
    GROUP,
    BLURB,
    DESCRIPTION,
  };
  virtual            ~PropertyWrapper  ();
  virtual std::string get_tag          (Tag) = 0;
  virtual bool        is_numeric       () = 0;
  virtual ChoiceSeq   choices          () = 0;
  virtual double      get_num          () = 0;
  virtual bool        set_num          (double v) = 0;
  virtual void        get_range        (double *min, double *max, double *step) = 0;
  virtual std::string get_text         () = 0;
  virtual bool        set_text         (const std::string &v) = 0;
  friend class PropertyImpl;
};
using PropertyWrapperP = std::unique_ptr<PropertyWrapper>;

class PropertyImpl : public ObjectImpl, public virtual PropertyIface {
public:
  virtual bool         is_numeric   () override                     { return wrapper_->is_numeric(); }
  virtual ChoiceSeq    choices      () override                     { return wrapper_->choices(); }
  virtual double       get_num      () override                     { return wrapper_->get_num(); }
  virtual bool         set_num      (double v) override             { return wrapper_->set_num (v); }
  virtual std::string  get_text     () override                     { return wrapper_->get_text(); }
  virtual bool         set_text     (const std::string &v) override { return wrapper_->set_text (v); }
  virtual std::string  identifier   () override { return wrapper_->get_tag (PropertyWrapper::IDENTIFIER); }
  virtual std::string  label        () override { return wrapper_->get_tag (PropertyWrapper::LABEL); }
  virtual std::string  nick         () override { return wrapper_->get_tag (PropertyWrapper::NICK); }
  virtual std::string  unit         () override { return wrapper_->get_tag (PropertyWrapper::UNIT); }
  virtual std::string  hints        () override { return wrapper_->get_tag (PropertyWrapper::HINTS); }
  virtual std::string  group        () override { return wrapper_->get_tag (PropertyWrapper::GROUP); }
  virtual std::string  blurb        () override { return wrapper_->get_tag (PropertyWrapper::BLURB); }
  virtual std::string  description  () override { return wrapper_->get_tag (PropertyWrapper::DESCRIPTION); }
  virtual double       get_min      () override                     { double v; wrapper_->get_range (&v, 0, 0); return v; }
  virtual double       get_max      () override                     { double v; wrapper_->get_range (0, &v, 0); return v; }
  virtual double       get_step     () override                     { double v; wrapper_->get_range (0, 0, &v); return v; }
  static PropertyImplP create       (PropertyWrapperP &&wrapper);
private:
  /*copy*/             PropertyImpl (const PropertyImpl&) = delete;
  explicit             PropertyImpl (PropertyWrapperP &&wrapper);
  virtual             ~PropertyImpl ();
  const PropertyWrapperP wrapper_;
  friend class FriendAllocator<PropertyImpl>;
};
typedef std::shared_ptr<PropertyImpl> PropertyImplP;

} // Bse

#endif // __BSE_PROPERTY_HH__
