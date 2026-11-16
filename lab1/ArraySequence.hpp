#ifndef ARRAYSEQUENCE_HPP
#define ARRAYSEQUENCE_HPP

#include <cstddef>

#include "Sequence.hpp"
#include "DynamicArray.hpp"

template<typename T>
class ArraySequence : public Sequence<T>
{
private:
    DynamicArray<T> items;

public:
    ArraySequence();
    explicit ArraySequence(std::size_t capacity);
    ArraySequence(const T* items, std::size_t count);
    ArraySequence(const ArraySequence<T>& other) = default;
    ArraySequence(ArraySequence<T>&& other) noexcept = default;
    ArraySequence<T>& operator=(const ArraySequence<T>& other) = default;
    ArraySequence<T>& operator=(ArraySequence<T>&& other) noexcept = default;
    ~ArraySequence() override = default;

    const T& Get(std::size_t index) const override;
    T& Get(std::size_t index);
    std::size_t GetLength() const override;
    std::size_t GetCapacity() const;
    void Append(const T& value) override;
    void Reserve(std::size_t capacity);
    void InsertAt(std::size_t index, const T& value);
    void RemoveAt(std::size_t index);
    void Clear();
};

template<typename T>
ArraySequence<T>::ArraySequence() { }

template<typename T>
ArraySequence<T>::ArraySequence(std::size_t capacity)
{
    this->items.Reserve(capacity);
}

template<typename T>
ArraySequence<T>::ArraySequence(const T* items, std::size_t count) : items(items, count) { }

template<typename T>
const T& ArraySequence<T>::Get(std::size_t index) const
{
    return this->items.Get(index);
}

template<typename T>
T& ArraySequence<T>::Get(std::size_t index)
{
    return this->items.Get(index);
}

template<typename T>
std::size_t ArraySequence<T>::GetLength() const
{
    return this->items.GetSize();
}

template<typename T>
void ArraySequence<T>::Append(const T& value)
{
    this->items.PushBack(value);
}

template<typename T>
std::size_t ArraySequence<T>::GetCapacity() const
{
    return this->items.GetCapacity();
}

template<typename T>
void ArraySequence<T>::Reserve(std::size_t capacity)
{
    this->items.Reserve(capacity);
}

template<typename T>
void ArraySequence<T>::InsertAt(std::size_t index, const T& value)
{
    this->items.InsertAt(index, value);
}

template<typename T>
void ArraySequence<T>::RemoveAt(std::size_t index)
{
    this->items.RemoveAt(index);
}

template<typename T>
void ArraySequence<T>::Clear()
{
    this->items.Clear();
}

#endif
