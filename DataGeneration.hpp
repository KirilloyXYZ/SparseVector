#ifndef DATAGENERATION_HPP
#define DATAGENERATION_HPP

#include <algorithm>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <utility>

#include "lab1/ArraySequence.hpp"
#include "lab1/BinarySearchTree.hpp"

using VectorEntry = std::pair<std::size_t, int>;

enum class DataOrder
{
    Random,
    Ascending,
    Descending
};

inline bool CompareIndices(const VectorEntry& left, const VectorEntry& right)
{
    return left.first < right.first;
}

inline const char* GetOrderName(DataOrder order)
{
    switch (order)
    {
        case DataOrder::Random: return "random";
        case DataOrder::Ascending: return "ascending";
        case DataOrder::Descending: return "descending";
    }
    throw std::invalid_argument("Unknown data order");
}

inline void ArrangeEntries(ArraySequence<VectorEntry>& entries, DataOrder order, unsigned int seed)
{
    GetOrderName(order);
    std::size_t count = entries.GetLength();
    if (count > 1)
    {
        VectorEntry* first = &entries.Get(0);
        std::sort(first, first + count, CompareIndices);
    }
    if (order == DataOrder::Descending)
    {
        for (std::size_t i = 0; i < count / 2; ++i) { std::swap(entries.Get(i), entries.Get(count - i - 1)); }
    }
    else if (order == DataOrder::Random)
    {
        std::mt19937 random(seed);
        for (std::size_t i = count; i > 1; --i)
        {
            std::uniform_int_distribution<std::size_t> index(0, i - 1);
            std::swap(entries.Get(i - 1), entries.Get(index(random)));
        }
    }
}

inline ArraySequence<VectorEntry> GenerateEntries(std::size_t length, std::size_t count, int minimum, int maximum,
                                                 DataOrder order, unsigned int seed)
{
    GetOrderName(order);
    if (count > length || minimum > maximum || (count > 0 && minimum == 0 && maximum == 0))
    {
        throw std::invalid_argument("GenerateEntries: invalid count or value range");
    }
    ArraySequence<VectorEntry> result(count);
    if (count == 0) { return result; }

    std::mt19937 random(seed);
    BinarySearchTree<bool, std::size_t> positions;
    for (std::size_t i = 0; i < count; ++i)
    {
        std::size_t last = length - count + i;
        std::uniform_int_distribution<std::size_t> index(0, last);
        if (!positions.Insert(index(random), true)) { positions.Insert(last, true); }
    }
    bool includesZero = minimum <= 0 && maximum >= 0;
    long long numberOfValues = static_cast<long long>(maximum) - minimum + 1 - (includesZero ? 1 : 0);
    std::uniform_int_distribution<long long> value(0, numberOfValues - 1);
    auto enumerator = positions.GetEnumerator();
    while (enumerator->MoveNext())
    {
        long long generated = static_cast<long long>(minimum) + value(random);
        if (includesZero && generated >= 0) { ++generated; }
        result.Append(VectorEntry{enumerator->GetCurrentKey(), static_cast<int>(generated)});
    }
    ArrangeEntries(result, order, seed);
    return result;
}

#endif
