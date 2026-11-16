#ifndef SPARSEVECTOR_HPP
#define SPARSEVECTOR_HPP

#include <cstddef>
#include <ostream>
#include <stdexcept>

#include "TreeStorage.hpp"

template<typename T, typename Storage = TreeStorage<T>>
class SparseVector
{
private:
    std::size_t length;
    Storage storage;

    void CheckIndex(std::size_t index) const;

public:
    explicit SparseVector(std::size_t length);

    T Get(std::size_t index) const;
    void Set(std::size_t index, const T& value);
    std::size_t GetLength() const;
    std::size_t GetNonZeroCount() const;
    void Clear();
    void Print(std::ostream& stream) const;
};

template<typename T, typename Storage>
SparseVector<T, Storage>::SparseVector(std::size_t length) : length(length) { }

template<typename T, typename Storage>
void SparseVector<T, Storage>::CheckIndex(std::size_t index) const
{
    if (index >= this->length)
    {
        throw std::out_of_range("SparseVector: index out of range");
    }
}

template<typename T, typename Storage>
T SparseVector<T, Storage>::Get(std::size_t index) const
{
    this->CheckIndex(index);
    const T* value = this->storage.Find(index);
    if (value == nullptr) { return T{}; }
    return *value;
}

template<typename T, typename Storage>
void SparseVector<T, Storage>::Set(std::size_t index, const T& value)
{
    this->CheckIndex(index);
    if (value == T{}) { this->storage.Remove(index); }
    else { this->storage.Set(index, value); }
}

template<typename T, typename Storage>
std::size_t SparseVector<T, Storage>::GetLength() const
{
    return this->length;
}

template<typename T, typename Storage>
std::size_t SparseVector<T, Storage>::GetNonZeroCount() const
{
    return this->storage.GetCount();
}

template<typename T, typename Storage>
void SparseVector<T, Storage>::Clear()
{
    this->storage.Clear();
}

template<typename T, typename Storage>
void SparseVector<T, Storage>::Print(std::ostream& stream) const
{
    stream << "Длина вектора — " << this->GetLength() << '\n';
    stream << "Ненулевых элементов — " << this->GetNonZeroCount() << '\n';
    this->storage.Print(stream);
}

#endif
