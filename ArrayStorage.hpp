#ifndef ARRAYSTORAGE_HPP
#define ARRAYSTORAGE_HPP

#include <cstddef>
#include <ostream>
#include <utility>

#include "lab1/ArraySequence.hpp"
#include "StorageValue.hpp"

template<typename T>
class ArrayStorage
{
private:
    using Entry = std::pair<std::size_t, StorageValue<T>>;
    ArraySequence<Entry> items;

    std::size_t FindPosition(std::size_t index) const;

public:
    const T* Find(std::size_t index) const;
    void Set(std::size_t index, const T& value);
    void Remove(std::size_t index);
    std::size_t GetCount() const;
    void Clear();
    void Print(std::ostream& stream) const;
};

template<typename T>
std::size_t ArrayStorage<T>::FindPosition(std::size_t index) const
{
    std::size_t left = 0;
    std::size_t right = this->items.GetLength();
    while (left < right)
    {
        std::size_t middle = left + (right - left) / 2;
        if (this->items.Get(middle).first < index) { left = middle + 1; }
        else { right = middle; }
    }
    return left;
}

template<typename T>
const T* ArrayStorage<T>::Find(std::size_t index) const
{
    std::size_t position = this->FindPosition(index);
    if (position == this->items.GetLength() || this->items.Get(position).first != index)
    {
        return nullptr;
    }
    return &this->items.Get(position).second.data;
}

template<typename T>
void ArrayStorage<T>::Set(std::size_t index, const T& value)
{
    std::size_t position = this->FindPosition(index);
    std::size_t count = this->items.GetLength();
    if (position < count && this->items.Get(position).first == index)
    {
        this->items.Get(position).second.data = value;
        return;
    }

    // Собираем последовательность с новой парой на найденной позиции.
    ArraySequence<Entry> result(count + 1);
    for (std::size_t i = 0; i < position; ++i) { result.Append(this->items.Get(i)); }
    result.Append(Entry{index, StorageValue<T>{value}});
    for (std::size_t i = position; i < count; ++i) { result.Append(this->items.Get(i)); }
    this->items = std::move(result);
}

template<typename T>
void ArrayStorage<T>::Remove(std::size_t index)
{
    std::size_t position = this->FindPosition(index);
    std::size_t count = this->items.GetLength();
    if (position == count || this->items.Get(position).first != index) { return; }

    ArraySequence<Entry> result(count - 1);
    for (std::size_t i = 0; i < position; ++i) { result.Append(this->items.Get(i)); }
    for (std::size_t i = position + 1; i < count; ++i) { result.Append(this->items.Get(i)); }
    this->items = std::move(result);
}

template<typename T>
std::size_t ArrayStorage<T>::GetCount() const
{
    return this->items.GetLength();
}

template<typename T>
void ArrayStorage<T>::Clear()
{
    this->items = ArraySequence<Entry>();
}

template<typename T>
void ArrayStorage<T>::Print(std::ostream& stream) const
{
    for (std::size_t i = 0; i < this->items.GetLength(); ++i)
    {
        const Entry& entry = this->items.Get(i);
        stream << entry.first << " -> " << entry.second.data << '\n';
    }
}

#endif
