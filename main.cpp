#include <iostream>

#include "SparseVector.hpp"
#include "ArrayStorage.hpp"

template<typename Storage>
void ShowExample(SparseVector<int, Storage>& vector)
{
    // Пары добавляем в разном порядке, выводим по возрастанию индекса.
    vector.Set(500, -3);
    vector.Set(12, 7);
    vector.Print(std::cout);

    std::cout << "\nЗначение на позиции 12 — " << vector.Get(12) << '\n';
    std::cout << "Значение на позиции 13 — " << vector.Get(13) << '\n';

    // Повторная запись заменяет число по существующему индексу.
    vector.Set(12, 9);
    std::cout << "\nПосле записи 9 на позицию 12\n";
    vector.Print(std::cout);

    // Ноль удаляет пару из хранилища.
    vector.Set(12, 0);
    std::cout << "\nПосле записи нуля на позицию 12\n";
    vector.Print(std::cout);
    std::cout << "Значение на позиции 12 — " << vector.Get(12) << '\n';

    vector.Clear();
    std::cout << "\nПосле очистки\n";
    vector.Print(std::cout);
}

int main()
{
    SparseVector<int> treeVector(1000);
    SparseVector<int, ArrayStorage<int>> arrayVector(1000);

    std::cout << "AVL-дерево\n\n";
    ShowExample(treeVector);

    std::cout << "\nОтсортированный массив\n\n";
    ShowExample(arrayVector);
}
