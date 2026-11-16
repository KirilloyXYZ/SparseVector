#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "SparseVector.hpp"
#include "ArrayStorage.hpp"
#include "DataGeneration.hpp"

struct BenchmarkData
{
    std::size_t length;
    ArraySequence<VectorEntry> entries;
    ArraySequence<VectorEntry> ordered;
    ArraySequence<std::size_t> missing;
    long long checksum = 0;
    int fold = 0;
    std::size_t positiveCount = 0;
};

struct BenchmarkResult
{
    double build;
    double bulk;
    double read;
    double missing;
    double update;
    double remove;
    double map;
    double where;
    double reduce;
    long long checksum = 0;
    int fold = 0;
};

template<typename Action>
double Measure(Action action)
{
    auto start = std::chrono::steady_clock::now();
    action();
    auto finish = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(finish - start).count();
}

inline int IdentityValue(const int& value) { return value; }
inline bool PositiveValue(const int& value) { return value > 0; }
inline int XorValues(const int& value, const int& result) { return value ^ result; }

inline BenchmarkData PrepareBenchmark(std::size_t length, const Sequence<VectorEntry>& entries)
{
    if (entries.GetLength() > 1000000) { throw std::length_error("Benchmark: at most 1000000 nonzero entries"); }
    BenchmarkData data;
    data.length = length;
    data.entries.Reserve(entries.GetLength());
    for (std::size_t i = 0; i < entries.GetLength(); ++i) { data.entries.Append(entries.Get(i)); }
    SortedSequence<VectorEntry> sorted(entries, CompareIndices);
    data.ordered.Reserve(entries.GetLength());
    std::size_t next = 0;
    for (std::size_t i = 0; i < sorted.GetLength(); ++i)
    {
        const VectorEntry& entry = sorted.Get(i);
        if (entry.first >= length || entry.second == 0 || (i > 0 && sorted.Get(i - 1).first == entry.first))
        {
            throw std::invalid_argument("Benchmark: entries must have distinct valid indices and nonzero values");
        }
        data.ordered.Append(entry);
        data.checksum += entry.second;
        data.fold ^= entry.second;
        if (entry.second > 0) { ++data.positiveCount; }
        while (next < entry.first && data.missing.GetLength() < entries.GetLength()) { data.missing.Append(next++); }
        next = entry.first + 1;
    }
    while (next < length && data.missing.GetLength() < entries.GetLength()) { data.missing.Append(next++); }
    return data;
}

template<typename Storage>
void CheckBenchmarkVector(const SparseVector<int, Storage>& vector, const BenchmarkData& data)
{
    if (vector.GetLength() != data.length || vector.GetNonZeroCount() != data.ordered.GetLength())
    {
        throw std::runtime_error("Benchmark: wrong vector size");
    }
    auto enumerator = vector.GetEnumerator();
    for (std::size_t i = 0; i < data.ordered.GetLength(); ++i)
    {
        if (!enumerator->MoveNext() || enumerator->GetCurrentKey() != data.ordered.Get(i).first ||
            enumerator->GetCurrent() != data.ordered.Get(i).second)
        {
            throw std::runtime_error("Benchmark: incorrect vector contents");
        }
    }
    if (enumerator->MoveNext()) { throw std::runtime_error("Benchmark: unexpected entry"); }
}

template<typename Storage>
BenchmarkResult RunBenchmark(const BenchmarkData& data)
{
    using Vector = SparseVector<int, Storage>;
    BenchmarkResult result;
    Vector vector(data.length);
    result.build = Measure([&]()
    {
        for (std::size_t i = 0; i < data.entries.GetLength(); ++i)
        {
            vector.Set(data.entries.Get(i).first, data.entries.Get(i).second);
        }
    });
    CheckBenchmarkVector(vector, data);

    Vector bulk(0);
    result.bulk = Measure([&]() { bulk = Vector::FromEntries(data.length, data.entries); });
    CheckBenchmarkVector(bulk, data);
    result.read = Measure([&]()
    {
        for (std::size_t i = 0; i < data.entries.GetLength(); ++i) { result.checksum += vector.Get(data.entries.Get(i).first); }
    });
    long long missingSum = 0;
    result.missing = Measure([&]()
    {
        for (std::size_t i = 0; i < data.missing.GetLength(); ++i) { missingSum += vector.Get(data.missing.Get(i)); }
    });
    if (result.checksum != data.checksum || missingSum != 0) { throw std::runtime_error("Benchmark: incorrect reads"); }

    Vector changed(vector);
    result.update = Measure([&]()
    {
        for (std::size_t i = 0; i < data.entries.GetLength(); ++i) { changed.Set(data.entries.Get(i).first, 1); }
    });
    if (changed.GetNonZeroCount() != data.entries.GetLength()) { throw std::runtime_error("Benchmark: wrong update count"); }
    for (std::size_t i = 0; i < data.entries.GetLength(); ++i)
    {
        if (changed.Get(data.entries.Get(i).first) != 1) { throw std::runtime_error("Benchmark: update failed"); }
    }

    Vector removed(vector);
    result.remove = Measure([&]()
    {
        for (std::size_t i = 0; i < data.entries.GetLength(); ++i) { removed.Set(data.entries.Get(i).first, 0); }
    });
    if (removed.GetNonZeroCount() != 0 || removed.GetLength() != data.length)
    {
        throw std::runtime_error("Benchmark: deletion failed");
    }

    Vector mapped(0);
    result.map = Measure([&]() { mapped = vector.Map(IdentityValue); });
    CheckBenchmarkVector(mapped, data);
    Vector filtered(0);
    result.where = Measure([&]() { filtered = vector.Where(PositiveValue); });
    if (filtered.GetLength() != data.length || filtered.GetNonZeroCount() != data.positiveCount)
    {
        throw std::runtime_error("Benchmark: wrong Where size");
    }
    for (std::size_t i = 0; i < data.ordered.GetLength(); ++i)
    {
        const VectorEntry& entry = data.ordered.Get(i);
        if (filtered.Get(entry.first) != (entry.second > 0 ? entry.second : 0))
        {
            throw std::runtime_error("Benchmark: Where failed");
        }
    }
    result.reduce = Measure([&]() { result.fold = vector.Reduce(XorValues, 0); });
    if (result.fold != data.fold) { throw std::runtime_error("Benchmark: Reduce failed"); }
    CheckBenchmarkVector(vector, data);
    return result;
}

inline void WriteBenchmarkHeader(std::ostream& output)
{
    output << "length,nonzero,storage,order,repeat,seed,missing_queries,build_ms,bulk_ms,read_ms,missing_ms,"
              "update_ms,remove_ms,map_ms,where_ms,reduce_ms,checksum,fold\n";
}

template<typename Storage>
void WriteBenchmark(std::ostream& output, const BenchmarkData& data, const char* storage,
                    DataOrder order, int repeat, unsigned int seed)
{
    BenchmarkResult result = RunBenchmark<Storage>(data);
    output << std::fixed << std::setprecision(6)
           << data.length << ',' << data.entries.GetLength() << ',' << storage << ',' << GetOrderName(order) << ','
           << repeat << ',' << seed << ',' << data.missing.GetLength() << ','
           << result.build << ',' << result.bulk << ',' << result.read << ',' << result.missing << ','
           << result.update << ',' << result.remove << ',' << result.map << ',' << result.where << ','
           << result.reduce << ',' << result.checksum << ',' << result.fold << '\n';
    output.flush();
    std::cout << storage << ", k=" << data.entries.GetLength() << ", " << GetOrderName(order)
              << ", повтор " << repeat << ": построение " << result.build << " мс, чтение " << result.read
              << " мс, удаление " << result.remove << " мс\n";
}

inline void CompareStorages(std::ostream& output, const BenchmarkData& data, DataOrder order,
                            int repeat, unsigned int seed)
{
    // Чередуем порядок запуска, чтобы одна реализация не была постоянно первой.
    if (repeat % 2 == 1)
    {
        WriteBenchmark<TreeStorage<int>>(output, data, "tree", order, repeat, seed);
        WriteBenchmark<ArrayStorage<int>>(output, data, "array", order, repeat, seed);
    }
    else
    {
        WriteBenchmark<ArrayStorage<int>>(output, data, "array", order, repeat, seed);
        WriteBenchmark<TreeStorage<int>>(output, data, "tree", order, repeat, seed);
    }
}

#endif
