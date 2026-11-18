# Зависимости графического интерфейса

- Dear ImGui **v1.93.0**, MIT: https://github.com/ocornut/imgui/releases/tag/v1.93.0
  В `imgui/` лежат неизменённые файлы ядра, backends GLFW/Metal и официальный адаптер `std::string`.
- GLFW **3.5.1**, zlib/libpng: https://github.com/glfw/glfw/releases/tag/3.5.1
  Заголовки и две официальные статические библиотеки взяты из `glfw-3.5.1.bin.MACOS.zip`.
  `glfw/lib/libglfw3.a` объединяет arm64 и x86_64 командой `lipo -create`.
- `gui/assets/DroidSans.ttf` взят из `misc/fonts/` Dear ImGui v1.93.0.
  Автор шрифта — Google/Ascender, лицензия Apache 2.0: `gui/assets/FONT-LICENSE.txt`.

Metal, Cocoa, IOKit, CoreVideo и QuartzCore входят в macOS SDK.
Для сборки нужны только Command Line Tools (`clang++`, `make`); Homebrew не требуется.
Версии зависимостей закреплены; сборка не скачивает файлы из сети.
