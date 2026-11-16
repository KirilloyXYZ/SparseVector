#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

#include "SparseVector.hpp"
#include "ArrayStorage.hpp"
#include "Benchmark.hpp"
#include "DataGeneration.hpp"
#include "VectorOperations.hpp"

long long ReadNumber(const char* prompt, long long minimum, long long maximum)
{
    std::string line;
    while (true)
    {
        std::cout << prompt;
        if (!std::getline(std::cin, line)) { throw std::runtime_error("Ввод завершён"); }
        std::istringstream input(line);
        long long value;
        if (input >> value && (input >> std::ws).eof() && value >= minimum && value <= maximum) { return value; }
        std::cout << "Введите целое число от " << minimum << " до " << maximum << ".\n";
    }
}

int ReadInt(const char* prompt)
{
    return static_cast<int>(ReadNumber(prompt, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
}

std::size_t ReadLength()
{
    return static_cast<std::size_t>(ReadNumber("Длина вектора (0–1000000000): ", 0, 1000000000));
}

std::size_t ReadIndex(std::size_t length)
{
    if (length == 0) { throw std::logic_error("Сначала создайте вектор ненулевой длины"); }
    std::cout << "Индекс от 0 до " << length - 1 << '\n';
    return static_cast<std::size_t>(ReadNumber("Индекс: ", 0, static_cast<long long>(length - 1)));
}

unsigned int ReadSeed()
{
    return static_cast<unsigned int>(ReadNumber("Seed: ", 0, std::numeric_limits<unsigned int>::max()));
}

template<typename Storage>
void ShowVector(const SparseVector<int, Storage>& vector)
{
    vector.Print(std::cout);
    if (vector.GetLength() <= 20)
    {
        std::cout << '[';
        for (std::size_t i = 0; i < vector.GetLength(); ++i)
        {
            if (i > 0) { std::cout << ", "; }
            std::cout << vector.Get(i);
        }
        std::cout << "]\n";
    }
}

template<typename Storage>
SparseVector<int, Storage> ReadVector()
{
    using Vector = SparseVector<int, Storage>;
    std::size_t length = ReadLength();
    int mode = static_cast<int>(ReadNumber("Ввод: 1 — все значения, 2 — пары индекс/значение: ", 1, 2));
    if (mode == 1)
    {
        if (length > 10000) { throw std::invalid_argument("Для полного ручного ввода доступно до 10000 значений; используйте пары"); }
        ArraySequence<int> values(length);
        for (std::size_t i = 0; i < length; ++i)
        {
            std::cout << "Позиция " << i << '\n';
            values.Append(ReadInt("Значение: "));
        }
        return Vector(values);
    }
    std::size_t count = static_cast<std::size_t>(ReadNumber("Количество пар (повторы допустимы): ", 0,
                                                                          length == 0 ? 0 : 1000000));
    ArraySequence<VectorEntry> entries(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        std::size_t index = ReadIndex(length);
        entries.Append(VectorEntry{index, ReadInt("Значение: ")});
    }
    return Vector::FromEntries(length, entries);
}

template<typename Storage>
SparseVector<int, Storage> GenerateVector()
{
    std::size_t length = ReadLength();
    std::size_t count = static_cast<std::size_t>(ReadNumber("Количество ненулевых элементов: ", 0,
        static_cast<long long>(std::min<std::size_t>(length, 1000000))));
    int minimum = ReadInt("Минимальное значение: ");
    int maximum = ReadInt("Максимальное значение: ");
    int order = static_cast<int>(ReadNumber("Порядок вставки: 1 — случайный, 2 — возрастающий, 3 — убывающий: ", 1, 3));
    unsigned int seed = ReadSeed();
    auto entries = GenerateEntries(length, count, minimum, maximum, static_cast<DataOrder>(order - 1), seed);
    SparseVector<int, Storage> result(length);
    for (std::size_t i = 0; i < entries.GetLength(); ++i) { result.Set(entries.Get(i).first, entries.Get(i).second); }
    return result;
}

template<typename Storage>
void CompareCurrent(const SparseVector<int, Storage>& vector)
{
    int repeats = static_cast<int>(ReadNumber("Количество повторов (1–15): ", 1, 15));
    unsigned int seed = ReadSeed();
    std::cout << "Путь CSV (Enter — results/interactive.csv): ";
    std::string name;
    if (!std::getline(std::cin, name)) { throw std::runtime_error("Ввод завершён"); }
    std::filesystem::path path = name.empty() ? "results/interactive.csv" : name;
    if (path.has_parent_path()) { std::filesystem::create_directories(path.parent_path()); }
    std::ofstream output(path);
    if (!output) { throw std::runtime_error("Не удалось открыть файл результатов"); }
    output.exceptions(std::ios::badbit | std::ios::failbit);
    WriteBenchmarkHeader(output);
    auto original = vector.GetEntries();
    std::cout << "Исходный вектор:\n";
    ShowVector(vector);
    std::cout << "Замеры обоих хранилищ для трёх порядков вставки. Все времена — в миллисекундах.\n";
    for (DataOrder order : {DataOrder::Random, DataOrder::Ascending, DataOrder::Descending})
    {
        for (int repeat = 1; repeat <= repeats; ++repeat)
        {
            unsigned int trialSeed = seed + static_cast<unsigned int>(repeat - 1);
            auto entries = original;
            ArrangeEntries(entries, order, trialSeed);
            auto data = PrepareBenchmark(vector.GetLength(), entries);
            CompareStorages(output, data, order, repeat, trialSeed);
        }
    }
    std::cout << "Результаты сохранены: " << path << '\n';
    std::cout << "Графики: python3 plot_benchmarks.py <путь к CSV>\n";
}

template<typename Storage>
void RunMenu(SparseVector<int, Storage>& vector, const char* name)
{
    while (true)
    {
        try
        {
            std::cout << '\n' << name << ": N=" << vector.GetLength() << ", k=" << vector.GetNonZeroCount() << '\n';
            std::cout << "1 — Создать и ввести  2 — Записать значение  3 — Прочитать значение\n"
                         "4 — Очистить         5 — Генерация          6 — Показать вектор\n"
                         "7 — Map              8 — Where              9 — Reduce\n"
                         "10 — Сравнить хранилища                     0 — Назад\n";
            int command = static_cast<int>(ReadNumber("> ", 0, 10));
            switch (command)
            {
                case 0:
                    return;
                case 1:
                    vector = ReadVector<Storage>();
                    ShowVector(vector);
                    break;
                case 2:
                {
                    std::size_t index = ReadIndex(vector.GetLength());
                    vector.Set(index, ReadInt("Значение (0 удаляет запись): "));
                    ShowVector(vector);
                    break;
                }
                case 3:
                {
                    std::size_t index = ReadIndex(vector.GetLength());
                    std::cout << "Значение: " << vector.Get(index) << '\n';
                    break;
                }
                case 4:
                    vector.Clear();
                    ShowVector(vector);
                    break;
                case 5:
                    vector = GenerateVector<Storage>();
                    ShowVector(vector);
                    break;
                case 6:
                    ShowVector(vector);
                    break;
                case 7:
                case 8:
                {
                    SparseVector<int, Storage> result(0);
                    if (command == 7)
                    {
                        int choice = static_cast<int>(ReadNumber("Map: 1 — сменить знак, 2 — умножить на 2, 3 — квадрат: ", 1, 3));
                        result = vector.Map(choice == 1 ? NegateValue : choice == 2 ? DoubleValue : SquareValue);
                    }
                    else
                    {
                        int choice = static_cast<int>(ReadNumber("Where: 1 — положительные, 2 — чётные: ", 1, 2));
                        result = vector.Where(choice == 1 ? IsPositive : IsEven);
                    }
                    std::cout << "Исходный вектор:\n";
                    ShowVector(vector);
                    std::cout << "Результат (исходный вектор сохранён):\n";
                    ShowVector(result);
                    break;
                }
                case 9:
                {
                    int start = ReadInt("Начальное значение суммы: ");
                    int result = vector.Reduce(SumValues, start);
                    std::cout << "Сумма ненулевых значений: " << result << '\n';
                    break;
                }
                case 10:
                    CompareCurrent(vector);
                    break;
            }
        }
        catch (const std::exception& error)
        {
            if (!std::cin) { return; }
            std::cout << "Ошибка: " << error.what() << '\n';
        }
    }
}

int main()
{
    SparseVector<int> treeVector(0);
    SparseVector<int, ArrayStorage<int>> arrayVector(0);
    try
    {
        while (true)
        {
            std::cout << "\nХранилище: 1 — AVL-дерево, 2 — отсортированный массив, 0 — выход\n";
            int choice = static_cast<int>(ReadNumber("> ", 0, 2));
            if (choice == 0) { return 0; }
            if (choice == 1) { RunMenu(treeVector, "AVL-дерево"); }
            else { RunMenu(arrayVector, "Отсортированный массив"); }
            if (!std::cin) { return 0; }
        }
    }
    catch (const std::exception& error)
    {
        if (!std::cin) { return 0; }
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
}
