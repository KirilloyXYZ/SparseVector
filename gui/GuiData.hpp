#ifndef GUIDATA_HPP
#define GUIDATA_HPP

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../benchmarks/Benchmark.hpp"

enum class VectorInputMode { Empty, Values, Entries };

inline long long ParseInteger(const std::string& text)
{
    const char* first = text.data();
    const char* last = first + text.size();
    bool plus = first != last && *first == '+';
    if (plus) { ++first; }
    long long value = 0;
    auto parsed = std::from_chars(first, last, value);
    if (parsed.ec != std::errc() || parsed.ptr != last || (plus && first != last && *first == '-'))
    {
        throw std::invalid_argument("Ожидалось целое число: " + text.substr(0, 40));
    }
    return value;
}

inline ArraySequence<VectorEntry> ParseVectorInput(std::size_t length, VectorInputMode mode,
                                                  const std::string& text)
{
    if (length > 1000000000) { throw std::invalid_argument("Длина должна быть от 0 до 1000000000"); }
    ArraySequence<VectorEntry> entries;
    if (mode == VectorInputMode::Empty) { return entries; }
    if (mode != VectorInputMode::Values && mode != VectorInputMode::Entries)
    {
        throw std::invalid_argument("Неизвестный способ ввода");
    }
    if (mode == VectorInputMode::Values && length > 10000)
    {
        throw std::invalid_argument("Для полного ввода доступно до 10000 значений. Используйте пары");
    }
    std::istringstream input(text);
    std::string token;
    std::size_t count = 0;
    while (input >> token)
    {
        if (count == 1000000) { throw std::length_error("Допустимо не более 1000000 пар"); }
        std::size_t index = count;
        long long value = ParseInteger(token);
        if (mode == VectorInputMode::Entries)
        {
            if (value < 0 || static_cast<unsigned long long>(value) >= length)
            {
                throw std::out_of_range("Индекс пары выходит за границы вектора");
            }
            index = static_cast<std::size_t>(value);
            if (!(input >> token)) { throw std::invalid_argument("Для последнего индекса не указано значение"); }
            value = ParseInteger(token);
        }
        else if (count >= length) { throw std::invalid_argument("Введено больше значений, чем длина вектора"); }
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::out_of_range("Значение должно помещаться в int");
        }
        // Нули в парах сохраняем: последняя запись может удалять предыдущее значение.
        if (mode == VectorInputMode::Entries || value != 0) { entries.Append({index, static_cast<int>(value)}); }
        ++count;
    }
    if (mode == VectorInputMode::Values && count != length)
    {
        throw std::invalid_argument("Количество значений должно совпадать с длиной вектора");
    }
    return entries;
}

inline double GetBenchmarkTime(const BenchmarkResult& result, int operation)
{
    switch (operation)
    {
        case 0: return result.build;
        case 1: return result.bulk;
        case 2: return result.read;
        case 3: return result.missing;
        case 4: return result.update;
        case 5: return result.remove;
        case 6: return result.map;
        case 7: return result.where;
        case 8: return result.reduce;
        default: throw std::out_of_range("Неизвестная операция замера");
    }
}

inline double Median(std::vector<double> values)
{
    if (values.empty()) { throw std::invalid_argument("Нет результатов для вычисления медианы"); }
    std::sort(values.begin(), values.end());
    std::size_t middle = values.size() / 2;
    if (values.size() % 2 == 1) { return values[middle]; }
    return values[middle - 1] / 2 + values[middle] / 2;
}

inline std::vector<std::size_t> GetExperimentSizes(std::size_t maximum)
{
    if (maximum == 0 || maximum > 1000000) { throw std::invalid_argument("Максимальное k должно быть от 1 до 1000000"); }
    std::vector<std::size_t> sizes;
    for (std::size_t size : {100, 300, 1000, 3000, 10000, 30000, 100000, 300000, 1000000})
    {
        if (size < maximum) { sizes.push_back(size); }
    }
    sizes.push_back(maximum);
    return sizes;
}

#endif
