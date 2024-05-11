#include "MemoryManager.h"

using namespace std;

// Constructor for MemoryManager class.
MemoryManager::MemoryManager(unsigned wordSize, function<int(int,void*)> allocator) {
    this->Allocator = allocator;
    this->word_size = wordSize;
    this->start = nullptr;
    this->node = nullptr;
}

// Destructor for MemoryManager class.
MemoryManager::~MemoryManager() {
    shutdown();
}

// Initializes the memory manager with the specified size in words.
void MemoryManager::initialize(size_t sizeInWords) {
    if (sizeInWords <= 0 || sizeInWords > 65536) {
        return;
    }
    this->size_in_words = sizeInWords;
    if (start != nullptr) {
        shutdown();
    }
    void* temp = sbrk(this->size_in_words * this->word_size); //malloc replacement
    //Used to adjust program break value, value is the amount of memory the program gets
    start = reinterpret_cast<uint8_t*>(temp);
    this->node = new Node(0, size_in_words, true, nullptr, nullptr);
}

// Shuts down the memory manager by deallocating allocated memory.
void MemoryManager::shutdown() {
    if (start != nullptr) {
        intptr_t currentBrk = reinterpret_cast<intptr_t>(sbrk(0));
        intptr_t newBrk = currentBrk - (size_in_words * word_size);
        sbrk(newBrk - currentBrk);
        start = nullptr;
        Node* currentNode = this->node;
        while (currentNode != nullptr) {
            Node* nextNode = currentNode->next;
            delete currentNode;
            currentNode = nextNode;
        }
        this->node = nullptr;
    }
}

// Allocates memory of specified size in bytes.
void *MemoryManager::allocate(size_t sizeInBytes) {
    if (this->node == nullptr) {
        initialize(size_in_words);
    }
    int requiredWords = (sizeInBytes + word_size - 1) / word_size; // Calculate required words
    char* list = static_cast<char*>(getList()); // Get list of holes
    int allocationPosition = Allocator(requiredWords, list); // Find suitable hole
    delete[] list; // Release memory allocated for the list
    if (allocationPosition == -1) { // Check if allocation failed
        return nullptr;
    }
    Node* currentNode = this->node; // Traverse nodes to find allocated position
    while (currentNode != nullptr && currentNode->head != allocationPosition) {
        currentNode = currentNode->next;
    }
    currentNode->hole = false; // Mark the selected hole as allocated
    if (currentNode->size != requiredWords) { // Adjust size if necessary
        Node* nextNode = currentNode->next;
        currentNode->next = new Node(currentNode->head + requiredWords, currentNode->size - requiredWords, true, nextNode, currentNode);
        currentNode->size = requiredWords;
        currentNode->hole = false;
    }
    void* ret = start + (allocationPosition * word_size); // Calculate memory address
    return ret; // Return pointer to allocated memory
}

// Frees the allocated memory at the specified address.
void MemoryManager::free(void* address) {
    uintptr_t startAddress = reinterpret_cast<uintptr_t>(this->start); // Get memory start address
    uintptr_t targetAddress = reinterpret_cast<uintptr_t>(address); // Get target address
    size_t targetPosition = (targetAddress - startAddress) / word_size; // Convert address to position
    for (Node* currentNode = node; currentNode != nullptr; currentNode = currentNode->next) {
        if (currentNode->head == targetPosition && currentNode->hole == false) {
            currentNode->hole = true; // Mark allocated block as hole
            break;
        }
    }
}


// Sets the allocator function to be used for allocation.
void MemoryManager::setAllocator(function<int(int,void*)> allocator) {
    this->Allocator = allocator;
}

// Dumps the memory map to a file.
int MemoryManager::dumpMemoryMap(char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if(fd == -1) {
        return -1;
    }

    uint16_t* holes = static_cast<uint16_t*>(getList());
    if (holes == nullptr) {
        close(fd);
        return -1;
    }

    int numHoles = holes[0];

    dprintf(fd, "[%d, %d]", holes[1], holes[2]);
    for(int i = 1; i < numHoles; i++) {
        dprintf(fd, " - [%d, %d]", holes[i * 2 + 1], holes[i * 2 + 2]);
    }

    delete[] holes;
    close(fd);
    return 0;
}

// Retrieves the list of holes in memory.
void* MemoryManager::getList() {
    vector<uint16_t> headList, sizeList;
    for (Node* currentNode = this->node; currentNode != nullptr; currentNode = currentNode->next) {
        if (currentNode->hole == true) {
            headList.push_back(currentNode->head);
            sizeList.push_back(currentNode->size);
        }
    }
    this->list = new uint16_t[headList.size() * 2 + 1];
    if (this->list == nullptr) {
        return nullptr;
    }
    this->list[0] = static_cast<uint16_t>(headList.size());
    for (size_t i = 0; i < headList.size(); i++) {
        this->list[i * 2 + 1] = headList[i];
        this->list[i * 2 + 2] = sizeList[i];
    }
    void* res = static_cast<void*>(this->list);
    return res;
}

// Retrieves a bitmap representing the memory allocation status.
void* MemoryManager::getBitmap() {
    uint8_t* bit_map = new uint8_t[(size_in_words + 7) / 8 + 2];
    size_t a = 0;
    uint8_t b = 0;
    uint8_t c = 0;
    for (Node* temp1 = this->node; temp1 != nullptr; temp1 = temp1->next) {
        for (size_t i = 0; i < temp1->size; i++){
            if (!temp1->hole) {
                b |= (1 << c);
            }
            if (++c == 8) {
                bit_map[a + 2] = b;
                b = 0;
                c = 0;
                a++;
            }
        }
    }
    if (c != 0 && (a + 2 < (size_in_words + 7) / 8 + 2)) {
        bit_map[a + 2] = b;
        a++;
    }
    bit_map[0] = (size_in_words + 7) / 8 & 0xFF;
    bit_map[1] = ((size_in_words + 7) / 8 >> 8) & 0xFF;
    return static_cast<void*>(bit_map);
}

// Retrieves the word size of the memory.
unsigned MemoryManager::getWordSize() {
    return this->word_size;
}

// Retrieves the start address of the allocated memory.
void* MemoryManager::getMemoryStart() {
    return this->start;
}

// Retrieves the limit of the allocated memory.
unsigned MemoryManager::getMemoryLimit() {
    return this->size_in_words * this-> word_size;
}

// Best fit allocator function.
int bestFit(int sizeInWords, void* list) {
    uint16_t* holeList = static_cast<uint16_t*>(list);
    int bestFitIndex = -1;
    int smallestFit = INT16_MAX;
    int size = *holeList++;
    for (uint16_t i = 0; i < size; i++) {
        uint16_t holeStart = *holeList++;
        uint16_t holeSize = *holeList++;
        if (holeSize >= sizeInWords && holeSize - sizeInWords < smallestFit) {
            bestFitIndex = holeStart;
            smallestFit = holeSize - sizeInWords;
        }
    }
    return bestFitIndex;
}

// Worst fit allocator function.
int worstFit(int sizeInWords, void* list) {
    uint16_t* holeList = static_cast<uint16_t*>(list);
    int worstFitIndex = -1;
    int largestGap = -1;
    int size = *holeList++;
    for (uint16_t i = 0; i < size; i++) {
        uint16_t holeStart = *holeList++;
        uint16_t holeSize = *holeList++;
        if (holeSize >= sizeInWords && holeSize - sizeInWords > largestGap) {
            worstFitIndex = holeStart;
            largestGap = holeSize - sizeInWords;
        }
    }
    return worstFitIndex;
}

