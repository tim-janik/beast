// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

IGNORE:
struct DUMMY { // dummy class for auto indentation


class_scope:Object:
  template<class BseObjectPtr>
  BseObjectPtr           as ()
  {
    static_assert (std::is_pointer<BseObjectPtr>::value, "'BseObject*' required");
    typedef typename std::remove_pointer<BseObjectPtr>::type BseObjectT;
    static_assert (std::is_base_of<GObject, BseObjectT>::value, "'BseObject*' required");
    return this ? (BseObjectPtr) this->as_bse_object() : NULL;
  }
protected:
  virtual BseObject* as_bse_object() = 0;


IGNORE: // close last _scope
}; // DUMMY
