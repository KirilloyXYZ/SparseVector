#include <complex>
#include <cstddef>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "SparseVector.hpp"
#include "ArrayStorage.hpp"
#include "Benchmark.hpp"
#include "VectorOperations.hpp"

void Check(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}

template<typename Exception, typename Action>
void CheckThrows(Action action)
{
    try { action(); }
    catch (const Exception&) { return; }
    throw std::runtime_error("Expected exception was not thrown");
}

int Negate(const int& value) { return -value; }
int Zero(const int&) { return 0; }
int Increase(const int& value) { return value + 1; }
int Fold(const int& value, const int& result) { return 2 * value + 3 * result; }
bool Positive(const int& value) { return value > 0; }
bool None(const int&) { return false; }
bool All(const int&) { return true; }
bool Descending(const int& left, const int& right) { return left > right; }

int FailingMap(const int& value)
{
    if (value == 7) { throw std::runtime_error("Map failed"); }
    return value;
}

bool FailingWhere(const int&)
{
    throw std::runtime_error("Where failed");
}

int FailingReduce(const int&, const int&)
{
    throw std::runtime_error("Reduce failed");
}

struct ThrowingValue
{
    int number;
    static int copiesLeft;

    explicit ThrowingValue(int number = 0) : number(number) { }
    ThrowingValue(const ThrowingValue& other) : number(other.number) { CheckCopy(); }
    ThrowingValue& operator=(const ThrowingValue& other)
    {
        this->number = other.number;
        CheckCopy();
        return *this;
    }

    static void CheckCopy()
    {
        if (copiesLeft == 0) { throw std::runtime_error("Copy failed"); }
        if (copiesLeft > 0) { --copiesLeft; }
    }

    bool operator==(const ThrowingValue& other) const { return this->number == other.number; }
    bool operator<(const ThrowingValue& other) const { return this->number < other.number; }
};

int ThrowingValue::copiesLeft = -1;

void TestDynamicArray()
{
    DynamicArray<int> values;
    CheckThrows<std::out_of_range>([&]() { values.Get(0); });
    CheckThrows<std::out_of_range>([&]() { values.RemoveAt(0); });
    CheckThrows<std::invalid_argument>([]() { DynamicArray<int> invalid(nullptr, 1); });
    values.PushBack(3);
    values.PushBack(values.Get(0));
    Check(values.GetSize() == 2 && values.Get(1) == 3, "Aliased append during growth");
    values.InsertAt(0, values.Get(1));
    values.InsertAt(1, 2);
    values.InsertAt(values.GetSize(), 4);
    Check(values.GetSize() == 5 && values.Get(1) == 2 && values.Get(4) == 4, "Array insertion");
    values.Reserve(20);
    const int* first = &values.Get(0);
    values.InsertAt(1, 8);
    values.RemoveAt(2);
    Check(&values.Get(0) == first && values.GetCapacity() == 20, "Array reuses capacity");
    Check(values.Get(0) == 3 && values.Get(1) == 8 && values.Get(2) == 3, "Array shifts elements");
    CheckThrows<std::out_of_range>([&]() { values.InsertAt(values.GetSize() + 1, 0); });
    DynamicArray<int> copy(values);
    copy.Get(0) = 100;
    Check(values.Get(0) == 3, "Independent array copy");
    DynamicArray<int> moved(std::move(copy));
    Check(copy.GetSize() == 0 && moved.Get(0) == 100, "Array move resets source");
}

void TestSortedSequence()
{
    int input[] = {4, 1, 3, 1, 2};
    ArraySequence<int> source(input, 5);
    SortedSequence<int> values(source);
    ISortedSequence<int>& base = values;
    Check(base.GetLength() == 5 && base.GetFirst() == 1 && base.GetLast() == 4, "Bulk sorting");
    Check(base.IndexOf(1) == 0 && base.IndexOf(5) == -1, "Binary search and duplicates");
    base.Add(0);
    base.Add(5);
    base.RemoveAt(1);
    Check(base.GetLength() == 6 && base.Get(0) == 0 && base.Get(1) == 1, "Sorted changes");
    auto part = base.GetSubsequence(1, 3);
    Check(part->GetLength() == 3 && part->GetFirst() == 1 && part->GetLast() == 3, "Inclusive subsequence");
    part->Add(2);
    Check(base.GetLength() == 6, "Independent subsequence");
    CheckThrows<std::out_of_range>([&]() { base.Get(6); });
    CheckThrows<std::invalid_argument>([&]() { base.GetSubsequence(2, 1); });
    CheckThrows<std::out_of_range>([&]() { base.GetSubsequence(0, 6); });
    CheckThrows<std::invalid_argument>([&]() { values.Replace(0, 100); });
    CheckThrows<std::invalid_argument>([]() { SortedSequence<int> invalid(nullptr); });

    SortedSequence<int> reversed(source, Descending);
    Check(reversed.GetFirst() == 4 && reversed.GetLast() == 1 && reversed.IndexOf(3) == 1,
          "Custom sorted comparison");
    auto iterator = reversed.GetEnumerator();
    CheckThrows<std::out_of_range>([&]() { iterator->GetCurrent(); });
    CheckThrows<std::out_of_range>([&]() { iterator->GetCurrentKey(); });
    Check(iterator->MoveNext() && iterator->GetCurrentKey() == 0 && iterator->GetCurrent() == 4,
          "Sequence enumerator starts at first element");
    Check(iterator->GetCurrent() == 4 && iterator->GetCurrentKey() == 0, "Reading does not advance enumerator");
    reversed.Add(6);
    CheckThrows<std::logic_error>([&]() { iterator->MoveNext(); });
    CheckThrows<std::logic_error>([&]() { iterator->GetCurrent(); });

    base.Clear();
    Check(base.IsEmpty() && !base.GetEnumerator()->MoveNext(), "Empty sorted sequence");
    CheckThrows<std::out_of_range>([&]() { base.GetFirst(); });
    CheckThrows<std::out_of_range>([&]() { base.GetLast(); });
}

void TestTree()
{
    BinarySearchTree<int, int> tree;
    int keys[] = {4, 2, 7, 1, 3, 6, 8, 5};
    for (int key : keys) { tree.Insert(key, key * 10); }
    Check(tree.IsValid() && tree.Find(100) == nullptr && *tree.Find(4) == 40, "Tree search");
    auto iterator = tree.GetEnumerator();
    CheckThrows<std::out_of_range>([&]() { iterator->GetCurrent(); });
    CheckThrows<std::out_of_range>([&]() { iterator->GetCurrentKey(); });
    Check(!tree.Update(100, 1), "Missing update");
    Check(iterator->MoveNext() && iterator->GetCurrentKey() == 1, "Missing update preserves enumerator");
    Check(tree.Update(4, 99) && tree.Get(4) == 99 && tree.GetCount() == 8 && tree.IsValid(), "Tree update");
    CheckThrows<std::logic_error>([&]() { iterator->GetCurrent(); });
    CheckThrows<std::logic_error>([&]() { iterator->MoveNext(); });
    iterator = tree.GetEnumerator();
    int expectedKey = 1;
    while (iterator->MoveNext())
    {
        Check(iterator->GetCurrentKey() == expectedKey++, "Tree ordered traversal");
    }
    Check(expectedKey == 9 && !iterator->MoveNext() && !iterator->MoveNext(), "Tree enumerator end");
    CheckThrows<std::out_of_range>([&]() { iterator->GetCurrent(); });
    CheckThrows<std::out_of_range>([&]() { iterator->GetCurrentKey(); });

    BinarySearchTree<int, int> copy(tree);
    copy.Remove(4);
    Check(tree.Contains(4) && copy.IsValid(), "Independent tree copy");
    auto oldSource = tree.GetEnumerator();
    BinarySearchTree<int, int> moved(std::move(tree));
    Check(tree.IsValid() && tree.GetCount() == 0 && moved.GetCount() == 8, "Tree move constructor");
    CheckThrows<std::logic_error>([&]() { oldSource->MoveNext(); });
    auto oldTarget = copy.GetEnumerator();
    oldSource = moved.GetEnumerator();
    copy = std::move(moved);
    Check(copy.IsValid() && copy.GetCount() == 8 && moved.IsValid() && moved.GetCount() == 0,
          "Tree move assignment");
    CheckThrows<std::logic_error>([&]() { oldTarget->MoveNext(); });
    CheckThrows<std::logic_error>([&]() { oldSource->MoveNext(); });
    oldTarget = copy.GetEnumerator();
    copy = tree;
    Check(copy.IsValid() && copy.GetCount() == 0, "Tree copy assignment resets count");
    CheckThrows<std::logic_error>([&]() { oldTarget->MoveNext(); });
}

void TestTreeTraversals()
{
    BinarySearchTree<int, int> tree;
    for (int key : {4, 2, 7, 1, 3, 6, 8, 5}) { tree.Insert(key, key * 10); }
    TraversalOrder orders[] = {TraversalOrder::KLP, TraversalOrder::KPL, TraversalOrder::LKP,
                               TraversalOrder::LPK, TraversalOrder::PLK, TraversalOrder::PKL};
    int expected[6][8] = {
        {4, 2, 1, 3, 7, 6, 5, 8}, {4, 7, 8, 6, 5, 2, 3, 1},
        {1, 2, 3, 4, 5, 6, 7, 8}, {1, 3, 2, 5, 6, 8, 7, 4},
        {8, 5, 6, 7, 3, 1, 2, 4}, {8, 7, 6, 5, 4, 3, 2, 1}
    };
    for (std::size_t i = 0; i < 6; ++i)
    {
        auto enumerator = tree.GetEnumerator(orders[i]);
        for (int key : expected[i])
        {
            Check(enumerator->MoveNext() && enumerator->GetCurrentKey() == key && enumerator->GetCurrent() == key * 10,
                  "Tree traversal order and values");
        }
        Check(!enumerator->MoveNext() && !enumerator->MoveNext(), "Tree traversal ends after all elements");
        CheckThrows<std::out_of_range>([&]() { enumerator->GetCurrent(); });
    }
}

void TestRandomTree()
{
    BinarySearchTree<int, int> tree;
    bool present[100] = {};
    int values[100] = {};
    std::size_t count = 0;
    std::mt19937 random(19);
    for (int step = 1; step <= 3000; ++step)
    {
        int key = static_cast<int>(random() % 100);
        int operation = static_cast<int>(random() % 3);
        if (operation == 0)
        {
            bool inserted = tree.Insert(key, step);
            Check(inserted == !present[key], "Random tree insert");
            if (inserted) { present[key] = true; values[key] = step; ++count; }
        }
        else if (operation == 1)
        {
            Check(tree.Update(key, step) == present[key], "Random tree update");
            if (present[key]) { values[key] = step; }
        }
        else
        {
            Check(tree.Remove(key) == present[key], "Random tree remove");
            if (present[key]) { present[key] = false; --count; }
        }
        Check(tree.IsValid() && tree.GetCount() == count, "Random AVL invariants");
        auto iterator = tree.GetEnumerator();
        for (int i = 0; i < 100; ++i)
        {
            if (present[i])
            {
                Check(iterator->MoveNext() && iterator->GetCurrentKey() == i && iterator->GetCurrent() == values[i],
                      "Random tree contents");
            }
        }
        Check(!iterator->MoveNext(), "Random tree traversal count");
    }
}

void TestDictionary()
{
    TreeDictionary<std::string, int> dictionary;
    IDictionary<std::string, int>& base = dictionary;
    Check(base.GetCount() == 0 && base.Find("missing") == nullptr, "Empty dictionary");
    CheckThrows<std::out_of_range>([&]() { base.Get("missing"); });
    CheckThrows<std::out_of_range>([&]() { base.Remove("missing"); });
    base.Add("b", 2);
    base.Add("a", 1);
    base.Set("b", 3);
    base.Set("c", 4);
    Check(base.GetCount() == 3 && base.Get("b") == 3 && base.ContainsKey("c"), "Dictionary operations");
    CheckThrows<std::invalid_argument>([&]() { base.Add("b", 99); });
    Check(base.Get("b") == 3 && dictionary.IsValid(), "Duplicate Add keeps value");
    auto keys = base.GetKeys();
    auto values = base.GetValues();
    Check(keys.Get(0) == "a" && keys.Get(2) == "c" && values.Get(1) == 3, "Dictionary extraction");
    TreeDictionary<std::string, int> copy(dictionary);
    copy.Remove("a");
    Check(dictionary.ContainsKey("a") && !copy.ContainsKey("a"), "Dictionary independent copy");
    base.Clear();
    Check(base.GetCount() == 0 && dictionary.IsValid(), "Dictionary clear");
}

template<typename Storage>
void TestVectorBasics()
{
    SparseVector<int, Storage> empty(0);
    Check(empty.GetNonZeroCount() == 0 && !empty.GetEnumerator()->MoveNext(), "Empty vector");
    CheckThrows<std::out_of_range>([&]() { empty.Get(0); });
    CheckThrows<std::out_of_range>([&]() { empty.Set(0, 1); });
    SparseVector<int, Storage> vector(10);
    vector.Set(9, -3);
    vector.Set(0, 7);
    vector.Set(5, 2);
    Check(vector.Get(1) == 0 && vector.GetNonZeroCount() == 3, "Missing value is zero");
    vector.Set(0, 8);
    vector.Set(1, 0);
    Check(vector.Get(0) == 8 && vector.GetNonZeroCount() == 3, "Replace and absent zero");
    vector.Set(5, 0);
    vector.Set(5, 0);
    Check(vector.Get(5) == 0 && vector.GetNonZeroCount() == 2, "Zero removes entry once");
    CheckThrows<std::out_of_range>([&]() { vector.Get(10); });
    CheckThrows<std::out_of_range>([&]() { vector.Set(std::numeric_limits<std::size_t>::max(), 1); });
    auto entries = vector.GetEntries();
    Check(entries.GetLength() == 2 && entries.Get(0).first == 0 && entries.Get(1).first == 9,
          "Sparse entries are ordered");
    vector.Clear();
    vector.Clear();
    Check(vector.GetLength() == 10 && vector.GetNonZeroCount() == 0 && vector.Get(9) == 0, "Clear preserves length");
    vector.Set(9, 4);
    Check(vector.Get(9) == 4, "Reuse cleared vector");
    SparseVector<int, Storage> large(1000000000);
    large.Set(999999999, 1);
    Check(large.GetNonZeroCount() == 1 && large.Get(500000000) == 0, "Large logical length");
}

template<typename Storage>
void TestVectorConstruction()
{
    int input[] = {0, 3, 0, -2, 0};
    ArraySequence<int> values(input, 5);
    SparseVector<int, Storage> dense(values);
    Check(dense.GetLength() == 5 && dense.GetNonZeroCount() == 2 && dense.Get(3) == -2, "Dense conversion");
    using Vector = SparseVector<int, Storage>;
    ArraySequence<typename Vector::Entry> entries;
    entries.Append({4, 8});
    entries.Append({1, 3});
    entries.Append({4, 0});
    entries.Append({2, 7});
    entries.Append({1, 9});
    Vector vector = Vector::FromEntries(6, entries);
    Check(vector.GetLength() == 6 && vector.GetNonZeroCount() == 2 && vector.Get(1) == 9 && vector.Get(4) == 0,
          "Bulk conversion uses last assignment, including zero");
    Check(entries.Get(0).first == 4 && entries.Get(0).second == 8, "Bulk conversion preserves input");
    entries.Append({6, 1});
    CheckThrows<std::out_of_range>([&]() { Vector::FromEntries(6, entries); });
    ArraySequence<typename Vector::Entry> noEntries;
    Check(Vector::FromEntries(0, noEntries).GetNonZeroCount() == 0, "Empty bulk construction");
}

template<typename Storage>
void TestVectorCopyAndMove()
{
    using Vector = SparseVector<int, Storage>;
    Vector vector(10);
    vector.Set(2, 7);
    Vector copy(vector);
    copy.Set(2, 8);
    Check(vector.Get(2) == 7, "Vector independent copy constructor");
    Vector target(1);
    auto oldTarget = target.GetEnumerator();
    target = vector;
    Check(target.GetLength() == 10 && target.Get(2) == 7, "Vector copy assignment changes length");
    CheckThrows<std::logic_error>([&]() { oldTarget->MoveNext(); });
    auto oldSource = copy.GetEnumerator();
    Vector moved(std::move(copy));
    Check(copy.GetLength() == 0 && copy.GetNonZeroCount() == 0 && moved.Get(2) == 8, "Vector move constructor");
    CheckThrows<std::logic_error>([&]() { oldSource->MoveNext(); });
    oldTarget = target.GetEnumerator();
    oldSource = moved.GetEnumerator();
    target = std::move(moved);
    Check(target.Get(2) == 8 && moved.GetLength() == 0 && moved.GetNonZeroCount() == 0, "Vector move assignment");
    CheckThrows<std::logic_error>([&]() { oldTarget->MoveNext(); });
    CheckThrows<std::logic_error>([&]() { oldSource->MoveNext(); });
    Vector& alias = target;
    target = alias;
    target = std::move(alias);
    Check(target.GetLength() == 10 && target.Get(2) == 8, "Self assignment");
}

template<typename Storage>
void TestVectorFunctions()
{
    using Vector = SparseVector<int, Storage>;
    Vector vector(10);
    vector.Set(8, 3);
    vector.Set(2, -2);
    vector.Set(5, 7);
    auto mapped = vector.Map(Negate);
    Check(mapped.GetLength() == 10 && mapped.Get(2) == 2 && mapped.Get(5) == -7 && mapped.Get(0) == 0,
          "Map preserves indices and implicit zeros");
    auto zero = vector.Map(Zero);
    Check(zero.GetLength() == 10 && zero.GetNonZeroCount() == 0, "Map drops zeros");
    auto filtered = vector.Where(Positive);
    Check(filtered.GetLength() == 10 && filtered.GetNonZeroCount() == 2 && filtered.Get(2) == 0 && filtered.Get(8) == 3,
          "Where keeps length and indices");
    Check(vector.Where(None).GetNonZeroCount() == 0 && vector.Where(All).GetNonZeroCount() == 3,
          "Where none and all");
    // Индексы 2, 5, 8: Fold(-2, 4) = 8; Fold(7, 8) = 38; Fold(3, 38) = 120.
    Check(vector.Reduce(Fold, 4) == 120, "Reduce order, argument order and skipped implicit zeros");
    mapped.Set(2, 0);
    filtered.Clear();
    Check(vector.GetNonZeroCount() == 3 && vector.Get(2) == -2, "Functional results are independent");
    CheckThrows<std::invalid_argument>([&]() { vector.Map(Increase); });
    CheckThrows<std::invalid_argument>([&]() { vector.Map(nullptr); });
    CheckThrows<std::invalid_argument>([&]() { vector.Where(nullptr); });
    CheckThrows<std::invalid_argument>([&]() { vector.Reduce(nullptr, 0); });
    CheckThrows<std::runtime_error>([&]() { vector.Map(FailingMap); });
    CheckThrows<std::runtime_error>([&]() { vector.Where(FailingWhere); });
    CheckThrows<std::runtime_error>([&]() { vector.Reduce(FailingReduce, 0); });
    Check(vector.GetNonZeroCount() == 3 && vector.Get(5) == 7, "Failed callbacks preserve source");
    Vector empty(10);
    Check(empty.Map(Negate).GetNonZeroCount() == 0 && empty.Where(Positive).GetLength() == 10 &&
          empty.Reduce(Fold, 9) == 9, "Empty functional operations");
    CheckThrows<std::invalid_argument>([&]() { empty.Map(Increase); });
}

template<typename Storage>
void TestVectorIterationAndPrint()
{
    SparseVector<int, Storage> vector(10);
    vector.Set(7, 2);
    vector.Set(1, 4);
    auto first = vector.GetEnumerator();
    auto second = vector.GetEnumerator();
    Check(first->MoveNext() && first->GetCurrentKey() == 1, "Vector enumerator first");
    Check(first->MoveNext() && first->GetCurrentKey() == 7 && first->GetCurrent() == 2, "Vector enumerator last");
    Check(!first->MoveNext() && !first->MoveNext(), "Vector enumerator end");
    CheckThrows<std::out_of_range>([&]() { first->GetCurrent(); });
    CheckThrows<std::out_of_range>([&]() { first->GetCurrentKey(); });
    Check(second->MoveNext() && second->GetCurrentKey() == 1, "Independent vector iterators");
    vector.Set(3, 0);
    Check(second->GetCurrent() == 4, "Absent zero keeps iterator valid");
    vector.Set(1, 9);
    CheckThrows<std::logic_error>([&]() { first->MoveNext(); });
    CheckThrows<std::logic_error>([&]() { second->GetCurrent(); });
    std::ostringstream output;
    vector.Print(output, 1);
    Check(output.str().find("1 -> 9") != std::string::npos && output.str().find("7 ->") == std::string::npos &&
          output.str().find("Показано 1") != std::string::npos, "Limited sparse output");
    CheckThrows<std::invalid_argument>([&]() { vector.Print(output, 0); });
}

template<typename Storage>
void TestRandomVector()
{
    SparseVector<int, Storage> vector(80);
    std::vector<int> model(80, 0);
    std::mt19937 random(27);
    for (int step = 0; step < 1500; ++step)
    {
        std::size_t index = random() % model.size();
        int value = static_cast<int>(random() % 11) - 5;
        vector.Set(index, value);
        model[index] = value;
        std::size_t count = 0;
        auto enumerator = vector.GetEnumerator();
        for (std::size_t i = 0; i < model.size(); ++i)
        {
            Check(vector.Get(i) == model[i], "Random vector matches independent dense model");
            if (model[i] != 0)
            {
                ++count;
                Check(enumerator->MoveNext() && enumerator->GetCurrentKey() == i && enumerator->GetCurrent() == model[i],
                      "Random sparse traversal");
            }
        }
        Check(count == vector.GetNonZeroCount() && !enumerator->MoveNext(), "Random nonzero count");
    }
}

template<template<typename> class Storage>
void TestVectorTypes()
{
    SparseVector<double, Storage<double>> real(4);
    real.Set(1, 1.5);
    real.Set(2, -0.0);
    Check(real.Get(1) == 1.5 && real.GetNonZeroCount() == 1, "Double and negative zero");
    real.Set(3, std::numeric_limits<double>::quiet_NaN());
    Check(real.GetNonZeroCount() == 2 && real.Get(3) != real.Get(3), "NaN is a stored value, not an index");
    SparseVector<std::string, Storage<std::string>> text(3);
    text.Set(2, "hello");
    Check(text.Get(0).empty() && text.Get(2) == "hello", "String default value");
    text.Set(2, "");
    Check(text.GetNonZeroCount() == 0, "Empty string removes entry");
    using Complex = std::complex<double>;
    SparseVector<Complex, Storage<Complex>> complex(3);
    complex.Set(1, Complex(2, -1));
    Check(complex.Get(1) == Complex(2, -1) && complex.Get(2) == Complex{}, "Complex values");
}

template<template<typename> class Storage>
void TestVectorExceptions()
{
    using Vector = SparseVector<ThrowingValue, Storage<ThrowingValue>>;
    Vector vector(10);
    vector.Set(8, ThrowingValue(7));
    Vector target(2);
    target.Set(1, ThrowingValue(3));
    ThrowingValue replacement(9);
    ThrowingValue::copiesLeft = 0;
    CheckThrows<std::runtime_error>([&]() { vector.Set(8, replacement); });
    CheckThrows<std::runtime_error>([&]() { target = vector; });
    ThrowingValue::copiesLeft = -1;
    Check(vector.Get(8).number == 7 && vector.GetNonZeroCount() == 1, "Failed update preserves entry");
    Check(target.GetLength() == 2 && target.Get(1).number == 3, "Failed copy preserves length and entries");
}

void TestArrayExceptions()
{
    DynamicArray<ThrowingValue> array;
    array.PushBack(ThrowingValue(1));
    array.PushBack(ThrowingValue(2));
    ThrowingValue value(3);
    ThrowingValue::copiesLeft = 2;
    CheckThrows<std::runtime_error>([&]() { array.InsertAt(1, value); });
    ThrowingValue::copiesLeft = 0;
    CheckThrows<std::runtime_error>([&]() { array.RemoveAt(0); });
    ThrowingValue::copiesLeft = -1;
    Check(array.GetSize() == 2 && array.Get(0).number == 1 && array.Get(1).number == 2,
          "Failed array shifts preserve contents");
}

bool CompareBuckets(const ThrowingValue& left, const ThrowingValue& right)
{
    return left.number / 10 < right.number / 10;
}

void TestReplacementExceptions()
{
    // Ошибка после нескольких удачных копирований не должна менять исходные данные.
    for (int limit = 0; limit <= 6; ++limit)
    {
        SortedSequence<ThrowingValue> values(CompareBuckets);
        values.Add(ThrowingValue(11));
        values.Add(ThrowingValue(21));
        values.Add(ThrowingValue(31));
        auto iterator = values.GetEnumerator();
        ThrowingValue replacement(22);
        bool failed = false;
        ThrowingValue::copiesLeft = limit;
        try { values.Replace(1, replacement); }
        catch (const std::runtime_error&) { failed = true; }
        ThrowingValue::copiesLeft = -1;
        Check(values.Get(0).number == 11 && values.Get(1).number == (failed ? 21 : 22) &&
              values.Get(2).number == 31, "Replace has strong exception guarantee");
        if (failed) { Check(iterator->MoveNext(), "Failed Replace preserves iterator"); }
        else { CheckThrows<std::logic_error>([&]() { iterator->MoveNext(); }); }
    }

    BinarySearchTree<ThrowingValue, int> tree;
    tree.Insert(2, ThrowingValue(20));
    tree.Insert(1, ThrowingValue(10));
    tree.Insert(3, ThrowingValue(30));
    ThrowingValue value(25);
    auto iterator = tree.GetEnumerator();
    ThrowingValue::copiesLeft = 0;
    CheckThrows<std::runtime_error>([&]() { tree.Update(2, value); });
    ThrowingValue::copiesLeft = -1;
    Check(tree.Get(2).number == 20 && tree.IsValid() && iterator->MoveNext(), "Failed tree replacement");
    Check(tree.Update(2, value) && tree.Get(2).number == 25 && tree.Get(1).number == 10 &&
          tree.Get(3).number == 30 && tree.GetCount() == 3 && tree.IsValid(), "Replacement keeps tree children");
    CheckThrows<std::logic_error>([&]() { iterator->MoveNext(); });
}

template<typename Storage>
void TestStorageLoad()
{
    Storage storage;
    storage.Set(1, 7);
    ArraySequence<VectorEntry> entries;
    entries.Append({3, 1});
    entries.Append({2, 2});
    CheckThrows<std::invalid_argument>([&]() { storage.Load(entries); });
    Check(storage.GetCount() == 1 && *storage.Find(1) == 7, "Failed bulk load preserves storage");
    entries.Get(0) = {2, 1};
    CheckThrows<std::invalid_argument>([&]() { storage.Load(entries); });
    entries.Get(1) = {3, 2};
    storage.Load(entries);
    Check(storage.GetCount() == 2 && storage.Find(1) == nullptr && *storage.Find(3) == 2, "Successful bulk load replaces data");
}

void TestGeneration()
{
    auto first = GenerateEntries(50, 30, -4, 4, DataOrder::Random, 17);
    auto repeated = GenerateEntries(50, 30, -4, 4, DataOrder::Random, 17);
    auto ascending = GenerateEntries(50, 30, -4, 4, DataOrder::Ascending, 17);
    auto descending = GenerateEntries(50, 30, -4, 4, DataOrder::Descending, 17);
    bool present[50] = {};
    for (std::size_t i = 0; i < first.GetLength(); ++i)
    {
        const auto& entry = first.Get(i);
        Check(entry == repeated.Get(i), "Reproducible generation");
        Check(entry.first < 50 && !present[entry.first], "Generated distinct indices");
        present[entry.first] = true;
        Check(entry.second >= -4 && entry.second <= 4 && entry.second != 0, "Generated nonzero values");
        Check(ascending.Get(i) == descending.Get(29 - i), "Orders use the same entries");
        if (i > 0) { Check(ascending.Get(i - 1).first < ascending.Get(i).first, "Ascending indices"); }
    }
    ArrangeEntries(first, DataOrder::Ascending, 0);
    for (std::size_t i = 0; i < first.GetLength(); ++i) { Check(first.Get(i) == ascending.Get(i), "Random order uses same data"); }
    Check(GenerateEntries(0, 0, 0, 0, DataOrder::Random, 0).GetLength() == 0, "Empty generation");
    auto full = GenerateEntries(5, 5, 1, 1, DataOrder::Ascending, 0);
    Check(full.GetLength() == 5 && full.Get(4) == VectorEntry(4, 1), "Full density generation");
    auto negative = GenerateEntries(5, 5, -1, 0, DataOrder::Ascending, 0);
    Check(negative.Get(0).second == -1, "Zero excluded from negative range");
    auto limits = GenerateEntries(10, 10, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(),
                                  DataOrder::Random, 1);
    Check(limits.GetLength() == 10, "Full integer range generation");
    CheckThrows<std::invalid_argument>([]() { GenerateEntries(1, 2, 1, 2, DataOrder::Random, 0); });
    CheckThrows<std::invalid_argument>([]() { GenerateEntries(2, 1, 0, 0, DataOrder::Random, 0); });
    CheckThrows<std::invalid_argument>([]() { GenerateEntries(2, 1, 2, 1, DataOrder::Random, 0); });
    CheckThrows<std::invalid_argument>([]() { GenerateEntries(0, 0, 0, 0, static_cast<DataOrder>(-1), 0); });
}

void TestBenchmark()
{
    auto entries = GenerateEntries(50, 20, -5, 5, DataOrder::Random, 3);
    auto data = PrepareBenchmark(50, entries);
    Check(data.missing.GetLength() == 20, "Prepared missing queries");
    auto tree = RunBenchmark<TreeStorage<int>>(data);
    auto array = RunBenchmark<ArrayStorage<int>>(data);
    Check(tree.checksum == array.checksum && tree.fold == array.fold, "Benchmark results agree");
    auto full = GenerateEntries(5, 5, -2, 2, DataOrder::Random, 0);
    auto dense = PrepareBenchmark(5, full);
    Check(dense.missing.GetLength() == 0, "No missing queries for full vector");
    RunBenchmark<TreeStorage<int>>(dense);
    RunBenchmark<ArrayStorage<int>>(dense);
    ArraySequence<VectorEntry> empty;
    auto noEntries = PrepareBenchmark(0, empty);
    RunBenchmark<TreeStorage<int>>(noEntries);
    RunBenchmark<ArrayStorage<int>>(noEntries);
    ArraySequence<VectorEntry> invalid;
    invalid.Append({1, 1});
    invalid.Append({1, 2});
    CheckThrows<std::invalid_argument>([&]() { PrepareBenchmark(5, invalid); });
    invalid.Get(1) = {2, 0};
    CheckThrows<std::invalid_argument>([&]() { PrepareBenchmark(5, invalid); });
    invalid.Get(1) = {5, 1};
    CheckThrows<std::invalid_argument>([&]() { PrepareBenchmark(5, invalid); });
    std::ostringstream output;
    WriteBenchmarkHeader(output);
    Check(output.str().find("missing_queries") != std::string::npos && output.str().find("reduce_ms") != std::string::npos,
          "Benchmark CSV header");
}

void TestArithmetic()
{
    Check(NegateValue(-2) == 2 && DoubleValue(3) == 6 && SquareValue(-4) == 16 && SumValues(2, 3) == 5,
          "Demonstration arithmetic");
    Check(SquareValue(46340) == 2147395600, "Largest safe integer square");
    CheckThrows<std::overflow_error>([]() { NegateValue(std::numeric_limits<int>::min()); });
    CheckThrows<std::overflow_error>([]() { DoubleValue(std::numeric_limits<int>::max()); });
    CheckThrows<std::overflow_error>([]() { SquareValue(46341); });
    CheckThrows<std::overflow_error>([]() { SumValues(1, std::numeric_limits<int>::max()); });
}

template<typename Storage>
void TestVector()
{
    TestVectorBasics<Storage>();
    TestVectorConstruction<Storage>();
    TestVectorCopyAndMove<Storage>();
    TestVectorFunctions<Storage>();
    TestVectorIterationAndPrint<Storage>();
    TestRandomVector<Storage>();
}

int main()
{
    try
    {
        TestDynamicArray();
        TestSortedSequence();
        TestTree();
        TestTreeTraversals();
        TestRandomTree();
        TestDictionary();
        TestVector<TreeStorage<int>>();
        TestVector<ArrayStorage<int>>();
        TestVectorTypes<TreeStorage>();
        TestVectorTypes<ArrayStorage>();
        TestVectorExceptions<TreeStorage>();
        TestVectorExceptions<ArrayStorage>();
        TestArrayExceptions();
        TestReplacementExceptions();
        TestStorageLoad<TreeStorage<int>>();
        TestStorageLoad<ArrayStorage<int>>();
        TestGeneration();
        TestBenchmark();
        TestArithmetic();
        std::cout << "All tests passed (both storages, containers, exceptions and random models)\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}
