#ifndef TREESTORAGE_HPP
#define TREESTORAGE_HPP

#include <cstddef>
#include <ostream>

#include "lab1/BinarySearchTree.hpp"
#include "StorageValue.hpp"

template<typename T>
class TreeStorage
{
private:
    BinarySearchTree<StorageValue<T>, std::size_t> items;

public:
    const T* Find(std::size_t index) const;
    void Set(std::size_t index, const T& value);
    void Remove(std::size_t index);
    std::size_t GetCount() const;
    void Clear();
    void Print(std::ostream& stream) const;
};

template<typename T>
const T* TreeStorage<T>::Find(std::size_t index) const
{
    if (!this->items.Contains(index)) { return nullptr; }
    return &this->items.Get(index).data;
}

template<typename T>
void TreeStorage<T>::Set(std::size_t index, const T& value)
{
    if (this->items.Contains(index))
    {
        this->items.Get(index).data = value;
    }
    else
    {
        this->items.Insert(index, StorageValue<T>{value});
    }
}

template<typename T>
void TreeStorage<T>::Remove(std::size_t index)
{
    this->items.Remove(index);
}

template<typename T>
std::size_t TreeStorage<T>::GetCount() const
{
    return this->items.GetCount();
}

template<typename T>
void TreeStorage<T>::Clear()
{
    this->items.Clear();
}

template<typename T>
void TreeStorage<T>::Print(std::ostream& stream) const
{
    auto enumerator = this->items.GetEnumerator();
    while (enumerator->MoveNext())
    {
        stream << enumerator->GetCurrentKey() << " -> " << enumerator->GetCurrent().data << '\n';
    }
}

#endif
