#ifndef TREEDICTIONARY_HPP
#define TREEDICTIONARY_HPP

#include <limits>
#include <stdexcept>

#include "IDictionary.hpp"
#include "lab1/BinarySearchTree.hpp"

template<typename K, typename T>
class TreeDictionary : public IDictionary<K, T>
{
private:
    BinarySearchTree<T, K> items;

public:
    explicit TreeDictionary(bool (*compare)(const K&, const K&) = CompareValues<K>);

    std::size_t GetCount() const override;
    std::size_t GetCapacity() const override;
    const T& Get(const K& key) const override;
    const T* Find(const K& key) const override;
    bool ContainsKey(const K& key) const override;
    void Add(const K& key, const T& value) override;
    void Set(const K& key, const T& value) override;
    void Remove(const K& key) override;
    bool TryRemove(const K& key);
    void Clear() override;
    bool IsValid() const;
    std::unique_ptr<IEnumerator<T, K>> GetEnumerator() const override;
};

template<typename K, typename T>
TreeDictionary<K, T>::TreeDictionary(bool (*compare)(const K&, const K&)) : items(compare) { }

template<typename K, typename T>
std::size_t TreeDictionary<K, T>::GetCount() const { return this->items.GetCount(); }

template<typename K, typename T>
std::size_t TreeDictionary<K, T>::GetCapacity() const
{
    // Дерево выделяет узлы по одному и не имеет фиксированного массива ячеек.
    return std::numeric_limits<std::size_t>::max();
}

template<typename K, typename T>
const T& TreeDictionary<K, T>::Get(const K& key) const { return this->items.Get(key); }

template<typename K, typename T>
const T* TreeDictionary<K, T>::Find(const K& key) const { return this->items.Find(key); }

template<typename K, typename T>
bool TreeDictionary<K, T>::ContainsKey(const K& key) const { return this->items.Contains(key); }

template<typename K, typename T>
void TreeDictionary<K, T>::Add(const K& key, const T& value)
{
    if (!this->items.Insert(key, value)) { throw std::invalid_argument("TreeDictionary::Add: duplicate key"); }
}

template<typename K, typename T>
void TreeDictionary<K, T>::Set(const K& key, const T& value)
{
    if (!this->items.Update(key, value)) { this->items.Insert(key, value); }
}

template<typename K, typename T>
void TreeDictionary<K, T>::Remove(const K& key)
{
    if (!this->TryRemove(key)) { throw std::out_of_range("TreeDictionary::Remove: key not found"); }
}

template<typename K, typename T>
bool TreeDictionary<K, T>::TryRemove(const K& key) { return this->items.Remove(key); }

template<typename K, typename T>
void TreeDictionary<K, T>::Clear() { this->items.Clear(); }

template<typename K, typename T>
bool TreeDictionary<K, T>::IsValid() const { return this->items.IsValid(); }

template<typename K, typename T>
std::unique_ptr<IEnumerator<T, K>> TreeDictionary<K, T>::GetEnumerator() const
{
    return this->items.GetEnumerator();
}

#endif
