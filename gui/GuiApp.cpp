#include "GuiApp.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <utility>

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "../lab2/VectorOperations.hpp"

namespace
{
    struct Cancelled { };

    const char* const operationNames[] = {
        "Построение через Set", "Массовая загрузка", "Чтение ненулевых", "Чтение нулевых",
        "Обновление", "Удаление", "Map (тождественный)", "Where (> 0)", "Reduce (XOR)"
    };
    const char* const orderNames[] = {"Случайный", "Возрастающий", "Убывающий"};

    void NumberInput(const char* label, long long& number)
    {
        if (label[0] != '#') { ImGui::TextUnformatted(label); }
        std::string id = std::string("##") + label;
        ImGui::InputScalar(id.c_str(), ImGuiDataType_S64, &number);
    }

    long long CheckRange(long long number, long long minimum, long long maximum, const char* name)
    {
        if (number < minimum || number > maximum)
        {
            throw std::out_of_range(std::string(name) + ": допустимо от " + std::to_string(minimum) + " до " + std::to_string(maximum));
        }
        return number;
    }

    int IntValue(long long number)
    {
        return static_cast<int>(CheckRange(number, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), "Значение"));
    }

    void DenseView(std::size_t length, const ArraySequence<VectorEntry>& entries)
    {
        if (length > 20) { return; }
        std::ostringstream output;
        output << '[';
        std::size_t position = 0;
        for (std::size_t i = 0; i < length; ++i)
        {
            if (i > 0) { output << ", "; }
            if (position < entries.GetLength() && entries.Get(position).first == i) { output << entries.Get(position++).second; }
            else { output << 0; }
        }
        output << ']';
        ImGui::TextWrapped("%s", output.str().c_str());
    }

    void EntryTable(const char* id, const ArraySequence<VectorEntry>& entries, float height,
                    long long* selectedIndex = nullptr, long long* selectedValue = nullptr)
    {
        ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
                                ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable;
        if (ImGui::BeginTable(id, 2, flags, ImVec2(0, height)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Индекс");
            ImGui::TableSetupColumn("Значение");
            ImGui::TableHeadersRow();
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(entries.GetLength()));
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
                {
                    const VectorEntry& entry = entries.Get(static_cast<std::size_t>(row));
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    if (selectedIndex != nullptr)
                    {
                        std::string label = std::to_string(entry.first);
                        bool selected = *selectedIndex >= 0 && static_cast<std::size_t>(*selectedIndex) == entry.first;
                        if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
                        {
                            *selectedIndex = static_cast<long long>(entry.first);
                            *selectedValue = entry.second;
                        }
                    }
                    else { ImGui::Text("%zu", entry.first); }
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", entry.second);
                }
            }
            if (entries.GetLength() == 0)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled("Нет ненулевых записей");
            }
            ImGui::EndTable();
        }
    }
}

GuiApp::~GuiApp()
{
    this->cancelRequested = true;
    if (this->worker.joinable()) { this->worker.join(); }
}

bool GuiApp::IsBusy() const { return this->worker.joinable(); }

void GuiApp::RequestClose()
{
    this->closeRequested = true;
    this->cancelRequested = true;
    this->status = "Завершение текущей операции перед выходом...";
}

bool GuiApp::ShouldClose() const { return this->closeRequested && !this->running.load(); }

void GuiApp::CheckCancellation() const
{
    if (this->cancelRequested.load()) { throw Cancelled{}; }
}

void GuiApp::ShowError(const std::exception& exception)
{
    this->error = exception.what();
    this->status.clear();
}

void GuiApp::StartTask(const char* name, std::function<void()> action, std::function<void(bool)> finish)
{
    if (this->IsBusy()) { return; }
    this->taskName = name;
    this->status = std::string(name) + "...";
    this->error.clear();
    this->taskError.clear();
    this->taskCancelled = false;
    this->cancelRequested = false;
    this->completed = 0;
    this->total = 0;
    this->finishTask = std::move(finish);
    this->running = true;
    try
    {
        this->worker = std::thread([this, action = std::move(action)]()
        {
            try { action(); }
            catch (const Cancelled&) { this->taskCancelled = true; }
            catch (const std::overflow_error&) { this->taskError = "Результат не помещается в int"; }
            catch (const std::exception& exception) { this->taskError = exception.what(); }
            catch (...) { this->taskError = "Неизвестная ошибка операции"; }
            this->running = false;
        });
    }
    catch (...)
    {
        this->running = false;
        this->finishTask = {};
        throw;
    }
}

void GuiApp::PollTask()
{
    if (!this->worker.joinable() || this->running.load()) { return; }
    this->worker.join();
    bool success = this->taskError.empty() && !this->taskCancelled;
    if (this->finishTask)
    {
        try { this->finishTask(success); }
        catch (const std::exception& exception) { this->taskError = exception.what(); success = false; }
        this->finishTask = {};
    }
    if (!this->taskError.empty()) { this->error = this->taskError; this->status.clear(); }
    else if (this->taskCancelled)
    {
        this->status = this->taskName == "Сравнение хранилищ" ?
            "Замеры отменены. Завершённые пары доступны для просмотра и сохранения" :
            "Операция отменена. Исходный вектор сохранён";
    }
    else if (success) { this->status = this->taskName + ": готово"; }
}

template<typename Storage>
void GuiApp::Refresh(GuiVector<Storage>& state)
{
    auto entries = state.vector.GetEntries();
    state.length = state.vector.GetLength();
    state.entries = std::move(entries);
    state.hasResult = false;
    state.result = SparseVector<int, Storage>(0);
    state.resultEntries.Clear();
    this->hasSum = false;
    this->error.clear();
}

void GuiApp::Draw()
{
    this->PollTask();
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("SparseVector", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextUnformatted("SparseVector  /  Лабораторная работа 2");
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 110);
    if (ImGui::Button("Выход", ImVec2(85, 0))) { this->RequestClose(); }
    ImGui::BeginDisabled(this->IsBusy() || this->closeRequested);
    ImGui::SetNextItemWidth(260);
    int previousStorage = this->storage;
    ImGui::Combo("Хранилище", &this->storage, "AVL-дерево\0Отсортированный массив\0");
    if (previousStorage != this->storage) { this->hasSum = false; this->error.clear(); }
    ImGui::SameLine();
    std::size_t length = this->storage == 0 ? this->tree.length : this->array.length;
    std::size_t count = this->storage == 0 ? this->tree.entries.GetLength() : this->array.entries.GetLength();
    ImGui::Text("N = %zu    k = %zu", length, count);
    ImGui::EndDisabled();
    ImGui::Separator();

    float footer = this->IsBusy() ? 120.0f : 72.0f;
    ImGui::BeginChild("Содержимое", ImVec2(0, -footer));
    if (ImGui::BeginTabBar("Вкладки"))
    {
        if (ImGui::BeginTabItem("Вектор"))
        {
            if (this->storage == 0) { this->DrawVector(this->tree); }
            else { this->DrawVector(this->array); }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Map / Where / Reduce"))
        {
            if (this->storage == 0) { this->DrawOperations(this->tree); }
            else { this->DrawOperations(this->array); }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Сравнение"))
        {
            this->DrawBenchmarks();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    ImGui::Separator();
    if (this->IsBusy())
    {
        std::size_t done = this->completed.load();
        std::size_t all = this->total.load();
        std::string progress = all == 0 ? "Выполнение..." : std::to_string(done) + " / " + std::to_string(all);
        ImGui::ProgressBar(all == 0 ? 0.0f : static_cast<float>(done) / static_cast<float>(all),
                           ImVec2(ImGui::GetContentRegionAvail().x - 220, 0), progress.c_str());
        ImGui::SameLine();
        ImGui::BeginDisabled(this->cancelRequested.load());
        if (ImGui::Button("Отменить выполнение")) { this->cancelRequested = true; }
        ImGui::EndDisabled();
        if (this->cancelRequested.load()) { ImGui::TextDisabled("Ожидаем завершения текущего этапа..."); }
    }
    if (!this->error.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.52f, 0.48f, 1));
        ImGui::TextWrapped("Ошибка: %s", this->error.c_str());
        ImGui::PopStyleColor();
    }
    else { ImGui::TextWrapped("%s", this->status.c_str()); }
    this->DrawCreateDialog();
    this->DrawGenerateDialog();
    ImGui::End();
}

template<typename Storage>
void GuiApp::DrawVector(GuiVector<Storage>& state)
{
    ImGui::BeginDisabled(this->IsBusy() || this->closeRequested);
    if (ImGui::Button("Создать вектор")) { this->openCreate = true; this->dialogError.clear(); }
    ImGui::SameLine();
    if (ImGui::Button("Генерация")) { this->openGenerate = true; this->dialogError.clear(); }
    ImGui::SameLine();
    if (ImGui::Button("Очистить"))
    {
        this->StartTask("Очистка вектора", [&state]() { state.vector.Clear(); },
                        [this, &state](bool success) { if (success) { this->Refresh(state); } });
    }
    ImGui::EndDisabled();
    ImGui::Spacing();
    float leftWidth = ImGui::GetContentRegionAvail().x - 315;
    ImGui::BeginChild("Таблица вектора", ImVec2(leftWidth, 0));
    ImGui::TextUnformatted("Ненулевые элементы по возрастанию индекса");
    float tableHeight = std::max(160.0f, ImGui::GetContentRegionAvail().y - (state.length <= 20 ? 76 : 12));
    EntryTable("Элементы", state.entries, tableHeight,
               this->IsBusy() ? nullptr : &this->index, this->IsBusy() ? nullptr : &this->value);
    if (state.length <= 20) { ImGui::TextDisabled("Полный вид:"); DenseView(state.length, state.entries); }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Работа с элементом", ImVec2(0, 0));
    ImGui::TextUnformatted("Работа с элементом");
    ImGui::BeginDisabled(this->IsBusy() || this->closeRequested);
    ImGui::PushItemWidth(-1);
    NumberInput("Индекс", this->index);
    NumberInput("Значение", this->value);
    ImGui::PopItemWidth();
    ImGui::TextDisabled("Индексы начинаются с 0");
    if (ImGui::Button("Прочитать", ImVec2(-1, 0)))
    {
        try
        {
            if (state.length == 0) { throw std::out_of_range("Сначала создайте вектор ненулевой длины"); }
            std::size_t position = static_cast<std::size_t>(CheckRange(this->index, 0, state.length - 1, "Индекс"));
            this->value = state.vector.Get(position);
            this->status = "Значение [" + std::to_string(position) + "] = " + std::to_string(this->value);
            this->error.clear();
        }
        catch (const std::exception& exception) { this->ShowError(exception); }
    }
    bool write = ImGui::Button("Записать", ImVec2(-1, 0));
    bool remove = ImGui::Button("Удалить запись", ImVec2(-1, 0));
    if (write || remove)
    {
        try
        {
            if (state.length == 0) { throw std::out_of_range("Сначала создайте вектор ненулевой длины"); }
            std::size_t position = static_cast<std::size_t>(CheckRange(this->index, 0, state.length - 1, "Индекс"));
            int number = remove ? 0 : IntValue(this->value);
            this->StartTask("Запись элемента", [&state, position, number]() { state.vector.Set(position, number); },
                            [this, &state](bool success) { if (success) { this->Refresh(state); } });
        }
        catch (const std::exception& exception) { this->ShowError(exception); }
    }
    ImGui::EndDisabled();
    ImGui::Spacing();
    ImGui::TextWrapped("Запись нуля удаляет сохранённую пару. При чтении такой позиции возвращается 0.");
    ImGui::EndChild();
}

template<typename Storage>
void GuiApp::CreateVector(GuiVector<Storage>& state, std::size_t length, ArraySequence<VectorEntry> entries)
{
    this->StartTask("Создание вектора", [this, &state, length, entries = std::move(entries)]()
    {
        this->CheckCancellation();
        auto replacement = SparseVector<int, Storage>::FromEntries(length, entries);
        this->CheckCancellation();
        state.vector = std::move(replacement);
    }, [this, &state](bool success) { if (success) { this->Refresh(state); } });
}

void GuiApp::DrawCreateDialog()
{
    if (this->openCreate) { ImGui::OpenPopup("Создать вектор"); this->openCreate = false; }
    ImGui::SetNextWindowSize(ImVec2(640, 0), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::BeginPopupModal("Создать вектор", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) { return; }
    ImGui::SetNextItemWidth(220);
    NumberInput("Длина N", this->createLength);
    ImGui::SetNextItemWidth(330);
    ImGui::Combo("Способ ввода", &this->inputMode, "Пустой вектор\0Все значения\0Пары индекс / значение\0");
    if (this->inputMode != 0)
    {
        ImGui::TextWrapped("%s", this->inputMode == 1 ? "Числа через пробел или перенос строки. Их количество должно совпадать с N." :
                           "По два числа: индекс и значение. Например: 1 5. Повторный индекс заменяется последней парой.");
        ImGui::InputTextMultiline("##Исходные данные", &this->inputText, ImVec2(600, 215));
    }
    else { ImGui::TextWrapped("Будет создан вектор заданной длины со значением 0 на каждой позиции."); }
    if (!this->dialogError.empty()) { ImGui::TextWrapped("Ошибка: %s", this->dialogError.c_str()); }
    if (ImGui::Button("Создать", ImVec2(150, 0)))
    {
        try
        {
            auto length = static_cast<std::size_t>(CheckRange(this->createLength, 0, 1000000000, "Длина"));
            if (this->inputMode != 0 && this->inputText.size() > 32 * 1024 * 1024)
            {
                throw std::length_error("Слишком большой текст ввода (максимум 32 МБ)");
            }
            auto entries = ParseVectorInput(length, static_cast<VectorInputMode>(this->inputMode), this->inputText);
            if (this->storage == 0) { this->CreateVector(this->tree, length, std::move(entries)); }
            else { this->CreateVector(this->array, length, std::move(entries)); }
            ImGui::CloseCurrentPopup();
        }
        catch (const std::exception& exception) { this->dialogError = exception.what(); }
    }
    ImGui::SameLine();
    if (ImGui::Button("Отмена", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

template<typename Storage>
void GuiApp::GenerateVector(GuiVector<Storage>& state)
{
    auto length = static_cast<std::size_t>(CheckRange(this->generateLength, 0, 1000000000, "Длина"));
    auto count = static_cast<std::size_t>(CheckRange(this->generateCount, 0, std::min<long long>(length, 1000000), "Количество ненулевых"));
    int minimum = IntValue(this->generateMinimum);
    int maximum = IntValue(this->generateMaximum);
    if (minimum > maximum || (count > 0 && minimum == 0 && maximum == 0))
    {
        throw std::invalid_argument("Укажите корректный диапазон с хотя бы одним ненулевым значением");
    }
    auto seed = static_cast<unsigned int>(CheckRange(this->generateSeed, 0, std::numeric_limits<unsigned int>::max(), "Seed"));
    auto order = static_cast<DataOrder>(this->generateOrder);
    this->StartTask("Генерация вектора", [this, &state, length, count, minimum, maximum, seed, order]()
    {
        this->total = count;
        this->CheckCancellation();
        auto entries = GenerateEntries(length, count, minimum, maximum, order, seed);
        SparseVector<int, Storage> replacement(length);
        for (std::size_t i = 0; i < entries.GetLength(); ++i)
        {
            this->CheckCancellation();
            const auto& entry = entries.Get(i);
            replacement.Set(entry.first, entry.second);
            if (i % 256 == 0 || i + 1 == count) { this->completed = i + 1; }
        }
        this->CheckCancellation();
        state.vector = std::move(replacement);
    }, [this, &state](bool success) { if (success) { this->Refresh(state); } });
}

void GuiApp::DrawGenerateDialog()
{
    if (this->openGenerate) { ImGui::OpenPopup("Генерация вектора"); this->openGenerate = false; }
    ImGui::SetNextWindowSize(ImVec2(600, 0), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::BeginPopupModal("Генерация вектора", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) { return; }
    ImGui::PushItemWidth(240);
    NumberInput("Длина N", this->generateLength);
    NumberInput("Ненулевых k", this->generateCount);
    NumberInput("Минимум", this->generateMinimum);
    NumberInput("Максимум", this->generateMaximum);
    NumberInput("Seed", this->generateSeed);
    ImGui::Combo("Порядок вставки", &this->generateOrder, orderNames, 3);
    ImGui::PopItemWidth();
    ImGui::TextWrapped("Одинаковые параметры и seed воспроизводят одинаковые данные. Диапазон задаёт значения, индексы выбираются в пределах N.");
    if (!this->dialogError.empty()) { ImGui::TextWrapped("Ошибка: %s", this->dialogError.c_str()); }
    if (ImGui::Button("Сгенерировать", ImVec2(180, 0)))
    {
        try
        {
            if (this->storage == 0) { this->GenerateVector(this->tree); }
            else { this->GenerateVector(this->array); }
            ImGui::CloseCurrentPopup();
        }
        catch (const std::exception& exception) { this->dialogError = exception.what(); }
    }
    ImGui::SameLine();
    if (ImGui::Button("Отмена", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

template<typename Storage>
void GuiApp::DrawOperations(GuiVector<Storage>& state)
{
    ImGui::TextWrapped("Map и Where показывают отдельный результат. Кнопка применения заменяет текущий вектор этим результатом.");
    ImGui::BeginDisabled(this->IsBusy() || this->closeRequested);
    if (ImGui::BeginTable("Параметры операций", 3, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Map");
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##Map", &this->mapChoice, "Сменить знак\0Умножить на 2\0Возвести в квадрат\0");
        if (ImGui::Button("Выполнить Map", ImVec2(-1, 0)))
        {
            int (*functions[])(const int&) = {NegateValue, DoubleValue, SquareValue};
            auto function = functions[this->mapChoice];
            this->StartTask("Map", [this, &state, function]()
            {
                auto result = state.vector.Map(function);
                this->CheckCancellation();
                state.result = std::move(result);
            }, [&state](bool success)
            {
                if (success) { state.resultEntries = state.result.GetEntries(); state.hasResult = true; state.resultTitle = "Результат Map"; }
            });
        }
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted("Where");
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##Where", &this->whereChoice, "Положительные\0Чётные\0");
        if (ImGui::Button("Выполнить Where", ImVec2(-1, 0)))
        {
            bool (*function)(const int&) = this->whereChoice == 0 ? IsPositive : IsEven;
            this->StartTask("Where", [this, &state, function]()
            {
                auto result = state.vector.Where(function);
                this->CheckCancellation();
                state.result = std::move(result);
            }, [&state](bool success)
            {
                if (success) { state.resultEntries = state.result.GetEntries(); state.hasResult = true; state.resultTitle = "Результат Where"; }
            });
        }
        ImGui::TableSetColumnIndex(2);
        ImGui::TextUnformatted("Reduce: начальная сумма");
        ImGui::SetNextItemWidth(-1);
        NumberInput("##Начальная сумма", this->sumStart);
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Начальное значение суммы"); }
        if (ImGui::Button("Выполнить Reduce", ImVec2(-1, 0)))
        {
            try
            {
                int start = IntValue(this->sumStart);
                this->hasSum = false;
                this->StartTask("Reduce", [this, &state, start]() { this->sum = state.vector.Reduce(SumValues, start); },
                                [this](bool success) { this->hasSum = success; });
            }
            catch (const std::exception& exception) { this->ShowError(exception); }
        }
        ImGui::EndTable();
    }
    ImGui::EndDisabled();
    if (this->hasSum) { ImGui::Text("Результат Reduce: %d", this->sum); }
    ImGui::Separator();
    if (state.hasResult)
    {
        ImGui::BeginDisabled(this->IsBusy() || this->closeRequested);
        if (ImGui::Button("Сделать результат текущим"))
        {
            this->StartTask("Применение результата", [this, &state]()
            {
                auto replacement = state.result;
                this->CheckCancellation();
                state.vector = std::move(replacement);
            },
                            [this, &state](bool success) { if (success) { this->Refresh(state); } });
        }
        ImGui::EndDisabled();
    }
    float height = std::max(160.0f, ImGui::GetContentRegionAvail().y - (state.length <= 20 ? 90 : 38));
    if (ImGui::BeginTable("Исходный и результат", 2, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Исходный: N = %zu, k = %zu", state.length, state.entries.GetLength());
        EntryTable("Исходный вектор", state.entries, height);
        DenseView(state.length, state.entries);
        ImGui::TableSetColumnIndex(1);
        if (state.hasResult)
        {
            ImGui::Text("%s: k = %zu", state.resultTitle.c_str(), state.resultEntries.GetLength());
            EntryTable("Результат вектора", state.resultEntries, height);
            DenseView(state.length, state.resultEntries);
        }
        else { ImGui::TextDisabled("Результат Map / Where появится здесь"); }
        ImGui::EndTable();
    }
}

void GuiApp::StartBenchmarks()
{
    int repeats = static_cast<int>(CheckRange(this->benchmarkRepeats, 1, 15, "Количество повторов"));
    auto seed = static_cast<unsigned int>(CheckRange(this->benchmarkSeed, 0, std::numeric_limits<unsigned int>::max(), "Seed"));
    bool series = this->benchmarkMode == 1;
    std::size_t length = this->storage == 0 ? this->tree.length : this->array.length;
    ArraySequence<VectorEntry> original = this->storage == 0 ? this->tree.entries : this->array.entries;
    std::vector<std::size_t> sizes{original.GetLength()};
    bool autoLength = this->automaticLength;
    if (series)
    {
        auto maximum = static_cast<std::size_t>(CheckRange(this->benchmarkMaximum, 1, 1000000, "Максимальное k"));
        length = autoLength ? 0 : static_cast<std::size_t>(CheckRange(this->benchmarkLength, maximum, 1000000000, "Длина N"));
        sizes = GetExperimentSizes(maximum);
        original.Clear();
    }
    std::string description = series ? (autoLength ? "Серия: N = 10 × k" : "Серия: фиксированное N = " + std::to_string(length)) :
        "Данные текущего вектора: N=" + std::to_string(length) + ", k=" + std::to_string(original.GetLength());
    this->pendingRecords.clear();
    this->StartTask("Сравнение хранилищ", [this, original = std::move(original), sizes, length, repeats, seed, series, autoLength]()
    {
        this->total = sizes.size() * 3 * static_cast<std::size_t>(repeats);
        this->pendingRecords.reserve(this->total.load() * 2);
        for (std::size_t count : sizes)
        {
            std::size_t trialLength = series && autoLength ? count * 10 : length;
            for (int orderIndex = 0; orderIndex < 3; ++orderIndex)
            {
                auto order = static_cast<DataOrder>(orderIndex);
                for (int repeat = 1; repeat <= repeats; ++repeat)
                {
                    this->CheckCancellation();
                    unsigned int trialSeed = seed + static_cast<unsigned int>(repeat - 1);
                    auto entries = series ? GenerateEntries(trialLength, count, -9, 9, order, trialSeed) : original;
                    if (!series) { ArrangeEntries(entries, order, trialSeed); }
                    auto data = PrepareBenchmark(trialLength, entries);
                    // Завершаем пару целиком: сравнение всегда содержит оба хранилища.
                    BenchmarkRecord first, second;
                    if (repeat % 2 == 1)
                    {
                        first = MeasureStorage<TreeStorage<int>>(data, "tree", order, repeat, trialSeed);
                        second = MeasureStorage<ArrayStorage<int>>(data, "array", order, repeat, trialSeed);
                    }
                    else
                    {
                        first = MeasureStorage<ArrayStorage<int>>(data, "array", order, repeat, trialSeed);
                        second = MeasureStorage<TreeStorage<int>>(data, "tree", order, repeat, trialSeed);
                    }
                    this->pendingRecords.push_back(std::move(first));
                    this->pendingRecords.push_back(std::move(second));
                    ++this->completed;
                }
            }
        }
    }, [this, description](bool)
    {
        if (!this->pendingRecords.empty())
        {
            this->records.swap(this->pendingRecords);
            this->pendingRecords.clear();
            this->benchmarkDescription = description;
            this->resultSize = 0;
            this->benchmarkSettingsOpen = false;
        }
    });
}

void GuiApp::SaveCsv()
{
    if (this->records.empty()) { throw std::invalid_argument("Сначала выполните замеры"); }
    if (this->csvPath.empty()) { throw std::invalid_argument("Укажите путь CSV"); }
    std::filesystem::path path(this->csvPath);
    if (path.has_parent_path()) { std::filesystem::create_directories(path.parent_path()); }
    std::ofstream output(path);
    if (!output) { throw std::runtime_error("Не удалось открыть файл: " + path.string()); }
    output.exceptions(std::ios::failbit | std::ios::badbit);
    WriteBenchmarkHeader(output);
    for (const auto& record : this->records) { WriteBenchmarkRecord(output, record); }
    this->status = "Сохранено " + std::to_string(this->records.size()) + " строк: " + std::filesystem::absolute(path).string();
    this->error.clear();
}

void GuiApp::DrawBenchmarks()
{
    ImGui::SetNextItemOpen(this->benchmarkSettingsOpen, ImGuiCond_Always);
    this->benchmarkSettingsOpen = ImGui::CollapsingHeader("Параметры эксперимента");
    if (this->benchmarkSettingsOpen)
    {
        ImGui::BeginDisabled(this->IsBusy() || this->closeRequested);
        ImGui::SetNextItemWidth(270);
        ImGui::Combo("Режим", &this->benchmarkMode, "На текущем векторе\0Серия измерений\0");
        if (ImGui::BeginTable("Настройки замеров", 3, ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::SetNextItemWidth(-1);
            NumberInput("Повторов (1–15)", this->benchmarkRepeats);
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1);
            NumberInput("Seed замеров", this->benchmarkSeed);
            ImGui::TableSetColumnIndex(2);
            if (this->benchmarkMode == 1)
            {
                ImGui::SetNextItemWidth(-1);
                NumberInput("Максимальное k", this->benchmarkMaximum);
            }
            else { ImGui::TextWrapped("Оба хранилища получают одни и те же пары текущего вектора."); }
            if (this->benchmarkMode == 1)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::BeginDisabled(this->automaticLength);
                ImGui::SetNextItemWidth(-1);
                NumberInput("Фиксированная длина N", this->benchmarkLength);
                ImGui::EndDisabled();
                ImGui::TableSetColumnIndex(1);
                ImGui::Checkbox("N = 10 × k", &this->automaticLength);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextWrapped("k: 100, 300, 1000, 3000… и заданный максимум.");
            }
            ImGui::EndTable();
        }
        if (ImGui::Button("Запустить сравнение", ImVec2(240, 0)))
        {
            try { this->StartBenchmarks(); }
            catch (const std::exception& exception) { this->ShowError(exception); }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextDisabled("Все 3 порядка вставки; время всего пакета в мс");
    }
    ImGui::Separator();
    if (this->records.empty())
    {
        ImGui::TextWrapped("После запуска здесь появятся таблица девяти операций и график. Отмена прекращает дальнейшие эксперименты после завершения текущей пары хранилищ.");
        return;
    }
    this->DrawBenchmarkResults();
    ImGui::Spacing();
    ImGui::BeginDisabled(this->IsBusy());
    ImGui::SetNextItemWidth(std::max(200.0f, ImGui::GetContentRegionAvail().x - 185));
    ImGui::InputText("##Путь CSV", &this->csvPath);
    ImGui::SameLine();
    if (ImGui::Button("Сохранить CSV", ImVec2(165, 0)))
    {
        try { this->SaveCsv(); }
        catch (const std::exception& exception) { this->ShowError(exception); }
    }
    ImGui::EndDisabled();
    ImGui::TextDisabled("CSV содержит каждый повтор; таблица и график показывают медианы.");
}

double GuiApp::GetMedianTime(std::size_t count, const char* storageName, int operation) const
{
    std::vector<double> times;
    for (const auto& record : this->records)
    {
        if (record.nonzero == count && record.storage == storageName && static_cast<int>(record.order) == this->resultOrder)
        {
            times.push_back(GetBenchmarkTime(record.result, operation));
        }
    }
    return times.empty() ? -1 : Median(std::move(times));
}

void GuiApp::DrawBenchmarkResults()
{
    std::vector<std::size_t> sizes;
    for (const auto& record : this->records) { sizes.push_back(record.nonzero); }
    std::sort(sizes.begin(), sizes.end());
    sizes.erase(std::unique(sizes.begin(), sizes.end()), sizes.end());
    if (this->resultSize >= static_cast<int>(sizes.size())) { this->resultSize = 0; }
    ImGui::TextWrapped("%s", this->benchmarkDescription.c_str());
    ImGui::SetNextItemWidth(190);
    ImGui::Combo("Порядок", &this->resultOrder, orderNames, 3);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    std::string selected = std::to_string(sizes[this->resultSize]);
    if (ImGui::BeginCombo("k таблицы", selected.c_str()))
    {
        for (std::size_t i = 0; i < sizes.size(); ++i)
        {
            std::string label = std::to_string(sizes[i]);
            if (ImGui::Selectable(label.c_str(), this->resultSize == static_cast<int>(i))) { this->resultSize = static_cast<int>(i); }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    std::size_t chosen = sizes[this->resultSize];
    for (const auto& record : this->records)
    {
        if (record.nonzero == chosen)
        {
            ImGui::Text("N = %zu; нулевых запросов: %zu", record.length, record.missingQueries);
            break;
        }
    }
    if (ImGui::BeginTable("Таблица и график", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Результаты", ImGuiTableColumnFlags_WidthStretch, 0.48f);
        ImGui::TableSetupColumn("График", ImGuiTableColumnFlags_WidthStretch, 0.52f);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Медианное время, мс");
        if (ImGui::BeginTable("Время операций", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH))
        {
            ImGui::TableSetupColumn("Операция", ImGuiTableColumnFlags_WidthStretch, 2.5f);
            ImGui::TableSetupColumn("AVL", ImGuiTableColumnFlags_WidthStretch, 1);
            ImGui::TableSetupColumn("Массив", ImGuiTableColumnFlags_WidthStretch, 1);
            ImGui::TableHeadersRow();
            for (int operation = 0; operation < 9; ++operation)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextWrapped("%s", operationNames[operation]);
                for (int which = 0; which < 2; ++which)
                {
                    ImGui::TableSetColumnIndex(which + 1);
                    double time = this->GetMedianTime(chosen, which == 0 ? "tree" : "array", operation);
                    if (time < 0) { ImGui::TextDisabled("—"); }
                    else { ImGui::Text("%.6f", time); }
                }
            }
            ImGui::EndTable();
        }
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##Операция графика", &this->resultOperation, operationNames, 9);
        if (sizes.size() > 1) { ImGui::Checkbox("Логарифмическая шкала k", &this->logarithmicX); }
        this->DrawBenchmarkGraph(sizes);
        ImGui::EndTable();
    }
}

void GuiApp::DrawBenchmarkGraph(const std::vector<std::size_t>& sizes)
{
    ImVec2 origin = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    float height = 250;
    if (width < 280) { ImGui::TextWrapped("Увеличьте ширину окна для отображения графика"); return; }
    ImGui::InvisibleButton("График времени", ImVec2(width, height));
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 top(origin.x + 88, origin.y + 24);
    ImVec2 bottom(origin.x + width - 44, origin.y + height - 45);
    ImU32 text = ImGui::GetColorU32(ImGuiCol_Text);
    ImU32 grid = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 colors[] = {IM_COL32(100, 175, 245, 255), IM_COL32(245, 173, 85, 255)};
    double maximum = 0;
    bool hasData = false;
    for (std::size_t count : sizes)
    {
        double treeTime = this->GetMedianTime(count, "tree", this->resultOperation);
        double arrayTime = this->GetMedianTime(count, "array", this->resultOperation);
        hasData = hasData || treeTime >= 0 || arrayTime >= 0;
        maximum = std::max({maximum, treeTime, arrayTime});
    }
    if (!hasData)
    {
        draw->AddText(origin, text, "Для этого порядка пока нет замеров");
        return;
    }
    if (maximum <= 0) { maximum = 0.001; }
    maximum *= 1.15;
    for (int tick = 0; tick <= 4; ++tick)
    {
        float y = bottom.y - (bottom.y - top.y) * tick / 4;
        draw->AddLine(ImVec2(top.x, y), ImVec2(bottom.x, y), grid);
        char label[40];
        std::snprintf(label, sizeof(label), "%.3g", maximum * tick / 4);
        draw->AddText(ImVec2(top.x - ImGui::CalcTextSize(label).x - 8, y - ImGui::GetFontSize() / 2), text, label);
    }
    draw->AddText(ImVec2(origin.x, origin.y), text, "мс");
    draw->AddLine(top, ImVec2(top.x, bottom.y), text);
    draw->AddLine(ImVec2(top.x, bottom.y), bottom, text);
    const char* names[] = {"AVL", "Массив"};
    bool hovered = ImGui::IsItemHovered();
    ImVec2 mouse = ImGui::GetIO().MousePos;
    if (sizes.size() == 1)
    {
        for (int which = 0; which < 2; ++which)
        {
            double time = this->GetMedianTime(sizes[0], which == 0 ? "tree" : "array", this->resultOperation);
            if (time < 0) { continue; }
            float x = top.x + (bottom.x - top.x) * (which == 0 ? 0.3f : 0.75f);
            float barWidth = std::min(65.0f, (bottom.x - top.x) / 4);
            float y = bottom.y - static_cast<float>(time / maximum) * (bottom.y - top.y);
            draw->AddRectFilled(ImVec2(x - barWidth / 2, y), ImVec2(x + barWidth / 2, bottom.y), colors[which]);
            draw->AddText(ImVec2(x - ImGui::CalcTextSize(names[which]).x / 2, bottom.y + 8), text, names[which]);
            if (hovered && std::abs(mouse.x - x) <= barWidth / 2)
            {
                ImGui::SetTooltip("%s: %.6f мс\nk = %zu", names[which], time, sizes[0]);
            }
        }
        return;
    }
    bool logX = this->logarithmicX && sizes.front() > 0;
    auto transform = [logX](double x) { return logX ? std::log10(x) : x; };
    double left = transform(static_cast<double>(sizes.front()));
    double right = transform(static_cast<double>(sizes.back()));
    for (int tick = 0; tick <= 3; ++tick)
    {
        double position = left + (right - left) * tick / 3;
        double value = logX ? std::pow(10.0, position) : position;
        float x = top.x + (bottom.x - top.x) * tick / 3;
        draw->AddLine(ImVec2(x, top.y), ImVec2(x, bottom.y), grid);
        char label[40];
        std::snprintf(label, sizeof(label), "%.0f", value);
        ImVec2 size = ImGui::CalcTextSize(label);
        draw->AddText(ImVec2(x - size.x / 2, bottom.y + 8), text, label);
    }
    draw->AddText(ImVec2(bottom.x - 10, bottom.y + 26), text, "k");
    for (int which = 0; which < 2; ++which)
    {
        bool hasPrevious = false;
        ImVec2 previous;
        for (std::size_t count : sizes)
        {
            double time = this->GetMedianTime(count, which == 0 ? "tree" : "array", this->resultOperation);
            if (time < 0) { hasPrevious = false; continue; }
            float fraction = static_cast<float>((transform(static_cast<double>(count)) - left) / (right - left));
            ImVec2 point(top.x + fraction * (bottom.x - top.x), bottom.y - static_cast<float>(time / maximum) * (bottom.y - top.y));
            if (hasPrevious) { draw->AddLine(previous, point, colors[which], 2); }
            draw->AddCircleFilled(point, 4, colors[which]);
            if (hovered && std::abs(mouse.x - point.x) < 9 && std::abs(mouse.y - point.y) < 12)
            {
                ImGui::SetTooltip("%s\nk = %zu\n%.6f мс", names[which], count, time);
            }
            previous = point;
            hasPrevious = true;
        }
        float legendX = top.x + which * 100;
        draw->AddLine(ImVec2(legendX, origin.y + 6), ImVec2(legendX + 15, origin.y + 6), colors[which], 3);
        draw->AddText(ImVec2(legendX + 22, origin.y - 2), text, names[which]);
    }
}
