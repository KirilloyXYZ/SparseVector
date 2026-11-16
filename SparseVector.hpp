#ifndef SPARSEVECTOR_HPP
#define SPARSEVECTOR_HPP

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "TreeStorage.hpp"

template<typename T, typename Storage = TreeStorage<T>>
class SparseVector
{
public:
    using Entry = std::pair<std::size_t, T>;

private:
    std::size_t length;
    Storage storage;

    void CheckIndex(std::size_t index) const;
    static bool CompareEntries(const Entry& left, const Entry& right);

public:
    explicit SparseVector(std::size_t length);
    explicit SparseVector(const Sequence<T>& values);
    SparseVector(const SparseVector<T, Storage>& other) = default;
    SparseVector(SparseVector<T, Storage>&& other) noexcept(std::is_nothrow_move_constructible<Storage>::value);
    SparseVector<T, Storage>& operator=(const SparseVector<T, Storage>& other);
    SparseVector<T, Storage>& operator=(SparseVector<T, Storage>&& other) noexcept(std::is_nothrow_move_assignable<Storage>::value);

    static SparseVector<T, Storage> FromEntries(std::size_t length, const Sequence<Entry>& entries);
    T Get(std::size_t index) const;
    void Set(std::size_t index, const T& value);
    std::size_t GetLength() const;
    std::size_t GetNonZeroCount() const;
    void Clear();
    ArraySequence<Entry> GetEntries() const;
    std::unique_ptr<IEnumerator<T, std::size_t>> GetEnumerator() const;
    SparseVector<T, Storage> Map(T (*func)(const T&)) const;
    SparseVector<T, Storage> Where(bool (*predicate)(const T&)) const;
    T Reduce(T (*func)(const T&, const T&), const T& startValue) const;
    void Print(std::ostream& stream, std::size_t maxItems = 100) const;
};

template<typename T, typename Storage>
SparseVector<T, Storage>::SparseVector(std::size_t length) : length(length) { }

template<typename T, typename Storage>
SparseVector<T, Storage>::SparseVector(const Sequence<T>& values) : length(values.GetLength())
{
    ArraySequence<Entry> entries;
    for (std::size_t i = 0; i < this->length; ++i)
    {
        if (!(values.Get(i) == T{})) { entries.Append(Entry{i, values.Get(i)}); }
    }
    this->storage.Load(entries);
}

template<typename T, typename Storage>
SparseVector<T, Storage>::SparseVector(SparseVector<T, Storage>&& other) noexcept(std::is_nothrow_move_constructible<Storage>::value)
    : length(other.length), storage(std::move(other.storage))
{
    other.length = 0;
}

template<typename T, typename Storage>
SparseVector<T, Storage>& SparseVector<T, Storage>::operator=(const SparseVector<T, Storage>& other)
{
    if (this != &other)
    {
        SparseVector<T, Storage> copy(other);
        *this = std::move(copy);
    }
    return *this;
}

template<typename T, typename Storage>
SparseVector<T, Storage>& SparseVector<T, Storage>::operator=(SparseVector<T, Storage>&& other) noexcept(std::is_nothrow_move_assignable<Storage>::value)
{
    if (this != &other)
    {
        this->storage = std::move(other.storage);
        this->length = other.length;
        other.length = 0;
    }
    return *this;
}

template<typename T, typename Storage>
bool SparseVector<T, Storage>::CompareEntries(const Entry& left, const Entry& right)
{
    return left.first < right.first;
}

template<typename T, typename Storage>
SparseVector<T, Storage> SparseVector<T, Storage>::FromEntries(std::size_t length, const Sequence<Entry>& entries)
{
    SparseVector<T, Storage> result(length);
    ArraySequence<Entry> sorted(entries.GetLength());
    for (std::size_t i = 0; i < entries.GetLength(); ++i)
    {
        result.CheckIndex(entries.Get(i).first);
        sorted.Append(entries.Get(i));
    }
    if (sorted.GetLength() > 1)
    {
        Entry* first = &sorted.Get(0);
        Entry* last = first + sorted.GetLength();
        if (!std::is_sorted(first, last, CompareEntries)) { std::stable_sort(first, last, CompareEntries); }
    }
    ArraySequence<Entry> unique;
    for (std::size_t i = 0; i < sorted.GetLength(); ++i)
    {
        // Последняя запись индекса заменяет предыдущие, в том числе записью нуля.
        if (i + 1 < sorted.GetLength() && sorted.Get(i).first == sorted.Get(i + 1).first) { continue; }
        if (!(sorted.Get(i).second == T{})) { unique.Append(sorted.Get(i)); }
    }
    result.storage.Load(unique);
    return result;
}

template<typename T, typename Storage>
void SparseVector<T, Storage>::CheckIndex(std::size_t index) const
{
    if (index >= this->length) { throw std::out_of_range("SparseVector: index out of range"); }
}

template<typename T, typename Storage>
T SparseVector<T, Storage>::Get(std::size_t index) const
{
    this->CheckIndex(index);
    const T* value = this->storage.Find(index);
    return value == nullptr ? T{} : *value;
}

template<typename T, typename Storage>
void SparseVector<T, Storage>::Set(std::size_t index, const T& value)
{
    this->CheckIndex(index);
    if (value == T{}) { this->storage.Remove(index); }
    else { this->storage.Set(index, value); }
}

template<typename T, typename Storage>
std::size_t SparseVector<T, Storage>::GetLength() const { return this->length; }

template<typename T, typename Storage>
std::size_t SparseVector<T, Storage>::GetNonZeroCount() const { return this->storage.GetCount(); }

template<typename T, typename Storage>
void SparseVector<T, Storage>::Clear() { this->storage.Clear(); }

template<typename T, typename Storage>
std::unique_ptr<IEnumerator<T, std::size_t>> SparseVector<T, Storage>::GetEnumerator() const
{
    return this->storage.GetEnumerator();
}

template<typename T, typename Storage>
ArraySequence<typename SparseVector<T, Storage>::Entry> SparseVector<T, Storage>::GetEntries() const
{
    ArraySequence<Entry> result(this->GetNonZeroCount());
    auto enumerator = this->GetEnumerator();
    while (enumerator->MoveNext()) { result.Append(Entry{enumerator->GetCurrentKey(), enumerator->GetCurrent()}); }
    return result;
}

template<typename T, typename Storage>
SparseVector<T, Storage> SparseVector<T, Storage>::Map(T (*func)(const T&)) const
{
    if (func == nullptr) { throw std::invalid_argument("SparseVector::Map: null function"); }
    if (!(func(T{}) == T{})) { throw std::invalid_argument("SparseVector::Map: function must preserve zero"); }

    SparseVector<T, Storage> result(this->length);
    ArraySequence<Entry> entries;
    auto enumerator = this->GetEnumerator();
    while (enumerator->MoveNext())
    {
        T value = func(enumerator->GetCurrent());
        if (!(value == T{})) { entries.Append(Entry{enumerator->GetCurrentKey(), value}); }
    }
    result.storage.Load(entries);
    return result;
}

template<typename T, typename Storage>
SparseVector<T, Storage> SparseVector<T, Storage>::Where(bool (*predicate)(const T&)) const
{
    if (predicate == nullptr) { throw std::invalid_argument("SparseVector::Where: null predicate"); }

    SparseVector<T, Storage> result(this->length);
    ArraySequence<Entry> entries;
    auto enumerator = this->GetEnumerator();
    while (enumerator->MoveNext())
    {
        if (predicate(enumerator->GetCurrent()))
        {
            entries.Append(Entry{enumerator->GetCurrentKey(), enumerator->GetCurrent()});
        }
    }
    result.storage.Load(entries);
    return result;
}

template<typename T, typename Storage>
T SparseVector<T, Storage>::Reduce(T (*func)(const T&, const T&), const T& startValue) const
{
    if (func == nullptr) { throw std::invalid_argument("SparseVector::Reduce: null function"); }
    T result = startValue;
    auto enumerator = this->GetEnumerator();
    while (enumerator->MoveNext()) { result = func(enumerator->GetCurrent(), result); }
    return result;
}

template<typename T, typename Storage>
void SparseVector<T, Storage>::Print(std::ostream& stream, std::size_t maxItems) const
{
    if (maxItems == 0) { throw std::invalid_argument("SparseVector::Print: item limit must be positive"); }
    stream << "Длина вектора — " << this->GetLength() << '\n';
    stream << "Ненулевых элементов — " << this->GetNonZeroCount() << '\n';
    std::size_t shown = 0;
    auto enumerator = this->GetEnumerator();
    while (shown < maxItems && enumerator->MoveNext())
    {
        stream << enumerator->GetCurrentKey() << " -> " << enumerator->GetCurrent() << '\n';
        ++shown;
    }
    if (shown < this->GetNonZeroCount()) { stream << "... Показано " << shown << " записей.\n"; }
}

#endif
