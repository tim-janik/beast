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
typedef SfiBBlock BBlock;
typedef SfiFBlock FBlock;
typedef SfiRec    Rec;

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
    g_free (cstring);
    cstring = g_strdup (cstr ? cstr : "");
    return *this;
  }
  String& operator= (const String &s)
  {
    g_free (cstring);
    cstring = g_strdup (s.cstring);
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
    delete record;
    record = new Type (rec);
    return *this;
  }
  void take (Type *rec)
  {
    delete record;
    record = rec;
  }
  RecordHandle& operator= (const RecordHandle &src)
  {
    delete record;
    if (src.record)
      record = new Type (*src.record);
    else
      record = NULL;
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
    return *operator-> ();
  }
};

template<typename Type>
class Sequence {
  struct CSeq {
    unsigned int n_elements;
    Type        *elements;
  };
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
    for (guint i = 0; i < length(); i++)
      cseq->elements[i].~Type();
    cseq->n_elements = sh->length();
    cseq->elements = g_renew (Type, cseq->elements, cseq->n_elements);
    for (guint i = 0; i < length(); i++)
      new (cseq->elements + i) Type (sh[i]);
    return *this;
  }
  unsigned int length() const
  {
    return cseq->n_elements;
  }
  ~Sequence()
  {
    for (guint i = 0; i < length(); i++)
      cseq->elements[i].~Type();
    g_free (cseq->elements);
    g_free (cseq);
  }
};

} // Sfi

/* extending sfi_value functions to C++ types */
inline Sfi::String sfi_value_get_cxxstring (const GValue *value) {
  return sfi_value_get_string (value);
}

inline void sfi_value_set_cxxstring (GValue *value, const Sfi::String& string) {
  sfi_value_set_string (value, string.c_str());
}

#endif /* __SFI_CXX_H__ */

/* vim:set ts=8 sts=2 sw=2: */
