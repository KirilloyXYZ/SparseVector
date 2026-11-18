#ifndef SORTEDSEQUENCE_HPP
#define SORTEDSEQUENCE_HPP

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "ISortedSequence.hpp"
#include "../lab1/ArraySequence.hpp"
#include "../lab1/Comparison.hpp"

template<typename T>
class SortedSequence : public ISortedSequence<T>
{
private:
    ArraySequence<T> items;
    bool (*compare)(const T&, const T&);
    std::size_t version;

    class Enumerator : public IEnumerator<T, std::size_t>
    {
    private:
        const SortedSequence<T>& sequence;
        std::size_t expectedVersion;
        std::size_t nextIndex;
        std::size_t currentIndex;
        bool hasCurrent;

        void CheckVersion() const;
        void CheckCurrent() const;

    public:
        explicit Enumerator(const SortedSequence<T>& sequence);
        bool MoveNext() override;
        const T& GetCurrent() const override;
        const std::size_t& GetCurrentKey() const override;
    };

public:
    explicit SortedSequence(bool (*compare)(const T&, const T&) = CompareValues<T>);
    SortedSequence(const Sequence<T>& values, bool (*compare)(const T&, const T&) = CompareValues<T>);
    SortedSequence(const SortedSequence<T>& other);
    SortedSequence(SortedSequence<T>&& other) noexcept;
    SortedSequence<T>& operator=(const SortedSequence<T>& other);
    SortedSequence<T>& operator=(SortedSequence<T>&& other) noexcept;

    std::size_t GetLength() const override;
    std::size_t GetCapacity() const;
    bool IsEmpty() const override;
    const T& Get(std::size_t index) const override;
    const T& GetFirst() const override;
    const T& GetLast() const override;
    std::size_t LowerBound(const T& value) const;
    std::ptrdiff_t IndexOf(const T& value) const override;
    void Add(const T& value) override;
    void Replace(std::size_t index, const T& value);
    void RemoveAt(std::size_t index) override;
    void Clear() override;
    std::unique_ptr<ISortedSequence<T>> GetSubsequence(std::size_t start, std::size_t end) const override;
    std::unique_ptr<IEnumerator<T, std::size_t>> GetEnumerator() const override;
};

template<typename T>
SortedSequence<T>::SortedSequence(bool (*compare)(const T&, const T&)) : compare(compare), version(0)
{
    if (compare == nullptr) { throw std::invalid_argument("SortedSequence: null comparator"); }
}

template<typename T>
SortedSequence<T>::SortedSequence(const Sequence<T>& values, bool (*compare)(const T&, const T&))
    : SortedSequence<T>(compare)
{
    this->items.Reserve(values.GetLength());
    for (std::size_t i = 0; i < values.GetLength(); ++i)
    {
        if (compare(values.Get(i), values.Get(i)))
        {
            throw std::invalid_argument("SortedSequence: comparator must be strict");
        }
        this->items.Append(values.Get(i));
    }
    if (this->items.GetLength() > 1)
    {
        T* first = &this->items.Get(0);
        T* last = first + this->items.GetLength();
        if (!std::is_sorted(first, last, compare)) { std::sort(first, last, compare); }
    }
}

template<typename T>
SortedSequence<T>::SortedSequence(const SortedSequence<T>& other)
    : items(other.items), compare(other.compare), version(0) { }

template<typename T>
SortedSequence<T>::SortedSequence(SortedSequence<T>&& other) noexcept
    : items(std::move(other.items)), compare(other.compare), version(0)
{
    ++other.version;
}

template<typename T>
SortedSequence<T>& SortedSequence<T>::operator=(const SortedSequence<T>& other)
{
    if (this != &other)
    {
        this->items = other.items;
        this->compare = other.compare;
        ++this->version;
    }
    return *this;
}

template<typename T>
SortedSequence<T>& SortedSequence<T>::operator=(SortedSequence<T>&& other) noexcept
{
    if (this != &other)
    {
        this->items = std::move(other.items);
        this->compare = other.compare;
        ++this->version;
        ++other.version;
    }
    return *this;
}

template<typename T>
std::size_t SortedSequence<T>::GetLength() const { return this->items.GetLength(); }

template<typename T>
std::size_t SortedSequence<T>::GetCapacity() const { return this->items.GetCapacity(); }

template<typename T>
bool SortedSequence<T>::IsEmpty() const { return this->GetLength() == 0; }

template<typename T>
const T& SortedSequence<T>::Get(std::size_t index) const { return this->items.Get(index); }

template<typename T>
const T& SortedSequence<T>::GetFirst() const { return this->Get(0); }

template<typename T>
const T& SortedSequence<T>::GetLast() const
{
    if (this->IsEmpty()) { throw std::out_of_range("SortedSequence::GetLast: empty sequence"); }
    return this->Get(this->GetLength() - 1);
}

template<typename T>
std::size_t SortedSequence<T>::LowerBound(const T& value) const
{
    std::size_t left = 0;
    std::size_t right = this->GetLength();
    while (left < right)
    {
        std::size_t middle = left + (right - left) / 2;
        if (this->compare(this->Get(middle), value)) { left = middle + 1; }
        else { right = middle; }
    }
    return left;
}

template<typename T>
std::ptrdiff_t SortedSequence<T>::IndexOf(const T& value) const
{
    std::size_t position = this->LowerBound(value);
    if (position == this->GetLength() || this->compare(value, this->Get(position))) { return -1; }
    return static_cast<std::ptrdiff_t>(position);
}

template<typename T>
void SortedSequence<T>::Add(const T& value)
{
    if (this->compare(value, value)) { throw std::invalid_argument("SortedSequence::Add: invalid comparison"); }
    this->items.InsertAt(this->LowerBound(value), value);
    ++this->version;
}

template<typename T>
void SortedSequence<T>::Replace(std::size_t index, const T& value)
{
    const T& old = this->Get(index);
    if (this->compare(old, value) || this->compare(value, old))
    {
        throw std::invalid_argument("SortedSequence::Replace: sorting key must stay the same");
    }
    if constexpr (std::is_nothrow_copy_assignable<T>::value) { this->items.Get(index) = value; }
    else
    {
        ArraySequence<T> copy(this->items);
        copy.Get(index) = value;
        this->items = std::move(copy);
    }
    ++this->version;
}

template<typename T>
void SortedSequence<T>::RemoveAt(std::size_t index)
{
    this->items.RemoveAt(index);
    ++this->version;
}

template<typename T>
void SortedSequence<T>::Clear()
{
    if (!this->IsEmpty())
    {
        this->items.Clear();
        ++this->version;
    }
}

template<typename T>
std::unique_ptr<ISortedSequence<T>> SortedSequence<T>::GetSubsequence(std::size_t start, std::size_t end) const
{
    if (start > end) { throw std::invalid_argument("SortedSequence::GetSubsequence: reversed range"); }
    if (end >= this->GetLength()) { throw std::out_of_range("SortedSequence::GetSubsequence: index out of range"); }
    auto result = std::make_unique<SortedSequence<T>>(this->compare);
    result->items.Reserve(end - start + 1);
    for (std::size_t i = start; i <= end; ++i) { result->items.Append(this->Get(i)); }
    return result;
}

template<typename T>
std::unique_ptr<IEnumerator<T, std::size_t>> SortedSequence<T>::GetEnumerator() const
{
    return std::make_unique<Enumerator>(*this);
}

template<typename T>
SortedSequence<T>::Enumerator::Enumerator(const SortedSequence<T>& sequence)
    : sequence(sequence), expectedVersion(sequence.version), nextIndex(0), currentIndex(0), hasCurrent(false) { }

template<typename T>
void SortedSequence<T>::Enumerator::CheckVersion() const
{
    if (this->expectedVersion != this->sequence.version) { throw std::logic_error("Enumerator: sequence changed"); }
}

template<typename T>
void SortedSequence<T>::Enumerator::CheckCurrent() const
{
    this->CheckVersion();
    if (!this->hasCurrent) { throw std::out_of_range("Enumerator: no current element"); }
}

template<typename T>
bool SortedSequence<T>::Enumerator::MoveNext()
{
    this->CheckVersion();
    this->hasCurrent = this->nextIndex < this->sequence.GetLength();
    if (this->hasCurrent) { this->currentIndex = this->nextIndex++; }
    return this->hasCurrent;
}

template<typename T>
const T& SortedSequence<T>::Enumerator::GetCurrent() const
{
    this->CheckCurrent();
    return this->sequence.Get(this->currentIndex);
}

template<typename T>
const std::size_t& SortedSequence<T>::Enumerator::GetCurrentKey() const
{
    this->CheckCurrent();
    return this->currentIndex;
}

#endif
