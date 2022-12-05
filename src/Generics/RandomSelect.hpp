#ifndef GENERICS_RANDOM_SELECT_HPP
#define GENERICS_RANDOM_SELECT_HPP

#include <iterator>
#include <functional>
#include <algorithm>

#include <Generics/Rand.hpp>


namespace Generics
{
  template <typename SumType, typename WeightFun, typename Iterator>
  Iterator
  random_select(const Iterator& begin, const Iterator& end,
    const WeightFun& weight_fun = WeightFun())
  {
    SumType cumulative_weight = 0;
    for (Iterator it = begin; it != end; ++it)
    {
      cumulative_weight += weight_fun(*it);
    }

    SumType rnd_weight = safe_rand(1, cumulative_weight);

    SumType cum_weight = 0;

    for (Iterator it = begin; it != end; ++it)
    {
      if (rnd_weight <= cum_weight + weight_fun(*it))
      {
        return it;
      }
      cum_weight += weight_fun(*it);
    }

    return end;
  }
}

#endif
