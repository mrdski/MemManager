#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <functional>
#include <cmath>
#include <fcntl.h>
#include <climits>

using namespace std;

int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);

class Node {
    public:
        Node* prev;
        Node* next;
        int head;
        bool hole;
        int size;

        Node(int _head, int _size, bool _hole, Node* _next, Node* _prev)
            : prev(_prev), next(_next), head(_head), hole(_hole), size(_size) {}
};

class MemoryManager{
    private:

    function<int(int, void *)> Allocator;
    Node* node = nullptr;
    uint8_t* start = nullptr;
    uint8_t* bit_map = nullptr;
    uint16_t* list = nullptr;
    unsigned word_size;
    size_t size_in_words;

    public:
    MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator);
	~MemoryManager();
	void initialize(size_t sizeInWords);
	void shutdown();
	void *allocate(size_t sizeInBytes);
	void free(void *address);
	void setAllocator(std::function<int(int, void *)> allocator);
	int dumpMemoryMap(char *filename);
	void *getList();
	void *getBitmap();
	unsigned getWordSize();
	void *getMemoryStart();
	unsigned getMemoryLimit();
};


