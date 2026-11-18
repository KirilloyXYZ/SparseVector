#ifndef IDICTIONARY_HPP
#define IDICTIONARY_HPP

#include <cstddef>
#include <memory>

#include "../lab1/ArraySequence.hpp"
#include "../lab1/IEnumerator.hpp"

template<typename K, typename T>
class IDictionary
{
public:
    virtual ~IDictionary() = default;

    virtual std::size_t GetCount() const = 0;
    virtual std::size_t GetCapacity() const = 0;
    virtual const T& Get(const K& key) const = 0;
    virtual const T* Find(const K& key) const = 0;
    virtual bool ContainsKey(const K& key) const = 0;
    virtual void Add(const K& key, const T& value) = 0;
    virtual void Set(const K& key, const T& value) = 0;
    virtual void Remove(const K& key) = 0;
    virtual void Clear() = 0;
    virtual std::unique_ptr<IEnumerator<T, K>> GetEnumerator() const = 0;

    ArraySequence<K> GetKeys() const;
    ArraySequence<T> GetValues() const;
};

template<typename K, typename T>
ArraySequence<K> IDictionary<K, T>::GetKeys() const
{
    ArraySequence<K> result(this->GetCount());
    auto enumerator = this->GetEnumerator();
    while (enumerator->MoveNext()) { result.Append(enumerator->GetCurrentKey()); }
    return result;
}

template<typename K, typename T>
ArraySequence<T> IDictionary<K, T>::GetValues() const
{
    ArraySequence<T> result(this->GetCount());
    auto enumerator = this->GetEnumerator();
    while (enumerator->MoveNext()) { result.Append(enumerator->GetCurrent()); }
    return result;
}

#endif
