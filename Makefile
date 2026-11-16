.PHONY: all run run_tests run_benchmark plots check clean

all: sparsevector_app

sparsevector_app: main.o
	g++ main.o -o sparsevector_app

main.o: main.cpp SparseVector.hpp TreeStorage.hpp ArrayStorage.hpp TreeDictionary.hpp IDictionary.hpp \
        SortedSequence.hpp ISortedSequence.hpp Benchmark.hpp DataGeneration.hpp VectorOperations.hpp \
        lab1/BinarySearchTree.hpp lab1/Tree.hpp lab1/IEnumerable.hpp lab1/IEnumerator.hpp \
        lab1/TraversalOrder.hpp lab1/Stack.hpp lab1/Comparison.hpp lab1/ArraySequence.hpp \
        lab1/Sequence.hpp lab1/DynamicArray.hpp
	g++ -std=c++17 -Wall -Wextra -Wpedantic -c main.cpp

tests: tests.o
	g++ tests.o -o tests

tests.o: tests.cpp SparseVector.hpp TreeStorage.hpp ArrayStorage.hpp TreeDictionary.hpp IDictionary.hpp \
         SortedSequence.hpp ISortedSequence.hpp Benchmark.hpp DataGeneration.hpp VectorOperations.hpp \
         lab1/BinarySearchTree.hpp lab1/Tree.hpp lab1/IEnumerable.hpp lab1/IEnumerator.hpp \
         lab1/TraversalOrder.hpp lab1/Stack.hpp lab1/Comparison.hpp lab1/ArraySequence.hpp \
         lab1/Sequence.hpp lab1/DynamicArray.hpp
	g++ -std=c++17 -Wall -Wextra -Wpedantic -c tests.cpp

benchmark: benchmark.cpp SparseVector.hpp TreeStorage.hpp ArrayStorage.hpp TreeDictionary.hpp IDictionary.hpp \
           SortedSequence.hpp ISortedSequence.hpp Benchmark.hpp DataGeneration.hpp \
           lab1/BinarySearchTree.hpp lab1/Tree.hpp lab1/IEnumerable.hpp lab1/IEnumerator.hpp \
           lab1/TraversalOrder.hpp lab1/Stack.hpp lab1/Comparison.hpp lab1/ArraySequence.hpp \
           lab1/Sequence.hpp lab1/DynamicArray.hpp
	g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 benchmark.cpp -o benchmark

run: sparsevector_app
	./sparsevector_app

run_tests: tests
	./tests

run_benchmark: benchmark
	./benchmark 10000 3 17 results/benchmark.csv

plots:
	python3 plot_benchmarks.py results/benchmark.csv

check: tests.cpp SparseVector.hpp TreeStorage.hpp ArrayStorage.hpp TreeDictionary.hpp IDictionary.hpp \
       SortedSequence.hpp ISortedSequence.hpp Benchmark.hpp DataGeneration.hpp VectorOperations.hpp \
       lab1/BinarySearchTree.hpp lab1/Tree.hpp lab1/IEnumerable.hpp lab1/IEnumerator.hpp \
       lab1/TraversalOrder.hpp lab1/Stack.hpp lab1/Comparison.hpp lab1/ArraySequence.hpp \
       lab1/Sequence.hpp lab1/DynamicArray.hpp
	g++ -std=c++17 -Wall -Wextra -Wpedantic -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer tests.cpp -o tests_sanitized
	./tests_sanitized

clean:
	rm -f *.o sparsevector_app tests benchmark tests_sanitized sparsevector_tests sparsevector_tests_sanitized sparsevector_bench
