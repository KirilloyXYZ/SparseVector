#ifndef ARRAYSTORAGE_HPP
#define ARRAYSTORAGE_HPP

#include <cstddef>
#include <utility>

#include "SortedSequence.hpp"

template<typename T>
class ArrayStorage
{
private:
    using Entry = std::pair<std::size_t, T>;
    SortedSequence<Entry> items;

    static bool CompareEntries(const Entry& left, const Entry& right);

    class Enumerator : public IEnumerator<T, std::size_t>
    {
    private:
        std::unique_ptr<IEnumerator<Entry, std::size_t>> entries;

    public:
        explicit Enumerator(const SortedSequence<Entry>& items) : entries(items.GetEnumerator()) { }
        bool MoveNext() override { return this->entries->MoveNext(); }
        const T& GetCurrent() const override { return this->entries->GetCurrent().second; }
        const std::size_t& GetCurrentKey() const override { return this->entries->GetCurrent().first; }
    };

public:
    ArrayStorage();

    const T* Find(std::size_t index) const;
    void Set(std::size_t index, const T& value);
    void Remove(std::size_t index);
    std::size_t GetCount() const;
    void Clear();
    void Load(const Sequence<Entry>& entries);
    std::unique_ptr<IEnumerator<T, std::size_t>> GetEnumerator() const;
};

template<typename T>
bool ArrayStorage<T>::CompareEntries(const Entry& left, const Entry& right) { return left.first < right.first; }

template<typename T>
ArrayStorage<T>::ArrayStorage() : items(CompareEntries) { }

template<typename T>
const T* ArrayStorage<T>::Find(std::size_t index) const
{
    std::size_t position = this->items.LowerBound(Entry{index, T{}});
    if (position == this->items.GetLength() || this->items.Get(position).first != index) { return nullptr; }
    return &this->items.Get(position).second;
}

template<typename T>
void ArrayStorage<T>::Set(std::size_t index, const T& value)
{
    Entry entry{index, value};
    std::size_t position = this->items.LowerBound(entry);
    if (position < this->items.GetLength() && this->items.Get(position).first == index)
    {
        this->items.Replace(position, entry);
    }
    else { this->items.Add(entry); }
}

template<typename T>
void ArrayStorage<T>::Remove(std::size_t index)
{
    std::size_t position = this->items.LowerBound(Entry{index, T{}});
    if (position < this->items.GetLength() && this->items.Get(position).first == index)
    {
        this->items.RemoveAt(position);
    }
}

template<typename T>
std::size_t ArrayStorage<T>::GetCount() const { return this->items.GetLength(); }

template<typename T>
void ArrayStorage<T>::Clear() { this->items.Clear(); }

template<typename T>
void ArrayStorage<T>::Load(const Sequence<Entry>& entries)
{
    for (std::size_t i = 1; i < entries.GetLength(); ++i)
    {
        if (entries.Get(i - 1).first >= entries.Get(i).first)
        {
            throw std::invalid_argument("ArrayStorage::Load: indices must be strictly increasing");
        }
    }
    SortedSequence<Entry> result(entries, CompareEntries);
    this->items = std::move(result);
}

template<typename T>
std::unique_ptr<IEnumerator<T, std::size_t>> ArrayStorage<T>::GetEnumerator() const
{
    return std::make_unique<Enumerator>(this->items);
}

#endif
