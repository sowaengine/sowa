#ifndef SW_ALGORITHM_HPP_
#define SW_ALGORITHM_HPP_

namespace Sowa
{
    template<typename T>
    T NotZero(T value, T fallback) {
        return value == 0 ? fallback : value;
    }
} // namespace Sowa


#endif // SW_ALGORITHM_HPP_