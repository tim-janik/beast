/* SFI - Synthesis Fusion Kit Interface                 -*-mode: c++;-*-
 * Copyright (C) 2003-2004 Tim Janik and Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __SFI_CXX_H__
#define __SFI_CXX_H__

#include <sfi/sfi.h>
#include <string>
#include <new>

namespace Sfi {

typedef SfiBool   Bool;    // FIXME: use bool instead?
typedef SfiInt    Int;
typedef SfiNum    Num;
typedef SfiReal   Real;

class String {
  char *cstring;
  int cmp (const char *ostring) const
  {
    if (cstring && ostring)
      return strcmp (cstring, ostring);
    else if (cstring)
      return +SFI_MAXINT;
    else
      return ostring ? SFI_MININT : 0;
  }
public:
  String()
  {
    cstring = g_strdup ("");
  }
  String (const String &s)
  {
    cstring = g_strdup (s.cstring);
  }
  String (const std::string &s)
  {
    cstring = g_strdup (s.c_str());
  }
  String (const char *cstr)
  {
    cstring = g_strdup (cstr ? cstr : "");
  }
  String& operator= (const std::string &s)
  {
    g_free (cstring);
    cstring = g_strdup (s.c_str());
    return *this;
  }
  String& operator= (const gchar *cstr)
  {
    if (cstr != cstring)
      {
        g_free (cstring);
        cstring = g_strdup (cstr ? cstr : "");
      }
    return *this;
  }
  String& operator= (const String &s)
  {
    if (s.cstring != cstring)
      {
        g_free (cstring);
        cstring = g_strdup (s.cstring);
      }
    return *this;
  }
  const char* c_str() const
  {
    return cstring;
  }
  String& operator+= (const gchar *cstr)
  {
    char *old = cstring;
    cstring = g_strconcat (old ? old : "", cstr, NULL);
    g_free (old);
    return *this;
  }
  String& operator+= (const String &src)
  {
    char *old = cstring;
    cstring = g_strconcat (old ? old : "", src.cstring, NULL);
    g_free (old);
    return *this;
  }
  String& operator+= (const std::string &src)
  {
    char *old = cstring;
    cstring = g_strconcat (old ? old : "", src.c_str(), NULL);
    g_free (old);
    return *this;
  }
  String operator+ (const gchar *cstr)
  {
    return String (cstring) += cstr;
  }
  String operator+ (const String &src)
  {
    return String (cstring) += src;
  }
  String operator+ (const std::string &src)
  {
    return String (cstring) += src;
  }
  bool operator<  (const char *src) const { return cmp (src) < 0; }
  bool operator<= (const char *src) const { return cmp (src) <= 0; }
  bool operator>  (const char *src) const { return cmp (src) > 0; }
  bool operator>= (const char *src) const { return cmp (src) >= 0; }
  bool operator!= (const char *src) const { return cmp (src) != 0; }
  bool operator== (const char *src) const { return cmp (src) == 0; }
  bool operator<  (const String &s) const { return cmp (s.cstring) < 0; }
  bool operator<= (const String &s) const { return cmp (s.cstring) <= 0; }
  bool operator>  (const String &s) const { return cmp (s.cstring) > 0; }
  bool operator>= (const String &s) const { return cmp (s.cstring) >= 0; }
  bool operator!= (const String &s) const { return cmp (s.cstring) != 0; }
  bool operator== (const String &s) const { return cmp (s.cstring) == 0; }
  bool operator<  (const std::string &s) const { return cmp (s.c_str()) < 0; }
  bool operator<= (const std::string &s) const { return cmp (s.c_str()) <= 0; }
  bool operator>  (const std::string &s) const { return cmp (s.c_str()) > 0; }
  bool operator>= (const std::string &s) const { return cmp (s.c_str()) >= 0; }
  bool operator!= (const std::string &s) const { return cmp (s.c_str()) != 0; }
  bool operator== (const std::string &s) const { return cmp (s.c_str()) == 0; }
  unsigned int length()
  {
    return cstring ? strlen (cstring) : 0;
  }
  ~String()
  {
    g_free (cstring);
  }
  /* provide GValue accessors */
  static String value_get (const GValue *value)
  {
    return sfi_value_get_string (value);
  }
  static void value_set (GValue       *value,
                         const String& str)
  {
    sfi_value_set_string (value, str.c_str());
  }
};

struct GNewable {
  gpointer operator new (size_t s)
  {
    return g_malloc0 (s);
  }
  void operator delete (gpointer mem)
  {
    g_free (mem);
  }
  gpointer operator new[] (size_t s)
  {
    return g_malloc0 (s);
  }
  void operator delete[] (gpointer mem)
  {
    g_free (mem);
  }
};

typedef enum {
  INIT_NULL,
  INIT_EMPTY,
  INIT_DEFAULT,
} InitializationType;

template<typename Type>
class RecordHandle {
  Type *record;
public:
  RecordHandle (InitializationType t = INIT_NULL)
  {
    record = NULL;
    if (t == INIT_DEFAULT || t == INIT_EMPTY)
      record = new Type();
  }
  RecordHandle (const RecordHandle &rh)
  {
    if (rh.record)
      record = new Type (*rh.record);
    else
      record = NULL;
  }
  RecordHandle (const Type &rec)
  {
    record = new Type (rec);
  }
  RecordHandle& operator= (const Type &rec)
  {
    if (record != &rec)
      {
        delete record;
        record = new Type (rec);
      }
    return *this;
  }
  void take (Type *rec)
  {
    delete record;
    record = rec;
  }
  Type* steal ()
  {
    Type *t = record;
    record = NULL;
    return t;
  }
  RecordHandle& operator= (const RecordHandle &src)
  {
    if (record != src.record)
      {
        delete record;
        if (src.record)
          record = new Type (*src.record);
        else
          record = NULL;
      }
    return *this;
  }
  Type* c_ptr() const
  {
    return record;
  }
  ~RecordHandle()
  {
    delete record;
  }
  Type* operator-> () const
  {
    return record;
  }
  Type& operator* ()
  {
    return *record;
  }
  Type& operator[] (unsigned int index)
  {
    if (index)
      g_critical ("%s: invalid array subscript: %u", G_STRFUNC, index);
    return *(index ? NULL : record);
  }
  bool is_null() const
  {
    return !record;
  }
  operator bool () const
  {
    return !is_null();
  }
};

template<typename Type>
class Sequence {
public:
  typedef Type ElementType;
  struct CSeq {
    unsigned int n_elements;
    Type        *elements;
  };
private:
  CSeq *cseq;
public:
  Sequence (unsigned int n = 0)
  {
    cseq = g_new0 (CSeq, 1);
    resize (n);
  }
  Sequence (const Sequence &sh)
  {
    cseq = g_new0 (CSeq, 1);
    *this = sh;
  }
  void take (CSeq *cs)
  {
    resize (0);
    if (cs)
      {
        g_free (cseq->elements);
        g_free (cseq);
        cseq = cs;
        /* a take(); steal(); sequence needs to preserve pointer */
      }
  }
  CSeq* steal ()
  {
    CSeq *cs = cseq;
    cseq = g_new0 (CSeq, 1);
    resize (0);
    /* a take(); steal(); sequence needs to preserve pointer */
    return cs;
  }
  void resize (unsigned int n)
  {
    guint i;
    // Note that this does *not* use an explicit copy-constructor call to relocate existing elements
    for (i = n; i < length(); i++)
      cseq->elements[i].~Type();
    i = cseq->n_elements;
    cseq->n_elements = n;
    cseq->elements = g_renew (Type, cseq->elements, cseq->n_elements);
    for (; i < length(); i++)
      new (cseq->elements + i) Type ();
  }
  Type& operator[] (unsigned int index)
  {
    if (index >= cseq->n_elements)
      g_critical ("%s: invalid array subscript: %u", G_STRFUNC, index);
    return cseq->elements[index];
  }
  const Type& operator[] (unsigned int index) const
  {
    if (index >= cseq->n_elements)
      g_critical ("%s: invalid array subscript: %u", G_STRFUNC, index);
    return cseq->elements[index];
  }
  Sequence& operator+= (const Type &elm)
  {
    // Note that this does *not* use an explicit copy-constructor call to relocate existing elements
    guint i = cseq->n_elements++;
    cseq->elements = g_renew (Type, cseq->elements, cseq->n_elements);
    new (cseq->elements + i) Type (elm);
    return *this;
  }
  Sequence& operator= (const Sequence &sh)
  {
    if (cseq != sh.cseq)
      {
        resize (0);
        cseq->n_elements = sh.length();
        cseq->elements = g_renew (Type, cseq->elements, cseq->n_elements);
        for (guint i = 0; i < length(); i++)
          new (cseq->elements + i) Type (sh[i]);
      }
    return *this;
  }
  unsigned int length() const
  {
    return cseq->n_elements;
  }
  ~Sequence()
  {
    resize (0);
    g_free (cseq->elements);
    g_free (cseq);
  }
};

class FBlock {
  SfiFBlock *block;
public:
  FBlock (unsigned int length = 0)
  {
    block = sfi_fblock_new_sized (length);
  }
  FBlock (SfiFBlock &fblock)
  {
    block = NULL;
    *this = fblock;
  }
  FBlock (unsigned int length,
          const float *values)
  {
    block = sfi_fblock_new();
    sfi_fblock_append (block, length, values);
  }
  FBlock (const FBlock &fb)
  {
    if (fb.block)
      block = sfi_fblock_ref (fb.block);
    else
      block = sfi_fblock_new();
  }
  FBlock& operator= (SfiFBlock &fb)
  {
    if (block != &fb)
      {
        if (block)
          sfi_fblock_unref (block);
        block = &fb;
        if (block)
          sfi_fblock_ref (block);
      }
    return *this;
  }
  FBlock& operator= (const FBlock &s)
  {
    if (block != s.block)
      {
        if (block)
          sfi_fblock_unref (block);
        block = s.block;
        if (block)
          sfi_fblock_ref (block);
      }
    return *this;
  }
  SfiFBlock* fblock()
  {
    return block;
  }
  ~FBlock()
  {
    if (block)
      sfi_fblock_unref (block);
  }
  void ref ()
  {
    if (block)
      sfi_fblock_ref (block);
    else
      block = sfi_fblock_new();
  }
  void unref ()
  {
    g_return_if_fail (block != NULL && block->ref_count > 0);
    sfi_fblock_unref (block);
  }
  void resize (unsigned int length)
  {
    if (block)
      sfi_fblock_resize (block, length);
    else
      block = sfi_fblock_new_sized (length);
  }
  void take (SfiFBlock *fb)
  {
    if (block)
      sfi_fblock_unref (block);
    block = fb;
  }
  FBlock copy_deep()
  {
    if (block)
      return FBlock (block->n_values, block->values);
    else
      return FBlock (0);
  }
  FBlock copy_shallow()
  {
    return FBlock (*this);
  }
  void append (unsigned int length,
               const float *values)
  {
    if (!block)
      block = sfi_fblock_new();
    sfi_fblock_append (block, length, values);
  }
  void append (float f)
  {
    append (1, &f);
  }
  unsigned int length()
  {
    return block ? block->n_values : 0;
  }
  const float* get()
  {
    return block ? block->values : NULL;
  }
  static FBlock value_get (const GValue *value)
  {
    SfiFBlock *fb = sfi_value_get_fblock (value);
    FBlock self (0);
    if (fb)
      self.take (sfi_fblock_ref (fb));
    return self;
  }
  static void value_set (GValue       *value,
                         const FBlock &self)
  {
    sfi_value_set_fblock (value, self.block);
  }
};

class BBlock {
  SfiBBlock *block;
public:
  BBlock (unsigned int length = 0)
  {
    block = sfi_bblock_new_sized (length);
  }
  BBlock (SfiBBlock &bblock)
  {
    block = NULL;
    *this = bblock;
  }
  BBlock (unsigned int  length,
          const guint8 *bytes)
  {
    block = sfi_bblock_new();
    sfi_bblock_append (block, length, bytes);
  }
  BBlock (const BBlock &bb)
  {
    if (bb.block)
      block = sfi_bblock_ref (bb.block);
    else
      block = sfi_bblock_new();
  }
  BBlock& operator= (SfiBBlock &bb)
  {
    if (block != &bb)
      {
        if (block)
          sfi_bblock_unref (block);
        block = &bb;
        if (block)
          sfi_bblock_ref (block);
      }
    return *this;
  }
  BBlock& operator= (const BBlock &s)
  {
    if (block != s.block)
      {
        if (block)
          sfi_bblock_unref (block);
        block = s.block;
        if (block)
          sfi_bblock_ref (block);
      }
    return *this;
  }
  SfiBBlock* bblock()
  {
    return block;
  }
  ~BBlock()
  {
    if (block)
      sfi_bblock_unref (block);
  }
  void ref ()
  {
    if (block)
      sfi_bblock_ref (block);
    else
      block = sfi_bblock_new();
  }
  void unref ()
  {
    g_return_if_fail (block != NULL && block->ref_count > 0);
    sfi_bblock_unref (block);
  }
  void resize (unsigned int length)
  {
    if (block)
      sfi_bblock_resize (block, length);
    else
      block = sfi_bblock_new_sized (length);
  }
  void take (SfiBBlock *bb)
  {
    if (block)
      sfi_bblock_unref (block);
    block = bb;
  }
  BBlock copy_deep()
  {
    if (block)
      return BBlock (block->n_bytes, block->bytes);
    else
      return BBlock (0);
  }
  BBlock copy_shallow()
  {
    return BBlock (*this);
  }
  void append (unsigned int  length,
               const guint8 *bytes)
  {
    if (!block)
      block = sfi_bblock_new();
    sfi_bblock_append (block, length, bytes);
  }
  void append (guint8 b)
  {
    append (1, &b);
  }
  unsigned int length()
  {
    return block ? block->n_bytes : 0;
  }
  const guint8* get()
  {
    return block ? block->bytes : NULL;
  }
  static BBlock value_get (const GValue *value)
  {
    SfiBBlock *bb = sfi_value_get_bblock (value);
    BBlock self (0);
    if (bb)
      self.take (sfi_bblock_ref (bb));
    return self;
  }
  static void value_set (GValue       *value,
                         const BBlock &self)
  {
    sfi_value_set_bblock (value, self.block);
  }
};

class Rec {
  SfiRec *crec;
public:
  Rec ()
  {
    crec = NULL;
  }
  Rec (SfiRec &sr)
  {
    crec = NULL;
    *this = sr;
  }
  Rec (const Rec &rr)
  {
    if (rr.crec)
      crec = sfi_rec_ref (rr.crec);
    else
      crec = NULL;
  }
  void clear ()
  {
    if (crec)
      sfi_rec_clear (crec);
  }
  Rec& operator= (SfiRec &sr)
  {
    if (crec != &sr)
      {
        if (crec)
          sfi_rec_unref (crec);
        crec = &sr;
        if (crec)
          sfi_rec_ref (crec);
      }
    return *this;
  }
  Rec& operator= (const Rec &rr)
  {
    if (crec != rr.crec)
      {
        if (crec)
          sfi_rec_unref (crec);
        crec = rr.crec;
        if (crec)
          sfi_rec_ref (crec);
      }
    return *this;
  }
  SfiRec* rec()
  {
    return crec;
  }
  ~Rec()
  {
    if (crec)
      sfi_rec_unref (crec);
  }
  void ref ()
  {
    if (crec)
      sfi_rec_ref (crec);
    else
      crec = sfi_rec_new();
  }
  void unref ()
  {
    g_return_if_fail (crec != NULL && crec->ref_count > 0);
    sfi_rec_unref (crec);
  }
  void take (SfiRec *sr)
  {
    if (crec)
      sfi_rec_unref (crec);
    crec = sr;
  }
  Rec copy_deep()
  {
    if (crec)
      return Rec (*sfi_rec_copy_deep (crec));
    else
      return Rec ();
  }
  Rec copy_shallow()
  {
    return Rec (*this);
  }
  void set (const gchar     *field_name,
            const GValue    *value)
  {
    if (!crec)
      crec = sfi_rec_new();
    sfi_rec_set (crec, field_name, value);
  }
  unsigned int length()
  {
    return crec ? crec->n_fields : 0;
  }
  GValue* get (const gchar *name)
  {
    return crec ? sfi_rec_get (crec, name) : NULL;
  }
  static Rec value_get (const GValue *value)
  {
    SfiRec *sr = sfi_value_get_rec (value);
    Rec self;
    if (sr)
      self.take (sfi_rec_ref (sr));
    return self;
  }
  static void value_set (GValue       *value,
                         const Rec &self)
  {
    sfi_value_set_rec (value, self.crec);
  }
};

class ObjectHandle {
  GObject *cobj;
public:
  ObjectHandle ()
  {
    cobj = NULL;
  }
  ObjectHandle (GObject &object)
  {
    cobj = NULL;
    *this = object;
  }
  ObjectHandle (const ObjectHandle &oh)
  {
    if (oh.cobj)
      cobj = (GObject*) g_object_ref (oh.cobj);
    else
      cobj = NULL;
  }
  ~ObjectHandle()
  {
    if (cobj)
      g_object_unref (cobj);
  }
  ObjectHandle& operator= (GObject &object)
  {
    if (cobj != &object)
      {
        if (cobj)
          g_object_unref (cobj);
        cobj = &object;
        if (cobj)
          g_object_ref (cobj);
      }
    return *this;
  }
  ObjectHandle& operator= (const ObjectHandle &oh)
  {
    if (cobj != oh.cobj)
      {
        if (cobj)
          g_object_unref (cobj);
        cobj = oh.cobj;
        if (cobj)
          g_object_ref (cobj);
      }
    return *this;
  }
  GObject* object()
  {
    return cobj;
  }
  void ref ()
  {
    g_return_if_fail (cobj != NULL && cobj->ref_count > 0);
    g_object_ref (cobj);
  }
  void unref ()
  {
    g_return_if_fail (cobj != NULL && cobj->ref_count > 0);
    g_object_unref (cobj);
  }
  void take (GObject *object)
  {
    if (cobj)
      g_object_unref (cobj);
    cobj = object;
  }
  ObjectHandle copy_deep()
  {
    if (cobj)
      return ObjectHandle (*cobj);
    else
      return ObjectHandle ();
  }
  ObjectHandle copy_shallow()
  {
    return ObjectHandle (*this);
  }
  static ObjectHandle value_get (const GValue *value)
  {
    GObject *object = (GObject*) g_value_get_object (value);
    ObjectHandle self;
    if (object)
      self.take ((GObject*) g_object_ref (object));
    return self;
  }
  static void value_set (GValue             *value,
                         const ObjectHandle &self)
  {
    g_value_set_object (value, self.cobj);
  }
};


template<typename Type> gpointer
cxx_boxed_copy (gpointer data)
{
  if (data)
    {
      Type *t = reinterpret_cast<Type*> (data);
      Type *d = new Type (*t);
      return d;
    }
  return NULL;
}

template<typename Type> void
cxx_boxed_free (gpointer data)
{
  if (data)
    {
      Type *t = reinterpret_cast<Type*> (data);
      delete t;
    }
}

template<typename Type> void
cxx_boxed_to_rec (const GValue *src_value,
                  GValue       *dest_value)
{
  SfiRec *rec = NULL;
  gpointer boxed = g_value_get_boxed (src_value);
  if (boxed)
    {
      Type *t = reinterpret_cast<Type*> (boxed);
      rec = Type::to_rec (*t);
    }
  sfi_value_take_rec (dest_value, rec);
}

template<typename Type> void
cxx_boxed_from_rec (const GValue *src_value,
                    GValue       *dest_value)
{
  gpointer boxed = NULL;
  SfiRec *rec = sfi_value_get_rec (src_value);
  if (rec)
    {
      RecordHandle<Type> rh = Type::from_rec (rec);
      Type *t = rh.steal();
      boxed = t;
    }
  g_value_set_boxed_take_ownership (dest_value, boxed);
}

template<typename SeqType> void
cxx_boxed_to_seq (const GValue *src_value,
                  GValue       *dest_value)
{
  SfiSeq *seq = NULL;
  gpointer boxed = g_value_get_boxed (src_value);
  if (boxed)
    {
      typename SeqType::CSeq *t = reinterpret_cast<typename SeqType::CSeq*> (boxed);
      SeqType cxxseq;
      cxxseq.take(t);   /* temporarily re-own */
      seq = SeqType::to_seq (cxxseq);
      cxxseq.steal();   /* get back */
    }
  sfi_value_take_seq (dest_value, seq);
}

template<typename SeqType> void
cxx_boxed_from_seq (const GValue *src_value,
                    GValue       *dest_value)
{
  gpointer boxed = NULL;
  SfiSeq *seq = sfi_value_get_seq (src_value);
  if (seq)
    {
      SeqType sh = SeqType::from_seq (seq);
      typename SeqType::CSeq *t = sh.steal();
      boxed = t;
    }
  g_value_set_boxed_take_ownership (dest_value, boxed);
}

template<typename Type> RecordHandle<Type>
cxx_value_get_record (const GValue *value)
{
  SfiRec *rec = sfi_value_get_rec (value);
  if (rec)
    return Type::from_rec (rec);
  else
    return INIT_NULL;
}

template<typename Type> void
cxx_value_set_record (GValue                   *value,
                      const RecordHandle<Type> &self)
{
  if (self)
    sfi_value_take_rec (value, Type::to_rec (self));
  else
    sfi_value_set_rec (value, NULL);
}

template<typename SeqType> SeqType
cxx_value_get_sequence (const GValue *value)
{
  SfiSeq *seq = sfi_value_get_seq (value);
  if (seq)
    return SeqType::from_seq (seq);
  else
    return SeqType();
}

template<typename SeqType> void
cxx_value_set_sequence (GValue        *value,
                        const SeqType &self)
{
  sfi_value_take_seq (value, SeqType::to_seq (self));
}

} // Sfi

#endif /* __SFI_CXX_H__ */

/* vim:set ts=8 sts=2 sw=2: */
