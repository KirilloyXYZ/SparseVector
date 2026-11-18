#ifndef GUIAPP_HPP
#define GUIAPP_HPP

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "GuiData.hpp"

// Снимки нужны только для отображения. Рабочий поток меняет контейнер,
// а окно продолжает читать последний готовый снимок.
template<typename Storage>
struct GuiVector
{
    SparseVector<int, Storage> vector{0};
    SparseVector<int, Storage> result{0};
    ArraySequence<VectorEntry> entries;
    ArraySequence<VectorEntry> resultEntries;
    std::size_t length = 0;
    bool hasResult = false;
    std::string resultTitle;
};

class GuiApp
{
public:
    GuiApp() = default;
    ~GuiApp();
    GuiApp(const GuiApp&) = delete;
    GuiApp& operator=(const GuiApp&) = delete;

    void Draw();
    void RequestClose();
    bool ShouldClose() const;

private:
    GuiVector<TreeStorage<int>> tree;
    GuiVector<ArrayStorage<int>> array;
    int storage = 0;
    long long index = 0;
    long long value = 0;
    std::string status = "Создайте вектор или воспользуйтесь генерацией";
    std::string error;
    bool closeRequested = false;

    bool openCreate = false;
    bool openGenerate = false;
    long long createLength = 10;
    int inputMode = 0;
    std::string inputText;
    std::string dialogError;
    long long generateLength = 1000;
    long long generateCount = 100;
    long long generateMinimum = -9;
    long long generateMaximum = 9;
    long long generateSeed = 17;
    int generateOrder = 0;

    int mapChoice = 0;
    int whereChoice = 0;
    long long sumStart = 0;
    bool hasSum = false;
    int sum = 0;

    int benchmarkMode = 0;
    bool benchmarkSettingsOpen = true;
    long long benchmarkMaximum = 1000;
    long long benchmarkLength = 10000;
    long long benchmarkRepeats = 3;
    long long benchmarkSeed = 17;
    bool automaticLength = false;
    int resultOrder = 0;
    int resultOperation = 0;
    int resultSize = 0;
    bool logarithmicX = true;
    std::string csvPath = "gui/results/gui-benchmark.csv";
    std::string benchmarkDescription;
    std::vector<BenchmarkRecord> records;
    std::vector<BenchmarkRecord> pendingRecords;

    std::thread worker;
    std::atomic<bool> running{false};
    std::atomic<bool> cancelRequested{false};
    std::atomic<std::size_t> completed{0};
    std::atomic<std::size_t> total{0};
    std::function<void(bool)> finishTask;
    std::string taskName;
    std::string taskError;
    bool taskCancelled = false;

    bool IsBusy() const;
    void StartTask(const char* name, std::function<void()> action, std::function<void(bool)> finish);
    void PollTask();
    void CheckCancellation() const;
    void ShowError(const std::exception& exception);
    void DrawCreateDialog();
    void DrawGenerateDialog();
    void DrawBenchmarks();
    void DrawBenchmarkResults();
    void DrawBenchmarkGraph(const std::vector<std::size_t>& sizes);
    void StartBenchmarks();
    void SaveCsv();
    double GetMedianTime(std::size_t count, const char* storageName, int operation) const;

    template<typename Storage> void Refresh(GuiVector<Storage>& state);
    template<typename Storage> void DrawVector(GuiVector<Storage>& state);
    template<typename Storage> void DrawOperations(GuiVector<Storage>& state);
    template<typename Storage> void CreateVector(GuiVector<Storage>& state, std::size_t length,
                                                ArraySequence<VectorEntry> entries);
    template<typename Storage> void GenerateVector(GuiVector<Storage>& state);
};

#endif
