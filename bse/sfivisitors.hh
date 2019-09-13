// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_VISITORS_HH__
#define __SFI_VISITORS_HH__

#include <bse/sfiparams.hh>

namespace Bse { // BseCore

// == Compatibility Imports ==
using Aida::VisitorDispatcher;
using Aida::StdVectorValueHandle;


template<class Visitable> void                            sfi_rec_to_visitable                      (SfiRec *rec, Visitable &visitable);
template<class Visitable> SfiRec*                         sfi_rec_new_from_visitable                (Visitable &visitable);
template<class Visitable> void                            sfi_seq_to_visitable                      (SfiSeq *seq, Visitable &visitable);
template<class Visitable> SfiSeq*                         sfi_seq_new_from_visitable                (Visitable &visitable);
template<class Visitable> SfiRecFields                    sfi_pspecs_rec_fields_from_visitable      (Visitable &visitable);
template<class Visitable> GParamSpec*                     sfi_pspec_seq_field_from_visitable        (Visitable &visitable);
template<class Visitable> const std::vector<GParamSpec*>& sfi_pspecs_fields_from_accessor_visitable (Visitable &visitable);
bool sfi_pspecs_rec_fields_cache (const std::type_info &type_info, SfiRecFields *rf, bool assign = false); // internal
bool sfi_pspecs_seq_field_cache  (const std::type_info &type_info, GParamSpec  **pp, bool assign = false); // internal
bool sfi_pspecs_acs_fields_cache (const std::type_info &type_info, std::vector<GParamSpec*>**, bool assign = false); // internal

class PspecVisitor : public Bse::VisitorDispatcher<PspecVisitor> {
  std::vector<GParamSpec*> &pspecs_;
  std::vector<String>       aux_;
  String get_min (Name name, Name fallback) { return get_aux (name, "min", fallback); }
  String get_max (Name name, Name fallback) { return get_aux (name, "max", fallback); }
  String get_step  (Name name)   { return get_aux (name, "step"); }
  String get_dflt  (Name name)   { return get_aux (name, "default"); }
  String get_label (Name name)   { return get_aux (name, "label"); }
  String get_blurb (Name name)   { return get_aux (name, "blurb"); }
  String get_hints (Name name)   { return get_aux (name, "hints"); }
  String get_aux (const char *field, const char *key, const char *fallback = "")
  {
    const String name = String() + field + "." + key + "=";
    for (const String &kv : aux_)
      if (name.compare (0, name.size(), kv, 0, name.size()) == 0)
        return kv.substr (name.size());
    return fallback;
  }
  void
  add_group (Name name, GParamSpec *pspec)
  {
    const String group = get_aux (name, "group");
    if (!group.empty())
      sfi_pspec_set_group (pspec, group.c_str());
  }
public:
  PspecVisitor (std::vector<GParamSpec*> &pspecs, const std::vector<String> &aux_data) : pspecs_ (pspecs), aux_ (aux_data) {}
  template<class A> void
  visit_bool (A &a, Name name)
  {
    const bool dfl = string_to_bool (get_dflt (name));
    GParamSpec *pspec = sfi_pspec_bool (name, get_label (name).c_str(), get_blurb (name).c_str(), dfl, get_hints (name).c_str());
    add_group (name, pspec);
    pspecs_.push_back (pspec);
  }
  template<class A> void
  visit_integral (A &a, Name name)
  {
    const int64 dfl = string_to_int (get_dflt (name));
    const int64 mi  = string_to_int (get_min (name, "-9223372036854775808"));
    const int64 ma  = string_to_int (get_max (name, "+9223372036854775807"));
    const int64 ms  = string_to_int (get_step (name));
    GParamSpec *pspec = sfi_pspec_num (name, get_label (name).c_str(), get_blurb (name).c_str(), dfl, mi, ma, ms, get_hints (name).c_str());
    add_group (name, pspec);
    pspecs_.push_back (pspec);
  }
  template<class A> void
  visit_float (A &a, Name name)
  {
    const double dfl = string_to_double (get_dflt (name));
    const double mi  = string_to_double (get_min (name, "-1.79769313486231570815e+308"));
    const double ma  = string_to_double (get_max (name, "+1.79769313486231570815e+308"));
    const double ms  = string_to_double (get_step (name));
    GParamSpec *pspec = sfi_pspec_real (name, get_label (name).c_str(), get_blurb (name).c_str(), dfl, mi, ma, ms, get_hints (name).c_str());
    add_group (name, pspec);
    pspecs_.push_back (pspec);
  }
  void
  visit_string (::std::string &a, Name name)
  {
    GParamSpec *pspec = sfi_pspec_string (name, get_label (name).c_str(), get_blurb (name).c_str(),
                                          get_dflt (name).c_str(), get_hints (name).c_str());
    add_group (name, pspec);
    pspecs_.push_back (pspec);
  }
  template<class A> void
  visit_enum (A &a, Name name)
  {
#if 0 // FIXME: broken
    GParamSpec *pspec = sfi_pspec_choice (name, get_label (name).c_str(), get_blurb (name).c_str(), get_dflt (name).c_str(),
                                          Bse::choice_values_from_enum<A>(), get_hints (name).c_str());
    add_group (name, pspec);
    pspecs_.push_back (pspec);
#endif
  }
  template<class A> void
  visit_visitable (A &a, Name name)
  {
    SfiRecFields recfields = sfi_pspecs_rec_fields_from_visitable (a);
    GParamSpec *pspec = sfi_pspec_rec (name, get_label (name).c_str(), get_blurb (name).c_str(), recfields, get_hints (name).c_str());
    add_group (name, pspec);
    pspecs_.push_back (pspec);
  }
  template<class SeqA> void
  visit_vector (SeqA &a, Name name)
  { // Note, if A=bool, SeqA is *derived* from std::vector<bool>, so StdVectorValueHandle<> doesn't match
    GParamSpec *pspec = sfi_pspec_seq_field_from_visitable (a); // this needs the real SeqA type
    if (pspec)
      {
        GParamSpec *seq_pspec = sfi_pspec_seq (name, get_label (name).c_str(), get_blurb (name).c_str(), pspec, get_hints (name).c_str());
        add_group (name, seq_pspec);
        pspecs_.push_back (seq_pspec);
      }
  }
  template<class A> void
  visit_class (A &a, Name name)
  {
    GParamSpec *pspec = sfi_pspec_proxy (name, get_label (name).c_str(), get_blurb (name).c_str(), get_hints (name).c_str());
    add_group (name, pspec);
    pspecs_.push_back (pspec);
  }
};

class ToRecVisitor : public Bse::VisitorDispatcher<ToRecVisitor> {
  SfiRec *rec_;
public:
  ToRecVisitor (SfiRec *rec) : rec_ (rec) {}
  template<class A> void
  visit_bool (A &a, Name name)
  {
    sfi_rec_set_bool (rec_, name, a);
  }
  template<class A> void
  visit_integral (A &a, Name name)
  {
    sfi_rec_set_num (rec_, name, a);
  }
  template<class A> void
  visit_float (A &a, Name name)
  {
    sfi_rec_set_real (rec_, name, a);
  }
  void
  visit_string (::std::string &a, Name name)
  {
    sfi_rec_set_string (rec_, name, a.c_str());
  }
  template<class A> void
  visit_enum (A &a, Name name)
  {
#if 0 // FIXME: broken
    sfi_rec_set_choice (rec_, name, Aida::enum_info<A>().value_to_string (a).c_str());
#endif
  }
  template<class SeqA> void
  visit_vector (SeqA &a, Name name)
  { // Note, if A=bool, SeqA is *derived* from std::vector<bool>, so StdVectorValueHandle<> doesn't match
    SfiSeq *field_seq = sfi_seq_new_from_visitable (a);
    sfi_rec_set_seq (rec_, name, field_seq);
    sfi_seq_unref (field_seq);
  }
  template<class A> void
  visit_visitable (A &a, Name name)
  {
    SfiRec *field_rec = sfi_rec_new();
    ToRecVisitor rec_visitor (field_rec);
    a.__visit__ (rec_visitor);
    sfi_rec_set_rec (rec_, name, field_rec);
    sfi_rec_unref (field_rec);
  }
  template<class A> void
  visit_class (A &a, Name name)
  {
    sfi_rec_set_proxy (rec_, name, a);
  }
};

class FromRecVisitor : public Bse::VisitorDispatcher<FromRecVisitor> {
  SfiRec *rec_;
public:
  FromRecVisitor (SfiRec *rec) : rec_ (rec) {}
  template<class A> void
  visit_bool (A &a, Name name)
  {
    a = sfi_rec_get_bool (rec_, name);
  }
  template<class A> void
  visit_integral (A &a, Name name)
  {
    a = sfi_rec_get_num (rec_, name);
  }
  template<class A> void
  visit_float (A &a, Name name)
  {
    a = sfi_rec_get_real (rec_, name);
  }
  void
  visit_string (::std::string &a, Name name)
  {
    const char *s = sfi_rec_get_string (rec_, name);
    a = s ? s : "";
  }
  template<class A> void
  visit_enum (A &a, Name name)
  {
    const char *c = sfi_rec_get_choice (rec_, name);
    a = !c ? (A) 0 : Aida::enum_value_from_string<A>(c);
  }
  template<class SeqA> void
  visit_vector (SeqA &a, Name name)
  { // Note, if A=bool, SeqA is *derived* from std::vector<bool>, so StdVectorValueHandle<> doesn't match
    SfiSeq *field_seq = sfi_rec_get_seq (rec_, name);
    sfi_seq_to_visitable (field_seq, a);
  }
  template<class A> void
  visit_visitable (A &a, Name name)
  {
    SfiRec *field_rec = sfi_rec_get_rec (rec_, name);
    if (field_rec)
      {
        FromRecVisitor rec_visitor (field_rec);
        a.__visit__ (rec_visitor);
      }
  }
  template<class A> void
  visit_class (A &a, Name name)
  {
    a = sfi_rec_get_proxy (rec_, name);
  }
};

template<class Visitable> void
sfi_seq_to_visitable (SfiSeq *seq, Visitable &visitable)
{
  if (!seq)
    {
      visitable.resize (0);
      return;
    }
  const size_t n = sfi_seq_length (seq);
  visitable.resize (n);
  SfiRec *tmp_rec = sfi_rec_new();
  for (size_t i = 0; i < n; i++)
    {
      sfi_rec_set (tmp_rec, "seqelement", sfi_seq_get (seq, i));
      FromRecVisitor rec_visitor (tmp_rec);
      typedef typename Visitable::value_type A; // assumes Visitable derives std::vector
      typename Bse::StdVectorValueHandle<::std::vector<A>>::type value_handle = visitable[i];
      rec_visitor (value_handle, "seqelement");
      if (Bse::StdVectorValueHandle<::std::vector<A>>::value) // copy-by-value
        visitable[i] = value_handle;
    }
  sfi_rec_unref (tmp_rec);
}

template<class Visitable> SfiSeq*
sfi_seq_new_from_visitable (Visitable &visitable)
{
  SfiSeq *seq = sfi_seq_new();
  SfiRec *tmp_rec = sfi_rec_new();
  for (size_t i = 0; i < visitable.size(); i++)
    {
      ToRecVisitor rec_visitor (tmp_rec);
      typedef typename Visitable::value_type A; // assumes Visitable derives std::vector
      typename Bse::StdVectorValueHandle<::std::vector<A>>::type value_handle = visitable[i];
      rec_visitor (value_handle, "seqelement");
      if (Bse::StdVectorValueHandle<::std::vector<A>>::value) // copy-by-value
        visitable[i] = value_handle;
      GValue *element = sfi_rec_get (tmp_rec, "seqelement");
      if (element)
        {
          sfi_seq_append (seq, element);
          sfi_rec_clear (tmp_rec);
        }
      else
        break;
    }
  sfi_rec_unref (tmp_rec);
  return seq;
}

template<class Visitable> SfiRec*
sfi_rec_new_from_visitable (Visitable &visitable)
{
  SfiRec *rec = sfi_rec_new();
  ToRecVisitor rec_visitor (rec);
  visitable.__visit__ (rec_visitor);
  return rec;
}

template<class Visitable> void
sfi_rec_to_visitable (SfiRec *rec, Visitable &visitable)
{
  FromRecVisitor rec_visitor (rec);
  visitable.__visit__ (rec_visitor);
}

template<class Visitable> SfiRecFields
sfi_pspecs_rec_fields_from_visitable (Visitable &visitable)
{
  SfiRecFields rec_fields;
  if (sfi_pspecs_rec_fields_cache (typeid (Visitable), &rec_fields))
    return rec_fields;
  std::vector<GParamSpec*> pspecs;
  PspecVisitor pspec_visitor (pspecs, visitable.__typedata__());
  visitable.__visit__ (pspec_visitor);
  rec_fields.n_fields = pspecs.size();
  rec_fields.fields = g_new0 (GParamSpec*, rec_fields.n_fields);
  for (size_t i = 0; i < rec_fields.n_fields; i++)
    {
      g_param_spec_ref (pspecs[i]);
      g_param_spec_sink (pspecs[i]);
      rec_fields.fields[i] = pspecs[i];
    }
  sfi_pspecs_rec_fields_cache (typeid (Visitable), &rec_fields, true);
  return rec_fields;
}

template<class Visitable> GParamSpec*
sfi_pspec_seq_field_from_visitable (Visitable &visitable)
{
  GParamSpec *pspec = NULL;
  if (sfi_pspecs_seq_field_cache (typeid (Visitable), &pspec))
    return pspec;
  std::vector<GParamSpec*> pspecs;
  PspecVisitor pspec_visitor (pspecs, visitable.__typedata__());
  typedef typename Visitable::value_type A;
  A example_element = A();
  pspec_visitor (example_element, "seqelement");
  if (pspecs.size() == 1)
    {
      pspec = pspecs[0];
      g_param_spec_ref (pspec);
      g_param_spec_sink (pspec);
      sfi_pspecs_seq_field_cache (typeid (Visitable), &pspec, true);
    }
  for (size_t i = 0; i < pspecs.size(); i++)
    {
      g_param_spec_ref (pspecs[i]);
      g_param_spec_sink (pspecs[i]);
      g_param_spec_unref (pspecs[i]);
    }
  return pspec;
}

template<class Visitable> const std::vector<GParamSpec*>&
sfi_pspecs_fields_from_accessor_visitable (Visitable &visitable)
{
  std::vector<GParamSpec*> *pspecsp = NULL;
  if (sfi_pspecs_acs_fields_cache (typeid (Visitable), &pspecsp))
    return *pspecsp;
  std::vector<GParamSpec*> pspecs;
  PspecVisitor pspec_visitor (pspecs, visitable.__typedata__());
  visitable.__accept_accessor__ (pspec_visitor);
  for (size_t i = 0; i < pspecs.size(); i++)
    {
      g_param_spec_ref (pspecs[i]);
      g_param_spec_sink (pspecs[i]);
    }
  pspecsp = &pspecs;
  sfi_pspecs_acs_fields_cache (typeid (Visitable), &pspecsp, true);
  pspecsp = NULL;
  bool success = sfi_pspecs_acs_fields_cache (typeid (Visitable), &pspecsp);
  BSE_ASSERT_RETURN (success && pspecsp, *pspecsp);
  return *pspecsp;
}

} // Bse

#endif // __SFI_VISITORS_HH__
