// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_DATALIST_HH__
#define __BSE_DATALIST_HH__

#include <sfi/bcore.hh>

namespace Bse {

// == DataList ==
/// DataKey objects are used to identify and manage custom data members of DataListContainer objects.
template<typename Type> class DataKey {
private:
  /*Copy*/        DataKey    (const DataKey&);
  DataKey&        operator=  (const DataKey&);
public:
  /* explicit */  DataKey    ()                 { }
  virtual Type    fallback   ()                 { Type d = Type(); return d; }  ///< Return the default value for Type.
  virtual void    destroy    (Type data)        { /* destruction hook */ }      ///< Hook invoked when @a data is deleted.
  virtual        ~DataKey    ()                 {}
};

/// Underlying storage implementation for a DataListContainer.
class DataList {
  class NodeBase {
  protected:
    NodeBase      *next;
    DataKey<void> *key;
    explicit       NodeBase (DataKey<void> *k) : next (NULL), key (k) {}
    virtual       ~NodeBase ();
    friend         class DataList;
  };
  template<typename T>
  class Node : public NodeBase {
    T data;
  public:
    T        get_data ()     { return data; }
    T        swap     (T d)  { T result = data; data = d; return result; }
    virtual ~Node()
    {
      if (key)
        {
          DataKey<T> *dkey = reinterpret_cast<DataKey<T>*> (key);
          dkey->destroy (data);
        }
    }
    explicit Node (DataKey<T> *k,
                   T           d) :
      NodeBase (reinterpret_cast<DataKey<void>*> (k)),
      data (d)
    {}
  };
  NodeBase *nodes;
  BSE_CLASS_NON_COPYABLE (DataList);
public:
  DataList() :
    nodes (NULL)
  {}
  template<typename T> void
  set (DataKey<T> *key,
       T           data)
  {
    Node<T> *node = new Node<T> (key, data);
    set_data (node);
  }
  template<typename T> T
  get (DataKey<T> *key) const
  {
    NodeBase *nb = get_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        return node->get_data();
      }
    else
      return key->fallback();
  }
  template<typename T> T
  swap (DataKey<T> *key,
        T           data)
  {
    NodeBase *nb = get_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        return node->swap (data);
      }
    else
      {
        set (key, data);
        return key->fallback();
      }
  }
  template<typename T> T
  swap (DataKey<T> *key)
  {
    NodeBase *nb = rip_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        T d = node->get_data();
        nb->key = NULL; // rip key to prevent data destruction
        delete nb;
        return d;
      }
    else
      return key->fallback();
  }
  template<typename T> void
  del (DataKey<T> *key)
  {
    NodeBase *nb = rip_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      delete nb;
  }
  void clear_like_destructor();
  ~DataList();
private:
  void      set_data (NodeBase      *node);
  NodeBase* get_data (DataKey<void> *key) const;
  NodeBase* rip_data (DataKey<void> *key);
};

/** DataListContainer - typesafe storage and retrieval of arbitrary members.
 * By using a DataKey, DataListContainer objects allow storage and retrieval of custom data members in a typesafe fashion.
 * The custom data members will initially default to DataKey::fallback and are deleted by the DataListContainer destructor.
 * Example: @snippet tests/t201/rcore-basics-datalist.cc DataListContainer-EXAMPLE
 */
class DataListContainer {
  DataList data_list;
public: /// @name Accessing custom data members
  /// Assign @a data to the custom keyed data member, deletes any previously set data.
  template<typename Type> inline void set_data    (DataKey<Type> *key, Type data) { data_list.set (key, data); }
  /// Retrieve contents of the custom keyed data member, returns DataKey::fallback if nothing was set.
  template<typename Type> inline Type get_data    (DataKey<Type> *key) const      { return data_list.get (key); }
  /// Swap @a data with the current contents of the custom keyed data member, returns the current contents.
  template<typename Type> inline Type swap_data   (DataKey<Type> *key, Type data) { return data_list.swap (key, data); }
  /// Removes and returns the current contents of the custom keyed data member without deleting it.
  template<typename Type> inline Type swap_data   (DataKey<Type> *key)            { return data_list.swap (key); }
  /// Delete the current contents of the custom keyed data member, invokes DataKey::destroy.
  template<typename Type> inline void delete_data (DataKey<Type> *key)            { data_list.del (key); }
};

} // Bse

#endif // __BSE_DATALIST_HH__
