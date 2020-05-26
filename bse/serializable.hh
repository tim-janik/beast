// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_SERIALIZABLE_HH__
#define __BSE_SERIALIZABLE_HH__

#include <bse/bseenums.hh>

namespace Bse {

/// Namespace for XML based serialization
namespace Xms {

// Forward declarations
class Reflink;
class SerializationNode;

/// Check for the writable and storage flags in the hints field of typedata.
bool typedata_is_loadable  (const StringVector &typedata, const std::string &field);
/// Check for the readable and storage flags in the hints field of typedata.
bool typedata_is_storable  (const StringVector &typedata, const std::string &field);
/// Find the minimum value for field of typedata.
bool typedata_find_minimum (const StringVector &typedata, const std::string &field, long double *limit);
/// Find the maximum value for field of typedata.
bool typedata_find_maximum (const StringVector &typedata, const std::string &field, long double *limit);

/// Interface for serializable objects with Reflink support.
class SerializableInterface {
  friend SerializationNode;
protected:
  virtual void xml_serialize (SerializationNode &xs) = 0; ///< Serialize members and childern
  virtual void xml_reflink   (SerializationNode &xs);     ///< Serialize object references (links)
};

/// Reference to a storage attribute (or child node) for serialization
class SerializationField {
  SerializationNode &xs_;
  const String   attrib_;
  bool           force_node_ = false;
  bool           hex_ = false;
 public:
  explicit                         SerializationField (SerializationNode &xs, const String &attrib);
  /// Serialization operator
  template<typename T> bool        operator& (T &v) { return serialize (v, StringVector(), ""); }
  /// Main serialization function.
  template<typename T,
           typename E = void> bool serialize (T&, const StringVector&, const std::string&);
  template<typename T> bool        serialize (std::vector<T> &vec, const StringVector&, const std::string&);
  bool                             serialize (Reflink&, const StringVector&, const std::string&);
  /*auto cast*/                    operator std::string ();
  SerializationField               node      ();       ///< Force storage into child node (not attribute)
  bool                             as_node   () const; ///< Retrive node() flag
  SerializationField               hex       ();       ///< Hint for unsigned storage as hexadecimal
  bool                             as_hex    () const; ///< Retrive hex() flag
  String                           attribute () const; ///< Associated attribute (or child node) name for serialization
  SerializationNode&      serialization_node ();       ///< Associated storage node for serialization
};

/// Representation of a storage node for serialization via save() and load()
class SerializationNode {
  friend class SerializationField;
  friend class Reflink;
  class XNode;
  class Context;
  struct QueuedArgs;
  using SerializationArgs = std::vector<QueuedArgs>;
  enum Phase { INACTIVE, LOAD_SERIALIZE, LOAD_REFLINK, SAVE_SERIALIZE, SAVE_REFLINK };
  std::shared_ptr<Context>      context_;
  void                         *xnodemem_ = nullptr;
  XNode&                        xnode_            () const;
  void                          enter_phase       (Phase phase);
  SerializationArgs&            queued_reflinks   () const;
  explicit                      SerializationNode (std::shared_ptr<Context> context, XNode &xnode);
protected:
  Reflink&                      manage_reflink    (const Reflink &rl);
  size_t                        count_children    ();   ///< Retrieve the number of available children.
  std::string                   first_child_name  ();   ///< Retrieve the name of the first child during load().
  void                          rotate_children   ();   ///< Rotate children, i.e. pop first and append it at the tail.
  void                          save_string       (const String &attrib, const String &value, bool force_node);
  bool                          load_string       (const String &attrib, String &value, bool force_node);
  void                          register_idnode   (SerializableInterface &s);
  SerializableInterface*        resolve_id        (const String &idstring);
  String                        retrieve_id       (SerializableInterface &s);
  template<typename T> T*       dynamic_resolve   (const String &attrib, T *fallback);
public:
  using Children = std::vector<SerializationNode>;
  static constexpr const char*const null_id = "none";                         ///< String representing a serialized `nullptr`
  explicit                    SerializationNode ();                           ///< Create a SerializationNode de-/serialization.
  String                      name              () const;                     ///< Tag name of this storage node
  explicit                    operator bool     () const noexcept;            ///< Check if `this` exists as deserialization node
  SerializationNode           create_child      (const String &childname);    ///< Create a child node for nested hierarchies
  SerializationNode           first_child       (const String &childname);    ///< Access the first stored child node
  Children                    children          (const String &childname);    ///< List all children by name
  SerializationField          get               (const String &attrib);       ///< Refer to a serialization field by name
  SerializationField          operator[]        (const String &attrib);       ///< Convenience operator alias for get()
  bool                        has               (const String &attrib) const; ///< Check for attribute or child presence
  bool                        loading           (const String &attrib) const; ///< Combines in_load() with has() check
  bool                        in_load           () const;     ///< Return `true` during deserialization
  bool                        in_save           () const;     ///< Return `true` during serialization
  bool                        with_default      () const;     ///< Always `true` during in_load(), toggled during in_save()
  void                        with_default      (bool dflt);  ///< Toggle with_default() during in_save()
  template<typename T>
  Reflink&                    reflink           (T* &serializable_ptr); ///< Wrap serializable object pointers.
  template<typename T> void   save_under        (const String &tag,
                                                 T &object);  ///< Serialize `object` into a cihld node
  template<typename T> void   load              (T &object);  ///< Deserialize `object` via xml_serialize() and xml_reflink()
  template<typename T> void   save              (T &object);  ///< Serialize `object` via xml_serialize() and xml_reflink()
  Bse::Error                  parse_xml         (const String &root, const String &xmltext,
                                                 String *ep = NULL);    ///< Parse `xmlstr`, returns error location and message in `ep`.
  String                      write_xml         (const String &root);   ///< Generate XML with toplevel tag `root` from `this` SerializationNode
};

/// Representation for an object reference between SerializableInterface objects.
class Reflink {
  SerializableInterface *serializable_ = nullptr;
  std::function<void (SerializationNode&, const String&)> assign_;
public:
  template<typename T>
  explicit Reflink  (T *&objptr);
  void     save_xml (SerializationNode &xs, const String &attrib);
  bool     load_xml (SerializationNode &xs, const String &attrib);
};

/// Template to specialize XML attribute conversion for various data types.
template<typename T, typename = void>
struct DataConverter {
  static_assert (!sizeof (T), "type serialization unimplemented");
  // bool save_xml (SerializationField field, const T&, const StringVector&, const std::string&);
  // bool load_xml (SerializationField field, T&, const StringVector&, const std::string&);
};

/// Helper for deferred xml_reflink() calls
struct SerializationNode::QueuedArgs {
  SerializationNode      snode;
  SerializableInterface &serializable;
};

// == Implementation details ==
template<typename T, typename> bool
SerializationField::serialize (T &value, const StringVector &typedata, const std::string &fieldname)
{
  if (xs_.in_save())
    return DataConverter<T>::save_xml (*this, value, typedata, fieldname);
  if (xs_.loading (attrib_))
    return DataConverter<T>::load_xml (*this, value, typedata, fieldname);
  return false;
}

template<> inline bool
SerializationField::serialize (String &value, const StringVector &typedata, const std::string &fieldname)
{
  if (xs_.in_save() && typedata_is_storable (typedata, fieldname))
    {
      xs_.save_string (attrib_, value, as_node());
      return true;
    }
  if (xs_.in_load() && typedata_is_loadable (typedata, fieldname))
    {
      String temp;
      if (xs_.load_string (attrib_, temp, as_node()))
        {
          value = temp;
          return true;
        }
    }
  return false;
}

template<typename T> bool
SerializationField::serialize (std::vector<T> &vec, const StringVector &typedata, const std::string &fieldname)
{
  const String item = "item";
  const auto &vec_typedata = Aida::typedata_from_type (vec);
  if (xs_.in_save() && typedata_is_storable (typedata, fieldname))
    {
      SerializationNode xv = xs_.create_child (attrib_);
      for (auto &el : vec)
        {
          auto field = xv[item].node(); // force node, because XML has no repeating attributes
          field.serialize (el, vec_typedata, "0");
        }
      return true;
    }
  if (!xs_.in_load() || !typedata_is_loadable (typedata, fieldname))
    return false;
  if (SerializationNode xv = xs_.first_child (attrib_)) // true only for in_load()
    {
      const size_t n_children = xv.count_children();
      for (size_t i = 0; i < n_children; i++)   // perform *exactly* n_children rotations
        {
          if (xv.first_child_name() == item)
            {
              vec.resize (vec.size() + 1);
              xv[item].node().serialize (vec.back(), vec_typedata, "0");
            }
          xv.rotate_children();                 // perform *exactly* n_children rotations
        }                                       // to preserve original order
      return true;
    }
  return false;
}

// Reflink
template<typename T>
Reflink::Reflink (T *&objptr) :
  serializable_ (objptr),
  assign_ ([&objptr] (SerializationNode &xs, const String &attrib) {
      objptr = xs.dynamic_resolve (attrib, objptr);
    })
{}

// SerializationNode
template<typename T> Reflink&
SerializationNode::reflink (T* &serializable_ptr)
{
  return manage_reflink (Reflink (serializable_ptr));
}

template<typename T> T*
SerializationNode::dynamic_resolve (const String &attrib, T *fallback)
{
  if (!loading (attrib))
    return fallback;
  const String idstring = get (attrib);
  SerializableInterface *s = resolve_id (idstring);
  return s ? dynamic_cast<T*> (s) : nullptr;
}

template<typename T> void
SerializationNode::load (T &object)
{
  if (in_save())
    return;
  const bool outmost = !in_load();
  SerializableInterface &serializable = object;
  if (outmost)
    enter_phase (LOAD_SERIALIZE);
  serializable.xml_serialize (*this);
  queued_reflinks().push_back ({ *this, serializable });
  if (outmost)
    {
      enter_phase (LOAD_REFLINK);
      for (QueuedArgs &qa : queued_reflinks())
        qa.snode.register_idnode (qa.serializable);
      for (QueuedArgs &qa : queued_reflinks())
        qa.serializable.xml_reflink (qa.snode);
    }
  if (outmost)
    enter_phase (INACTIVE);
}

template<typename T> void
SerializationNode::save (T &object)
{
  if (in_load())
    return;
  const bool outmost = !in_save();
  SerializableInterface &serializable = object;
  if (outmost)
    enter_phase (SAVE_SERIALIZE);
  serializable.xml_serialize (*this);
  queued_reflinks().push_back ({ *this, serializable });
  if (outmost)
    {
      enter_phase (SAVE_REFLINK);
      for (QueuedArgs &qa : queued_reflinks())
        qa.snode.register_idnode (qa.serializable);
      for (QueuedArgs &qa : queued_reflinks())
        qa.serializable.xml_reflink (qa.snode);
    }
  if (outmost)
    enter_phase (INACTIVE);
}

template<typename T> void
SerializationNode::save_under (const String &tag, T &object)
{
  if (in_load())
    return;
  if (!in_save())
    return;
  auto xc = create_child (tag);
  xc.save (object);
}

// DataConverter
template<typename T>
struct DataConverter<T, typename ::std::enable_if<
                          std::is_integral<T>::value || std::is_floating_point<T>::value,
                          void>::type>
{
  static bool
  load_xml (SerializationField field, T &v, const StringVector &typedata, const std::string &fieldname)
  {
    String str;
    if (field.serialize (str, typedata, fieldname))
      {
        T tmp = string_to_type<T> (str);
        bool valid = true;
        long double limit;
        if (typedata_find_minimum (typedata, fieldname, &limit))
          valid = valid && tmp >= limit;
        if (typedata_find_maximum (typedata, fieldname, &limit))
          valid = valid && tmp <= limit;
        if (valid)
          {
            v = tmp;
            return true;
          }
      }
    return false;
  }
  static bool
  save_xml (SerializationField field, T i, const StringVector &typedata, const std::string &fieldname)
  {
    String str;
    if (std::is_same<bool, T>::value)
      str = i ? "true" : "false";
    else if (std::is_unsigned<T>::value && sizeof (T) == 1 && field.as_hex())
      str = string_format ("0x%02x", i);
    else if (std::is_unsigned<T>::value && sizeof (T) == 2 && field.as_hex())
      str = string_format ("0x%04x", i);
    else if (std::is_unsigned<T>::value && sizeof (T) >= 4 && field.as_hex())
      str = string_format ("0x%08x", i);
    else
      str = string_from_type<T> (i);
    return field.serialize (str, typedata, fieldname);
  }
};

// Aida::enum_* convertibles
template<typename Enum>
struct DataConverterAidaEnum {
  static bool
  load_xml (SerializationField field, Enum &v, const StringVector &typedata, const std::string &fieldname)
  {
    String valuename;
    if (field.serialize (valuename, typedata, fieldname))
      {
        v = Aida::enum_value_from_string<Enum> (valuename);
        return true;
      }
    return false;
  }
  static bool
  save_xml (SerializationField field, Enum val, const StringVector &typedata, const std::string &fieldname)
  {
    String valuename = Aida::enum_value_to_string (val);
    return field.serialize (valuename, typedata, fieldname);
  }
};
template<> struct DataConverter<Bse::Error> : DataConverterAidaEnum<Bse::Error> {};

// Aida::__visit__ records
template<typename Record>
struct DataConverterAidaVisitRecord {
  static bool
  load_xml (SerializationField field, Record &r, const StringVector &typedata, const std::string &fieldname)
  {
    if (!typedata_is_loadable (typedata, fieldname))
      return false;
    if (SerializationNode xr = field.serialization_node().first_child (field.attribute()))
      {
        bool fany = false;
        const auto &rec_typedata = Aida::typedata_from_type (r);
        r.__visit__ ([&xr,&fany,&rec_typedata] (auto &v, const char *n)
        {
          if (xr[n].serialize (v, rec_typedata, n))
            fany = true;
        });
        return fany;
      }
    return false;
  }
  static bool
  save_xml (SerializationField field, Record &rec, const StringVector &typedata, const std::string &fieldname)
  {
    if (!typedata_is_storable (typedata, fieldname))
      return false;
    const auto &rec_typedata = Aida::typedata_from_type (rec);
    SerializationNode xr = field.serialization_node().create_child (field.attribute());
    rec.__visit__ ([&xr,&rec_typedata] (auto &v, const char *n)
    {
      xr[n].serialize (v, rec_typedata, n);
    });
    return true;
  }
};
template<typename T>
struct DataConverter<T, typename ::std::enable_if<
                          Aida::Has___visit__<T>::value && !std::is_polymorphic<T>::value,
                          void>::type> :
  DataConverterAidaVisitRecord<T>
{};

} // Xms

using SerializationNode = Xms::SerializationNode;
using SerializableInterface = Xms::SerializableInterface;

} // Bse

#endif // __BSE_SERIALIZABLE_HH__
