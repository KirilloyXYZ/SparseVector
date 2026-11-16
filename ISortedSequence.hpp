#ifndef ISORTEDSEQUENCE_HPP
#define ISORTEDSEQUENCE_HPP

#include <cstddef>
#include <memory>

#include "lab1/IEnumerator.hpp"

template<typename T>
class ISortedSequence
{
public:
    virtual ~ISortedSequence() = default;

    virtual std::size_t GetLength() const = 0;
    virtual bool IsEmpty() const = 0;
    virtual const T& Get(std::size_t index) const = 0;
    virtual const T& GetFirst() const = 0;
    virtual const T& GetLast() const = 0;
    virtual std::ptrdiff_t IndexOf(const T& value) const = 0;
    virtual void Add(const T& value) = 0;
    virtual void RemoveAt(std::size_t index) = 0;
    virtual void Clear() = 0;
    virtual std::unique_ptr<ISortedSequence<T>> GetSubsequence(std::size_t start, std::size_t end) const = 0;
    virtual std::unique_ptr<IEnumerator<T, std::size_t>> GetEnumerator() const = 0;
};

#endif
