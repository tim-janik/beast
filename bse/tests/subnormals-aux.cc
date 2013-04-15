// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bse.hh>
#include <bse/bseieee754.hh>

float
test1f (float v)
{
  return v;
}

float
test2f (float v)
{
  return bse_float_zap_denormal (v);
}

float
test3f (float v)
{
  BSE_FLOAT_FLUSH_with_cond (v);
  return v;
}

float
test4f (float v)
{
  BSE_FLOAT_FLUSH_with_if (v);
  return v;
}

float
test5f (float v)
{
  BSE_FLOAT_FLUSH_with_threshold (v);
  return v;
}
