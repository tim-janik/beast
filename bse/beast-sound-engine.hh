// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

#include <jsonipc/jsonipc.hh>
#include <bse/bseenums.hh>      // enums API interfaces, etc
#include <bse/platform.hh>
#include <bse/randomhash.hh>
#include <bse/regex.hh>
#include <bse/bse.hh>   // Bse::init_async

// == Aida Workarounds ==
// Manually convert between Aida Handle types (that Jsonipc cannot know about)
// and shared_ptr to Iface types.
// TODO: remove this, once Handle types are removed
// Bse::*Seq as std::vector
template<typename HandleSeq, typename IfaceP>
struct ConvertSeq {
  static HandleSeq
  from_json (const Jsonipc::JsonValue &jarray)
  {
    std::vector<IfaceP> pointers = Jsonipc::from_json<std::vector<IfaceP>> (jarray);
    HandleSeq seq;
    seq.reserve (pointers.size());
    for (auto &ptr : pointers)
      seq.emplace_back (ptr->__handle__());
    return seq;
  }
  static Jsonipc::JsonValue
  to_json (const HandleSeq &vec, Jsonipc::JsonAllocator &allocator)
  {
    std::vector<IfaceP> pointers;
    pointers.reserve (vec.size());
    for (auto &handle : vec)
      {
        typedef typename IfaceP::element_type IfaceT;
        static_assert (std::is_base_of<Aida::ImplicitBase, IfaceT>::value, "");
        auto *iface = handle.__iface__();
        IfaceP ptr = iface ? Bse::shared_ptr_cast<IfaceT> (iface) : nullptr;
        pointers.emplace_back (ptr); // preserve element count, possibly adding NULL
      }
    return Jsonipc::to_json (pointers, allocator);
  }
};

/// Convert between Aida::Any and Jsonipc::JsonValue
struct ConvertAny {
  static Aida::Any
  from_json (const Jsonipc::JsonValue &v, const Aida::Any &fallback = Aida::Any())
  {
    Aida::Any any;
    switch (v.GetType())
      {
      case rapidjson::kNullType:
        return fallback;
      case rapidjson::kFalseType:
        any.set<bool> (false);
        break;
      case rapidjson::kTrueType:
        any.set<bool> (true);
        break;
      case rapidjson::kStringType:
        any.set<std::string> (Jsonipc::from_json<std::string> (v));
        break;
      case rapidjson::kNumberType:
        if      (v.IsInt())     any.set<int32_t>  (v.GetInt());
        else if (v.IsUint())    any.set<int64_t>  (v.GetUint());
        else if (v.IsInt64())   any.set<int64_t>  (v.GetInt64());
        else if (v.IsUint64())  any.set<uint64_t> (v.GetUint64());
        else                    any.set<double>   (v.GetDouble());
        break;
      case rapidjson::kArrayType:
        sequence_from_json_array (any, v);
        break;
      case rapidjson::kObjectType:
        record_from_json_object (any, v);
        break;
    };
    return any;
  }
  static Jsonipc::JsonValue
  to_json (const Aida::Any &any, Jsonipc::JsonAllocator &allocator)
  {
    using namespace Jsonipc;
    switch (int (any.kind()))
      {
      case Aida::BOOL:          return JsonValue (any.get<bool>());
      case Aida::INT32:         return JsonValue (any.get<int32_t>());
      case Aida::INT64:         return JsonValue (any.get<int64_t>());
      case Aida::FLOAT64:       return JsonValue (any.get<double>());
      case Aida::ENUM:          return Jsonipc::to_json (any.get<std::string>(), allocator);
      case Aida::STRING:        return Jsonipc::to_json (any.get<std::string>(), allocator);
      case Aida::SEQUENCE:      return sequence_to_json_array (any.get<const Aida::AnySeq&>(), allocator);
      case Aida::RECORD:        return record_to_json_object (any.get<const Aida::AnyRec&>(), allocator);
      case Aida::INSTANCE:      return instance_to_json_object (any, allocator);
      }
    return JsonValue(); // null
  }
  static void
  sequence_from_json_array (Aida::Any &any, const Jsonipc::JsonValue &v)
  {
    const size_t l = v.Size();
    Aida::AnySeq s;
    for (size_t i = 0; i < l; ++i)
      s.push_back (ConvertAny::from_json (v[i]));
    any.set (s);
  }
  static Jsonipc::JsonValue
  sequence_to_json_array (const Aida::AnySeq &seq, Jsonipc::JsonAllocator &allocator)
  {
    const size_t l = seq.size();
    Jsonipc::JsonValue jarray (rapidjson::kArrayType);
    jarray.Reserve (l, allocator);
    for (size_t i = 0; i < l; ++i)
      jarray.PushBack (ConvertAny::to_json (seq[i], allocator).Move(), allocator);
    return jarray;
  }
  static void
  record_from_json_object (Aida::Any &any, const Jsonipc::JsonValue &v)
  {
    Aida::AnyRec r;
    for (const auto &field : v.GetObject())
      {
        const std::string key = field.name.GetString();
        if (key == "$class" || key == "$id")    // actually Aida::INSTANCE
          return instance_from_json_object (any, v);
        r[key] = ConvertAny::from_json (field.value);
      }
    any.set (r);
  }
  static Jsonipc::JsonValue
  record_to_json_object (const Aida::AnyRec &r, Jsonipc::JsonAllocator &allocator)
  {
    Jsonipc::JsonValue jobject (rapidjson::kObjectType);
    jobject.MemberReserve (r.size(), allocator);
    for (auto const &field : r)
      jobject.AddMember (Jsonipc::JsonValue (field.name.c_str(), allocator),
                         ConvertAny::to_json (field, allocator).Move(), allocator);
    return jobject;
  }
  static void
  instance_from_json_object (Aida::Any &any, const Jsonipc::JsonValue &v)
  {
    Aida::ImplicitBaseP basep = Jsonipc::Convert<Aida::ImplicitBaseP>::from_json (v);
    Bse::ObjectIfaceP objectp = basep ? std::dynamic_pointer_cast<Bse::ObjectIface> (basep) : NULL;
    any.set (objectp);
  }
  static Jsonipc::JsonValue
  instance_to_json_object (const Aida::Any &any, Jsonipc::JsonAllocator &allocator)
  {
    Aida::ImplicitBaseP basep = any.get<Aida::ImplicitBaseP> ();
    return Jsonipc::Convert<Aida::ImplicitBaseP>::to_json (basep, allocator);
  }
};


namespace Jsonipc {

/* TODO: remove this, once Handle types are removed
 * Given `ItemSeq` is a type derived from `std::vector<Aida::RemoteHandle derived>` and
 * assuming its remote handle has a `ItemIface* __iface__()` member function, create
 * a partial specialisation for sequence conversion of this form:
 * template<> struct Convert<ItemSeq> : ConvertSeq<ItemSeq, std::shared_ptr<ItemIface> > {};
 */
#define bse_bseapi_idl_FOREACH_STEP(ItemSeq)    \
  template<> struct Convert<ItemSeq> :          \
    ConvertSeq<ItemSeq,                         \
               std::shared_ptr< std::remove_reference< decltype (*std::declval<ItemSeq::value_type>().__iface__()) >::type > \
               > {};
// FIXME: bse_bseapi_idl_FOREACH_IFACE_SEQ(); // uses bse_bseapi_idl_FOREACH_STEP
#undef bse_bseapi_idl_FOREACH_STEP

// Aida::Any
template<>      struct Convert<Aida::Any> : ConvertAny {};

#if 0 // FIXME
// Bse::PartHandle as Bse::PartIfaceP (in records)
template<>
struct Convert<Aida::RemoteMember<Bse::PartHandle>> {
  static Aida::RemoteMember<Bse::PartHandle>
  from_json (const Jsonipc::JsonValue &jvalue)
  {
    Bse::PartIfaceP ptr = Jsonipc::from_json<Bse::PartIfaceP> (jvalue);
    return ptr->__handle__();
  }
  static Jsonipc::JsonValue
  to_json (const Aida::RemoteMember<Bse::PartHandle> &handle, Jsonipc::JsonAllocator &allocator)
  {
    Bse::PartIfaceP ptr = handle.__iface__()->template as<Bse::PartIfaceP>();
    return Jsonipc::to_json (ptr, allocator);
  }
};
#endif

} // Jsonipc

// types used by "bse/bseapi_jsonipc.cc:Bse_jsonipc_stub()"
#include "bsebus.hh"
#include "bsecontextmerger.hh"
#include "bsecsynth.hh"
#include "bseeditablesample.hh"
#include "bsemidisynth.hh"
#include "bsepart.hh"
#include "module.hh"
#include "bsepcmwriter.hh"
#include "bseproject.hh"
#include "bseserver.hh"
#include "devicecrawler.hh"
#include "bsesnet.hh"
#include "bsesong.hh"
#include "device.hh"
#include "bsesoundfont.hh"
#include "bsesoundfontrepo.hh"
#include "bsetrack.hh"
#include "bsewave.hh"
#include "bsewaveosc.hh"
#include "bsewaverepo.hh"
#include "monitor.hh"

// wrappers for Bse_jsonipc_stub()
void    bse_jsonipc_stub1 ();
void    bse_jsonipc_stub2 ();
void    bse_jsonipc_stub3 ();
void    bse_jsonipc_stub4 ();
