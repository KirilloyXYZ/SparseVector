#ifndef TREESTORAGE_HPP
#define TREESTORAGE_HPP

#include <cstddef>
#include <utility>

#include "TreeDictionary.hpp"

template<typename T>
class TreeStorage
{
private:
    TreeDictionary<std::size_t, T> items;

public:
    const T* Find(std::size_t index) const;
    void Set(std::size_t index, const T& value);
    void Remove(std::size_t index);
    std::size_t GetCount() const;
    void Clear();
    void Load(const Sequence<std::pair<std::size_t, T>>& entries);
    std::unique_ptr<IEnumerator<T, std::size_t>> GetEnumerator() const;
};

template<typename T>
const T* TreeStorage<T>::Find(std::size_t index) const { return this->items.Find(index); }

template<typename T>
void TreeStorage<T>::Set(std::size_t index, const T& value) { this->items.Set(index, value); }

template<typename T>
void TreeStorage<T>::Remove(std::size_t index) { this->items.TryRemove(index); }

template<typename T>
std::size_t TreeStorage<T>::GetCount() const { return this->items.GetCount(); }

template<typename T>
void TreeStorage<T>::Clear() { this->items.Clear(); }

template<typename T>
void TreeStorage<T>::Load(const Sequence<std::pair<std::size_t, T>>& entries)
{
    TreeDictionary<std::size_t, T> result;
    for (std::size_t i = 0; i < entries.GetLength(); ++i)
    {
        if (i > 0 && entries.Get(i - 1).first >= entries.Get(i).first)
        {
            throw std::invalid_argument("TreeStorage::Load: indices must be strictly increasing");
        }
        result.Add(entries.Get(i).first, entries.Get(i).second);
    }
    this->items = std::move(result);
}

template<typename T>
std::unique_ptr<IEnumerator<T, std::size_t>> TreeStorage<T>::GetEnumerator() const
{
    return this->items.GetEnumerator();
}

#endif
