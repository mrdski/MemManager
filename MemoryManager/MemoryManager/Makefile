MemoryManager: MemoryManager.cpp
	g++ -c -o MemoryManager.o MemoryManager.cpp
	ar cr libMemoryManager.a MemoryManager.o


#g++ -std=c++17 -o CommandLineTest CommandLineTest.cpp -L./MemoryManager -lMemoryManager
#valgrind --leak-check=full -s ./CommandLineTest