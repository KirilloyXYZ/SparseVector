#ifndef COMPARISON_HPP
#define COMPARISON_HPP

#include <type_traits>
#include <utility>

template<typename T, typename = void>
struct IsLessComparable : std::false_type { };

template<typename T>
struct IsLessComparable<T, std::void_t<decltype(std::declval<const T&>() < std::declval<const T&>())>>
    : std::is_convertible<decltype(std::declval<const T&>() < std::declval<const T&>()), bool> { };

template<typename T>
bool CompareValues(const T& left, const T& right)
{
    static_assert(IsLessComparable<T>::value,
                  "CompareValues: key must support operator< with a result convertible to bool");
    if constexpr (IsLessComparable<T>::value) { return left < right; }
}

template<typename T>
bool EqualValues(const T& left, const T& right)
{
    return left == right;
}

#endif
