// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#include "bsefilter.hh"
#include <sfi/sfi.hh>

using namespace Bse;

const gchar*
bse_iir_filter_kind_string (BseIIRFilterKind fkind)
{
  switch (fkind)
    {
    case BSE_IIR_FILTER_BUTTERWORTH:    return "Butterworth";
    case BSE_IIR_FILTER_BESSEL:         return "Bessel";
    case BSE_IIR_FILTER_CHEBYSHEV1:     return "Chebyshev1";
    case BSE_IIR_FILTER_CHEBYSHEV2:     return "Chebyshev2";
    case BSE_IIR_FILTER_ELLIPTIC:       return "Elliptic";
    default:                            return "?unknown?";
    }
}

const gchar*
bse_iir_filter_type_string (BseIIRFilterType ftype)
{
  switch (ftype)
    {
    case BSE_IIR_FILTER_LOW_PASS:       return "Low-pass";
    case BSE_IIR_FILTER_BAND_PASS:      return "Band-pass";
    case BSE_IIR_FILTER_HIGH_PASS:      return "High-pass";
    case BSE_IIR_FILTER_BAND_STOP:      return "Band-stop";
    default:                            return "?unknown?";
    }
}

gchar*
bse_iir_filter_request_string (const BseIIRFilterRequest *ifr)
{
  String s;
  s += bse_iir_filter_kind_string (ifr->kind);
  s += " ";
  s += bse_iir_filter_type_string (ifr->type);
  s += " order=" + string_from_int (ifr->order);
  s += " sample-rate=" + string_from_float (ifr->sampling_frequency);
  if (ifr->kind == BSE_IIR_FILTER_CHEBYSHEV1 || ifr->kind == BSE_IIR_FILTER_ELLIPTIC)
    s += " passband-ripple-db=" + string_from_float (ifr->passband_ripple_db);
  s += " passband-edge=" + string_from_float (ifr->passband_edge);
  if (ifr->type == BSE_IIR_FILTER_BAND_PASS || ifr->type == BSE_IIR_FILTER_BAND_STOP)
    s += " passband-edge2=" + string_from_float (ifr->passband_edge2);
  if (ifr->kind == BSE_IIR_FILTER_ELLIPTIC && ifr->stopband_db < 0)
    s += " stopband-db=" + string_from_float (ifr->stopband_db);
  if (ifr->kind == BSE_IIR_FILTER_ELLIPTIC && ifr->stopband_edge > 0)
    s += " stopband-edge=" + string_from_float (ifr->stopband_edge);
  return g_strdup (s.c_str());
}

gchar*
bse_iir_filter_design_string (const BseIIRFilterDesign *fid)
{
  String s;
  s += "order=" + string_from_int (fid->order);
  s += " sampling-frequency=" + string_from_float (fid->sampling_frequency);
  s += " center-frequency=" + string_from_float (fid->center_frequency);
  s += " gain=" + string_from_double (fid->gain);
  s += " n_zeros=" + string_from_int (fid->n_zeros);
  s += " n_poles=" + string_from_int (fid->n_poles);
  for (uint i = 0; i < fid->n_zeros; i++)
    {
      String u ("Zero:");
      u += " " + string_from_double (fid->zz[i].re);
      u += " + " + string_from_double (fid->zz[i].im) + "*i";
      s += "\n" + u;
    }
  for (uint i = 0; i < fid->n_poles; i++)
    {
      String u ("Pole:");
      u += " " + string_from_double (fid->zp[i].re);
      u += " + " + string_from_double (fid->zp[i].im) + "*i";
      s += "\n" + u;
    }
  String u;
#if 0
  uint o = fid->order;
  u = string_from_double (fid->zn[o]);
  while (o--)
    u = "(" + u + ") * z + " + string_from_double (fid->zn[o]);
  s += "\nNominator: " + u;
  o = fid->order;
  u = string_from_double (fid->zd[o]);
  while (o--)
    u = "(" + u + ") * z + " + string_from_double (fid->zd[o]);
  s += "\nDenominator: " + u;
#endif
  return g_strdup (s.c_str());
}

bool
bse_iir_filter_design (const BseIIRFilterRequest  *filter_request,
                       BseIIRFilterDesign         *filter_design)
{
  if (filter_request->kind == BSE_IIR_FILTER_BUTTERWORTH ||
      filter_request->kind == BSE_IIR_FILTER_CHEBYSHEV1 ||
      filter_request->kind == BSE_IIR_FILTER_ELLIPTIC)
    return _bse_filter_design_ellf (filter_request, filter_design);
  return false;
}
