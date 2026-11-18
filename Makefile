.PHONY: all sparsevector_app tests benchmark run run_tests run_benchmark plots check clean

all: build/SparseVector.app

sparsevector_app: build/sparsevector_app
tests: build/tests
benchmark: build/benchmark

build:
	mkdir -p build

build/SparseVector.app: build/sparsevector_app gui/assets/Info.plist gui/assets/DroidSans.ttf gui/assets/FONT-LICENSE.txt
	mkdir -p build/SparseVector.app/Contents/MacOS build/SparseVector.app/Contents/Resources
	cp build/sparsevector_app build/SparseVector.app/Contents/MacOS/SparseVector
	cp gui/assets/Info.plist build/SparseVector.app/Contents/Info.plist
	cp gui/assets/DroidSans.ttf gui/assets/FONT-LICENSE.txt build/SparseVector.app/Contents/Resources/
	touch build/SparseVector.app

build/sparsevector_app: build/main.o build/GuiApp.o build/imgui.o build/imgui_draw.o build/imgui_widgets.o build/imgui_tables.o \
                  build/imgui_stdlib.o build/imgui_impl_glfw.o build/imgui_impl_metal.o gui/third_party/glfw/lib/libglfw3.a | build
	clang++ build/main.o build/GuiApp.o build/imgui.o build/imgui_draw.o build/imgui_widgets.o build/imgui_tables.o build/imgui_stdlib.o \
	        build/imgui_impl_glfw.o build/imgui_impl_metal.o gui/third_party/glfw/lib/libglfw3.a \
	        -framework Metal -framework Cocoa -framework IOKit -framework CoreVideo -framework QuartzCore -o build/sparsevector_app

build/main.o: gui/main.mm gui/GuiApp.hpp gui/GuiData.hpp lab2/SparseVector.hpp lab2/TreeStorage.hpp lab2/ArrayStorage.hpp lab2/TreeDictionary.hpp \
        lab2/IDictionary.hpp lab2/SortedSequence.hpp lab2/ISortedSequence.hpp benchmarks/Benchmark.hpp lab2/DataGeneration.hpp \
        lab1/BinarySearchTree.hpp lab1/Tree.hpp lab1/IEnumerable.hpp lab1/IEnumerator.hpp \
        lab1/TraversalOrder.hpp lab1/Stack.hpp lab1/Comparison.hpp lab1/ArraySequence.hpp \
        lab1/Sequence.hpp lab1/DynamicArray.hpp gui/third_party/imgui/imgui.h gui/third_party/imgui/imconfig.h \
        gui/third_party/imgui/backends/imgui_impl_glfw.h gui/third_party/imgui/backends/imgui_impl_metal.h \
        gui/third_party/glfw/include/GLFW/glfw3.h gui/third_party/glfw/include/GLFW/glfw3native.h | build
	clang++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -fobjc-arc -fobjc-weak \
	        -Igui/third_party/imgui -Igui/third_party/imgui/backends -Igui/third_party/glfw/include -c gui/main.mm -o build/main.o

build/GuiApp.o: gui/GuiApp.cpp gui/GuiApp.hpp gui/GuiData.hpp lab2/SparseVector.hpp lab2/TreeStorage.hpp lab2/ArrayStorage.hpp lab2/TreeDictionary.hpp \
          lab2/IDictionary.hpp lab2/SortedSequence.hpp lab2/ISortedSequence.hpp benchmarks/Benchmark.hpp lab2/DataGeneration.hpp lab2/VectorOperations.hpp \
          lab1/BinarySearchTree.hpp lab1/Tree.hpp lab1/IEnumerable.hpp lab1/IEnumerator.hpp \
          lab1/TraversalOrder.hpp lab1/Stack.hpp lab1/Comparison.hpp lab1/ArraySequence.hpp \
          lab1/Sequence.hpp lab1/DynamicArray.hpp gui/third_party/imgui/imgui.h gui/third_party/imgui/imconfig.h \
          gui/third_party/imgui/misc/cpp/imgui_stdlib.h | build
	clang++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -Igui/third_party/imgui -c gui/GuiApp.cpp -o build/GuiApp.o

build/imgui.o: gui/third_party/imgui/imgui.cpp gui/third_party/imgui/imgui.h gui/third_party/imgui/imgui_internal.h \
         gui/third_party/imgui/imconfig.h gui/third_party/imgui/imstb_rectpack.h gui/third_party/imgui/imstb_textedit.h gui/third_party/imgui/imstb_truetype.h | build
	clang++ -std=c++17 -O2 -c gui/third_party/imgui/imgui.cpp -o build/imgui.o

build/imgui_draw.o: gui/third_party/imgui/imgui_draw.cpp gui/third_party/imgui/imgui.h gui/third_party/imgui/imgui_internal.h \
              gui/third_party/imgui/imconfig.h gui/third_party/imgui/imstb_rectpack.h gui/third_party/imgui/imstb_textedit.h gui/third_party/imgui/imstb_truetype.h | build
	clang++ -std=c++17 -O2 -c gui/third_party/imgui/imgui_draw.cpp -o build/imgui_draw.o

build/imgui_widgets.o: gui/third_party/imgui/imgui_widgets.cpp gui/third_party/imgui/imgui.h gui/third_party/imgui/imgui_internal.h \
                 gui/third_party/imgui/imconfig.h gui/third_party/imgui/imstb_rectpack.h gui/third_party/imgui/imstb_textedit.h gui/third_party/imgui/imstb_truetype.h | build
	clang++ -std=c++17 -O2 -c gui/third_party/imgui/imgui_widgets.cpp -o build/imgui_widgets.o

build/imgui_tables.o: gui/third_party/imgui/imgui_tables.cpp gui/third_party/imgui/imgui.h gui/third_party/imgui/imgui_internal.h \
                gui/third_party/imgui/imconfig.h gui/third_party/imgui/imstb_rectpack.h gui/third_party/imgui/imstb_textedit.h gui/third_party/imgui/imstb_truetype.h | build
	clang++ -std=c++17 -O2 -c gui/third_party/imgui/imgui_tables.cpp -o build/imgui_tables.o

build/imgui_stdlib.o: gui/third_party/imgui/misc/cpp/imgui_stdlib.cpp gui/third_party/imgui/misc/cpp/imgui_stdlib.h \
                gui/third_party/imgui/imgui.h gui/third_party/imgui/imconfig.h | build
	clang++ -std=c++17 -O2 -Igui/third_party/imgui -c gui/third_party/imgui/misc/cpp/imgui_stdlib.cpp -o build/imgui_stdlib.o

build/imgui_impl_glfw.o: gui/third_party/imgui/backends/imgui_impl_glfw.cpp gui/third_party/imgui/backends/imgui_impl_glfw.h \
                   gui/third_party/imgui/imgui.h gui/third_party/imgui/imconfig.h \
                   gui/third_party/glfw/include/GLFW/glfw3.h gui/third_party/glfw/include/GLFW/glfw3native.h | build
	clang++ -std=c++17 -O2 -Igui/third_party/imgui -Igui/third_party/glfw/include \
	        -c gui/third_party/imgui/backends/imgui_impl_glfw.cpp -o build/imgui_impl_glfw.o

build/imgui_impl_metal.o: gui/third_party/imgui/backends/imgui_impl_metal.mm gui/third_party/imgui/backends/imgui_impl_metal.h \
                    gui/third_party/imgui/imgui.h gui/third_party/imgui/imconfig.h | build
	clang++ -std=c++17 -O2 -fobjc-arc -fobjc-weak -Igui/third_party/imgui \
	        -c gui/third_party/imgui/backends/imgui_impl_metal.mm -o build/imgui_impl_metal.o

build/tests: build/tests.o | build
	g++ build/tests.o -o build/tests

build/tests.o: tests/tests.cpp gui/GuiData.hpp lab2/SparseVector.hpp lab2/TreeStorage.hpp lab2/ArrayStorage.hpp lab2/TreeDictionary.hpp lab2/IDictionary.hpp \
         lab2/SortedSequence.hpp lab2/ISortedSequence.hpp benchmarks/Benchmark.hpp lab2/DataGeneration.hpp lab2/VectorOperations.hpp \
         lab1/BinarySearchTree.hpp lab1/Tree.hpp lab1/IEnumerable.hpp lab1/IEnumerator.hpp \
         lab1/TraversalOrder.hpp lab1/Stack.hpp lab1/Comparison.hpp lab1/ArraySequence.hpp \
         lab1/Sequence.hpp lab1/DynamicArray.hpp | build
	g++ -std=c++17 -Wall -Wextra -Wpedantic -c tests/tests.cpp -o build/tests.o

build/benchmark: benchmarks/benchmark.cpp lab2/SparseVector.hpp lab2/TreeStorage.hpp lab2/ArrayStorage.hpp lab2/TreeDictionary.hpp lab2/IDictionary.hpp \
           lab2/SortedSequence.hpp lab2/ISortedSequence.hpp benchmarks/Benchmark.hpp lab2/DataGeneration.hpp \
           lab1/BinarySearchTree.hpp lab1/Tree.hpp lab1/IEnumerable.hpp lab1/IEnumerator.hpp \
           lab1/TraversalOrder.hpp lab1/Stack.hpp lab1/Comparison.hpp lab1/ArraySequence.hpp \
           lab1/Sequence.hpp lab1/DynamicArray.hpp | build
	g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 benchmarks/benchmark.cpp -o build/benchmark

run: build/SparseVector.app
	open -n build/SparseVector.app

run_tests: build/tests
	./build/tests

run_benchmark: build/benchmark
	./build/benchmark 10000 3 17 gui/results/benchmark.csv

plots:
	python3 gui/plot_benchmarks.py gui/results/benchmark.csv

check: tests/tests.cpp gui/GuiData.hpp lab2/SparseVector.hpp lab2/TreeStorage.hpp lab2/ArrayStorage.hpp lab2/TreeDictionary.hpp lab2/IDictionary.hpp \
       lab2/SortedSequence.hpp lab2/ISortedSequence.hpp benchmarks/Benchmark.hpp lab2/DataGeneration.hpp lab2/VectorOperations.hpp \
       lab1/BinarySearchTree.hpp lab1/Tree.hpp lab1/IEnumerable.hpp lab1/IEnumerator.hpp \
       lab1/TraversalOrder.hpp lab1/Stack.hpp lab1/Comparison.hpp lab1/ArraySequence.hpp \
       lab1/Sequence.hpp lab1/DynamicArray.hpp | build
	g++ -std=c++17 -Wall -Wextra -Wpedantic -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer tests/tests.cpp -o build/tests_sanitized
	./build/tests_sanitized

clean:
	rm -rf build
