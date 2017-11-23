// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

IGNORE:
struct DUMMY { // dummy class for auto indentation


handle_scope:Object:
  uint64_t      on       (const ::std::string &type, ::Aida::EventHandlerF handler) { return this->RemoteHandle::__event_attach__ (type, handler); }
  bool          off      (uint64_t connection_id)                                   { return this->RemoteHandle::__event_detach__ (connection_id); }
  uint64_t      off      (const ::std::string &type)                                { return this->RemoteHandle::__event_detach__ (type); }

interface_scope:Object:
  uint64_t      on       (const ::std::string &type, ::Aida::EventHandlerF handler) { return this->ImplicitBase::__event_attach__ (type, handler); }
  bool          off      (uint64_t connection_id)                                   { return this->ImplicitBase::__event_detach__ (connection_id); }
  uint64_t      off      (const ::std::string &type)                                { return this->ImplicitBase::__event_detach__ (type); }
  void          trigger  (const ::Aida::Event &event)                               { return this->ImplicitBase::__event_emit__ (event); }
  // as<BseObjectPtr>()
  template<class BseObjectPtr, typename ::std::enable_if<std::is_pointer<BseObjectPtr>::value, bool>::type = true>
  BseObjectPtr           as ()
  {
    static_assert (std::is_pointer<BseObjectPtr>::value, "'BseObject*' required");
    typedef typename std::remove_pointer<BseObjectPtr>::type BseObjectT;
    static_assert (std::is_base_of<GObject, BseObjectT>::value, "'BseObject*' required");
    return this ? (BseObjectPtr) this->as_bse_object() : NULL;
  }
  // DERIVES_shared_ptr (uses void_t to prevent errors for T without shared_ptr's typedefs)
  template<class T, typename = void> struct DERIVES_shared_ptr : std::false_type {};
  template<class T> struct DERIVES_shared_ptr<T, Bse::void_t< typename T::element_type > > :
  std::is_base_of< std::shared_ptr<typename T::element_type>, T > {};
  // as<shared_ptr<T>>()
  template<class ObjectImplP, typename ::std::enable_if<DERIVES_shared_ptr<ObjectImplP>::value, bool>::type = true>
  ObjectImplP            as ()
  {
    typedef typename ObjectImplP::element_type ObjectImplT;
    static_assert (std::is_base_of<Aida::ImplicitBase, ObjectImplT>::value, "");
    ObjectImplT *impl = this ? dynamic_cast<ObjectImplT*> (this) : NULL;
    return impl ? Bse::shared_ptr_cast<ObjectImplT> (impl) : NULL;
  }
protected:
  virtual BseObject* as_bse_object() = 0;


IGNORE: // close last _scope
}; // DUMMY
