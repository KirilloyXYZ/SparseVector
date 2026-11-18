#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "Benchmark.hpp"

long long ReadArgument(const char* text, long long minimum, long long maximum)
{
    std::string argument(text);
    std::size_t end = 0;
    long long value = std::stoll(argument, &end);
    if (end != argument.size() || value < minimum || value > maximum)
    {
        throw std::invalid_argument("Argument is outside the allowed range");
    }
    return value;
}

void RunExperiments(std::ostream& output, std::size_t count, int repeats, unsigned int seed, std::size_t fixedLength)
{
    std::size_t length = fixedLength == 0 ? count * 10 : fixedLength;
    for (DataOrder order : {DataOrder::Random, DataOrder::Ascending, DataOrder::Descending})
    {
        for (int repeat = 1; repeat <= repeats; ++repeat)
        {
            unsigned int trialSeed = seed + static_cast<unsigned int>(repeat - 1);
            auto entries = GenerateEntries(length, count, -9, 9, order, trialSeed);
            BenchmarkData data = PrepareBenchmark(length, entries);
            CompareStorages(output, data, order, repeat, trialSeed);
        }
    }
}

int main(int argc, char* argv[])
{
    try
    {
        if (argc > 6)
        {
            throw std::invalid_argument("Usage: benchmark [max_nonzero] [repeats] [seed] [output.csv] [length]");
        }
        std::size_t maximum = argc > 1 ? static_cast<std::size_t>(ReadArgument(argv[1], 1, 1000000)) : 10000;
        int repeats = argc > 2 ? static_cast<int>(ReadArgument(argv[2], 1, 15)) : 3;
        unsigned int seed = argc > 3 ? static_cast<unsigned int>(ReadArgument(
            argv[3], 0, std::numeric_limits<unsigned int>::max())) : 17;
        std::filesystem::path path = argc > 4 ? argv[4] : "gui/results/benchmark.csv";
        std::size_t length = argc > 5 ? static_cast<std::size_t>(ReadArgument(argv[5], 1, 1000000000)) : 0;
        if (length != 0 && length < maximum) { throw std::invalid_argument("Length must be at least max_nonzero"); }
        if (path.has_parent_path()) { std::filesystem::create_directories(path.parent_path()); }
        std::ofstream output(path);
        if (!output) { throw std::runtime_error("Cannot open benchmark output"); }
        output.exceptions(std::ios::badbit | std::ios::failbit);
        WriteBenchmarkHeader(output);
        for (std::size_t count : {100, 300, 1000, 3000, 10000, 30000, 100000, 300000, 1000000})
        {
            if (count < maximum) { RunExperiments(output, count, repeats, seed, length); }
        }
        RunExperiments(output, maximum, repeats, seed, length);
        std::cout << "Результаты сохранены: " << path << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
