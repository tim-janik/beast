// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_DEVICES_LINEAR_SMOOTH_HH__
#define __BSE_DEVICES_LINEAR_SMOOTH_HH__

/* from liquidsfz utils */

#include <math.h>

#include <string>

namespace Bse {

class LinearSmooth
{
  float value_ = 0;
  float linear_value_ = 0;
  float linear_step_ = 0;
  uint  total_steps_ = 1;
  uint  steps_ = 0;
public:
  void
  reset (uint rate, float time)
  {
    total_steps_ = std::max<int> (rate * time, 1);
  }
  void
  set (float new_value, bool now = false)
  {
    if (now)
      {
        steps_ = 0;
        value_ = new_value;
      }
    else if (new_value != value_)
      {
        if (!steps_)
          linear_value_ = value_;

        linear_step_ = (new_value - linear_value_) / total_steps_;
        steps_ = total_steps_;
        value_ = new_value;
      }
  }
  float
  get_next()
  {
    if (!steps_)
      return value_;
    else
      {
        steps_--;
        linear_value_ += linear_step_;
        return linear_value_;
      }
  }
};

}

#endif /* __BSE_DEVICES_LINEAR_SMOOTH_HH__ */
