#ifndef VECTOROPERATIONS_HPP
#define VECTOROPERATIONS_HPP

#include <limits>
#include <stdexcept>

inline int CheckedInt(long long value)
{
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
    {
        throw std::overflow_error("Result does not fit in int");
    }
    return static_cast<int>(value);
}

inline int NegateValue(const int& value) { return CheckedInt(-static_cast<long long>(value)); }
inline int DoubleValue(const int& value) { return CheckedInt(2LL * value); }
inline int SquareValue(const int& value) { return CheckedInt(static_cast<long long>(value) * value); }
inline bool IsPositive(const int& value) { return value > 0; }
inline bool IsEven(const int& value) { return value % 2 == 0; }
inline int SumValues(const int& value, const int& result) { return CheckedInt(static_cast<long long>(value) + result); }

#endif
