#ifndef SW_ALGORITHM_HPP_
#define SW_ALGORITHM_HPP_

#include "sowa.hpp"

namespace Sowa
{
    template<typename T>
    T NotZero(T value, T fallback) {
        return value == 0 ? fallback : value;
    }

    template<typename T>
    T map(T v, T input_min, T input_max, T output_min, T output_max) {
        return (v - input_min) * (output_max - output_min) / (input_max - input_min) + output_min;
    }

    template<typename T>
    T minmax(T v, T min, T max) {
        return MIN(max, MAX(v, min));
    }

    template<typename T>
    void Swap(T& a, T& b) {
        T t = b;
        b = a;
        a = t;
    }
} // namespace Sowa


#endif // SW_ALGORITHM_HPP_